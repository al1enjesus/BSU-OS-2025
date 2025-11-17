#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

// Структура для хранения метрик памяти
typedef struct {
    unsigned long vm_size;      // VSZ - виртуальный размер (KB)
    unsigned long vm_rss;       // RSS - резидентная память (KB)
    unsigned long vm_data;      // Data + Heap (KB)
    unsigned long vm_stk;       // Stack (KB)
    unsigned long vm_exe;       // Text (код) (KB)
    unsigned long vm_lib;       // Shared libraries (KB)
    unsigned long pss;          // Proportional Set Size (KB)
    unsigned long shared_clean; // Чистая разделяемая память (KB)
    unsigned long shared_dirty; // Грязная разделяемая память (KB)
    unsigned long private_clean;// Чистая приватная память (KB)
    unsigned long private_dirty;// Грязная приватная память (KB)
} MemoryMetrics;

// Структура для page faults
typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

// Структура для сегмента памяти
typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[5];
    char path[256];
} MemorySegment;

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open proc/[PID]/status");
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
    }

    fclose(f);
    return 0;
}

int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        metrics->pss = 0;
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Pss: %lu kB", &metrics->pss) == 1) continue;
        if (sscanf(line, "Shared_Clean: %lu kB", &metrics->shared_clean) == 1) continue;
        if (sscanf(line, "Shared_Dirty: %lu kB", &metrics->shared_dirty) == 1) continue;
        if (sscanf(line, "Private_Clean: %lu kB", &metrics->private_clean) == 1) continue;
        if (sscanf(line, "Private_Dirty: %lu kB", &metrics->private_dirty) == 1) continue;
    }

    fclose(f);
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open /proc/[PID]/stat");
        return -1;
    }

    char line[1024];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }

    char *p = strrchr(line, ')');
    if (!p) {
        fclose(f);
        return -1;
    }
    p += 2; 
    
    unsigned long minflt = 0, majflt = 0;
    int items = sscanf(p, "%*c %*d %*d %*d %*d %*d %*u %lu %*u %lu", &minflt, &majflt);

    if (items != 2) {
        fprintf(stderr, "Warning: failed to parse /proc/%d/stat\n", pid);
        fclose(f);
        return -1;
    }
    
    faults->minor_faults = minflt;
    faults->major_faults = majflt;

    fclose(f);
    return 0;
}

int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open /proc/[PID]/maps");
        return -1;
    }

    char line[512];
    int lines = 0;
    while (fgets(line, sizeof(line), f)) {
        lines++;
    }
    
    *count = lines;
    if (*count == 0) {
        *segments = NULL;
        fclose(f);
        return 0;
    }

    *segments = malloc(*count * sizeof(MemorySegment));
    if (!*segments) {
        perror("malloc for segments");
        fclose(f);
        return -1;
    }
    
    rewind(f);
    
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < *count) {
        MemorySegment *seg = &(*segments)[i];
        seg->path[0] = '\0';
        
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
               &seg->start, &seg->end, seg->perms, seg->path);
        i++;
    }

    fclose(f);
    return 0;
}

void print_size(unsigned long kb) {
    if (kb == 0) {
        printf("%6s KB", "0");
    } else if (kb < 1024) {
        printf("%6lu KB", kb);
    } else if (kb < 1024 * 1024) {
        printf("%6.1f MB", kb / 1024.0);
    } else {
        printf("%6.2f GB", kb / (1024.0 * 1024.0));
    }
}

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
    } else {
        snprintf(name, len, "unknown");
    }

    fclose(f);
    return 0;
}

void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    // Метрики памяти
    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");

        if (read_pss(pid, &metrics) == 0) {
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(" (more accurate)\n");

            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf("\n");

            printf("\n");
            printf("Memory Breakdown (from smaps_rollup):\n");
            printf("  Shared (clean):    "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared (dirty):    "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private (clean):   "); print_size(metrics.private_clean); printf("\n");
            printf("  Private (dirty):   "); print_size(metrics.private_dirty); printf("\n");
        } else {
            printf("  (Run as root or check kernel version for PSS/USS details)\n");
        }

        printf("\n");
        printf("Memory Regions (from status):\n");
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf("\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
    }

    // Page faults
    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    }
}

void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }

    printf("Memory Map (%d segments):\n", count);
    printf("%-34s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < count && i < 20; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;

        printf("%016lx-%016lx %-6s ", seg->start, seg->end, seg->perms);
        print_size(size_kb);
        printf("  %s\n", seg->path[0] ? seg->path : "(anonymous)");
    }

    if (count > 20) {
        printf("... (%d more segments)\n", count - 20);
    }

    free(segments);
}

void watch_process(pid_t pid, int interval) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    int first_iteration = 1;

    while (1) {
        // Очистить экран
        printf("\033[2J\033[H");

        printf("========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));

        // Текущие метрики
        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Process %d no longer exists or not accessible.\n", pid);
            break;
        }

        int pss_ok = (read_pss(pid, &metrics) == 0);
        read_page_faults(pid, &faults);

        // Вывести метрики
        char proc_name[256];
        get_process_name(pid, proc_name, sizeof(proc_name));
        printf("Process: %s (PID %d)\n\n", proc_name, pid);

        // VSZ
        printf("VSZ:  "); print_size(metrics.vm_size);
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        // RSS
        printf("RSS:  "); print_size(metrics.vm_rss);
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        if (pss_ok) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration) {
                long delta = (long)metrics.pss - (long)prev_metrics.pss;
                if (delta != 0) {
                    printf("  (%+ld KB)", delta);
                }
            }
            printf("\n");
        }

        printf("\nPage Faults (delta per %d sec):\n", interval);
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

        // Сохранить текущие значения
        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }
}

void print_compare_row(const char *label, unsigned long v1, unsigned long v2) {
    printf("%-20s ", label);
    print_size(v1);
    printf(" | ");
    print_size(v2);
    printf("\n");
}
void compare_processes(pid_t pid1, pid_t pid2) {
    char name1[256], name2[256];
    get_process_name(pid1, name1, sizeof(name1));
    get_process_name(pid2, name2, sizeof(name2));
    
    printf("Comparing processes:\n");
    printf("  PID 1: %d (%s)\n", pid1, name1);
    printf("  PID 2: %d (%s)\n", pid2, name2);
    printf("================================================\n\n");

    MemoryMetrics m1, m2;

    if (read_memory_metrics(pid1, &m1) != 0) {
        printf("Failed to read metrics for PID %d\n", pid1);
        return;
    }
    if (read_memory_metrics(pid2, &m2) != 0) {
        printf("Failed to read metrics for PID %d\n", pid2);
        return;
    }

    read_pss(pid1, &m1);
    read_pss(pid2, &m2);
    
    PageFaults f1, f2;
    read_page_faults(pid1, &f1);
    read_page_faults(pid2, &f2);

    printf("%-20s %10s   | %10s\n", "Metric", "PID 1", "PID 2");
    printf("------------------------------------------------\n");
    
    print_compare_row("VSZ (Virtual)", m1.vm_size, m2.vm_size);
    print_compare_row("RSS (Resident)", m1.vm_rss, m2.vm_rss);
    print_compare_row("PSS (Proportional)", m1.pss, m2.pss);
    
    unsigned long uss1 = m1.private_clean + m1.private_dirty;
    unsigned long uss2 = m2.private_clean + m2.private_dirty;
    print_compare_row("USS (Unique)", uss1, uss2);

    printf("------------------------------------------------\n");
    print_compare_row("Shared Clean", m1.shared_clean, m2.shared_clean);
    print_compare_row("Shared Dirty", m1.shared_dirty, m2.shared_dirty);
    print_compare_row("Private Clean", m1.private_clean, m2.private_clean);
    print_compare_row("Private Dirty", m1.private_dirty, m2.private_dirty);
    
    printf("------------------------------------------------\n");
    print_compare_row("Data + Heap", m1.vm_data, m2.vm_data);
    print_compare_row("Stack", m1.vm_stk, m2.vm_stk);
    print_compare_row("Text (Code)", m1.vm_exe, m2.vm_exe);
    print_compare_row("Libraries", m1.vm_lib, m2.vm_lib);

    printf("------------------------------------------------\n");
    printf("%-20s %10lu | %10lu\n", "Minor Faults", f1.minor_faults, f2.minor_faults);
    printf("%-20s %10lu | %10lu\n", "Major Faults", f1.major_faults, f2.major_faults);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --watch [interval]     Monitor process continuously (default: 1 sec)\n");
        printf("  --compare <PID2>       Compare two processes\n");
        printf("  --map                  Show detailed memory map\n");
        printf("\nExamples:\n");
        printf("  %s 1234                # Show info for PID 1234\n", argv[0]);
        printf("  %s 1234 --watch        # Monitor PID 1234\n", argv[0]);
        printf("  %s 1234 --watch 5      # Monitor with 5 sec interval\n", argv[0]);
        printf("  %s 1234 --compare 5678 # Compare two processes\n", argv[0]);
        printf("  %s 1234 --map          # Show memory map\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    // Проверить существование процесса
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
        return 1;
    }

    // Парсинг опций
    int watch_mode = 0;
    int watch_interval = 1;
    int compare_mode = 0;
    pid_t compare_pid = 0;
    int show_map = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                watch_interval = atoi(argv[i + 1]);
                if (watch_interval <= 0) watch_interval = 1;
                i++;
            }
        } else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            compare_mode = 1;
            compare_pid = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--map") == 0) {
            show_map = 1;
        }
    }

    // Выполнить нужный режим
    if (compare_mode) {
        compare_processes(pid, compare_pid);
    } else if (watch_mode) {
        watch_process(pid, watch_interval);
    } else {
        // Обычный режим - показать информацию один раз
        print_process_info(pid);

        if (show_map) {
            printf("\n");
            print_memory_map_summary(pid);
        }
    }

    return 0;
}