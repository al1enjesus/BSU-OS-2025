/*
 * memory_profiler.c - Профилировщик памяти процессов
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

typedef struct {
    unsigned long vm_size;      // VSZ
    unsigned long vm_rss;       // RSS
} MemoryMetrics;

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
    }

    fclose(f);
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
        
        printf("\n");
        printf("Interpretation:\n");
        printf("  VSZ - Total virtual memory used by process\n");
        printf("  RSS - Physical memory currently used\n");
        printf("  Difference: "); 
        print_size(metrics.vm_size - metrics.vm_rss); 
        printf(" (memory allocated but not in RAM)\n");
    } else {
        printf("Failed to read memory metrics\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID>\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s 1234                # Show info for PID 1234\n", argv[0]);
        printf("  %s %d                  # Show info for current shell\n", argv[0], getpid());
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
        return 1;
    }

    print_process_info(pid);

    return 0;
}
