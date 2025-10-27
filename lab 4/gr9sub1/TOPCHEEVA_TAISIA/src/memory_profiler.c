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
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/proc/%d/status", pid);
    if (path_len < 0 || path_len >= (int)sizeof(path)) {
        fprintf(stderr, "Error: Path too long for PID %d\n", pid);
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open /proc/%d/status: %s\n", pid, strerror(errno));
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
        fprintf(stderr, "Error: Could not read memory metrics for PID %d\n", pid);
        return -1;
    }
    
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if (path_len < 0 || path_len >= (int)sizeof(path)) {
        fprintf(stderr, "Error: Path too long for PID %d\n", pid);
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open /proc/%d/stat: %s\n", pid, strerror(errno));
        return -1;
    }

    unsigned long minflt, majflt;
    int result = fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*lu %lu", &minflt, &majflt);
    
    fclose(f);
    
    if (result != 2) {
        fprintf(stderr, "Error: Could not read page faults for PID %d (read %d fields, expected 2)\n", pid, result);
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
    if (path_len < 0 || path_len >= (int)sizeof(path)) {
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
        fprintf(stderr, "Error: Process %d does not exist\n", pid);
        return;
    }

    char proc_name[256];
    if (get_process_name(pid, proc_name, sizeof(proc_name)) != 0) {
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
        fprintf(stderr, "Error: Process %d does not exist\n", pid);
        return;
    }

    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    int first_iteration = 1;

    while (1) {
        if (!process_exists(pid)) {
            printf("Process %d no longer exists.\n", pid);
            break;
        }

        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));

        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Failed to read memory metrics.\n");
            break;
        }

        if (read_page_faults(pid, &faults) != 0) {
            printf("Failed to read page faults.\n");
            // Continue monitoring even if page faults fail
        }

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

        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }
}

void print_usage(const char *program_name) {
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
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Error: Invalid PID '%s'\n", argv[1]);
        return 1;
    }

    if (!process_exists(pid)) {
        fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
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
                    fprintf(stderr, "Error: Invalid interval '%s'\n", argv[i + 1]);
                    return 1;
                }
                i++;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (watch_mode) {
        watch_process(pid, watch_interval);
    } else {
        print_process_info(pid);
    }

    return 0;
}

