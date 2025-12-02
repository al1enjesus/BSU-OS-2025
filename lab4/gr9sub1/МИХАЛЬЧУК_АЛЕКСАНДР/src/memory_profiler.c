#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

typedef struct {
    unsigned long vsz;     // Virtual Size (KB)
    unsigned long rss;     // Resident Set Size (KB)
    unsigned long pss;     // Proportional Set Size (KB)
    unsigned long uss;     // Unique Set Size (KB)
    unsigned long minflt;  // Minor page faults
    unsigned long majflt;  // Major page faults
    unsigned long shared;  // Shared memory (KB)
    unsigned long private; // Private memory (KB)
} MemoryInfo;

typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[8];
    char path[256];
    unsigned long size_kb;
    char type[32];
} MemoryRegion;

void print_human_readable(unsigned long bytes, int is_kb) {
    double size = is_kb ? bytes / 1024.0 : bytes / (1024.0 * 1024.0);
    const char* unit = is_kb ? "MB" : "GB";
    
    if (size < 1024) {
        printf("%.1f %s", size, unit);
    } else {
        printf("%.1f GB", size / 1024.0);
    }
}

int get_memory_info(pid_t pid, MemoryInfo* info) {
    char path[256];
    FILE* file;
    char line[1024];
    
    // Initialize struct
    memset(info, 0, sizeof(MemoryInfo));
    
    // Read /proc/pid/status
    sprintf(path, "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) {
        printf("Ошибка: процесс с PID %d не найден\n", pid);
        return -1;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line + 7, "%lu", &info->vsz);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", &info->rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            // Data segment
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            // Stack
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            // Executable
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            // Shared libraries
        }
    }
    fclose(file);
    
    // Read page faults from /proc/pid/stat - FIXED VERSION
    sprintf(path, "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (file) {
        // Read the entire line and parse manually
        if (fgets(line, sizeof(line), file)) {
            char* token = strtok(line, " ");
            int field = 1;
            
            while (token != NULL) {
                if (field == 10) { // minflt is field 10
                    info->minflt = strtoul(token, NULL, 10);
                } else if (field == 12) { // majflt is field 12
                    info->majflt = strtoul(token, NULL, 10);
                    break; // We got what we need
                }
                token = strtok(NULL, " ");
                field++;
            }
        }
        fclose(file);
    }
    
    // Try to get PSS from smaps_rollup
    sprintf(path, "/proc/%d/smaps_rollup", pid);
    file = fopen(path, "r");
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Pss:", 4) == 0) {
                sscanf(line + 4, "%lu", &info->pss);
            } else if (strncmp(line, "Private_Clean:", 14) == 0) {
                unsigned long private;
                sscanf(line + 14, "%lu", &private);
                info->uss += private;
                info->private += private;
            } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
                unsigned long private;
                sscanf(line + 14, "%lu", &private);
                info->uss += private;
                info->private += private;
            } else if (strncmp(line, "Shared_Clean:", 13) == 0) {
                sscanf(line + 13, "%lu", &info->shared);
            } else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
                unsigned long shared;
                sscanf(line + 13, "%lu", &shared);
                info->shared += shared;
            }
        }
        fclose(file);
    }
    
    return 0;
}

void analyze_memory_regions(pid_t pid) {
    char path[256];
    sprintf(path, "/proc/%d/maps", pid);
    
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Не удалось открыть карту памяти\n");
        return;
    }
    
    MemoryRegion regions[1000];
    int region_count = 0;
    unsigned long total_heap = 0, total_stack = 0, total_libs = 0, total_anon = 0;
    
    char line[512];
    while (fgets(line, sizeof(line), file) && region_count < 1000) {
        MemoryRegion* region = &regions[region_count];
        region->path[0] = '\0';
        
        sscanf(line, "%lx-%lx %7s %*s %*s %*s %255[^\n]",
               &region->start, &region->end, region->perms, region->path);
        
        region->size_kb = (region->end - region->start) / 1024;
        
        // Classify region type
        if (strstr(region->path, "[heap]")) {
            strcpy(region->type, "Heap");
            total_heap += region->size_kb;
        } else if (strstr(region->path, "[stack]")) {
            strcpy(region->type, "Stack");
            total_stack += region->size_kb;
        } else if (strstr(region->path, ".so") || 
                   strstr(region->path, "lib") || 
                   strstr(region->path, "/usr/lib")) {
            strcpy(region->type, "Library");
            total_libs += region->size_kb;
        } else if (region->path[0] == '\0' || 
                   strstr(region->perms, "rw") != NULL) {
            strcpy(region->type, "Anonymous");
            total_anon += region->size_kb;
        } else if (strstr(region->path, "/bin/") || 
                   strstr(region->path, "/usr/bin")) {
            strcpy(region->type, "Executable");
        } else {
            strcpy(region->type, "Other");
        }
        
        region_count++;
    }
    fclose(file);
    
    // Print summary
    printf("\nРаспределение памяти по типам:\n");
    printf("-------------------------------\n");
    printf("Тип             | Размер      \n");
    printf("-------------------------------\n");
    printf("Heap            | %8lu KB\n", total_heap);
    printf("Stack           | %8lu KB\n", total_stack);
    printf("Libraries       | %8lu KB\n", total_libs);
    printf("Anonymous       | %8lu KB\n", total_anon);
    printf("-------------------------------\n");
    
    // Print first 10 regions as sample
    printf("\nПримеры регионов памяти (первые 10):\n");
    printf("Адрес           Размер   Права Тип       Путь\n");
    printf("------------------------------------------------\n");
    
    for (int i = 0; i < 10 && i < region_count; i++) {
        MemoryRegion* r = &regions[i];
        printf("%012lx-%012lx %6lu KB %-4s %-10s %s\n",
               r->start, r->end, r->size_kb, r->perms, r->type, r->path);
    }
}

void print_memory_stats(pid_t pid, MemoryInfo* current, MemoryInfo* previous, int is_watch) {
    char proc_name[256] = "unknown";
    char path[256];
    
    // Get process name
    sprintf(path, "/proc/%d/comm", pid);
    FILE* comm_file = fopen(path, "r");
    if (comm_file) {
        if (fgets(proc_name, sizeof(proc_name), comm_file) != NULL) {
            // Remove newline
            proc_name[strcspn(proc_name, "\n")] = 0;
        }
        fclose(comm_file);
    }
    
    printf("\n");
    printf("===============================================\n");
    printf("Memory Profiler - %s (PID: %d)\n", proc_name, pid);
    printf("===============================================\n");
    
    // Print memory metrics
    printf("\nОсновные метрики памяти:\n");
    printf("-------------------------------\n");
    printf("Метрика          | Значение    | Изменение\n");
    printf("-------------------------------\n");
    
    printf("VSZ              | ");
    print_human_readable(current->vsz, 1);
    printf("   | ");
    if (is_watch && previous->vsz > 0) {
        long delta = current->vsz - previous->vsz;
        if (delta != 0) printf("%+6ld KB", delta);
        else printf("      -     ");
    } else {
        printf("      -     ");
    }
    printf("\n");
    
    printf("RSS              | ");
    print_human_readable(current->rss, 1);
    printf("   | ");
    if (is_watch && previous->rss > 0) {
        long delta = current->rss - previous->rss;
        if (delta != 0) printf("%+6ld KB", delta);
        else printf("      -     ");
    } else {
        printf("      -     ");
    }
    printf("\n");
    
    if (current->pss > 0) {
        printf("PSS              | ");
        print_human_readable(current->pss, 1);
        printf("   | ");
        if (is_watch && previous->pss > 0) {
            long delta = current->pss - previous->pss;
            if (delta != 0) printf("%+6ld KB", delta);
            else printf("      -     ");
        } else {
            printf("      -     ");
        }
        printf("\n");
    }
    
    if (current->uss > 0) {
        printf("USS              | ");
        print_human_readable(current->uss, 1);
        printf("   | ");
        if (is_watch && previous->uss > 0) {
            long delta = current->uss - previous->uss;
            if (delta != 0) printf("%+6ld KB", delta);
            else printf("      -     ");
        } else {
            printf("      -     ");
        }
        printf("\n");
    }
    
    printf("-------------------------------\n");
    
    // Print shared/private memory
    if (current->shared > 0 || current->private > 0) {
        printf("\nShared/Private память:\n");
        printf("- Shared:  ");
        print_human_readable(current->shared, 1);
        printf("\n- Private: ");
        print_human_readable(current->private, 1);
        printf("\n- Ratio:   %.1f%% shared\n", 
               (double)current->shared / (current->shared + current->private) * 100);
    }
    
    // Print page faults
    printf("\nPage Faults:\n");
    printf("- Minor: %lu", current->minflt);
    if (is_watch && previous->minflt > 0) {
        printf(" (+%lu)", current->minflt - previous->minflt);
    }
    printf("\n- Major: %lu", current->majflt);
    if (is_watch && previous->majflt > 0) {
        printf(" (+%lu)", current->majflt - previous->majflt);
    }
    printf("\n");
}

void print_usage() {
    printf("Использование:\n");
    printf("  ./memory_profiler <PID>           - Статический анализ\n");
    printf("  ./memory_profiler --watch <PID>   - Мониторинг в реальном времени\n");
    printf("  ./memory_profiler --compare <PID1> <PID2> - Сравнение процессов\n");
    printf("\nПримеры:\n");
    printf("  ./memory_profiler 1234\n");
    printf("  ./memory_profiler --watch 1234\n");
    printf("  ./memory_profiler --compare 1234 5678\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    int watch_mode = 0;
    int compare_mode = 0;
    pid_t pid1 = 0, pid2 = 0;
    
    // Parse arguments
    if (strcmp(argv[1], "--watch") == 0 && argc >= 3) {
        watch_mode = 1;
        pid1 = atoi(argv[2]);
    } else if (strcmp(argv[1], "--compare") == 0 && argc >= 4) {
        compare_mode = 1;
        pid1 = atoi(argv[2]);
        pid2 = atoi(argv[3]);
    } else {
        pid1 = atoi(argv[1]);
    }
    
    if (pid1 <= 0) {
        printf("Ошибка: неверный PID\n");
        return 1;
    }
    
    if (compare_mode) {
        printf("Режим сравнения процессов:\n");
        printf("PID1: %d vs PID2: %d\n\n", pid1, pid2);
        
        MemoryInfo info1, info2;
        if (get_memory_info(pid1, &info1) == 0 && 
            get_memory_info(pid2, &info2) == 0) {
            
            printf("-------------------------------\n");
            printf("Метрика          | PID %-7d | PID %-7d | Разница\n", pid1, pid2);
            printf("-------------------------------\n");
            
            printf("VSZ              | ");
            print_human_readable(info1.vsz, 1);
            printf(" | ");
            print_human_readable(info2.vsz, 1);
            printf(" | %+6ld KB\n", (long)info1.vsz - info2.vsz);
            
            printf("RSS              | ");
            print_human_readable(info1.rss, 1);
            printf(" | ");
            print_human_readable(info2.rss, 1);
            printf(" | %+6ld KB\n", (long)info1.rss - info2.rss);
            
            printf("-------------------------------\n");
        }
        return 0;
    }
    
    MemoryInfo current, previous;
    memset(&previous, 0, sizeof(MemoryInfo));
    
    if (watch_mode) {
        printf("Режим мониторинга. Для остановки нажмите Ctrl+C\n");
        
        while (1) {
            if (get_memory_info(pid1, &current) != 0) {
                break;
            }
            
            // Clear screen and move cursor to top
            printf("\033[2J\033[H");
            
            print_memory_stats(pid1, &current, &previous, 1);
            analyze_memory_regions(pid1);
            
            printf("\nОбновление через 2 секунды...");
            fflush(stdout);
            
            memcpy(&previous, &current, sizeof(MemoryInfo));
            sleep(2);
        }
    } else {
        // Single analysis mode
        if (get_memory_info(pid1, &current) == 0) {
            print_memory_stats(pid1, &current, &previous, 0);
            analyze_memory_regions(pid1);
        }
    }
    
    return 0;
}
