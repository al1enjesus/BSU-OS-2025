
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

static volatile sig_atomic_t monitoring_active = 1;

typedef struct {
    unsigned long vm_size;
    unsigned long vm_rss;
    unsigned long vm_data;
    unsigned long vm_stk;
    unsigned long vm_exe;
    unsigned long vm_lib;
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;


void signal_handler(int sig) {
    monitoring_active = 0;
    printf("\nReceived signal %d, stopping monitoring...\n", sig);
}

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/proc/%d/status", pid);
    if (path_len < 0) {
        fprintf(stderr, "Error: snprintf failed for PID %d\n", pid);
        return -1;
    }
    if (path_len >= (int)sizeof(path)) {
        fprintf(stderr, "Error: Path too long for PID %d (max %zu chars)\n", pid, sizeof(path));
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));

    char line[256];
    int found_metrics = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &metrics->vm_size) == 1) found_metrics++;
        else if (sscanf(line, "VmRSS: %lu kB", &metrics->vm_rss) == 1) found_metrics++;
        else if (sscanf(line, "VmData: %lu kB", &metrics->vm_data) == 1) found_metrics++;
        else if (sscanf(line, "VmStk: %lu kB", &metrics->vm_stk) == 1) found_metrics++;
        else if (sscanf(line, "VmExe: %lu kB", &metrics->vm_exe) == 1) found_metrics++;
        else if (sscanf(line, "VmLib: %lu kB", &metrics->vm_lib) == 1) found_metrics++;
    }

    fclose(f);
    
    if (found_metrics < 2) {
        fprintf(stderr, "Error: Could not read sufficient memory metrics for PID %d (found %d)\n", pid, found_metrics);
        return -1;
    }
    
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if (path_len < 0) {
        fprintf(stderr, "Error: snprintf failed for PID %d\n", pid);
        return -1;
    }
    if (path_len >= (int)sizeof(path)) {
        fprintf(stderr, "Error: Path too long for PID %d (max %zu chars)\n", pid, sizeof(path));
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    unsigned long minflt, majflt;
    int result = fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*lu %lu", &minflt, &majflt);
    
    fclose(f);
    
    if (result != 2) {
        fprintf(stderr, "Error: Failed to parse page faults from %s (read %d fields, expected 2)\n", path, result);
        return -1;
    }
    
    faults->minor_faults = minflt;
    faults->major_faults = majflt;
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
    int path_len = snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    if (path_len < 0) {
        strncpy(name, "unknown", len);
        return -1;
    }
    if (path_len >= (int)sizeof(path)) {
        strncpy(name, "unknown", len);
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        strncpy(name, "unknown", len);
        return -1;
    }

    if (!fgets(name, len, f)) {
        strncpy(name, "unknown", len);
        fclose(f);
        return -1;
    }
    
    name[strcspn(name, "\n")] = 0;
    fclose(f);
    return 0;
}

int process_exists(pid_t pid) {
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/proc/%d", pid);
    if (path_len < 0 || path_len >= (int)sizeof(path)) {
        return 0;
    }
    return access(path, F_OK) == 0;
}

void print_process_info(pid_t pid) {
    if (!process_exists(pid)) {
        fprintf(stderr, "Error: Process %d does not exist or cannot be accessed\n", pid);
        return;
    }

    char proc_name[256];
    if (get_process_name(pid, proc_name, sizeof(proc_name)) != 0) {
        fprintf(stderr, "Warning: Could not read process name for PID %d\n", pid);
        strcpy(proc_name, "unknown");
    }

    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");
        
        printf("\n");
        printf("Memory Regions:\n");
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf("\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
    } else {
        printf("Memory Metrics: unavailable\n");
    }

    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    } else {
        printf("Page Faults: unavailable\n");
    }
}

void watch_process(pid_t pid, int interval) {
    if (!process_exists(pid)) {
        fprintf(stderr, "Error: Process %d does not exist or cannot be accessed\n", pid);
        return;
    }

  
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    int first_iteration = 1;
    int iteration_count = 0;

    while (monitoring_active) {
        if (!process_exists(pid)) {
            printf("Process %d no longer exists.\n", pid);
            break;
        }

        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));
        printf("Iteration: %d\n", ++iteration_count);

        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Error: Failed to read memory metrics for PID %d\n", pid);
            break;
        }

        if (read_page_faults(pid, &faults) != 0) {
            printf("Warning: Failed to read page faults for PID %d\n", pid);
            // Continue monitoring even if page faults fail
        }

        char proc_name[256];
        if (get_process_name(pid, proc_name, sizeof(proc_name)) != 0) {
            strcpy(proc_name, "unknown");
        }
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

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = faults.minor_faults - prev_faults.minor_faults;
            if (delta != 0) {
                printf("  (%+ld)", delta);
            }
        }
        printf("\n");

        printf("  Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = faults.major_faults - prev_faults.major_faults;
            if (delta != 0) {
                printf("  (%+ld)", delta);
            }
        }
        printf("\n");

        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        // Sleep with interruption check
        for (int i = 0; i < interval && monitoring_active; i++) {
            sleep(1);
        }
    }

    printf("\nMonitoring stopped. Total iterations: %d\n", iteration_count);
}

void print_usage(const char *program_name) {
    printf("Memory Profiler - Monitor process memory usage\n");
    printf("Usage: %s <PID> [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  --watch [interval]     Monitor process continuously (default: 1 sec)\n");
    printf("  --compare <PID2>       Compare two processes\n");
    printf("  --map                  Show detailed memory map\n");
    printf("  --help                 Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s 1234                # Show info for PID 1234\n", program_name);
    printf("  %s 1234 --watch        # Monitor PID 1234\n", program_name);
    printf("  %s 1234 --watch 5      # Monitor with 5 sec interval\n", program_name);
    printf("  %s --help              # Show help\n", program_name);
    printf("\nMonitoring can be stopped with Ctrl+C\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Error: Invalid PID '%s'. PID must be a positive integer.\n", argv[1]);
        return 1;
    }

    if (!process_exists(pid)) {
        fprintf(stderr, "Error: Process %d does not exist or cannot be accessed.\n", pid);
        fprintf(stderr, "Check if the process exists and you have permission to access it.\n");
        return 1;
    }

    int watch_mode = 0;
    int watch_interval = 1;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                watch_interval = atoi(argv[i + 1]);
                if (watch_interval <= 0) {
                    fprintf(stderr, "Error: Invalid interval '%s'. Interval must be positive.\n", argv[i + 1]);
                    return 1;
                }
                i++;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (watch_mode) {
        watch_process(pid, watch_interval);
    } else {
        print_process_info(pid);
    }

    return 0;
}

