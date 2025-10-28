#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

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
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[5];
    char path[256];
} MemorySegment;

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/status");
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
        snprintf(path, sizeof(path), "/proc/%d/smaps", pid);
        f = fopen(path, "r");
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
            if (strncmp(line, "Pss:", 4) == 0) {
                unsigned long val;
                sscanf(line + 4, "%lu", &val);
                total_pss += val;
            }
            else if (strncmp(line, "Shared_Clean:", 13) == 0) {
                unsigned long val;
                sscanf(line + 13, "%lu", &val);
                total_shared_clean += val;
            }
            else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
                unsigned long val;
                sscanf(line + 13, "%lu", &val);
                total_shared_dirty += val;
            }
            else if (strncmp(line, "Private_Clean:", 14) == 0) {
                unsigned long val;
                sscanf(line + 14, "%lu", &val);
                total_private_clean += val;
            }
            else if (strncmp(line, "Private_Dirty:", 14) == 0) {
                unsigned long val;
                sscanf(line + 14, "%lu", &val);
                total_private_dirty += val;
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
        perror("Failed to open /proc/[PID]/stat");
        return -1;
    }

    char line[1024];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    char *p = line;
    int field = 1;
    unsigned long minflt = 0, majflt = 0;
    
    while (*p) {
        if (*p == ' ') {
            field++;
            while (*p == ' ') p++;
            continue;
        }
        
        if (field == 10) {
            minflt = strtoul(p, &p, 10);
        } else if (field == 12) {
            majflt = strtoul(p, &p, 10);
            break;
        } else {
            while (*p && *p != ' ') p++;
        }
    }
    
    faults->minor_faults = minflt;
    faults->major_faults = majflt;
    return 0;
}

int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/maps");
        return -1;
    }

    *count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        (*count)++;
    }
    
    rewind(f);
    
    *segments = malloc(*count * sizeof(MemorySegment));
    if (!*segments) {
        fclose(f);
        return -1;
    }
    
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < *count) {
        MemorySegment *seg = &(*segments)[i];
        seg->path[0] = '\0';
        
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
                   &seg->start, &seg->end, seg->perms, seg->path) >= 3) {
            i++;
        }
    }

    fclose(f);
    *count = i;
    return 0;
}

void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%4lu KB", kb);
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
    }

    fclose(f);
    return 0;
}

void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }

    printf(COLOR_CYAN "Memory Map (%d segments):\n" COLOR_RESET, count);
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    int heap_count = 0, stack_count = 0, lib_count = 0, anonymous_count = 0, exec_count = 0, other_count = 0;
    
    for (int i = 0; i < count && i < 25; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;

        const char *color = COLOR_RESET;
        if (strstr(seg->path, "[heap]")) {
            color = COLOR_GREEN;
            heap_count++;
        } else if (strstr(seg->path, "[stack]")) {
            color = COLOR_BLUE;
            stack_count++;
        } else if (strstr(seg->path, ".so") || strstr(seg->path, "/lib/")) {
            color = COLOR_YELLOW;
            lib_count++;
        } else if (strlen(seg->path) == 0 || strstr(seg->path, "anonymous")) {
            color = COLOR_MAGENTA;
            anonymous_count++;
        } else if (seg->perms[2] == 'x') {
            color = COLOR_RED;
            exec_count++;
        } else {
            other_count++;
        }

        printf("%s%08lx-%08lx %-6s ", color, seg->start, seg->end, seg->perms);
        print_size(size_kb);
        printf("  %s%s\n", seg->path[0] ? seg->path : "[anonymous]", COLOR_RESET);
    }

    if (count > 25) {
        printf(COLOR_CYAN "... (%d more segments)\n" COLOR_RESET, count - 25);
    }

    printf("\n" COLOR_CYAN "Segment Statistics:\n" COLOR_RESET);
    printf("  Heap segments:     %d\n", heap_count);
    printf("  Stack segments:    %d\n", stack_count);
    printf("  Library segments:  %d\n", lib_count);
    printf("  Anonymous segments:%d\n", anonymous_count);
    printf("  Executable segments:%d\n", exec_count);
    printf("  Other segments:    %d\n", other_count);

    free(segments);
}

void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    printf(COLOR_GREEN "Process: %s (PID %d)\n" COLOR_RESET, proc_name, pid);
    printf("=====================================\n\n");

    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf(COLOR_CYAN "Memory Metrics:\n" COLOR_RESET);
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");

        if (read_pss(pid, &metrics) == 0) {
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(COLOR_YELLOW " (more accurate)\n" COLOR_RESET);

            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf("\n");

            printf("\n");
            printf(COLOR_CYAN "Memory Breakdown:\n" COLOR_RESET);
            printf("  Shared (clean):    "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared (dirty):    "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private (clean):   "); print_size(metrics.private_clean); printf("\n");
            printf("  Private (dirty):   "); print_size(metrics.private_dirty); printf("\n");
        }

        printf("\n");
        printf(COLOR_CYAN "Memory Regions:\n" COLOR_RESET);
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf("\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
    }

    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf(COLOR_CYAN "Page Faults:\n" COLOR_RESET);
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    }
}

void watch_process(pid_t pid, int interval) {
    printf(COLOR_GREEN "Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n" COLOR_RESET, pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    int first_iteration = 1;

    #define HISTORY_SIZE 20
    unsigned long rss_history[HISTORY_SIZE] = {0};
    int history_index = 0;

    while (1) {
        printf("\033[2J\033[H");

        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));

        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Process no longer exists or not accessible.\n");
            break;
        }

        read_pss(pid, &metrics);
        read_page_faults(pid, &faults);

        char proc_name[256];
        get_process_name(pid, proc_name, sizeof(proc_name));
        printf(COLOR_GREEN "Process: %s (PID %d)\n\n" COLOR_RESET, proc_name, pid);

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

        if (metrics.pss > 0) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration && prev_metrics.pss > 0) {
                long delta = (long)metrics.pss - (long)prev_metrics.pss;
                if (delta != 0) {
                    printf("  (%+ld KB)", delta);
                }
            }
            printf("\n");
        }

        printf("\n" COLOR_CYAN "Page Faults:\n" COLOR_RESET);
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

        if (!first_iteration) {
            printf("\n" COLOR_CYAN "RSS History:\n" COLOR_RESET);
            rss_history[history_index] = metrics.vm_rss;
            history_index = (history_index + 1) % HISTORY_SIZE;
            
            unsigned long min_rss = metrics.vm_rss, max_rss = metrics.vm_rss;
            for (int i = 0; i < HISTORY_SIZE; i++) {
                if (rss_history[i] > 0) {
                    if (rss_history[i] < min_rss) min_rss = rss_history[i];
                    if (rss_history[i] > max_rss) max_rss = rss_history[i];
                }
            }
            
            if (max_rss > min_rss) {
                int graph_width = 40;
                for (int i = 0; i < HISTORY_SIZE; i++) {
                    if (rss_history[i] > 0) {
                        int bar_length = (int)((rss_history[i] - min_rss) * graph_width / (max_rss - min_rss));
                        printf("  [");
                        for (int j = 0; j < bar_length; j++) {
                            printf("#");
                        }
                        for (int j = bar_length; j < graph_width; j++) {
                            printf(" ");
                        }
                        printf("] ");
                        print_size(rss_history[i]);
                        printf("\n");
                    }
                }
            }
        }

        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }
}

void compare_processes(pid_t pid1, pid_t pid2) {
    printf(COLOR_GREEN "Comparing processes: %d vs %d\n" COLOR_RESET, pid1, pid2);
    printf("=====================================\n\n");

    MemoryMetrics m1, m2;
    PageFaults f1, f2;
    char name1[256], name2[256];

    get_process_name(pid1, name1, sizeof(name1));
    get_process_name(pid2, name2, sizeof(name2));

    int success1 = (read_memory_metrics(pid1, &m1) == 0);
    int success2 = (read_memory_metrics(pid2, &m2) == 0);
    
    if (success1) read_pss(pid1, &m1);
    if (success2) read_pss(pid2, &m2);
    
    read_page_faults(pid1, &f1);
    read_page_faults(pid2, &f2);

    printf("%-25s %15s %15s %10s\n", "Metric", name1, name2, "Difference");
    printf("------------------------------------------------------------------------\n");

    if (success1 && success2) {
        printf("%-25s ", "VSZ"); 
        print_size(m1.vm_size); 
        printf(" "); 
        print_size(m2.vm_size);
        long diff = (long)m1.vm_size - (long)m2.vm_size;
        printf(" %+ld KB\n", diff);
        
        printf("%-25s ", "RSS"); 
        print_size(m1.vm_rss); 
        printf(" "); 
        print_size(m2.vm_rss);
        diff = (long)m1.vm_rss - (long)m2.vm_rss;
        printf(" %+ld KB\n", diff);
        
        if (m1.pss > 0 && m2.pss > 0) {
            printf("%-25s ", "PSS"); 
            print_size(m1.pss); 
            printf(" "); 
            print_size(m2.pss);
            diff = (long)m1.pss - (long)m2.pss;
            printf(" %+ld KB\n", diff);
        }
    }
    
    printf("%-25s %15lu %15lu %+10ld\n", "Minor Page Faults", 
           f1.minor_faults, f2.minor_faults, 
           (long)f1.minor_faults - (long)f2.minor_faults);
           
    printf("%-25s %15lu %15lu %+10ld\n", "Major Page Faults", 
           f1.major_faults, f2.major_faults, 
           (long)f1.major_faults - (long)f2.major_faults);
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

    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
        return 1;
    }

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

    if (compare_mode) {
        compare_processes(pid, compare_pid);
    } else if (watch_mode) {
        watch_process(pid, watch_interval);
    } else {
        print_process_info(pid);

        if (show_map) {
            printf("\n");
            print_memory_map_summary(pid);
        }
    }

    return 0;
}
