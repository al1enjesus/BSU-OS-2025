// memory_profiler.c — профилировщик памяти процессов для LAB4, с ограничением размера в sscanf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>

#define STATUS_BUF_SIZE 256
#define SMAPS_BUF_SIZE 512

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty, private_clean, private_dirty;
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[64], line[STATUS_BUF_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open status file");
        return -1;
    }
    memset(metrics, 0, sizeof(MemoryMetrics));
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
    char path[64], line[SMAPS_BUF_SIZE];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        // smaps_rollup отсутствует - можно реализовать /proc/[pid]/smaps парсинг
        return -1;
    }
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
    char path[64], buf[1024];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open stat file");
        return -1;
    }
    if (!fgets(buf, sizeof(buf), f)) {
        perror("Failed to read stat file");
        fclose(f);
        return -1;
    }
    fclose(f);

    unsigned long minflt = 0, majflt = 0;
    char comm[256], state;

    int ret = sscanf(buf,
        "%*d (%255[^)]) %c %*d %*d %*d %*d %*d %*u %lu %*u %lu",
        comm, &state, &minflt, &majflt);

    if (ret != 4) {
        fprintf(stderr, "Failed to parse /proc stat fields\n");
        return -1;
    }

    faults->minor_faults = minflt;
    faults->major_faults = majflt;
    return 0;
}

int get_process_name(pid_t pid, char *name, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
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
    name[strcspn(name, "\n")] = 0; // Remove newline
    fclose(f);
    return 0;
}

void print_size(unsigned long kb) {
    if (kb < 1024)
        printf("%lu KB", kb);
    else if (kb < 1024 * 1024)
        printf("%.1f MB", kb / 1024.0);
    else
        printf("%.2f GB", kb / (1024.0 * 1024.0));
}

void print_process_info(pid_t pid) {
    char proc_name[128];
    get_process_name(pid, proc_name, sizeof(proc_name));
    printf("Process: %s (PID %d)\n", proc_name, pid);

    MemoryMetrics mm;
    if (read_memory_metrics(pid, &mm) != 0) {
        fprintf(stderr, "Error reading memory metrics\n");
        return;
    }

    if (read_pss(pid, &mm) != 0) {
        fprintf(stderr, "Warning: smaps_rollup not available, PSS unavailable\n");
    }

    PageFaults pf = {0};
    if (read_page_faults(pid, &pf) != 0) {
        fprintf(stderr, "Error reading page faults\n");
    }

    printf("VSZ: "); print_size(mm.vm_size); printf("\n");
    printf("RSS: "); print_size(mm.vm_rss); printf("\n");
    printf("PSS: "); print_size(mm.pss); printf("\n");
    unsigned long uss = mm.private_clean + mm.private_dirty;
    printf("USS: "); print_size(uss); printf("\n\n");

    printf("Shared memory: "); print_size(mm.shared_clean + mm.shared_dirty); printf("\n");
    printf("Private memory: "); print_size(uss); printf("\n");
    printf("Heap: "); print_size(mm.vm_data); printf("\n");
    printf("Stack: "); print_size(mm.vm_stk); printf("\n");
    printf("Libs: "); print_size(mm.vm_lib); printf("\n\n");

    printf("Page faults: Minor: %lu Major: %lu\n", pf.minor_faults, pf.major_faults);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s PID\n", argv[0]);
        return 1;
    }
    pid_t pid = atoi(argv[1]);
    print_process_info(pid);
    return 0;
}
