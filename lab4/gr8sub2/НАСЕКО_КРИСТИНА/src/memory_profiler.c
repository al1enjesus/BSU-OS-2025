#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_HISTORY 50
#define MAX_PATHNAME 256
#define MAX_LINE 512

typedef struct {
    unsigned long vm_size;
    unsigned long vm_rss;
    unsigned long vm_data;
    unsigned long vm_stk;
    unsigned long vm_exe;
    unsigned long vm_lib;
    unsigned long pss;
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
    unsigned long swap;
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[8];
    unsigned long offset;
    char dev[16];
    unsigned long inode;
    char pathname[MAX_PATHNAME];
} MemoryMapEntry;

typedef struct {
    unsigned long rss_values[MAX_HISTORY];
    unsigned long pss_values[MAX_HISTORY];
    int count;
    int head;
} MemoryHistory;

// ============ БАЗОВЫЕ ФУНКЦИИ ============

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open status file");
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &metrics->vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &metrics->vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &metrics->vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &metrics->vm_stk) == 1) continue;
        if (sscanf(line, "VmExe: %lu kB", &metrics->vm_exe) == 1) continue;
        if (sscanf(line, "VmLib: %lu kB", &metrics->vm_lib) == 1) continue;
        if (sscanf(line, "VmSwap: %lu kB", &metrics->swap) == 1) continue;
    }
    fclose(f);
    return 0;
}

int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    char line[256];
    unsigned long total_pss = 0;
    unsigned long total_shared_clean = 0;
    unsigned long total_shared_dirty = 0;
    unsigned long total_private_clean = 0;
    unsigned long total_private_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned long value;
        if (sscanf(line, "Pss: %lu kB", &value) == 1) {
            total_pss += value;
            continue;
        }
        if (sscanf(line, "Shared_Clean: %lu kB", &value) == 1) {
            total_shared_clean += value;
            continue;
        }
        if (sscanf(line, "Shared_Dirty: %lu kB", &value) == 1) {
            total_shared_dirty += value;
            continue;
        }
        if (sscanf(line, "Private_Clean: %lu kB", &value) == 1) {
            total_private_clean += value;
            continue;
        }
        if (sscanf(line, "Private_Dirty: %lu kB", &value) == 1) {
            total_private_dirty += value;
            continue;
        }
    }

    metrics->pss = total_pss;
    metrics->shared_clean = total_shared_clean;
    metrics->shared_dirty = total_shared_dirty;
    metrics->private_clean = total_private_clean;
    metrics->private_dirty = total_private_dirty;

    fclose(f);
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open stat file");
        return -1;
    }

    unsigned long minflt, majflt;
    // Исправленный формат без предупреждений
    if (fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %lu", 
               &minflt, &majflt) != 2) {
        fclose(f);
        return -1;
    }
    faults->minor_faults = minflt;
    faults->major_faults = majflt;

    fclose(f);
    return 0;
}

int read_memory_map(pid_t pid, MemoryMapEntry **entries, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open maps file");
        return -1;
    }

    *count = 0;
    int capacity = 100;
    *entries = malloc(capacity * sizeof(MemoryMapEntry));
    if (!*entries) {
        fclose(f);
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (*count >= capacity) {
            capacity *= 2;
            MemoryMapEntry *new_entries = realloc(*entries, capacity * sizeof(MemoryMapEntry));
            if (!new_entries) {
                // Сохраняем то, что успели прочитать
                break;
            }
            *entries = new_entries;
        }

        MemoryMapEntry *entry = &(*entries)[(*count)];
        memset(entry, 0, sizeof(MemoryMapEntry));

        // Безопасный парсинг строки
        char perms_buf[8] = {0};
        char dev_buf[16] = {0};
        char pathname_buf[MAX_PATHNAME] = {0};
        
        int parsed = sscanf(line, "%lx-%lx %7s %lx %15s %lu %255[^\n]",
               &entry->start, &entry->end, perms_buf, &entry->offset,
               dev_buf, &entry->inode, pathname_buf);

        // Минимально должно быть 6 полей
        if (parsed >= 6) {
            // Копируем с проверкой длины
            strncpy(entry->perms, perms_buf, sizeof(entry->perms) - 1);
            strncpy(entry->dev, dev_buf, sizeof(entry->dev) - 1);
            
            if (parsed >= 7) {
                strncpy(entry->pathname, pathname_buf, sizeof(entry->pathname) - 1);
            } else {
                entry->pathname[0] = '\0';
            }
            
            (*count)++;
        }
    }

    fclose(f);
    return 0;
}

// ============ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ============

int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(name, len, "unknown");
        return -1;
    }

    if (fgets(name, len, f)) {
        name[strcspn(name, "\n")] = 0;
    }

    fclose(f);
    return 0;
}

// ============ ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ ============

int find_process_by_name(const char *name, pid_t *pids, int max_pids) {
    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max_pids) {
        if (entry->d_type != DT_DIR) continue;

        char *endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        char comm_path[256];
        snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);

        FILE *f = fopen(comm_path, "r");
        if (!f) continue;

        char proc_name[256];
        if (fgets(proc_name, sizeof(proc_name), f)) {
            proc_name[strcspn(proc_name, "\n")] = 0;
            if (strstr(proc_name, name)) {
                pids[count++] = pid;
            }
        }
        fclose(f);
    }

    closedir(dir);
    return count;
}

void print_memory_map_summary(MemoryMapEntry *entries, int count) {
    printf("\nMemory Map Summary (Top 10 segments):\n");
    printf("=====================================\n");

    // Сортируем по размеру
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            unsigned long size_i = entries[i].end - entries[i].start;
            unsigned long size_j = entries[j].end - entries[j].start;
            if (size_i < size_j) {
                MemoryMapEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    for (int i = 0; i < 10 && i < count; i++) {
        MemoryMapEntry *entry = &entries[i];
        unsigned long size_kb = (entry->end - entry->start) / 1024;
        
        const char *color = "";
        const char *type = "unknown";
        
        if (strstr(entry->pathname, "[heap]")) {
            color = "\033[32m";  // зеленый
            type = "heap";
        } else if (strstr(entry->pathname, "[stack]")) {
            color = "\033[34m";  // синий
            type = "stack";
        } else if (strstr(entry->pathname, ".so")) {
            color = "\033[33m";  // желтый
            type = "library";
        } else if (entry->pathname[0] == '/') {
            type = "file";
        } else if (entry->pathname[0] == '[') {
            type = "special";
        }

        printf("%s%12lu KB\033[0m %-8s %-6s %s\n", 
               color, size_kb, entry->perms, type, 
               entry->pathname[0] ? entry->pathname : "[anonymous]");
    }
}

void analyze_shared_libraries(MemoryMapEntry *entries, int count) {
    printf("\nShared Libraries Analysis:\n");
    printf("==========================\n");

    int lib_count = 0;
    struct {
        char path[MAX_PATHNAME];
        unsigned long total_size;
        int segments;
    } libraries[100];

    for (int i = 0; i < count; i++) {
        MemoryMapEntry *entry = &entries[i];
        if (strstr(entry->pathname, ".so") && entry->pathname[0] == '/') {
            int found = 0;
            for (int j = 0; j < lib_count; j++) {
                if (strcmp(libraries[j].path, entry->pathname) == 0) {
                    libraries[j].total_size += (entry->end - entry->start);
                    libraries[j].segments++;
                    found = 1;
                    break;
                }
            }
            if (!found && lib_count < 100) {
                strncpy(libraries[lib_count].path, entry->pathname, sizeof(libraries[lib_count].path) - 1);
                libraries[lib_count].total_size = (entry->end - entry->start);
                libraries[lib_count].segments = 1;
                lib_count++;
            }
        }
    }

    for (int i = 0; i < lib_count; i++) {
        printf("%8lu KB (%2d seg) %s\n", 
               libraries[i].total_size / 1024,
               libraries[i].segments,
               libraries[i].path);
    }
}

void compare_processes(pid_t pid1, pid_t pid2) {
    char name1[256], name2[256];
    get_process_name(pid1, name1, sizeof(name1));
    get_process_name(pid2, name2, sizeof(name2));
    
    printf("\nComparing Processes:\n");
    printf("===================\n");
    printf("%s (PID %d) vs %s (PID %d)\n\n", name1, pid1, name2, pid2);
    
    MemoryMetrics metrics1, metrics2;
    read_memory_metrics(pid1, &metrics1);
    read_memory_metrics(pid2, &metrics2);
    read_pss(pid1, &metrics1);
    read_pss(pid2, &metrics2);
    
    printf("%-20s %12s %12s %12s\n", "Metric", "Process 1", "Process 2", "Difference");
    printf("%-20s %12s %12s %12s\n", "------", "---------", "---------", "----------");
    
    printf("%-20s %12lu %12lu %12ld\n", "VSZ (KB)", metrics1.vm_size, metrics2.vm_size, 
           (long)metrics1.vm_size - (long)metrics2.vm_size);
    printf("%-20s %12lu %12lu %12ld\n", "RSS (KB)", metrics1.vm_rss, metrics2.vm_rss, 
           (long)metrics1.vm_rss - (long)metrics2.vm_rss);
    printf("%-20s %12lu %12lu %12ld\n", "PSS (KB)", metrics1.pss, metrics2.pss, 
           (long)metrics1.pss - (long)metrics2.pss);
}

// ============ ФУНКЦИИ ДЛЯ ГРАФИКА И CSV ============

void init_memory_history(MemoryHistory *history) {
    memset(history, 0, sizeof(MemoryHistory));
}

void add_to_history(MemoryHistory *history, unsigned long rss, unsigned long pss) {
    history->rss_values[history->head] = rss;
    history->pss_values[history->head] = pss;
    history->head = (history->head + 1) % MAX_HISTORY;
    if (history->count < MAX_HISTORY) {
        history->count++;
    }
}

void print_ascii_graph(MemoryHistory *history, int height, int width) {
    if (history->count < 2) return;
    
    printf("\nMemory Usage Graph (RSS):\n");
    printf("┌");
    for (int i = 0; i < width; i++) printf("─");
    printf("┐\n");
    
    // Находим min и max значения
    unsigned long min_rss = history->rss_values[0];
    unsigned long max_rss = history->rss_values[0];
    
    for (int i = 1; i < history->count; i++) {
        if (history->rss_values[i] < min_rss) min_rss = history->rss_values[i];
        if (history->rss_values[i] > max_rss) max_rss = history->rss_values[i];
    }
    
    // Добавляем небольшой запас
    if (max_rss == min_rss) {
        max_rss = min_rss + 1;
    }
    
    double scale = (double)(max_rss - min_rss) / height;
    if (scale == 0) scale = 1;
    
    // Рисуем график
    for (int y = height - 1; y >= 0; y--) {
        printf("│");
        double threshold = min_rss + (y * scale);
        
        for (int x = 0; x < history->count && x < width; x++) {
            int idx = (history->head - history->count + x + MAX_HISTORY) % MAX_HISTORY;
            if (history->rss_values[idx] >= threshold) {
                printf("█");
            } else {
                printf(" ");
            }
        }
        printf("│ %6.1f MB\n", (min_rss + (y + 1) * scale) / 1024.0);
    }
    
    printf("└");
    for (int i = 0; i < width; i++) printf("─");
    printf("┘\n");
    printf("  Time → (last %d samples)\n", history->count);
}

void save_to_csv(pid_t pid, MemoryHistory *history, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open CSV file");
        return;
    }
    
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));
    
    fprintf(f, "Timestamp,Process_Name,PID,RSS_KB,PSS_KB,RSS_MB,PSS_MB\n");
    
    for (int i = 0; i < history->count; i++) {
        int idx = (history->head - history->count + i + MAX_HISTORY) % MAX_HISTORY;
        fprintf(f, "%d,%s,%d,%lu,%lu,%.2f,%.2f\n",
                i, proc_name, pid,
                history->rss_values[idx], history->pss_values[idx],
                history->rss_values[idx] / 1024.0,
                history->pss_values[idx] / 1024.0);
    }
    
    fclose(f);
    printf("Data saved to %s\n", filename);
}

// ============ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ============

void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%4lu KB", kb);
    } else if (kb < 1024 * 1024) {
        printf("%6.1f MB", kb / 1024.0);
    } else {
        printf("%6.2f GB", kb / (1024.0 * 1024.0));
    }
}

void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");
        printf("  Data/Heap:         "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Executable:        "); print_size(metrics.vm_exe); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
        printf("  Swap:              "); print_size(metrics.swap); printf("\n");
    }

    if (read_pss(pid, &metrics) == 0) {
        printf("\nDetailed Memory (from smaps):\n");
        printf("  PSS (Proportional):"); print_size(metrics.pss); printf("\n");
        printf("  Shared Clean:      "); print_size(metrics.shared_clean); printf("\n");
        printf("  Shared Dirty:      "); print_size(metrics.shared_dirty); printf("\n");
        printf("  Private Clean:     "); print_size(metrics.private_clean); printf("\n");
        printf("  Private Dirty:     "); print_size(metrics.private_dirty); printf("\n");
    }

    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    }

    // Читаем и анализируем карту памяти
    MemoryMapEntry *entries;
    int entry_count;
    if (read_memory_map(pid, &entries, &entry_count) == 0) {
        print_memory_map_summary(entries, entry_count);
        analyze_shared_libraries(entries, entry_count);
        free(entries);
    }
}

void watch_process(pid_t pid, int interval, int graph_height, const char *csv_filename) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    MemoryHistory history;
    init_memory_history(&history);
    int first_iteration = 1;
    int iteration = 0;

    while (1) {
        printf("\033[2J\033[H");  // Clear screen

        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));
        printf("Iteration: %d\n", iteration++);

        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Process no longer exists or not accessible.\n");
            break;
        }

        read_pss(pid, &metrics);
        read_page_faults(pid, &faults);

        // Добавляем в историю
        add_to_history(&history, metrics.vm_rss, metrics.pss);

        char proc_name[256];
        get_process_name(pid, proc_name, sizeof(proc_name));
        printf("Process: %s (PID %d)\n\n", proc_name, pid);

        printf("VSZ:  "); print_size(metrics.vm_size);
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        printf("RSS:  "); print_size(metrics.vm_rss);
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        printf("PSS:  "); print_size(metrics.pss);
        printf("\n");

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = faults.minor_faults - prev_faults.minor_faults;
            if (delta > 0) {
                printf("  (+%ld)", delta);
            }
        }
        printf("\n");

        printf("  Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = faults.major_faults - prev_faults.major_faults;
            if (delta > 0) {
                printf("  (+%ld)", delta);
            }
        }
        printf("\n");

        // Показываем ASCII-график
        if (history.count >= 2) {
            print_ascii_graph(&history, graph_height, 40);
        }

        // Сохраняем в CSV если нужно
        if (csv_filename && iteration % 10 == 0) {
            save_to_csv(pid, &history, csv_filename);
        }

        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }

    // Финальное сохранение
    if (csv_filename) {
        save_to_csv(pid, &history, csv_filename);
    }
}

void interactive_mode() {
    printf("Interactive Process Selection\n");
    printf("=============================\n");
    
    pid_t pids[100];
    int count = find_process_by_name("", pids, 100);
    
    if (count == 0) {
        printf("No processes found.\n");
        return;
    }
    
    printf("\nAvailable processes:\n");
    for (int i = 0; i < count && i < 20; i++) {
        char name[256];
        get_process_name(pids[i], name, sizeof(name));
        printf("%2d. PID %5d: %s\n", i + 1, pids[i], name);
    }
    
    printf("\nEnter process number (1-%d): ", count > 20 ? 20 : count);
    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input.\n");
        // Очищаем буфер ввода
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return;
    }
    
    if (choice < 1 || choice > count) {
        printf("Invalid choice.\n");
        return;
    }
    
    pid_t selected_pid = pids[choice - 1];
    printf("\n");
    print_process_info(selected_pid);
    
    printf("\nPress Enter to continue...");
    // Очищаем буфер перед getchar()
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID|NAME> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  --watch [interval]    Monitor process continuously\n");
        printf("  --graph [height]      Show ASCII graph (default: 10)\n");
        printf("  --csv [filename]      Save data to CSV file\n");
        printf("  --compare <PID2>      Compare with another process\n");
        printf("  --interactive         Interactive process selection\n");
        printf("Examples:\n");
        printf("  %s 1234\n", argv[0]);
        printf("  %s 1234 --watch 2\n", argv[0]);
        printf("  %s 1234 --watch 2 --graph 8\n", argv[0]);
        printf("  %s firefox --watch --csv memory_data.csv\n", argv[0]);
        printf("  %s 1234 --compare 5678\n", argv[0]);
        printf("  %s --interactive\n", argv[0]);
        return 1;
    }

    // Interactive mode
    if (strcmp(argv[1], "--interactive") == 0) {
        interactive_mode();
        return 0;
    }

    pid_t pid;
    int watch_mode = 0;
    int watch_interval = 1;
    int compare_mode = 0;
    pid_t compare_pid = 0;
    int graph_height = 10;
    char *csv_filename = NULL;

    // Try to parse as PID first
    char *endptr;
    pid = strtol(argv[1], &endptr, 10);
    
    // If not a PID, try to find by name
    if (*endptr != '\0') {
        pid_t pids[10];
        int count = find_process_by_name(argv[1], pids, 10);
        if (count == 0) {
            printf("No process found with name containing '%s'\n", argv[1]);
            return 1;
        }
        pid = pids[0];
        if (count > 1) {
            printf("Multiple processes found, using PID %d\n", pid);
        }
    }

    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                watch_interval = atoi(argv[i + 1]);
                i++;  // Skip the interval value
            }
        } else if (strcmp(argv[i], "--graph") == 0) {
            if (i + 1 < argc) {
                graph_height = atoi(argv[i + 1]);
                i++;  // Skip the height value
            }
        } else if (strcmp(argv[i], "--csv") == 0) {
            if (i + 1 < argc) {
                csv_filename = argv[i + 1];
                i++;  // Skip the filename
            }
        } else if (strcmp(argv[i], "--compare") == 0) {
            compare_mode = 1;
            if (i + 1 < argc) {
                compare_pid = atoi(argv[i + 1]);
                i++;  // Skip the PID value
            }
        }
    }

    if (compare_mode && compare_pid > 0) {
        compare_processes(pid, compare_pid);
    } else if (watch_mode) {
        watch_process(pid, watch_interval, graph_height, csv_filename);
    } else {
        print_process_info(pid);
    }

    return 0;
}
