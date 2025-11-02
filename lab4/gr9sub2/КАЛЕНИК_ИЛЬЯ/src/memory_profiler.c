// memory_profiler.c — профилировщик памяти процессов для LAB4
// Компиляция: gcc -Wall -Wextra -O2 memory_profiler.c -o memory_profiler

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty, private_clean, private_dirty;
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

typedef struct {
    unsigned long start, end;
    char perms[5];
    char path[256];
} MemorySegment;

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    memset(metrics, 0, sizeof(MemoryMetrics));
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "VmSize: %lu kB", &metrics->vm_size);
        sscanf(line, "VmRSS: %lu kB", &metrics->vm_rss);
        sscanf(line, "VmData: %lu kB", &metrics->vm_data);
        sscanf(line, "VmStk: %lu kB", &metrics->vm_stk);
        sscanf(line, "VmExe: %lu kB", &metrics->vm_exe);
        sscanf(line, "VmLib: %lu kB", &metrics->vm_lib);
    }
    fclose(f);
    return 0;
}

int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1; // можно добавить парсинг /proc/[pid]/smaps
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "Pss: %lu kB", &metrics->pss);
        sscanf(line, "Shared_Clean: %lu kB", &metrics->shared_clean);
        sscanf(line, "Shared_Dirty: %lu kB", &metrics->shared_dirty);
        sscanf(line, "Private_Clean: %lu kB", &metrics->private_clean);
        sscanf(line, "Private_Dirty: %lu kB", &metrics->private_dirty);
    }
    fclose(f);
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char buf[2048];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return -1; }
    // minflt: 10 поле, majflt: 12
    sscanf(buf,
           "%*d %*s %*c %*d %*d %*d %*d %*d %*u"
           " %lu %*lu %lu",
           &faults->minor_faults, &faults->major_faults);
    fclose(f);
    return 0;
}

int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) { strncpy(name, "unknown", len); return -1; }
    if (fgets(name, len, f)) name[strcspn(name, "\n")] = 0;
    fclose(f);
    return 0;
}

void print_size(unsigned long kb) {
    if (kb < 1024) printf("%lu KB", kb);
    else if (kb < 1024 * 1024) printf("%.1f MB", kb / 1024.0);
    else printf("%.2f GB", kb / (1024.0 * 1024.0));
}

void print_process_info(pid_t pid) {
    char proc_name[256]; get_process_name(pid, proc_name, sizeof(proc_name));
    printf("Process: %s (PID %d)\n", proc_name, pid);

    MemoryMetrics mm;
    read_memory_metrics(pid, &mm);
    read_pss(pid, &mm);

    printf("VSZ: "); print_size(mm.vm_size); printf("\n");
    printf("RSS: "); print_size(mm.vm_rss); printf("\n");
    printf("PSS: "); print_size(mm.pss); printf("\n");
    unsigned long uss = mm.private_clean + mm.private_dirty;
    printf("USS: "); print_size(uss); printf("\n");

    printf("\nShared memory: "); print_size(mm.shared_clean + mm.shared_dirty); printf("\n");
    printf("Private memory: "); print_size(mm.private_clean + mm.private_dirty); printf("\n");
    printf("Heap: "); print_size(mm.vm_data); printf("\n");
    printf("Stack: "); print_size(mm.vm_stk); printf("\n");
    printf("Libs: "); print_size(mm.vm_lib); printf("\n");

    PageFaults pf; read_page_faults(pid, &pf);
    printf("\nPage faults: Minor: %lu Major: %lu\n", pf.minor_faults, pf.major_faults);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s PID\n", argv[0]); return 1;
    }
    pid_t pid = atoi(argv[1]);
    print_process_info(pid);
    return 0;
}
