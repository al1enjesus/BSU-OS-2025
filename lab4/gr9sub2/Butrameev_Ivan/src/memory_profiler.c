/*
 * memory_profiler.c - Профилировщик памяти процессов
 * gcc -O2 memory_profiler.c -o memory_profiler
 * ./memory_profiler PID
 * ./memory_profiler --watch PID 10
 * ./memory_profiler --compare PID1 PID2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty;
    unsigned long private_clean, private_dirty;
} MemoryMetrics;

typedef struct {
    unsigned long minor, major;
} PageFaults;

int read_memory_metrics(pid_t pid, MemoryMetrics *m) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "VmSize: %lu", &m->vm_size);
        sscanf(line, "VmRSS: %lu", &m->vm_rss);
        sscanf(line, "VmData: %lu", &m->vm_data);
        sscanf(line, "VmStk: %lu", &m->vm_stk);
        sscanf(line, "VmExe: %lu", &m->vm_exe);
        sscanf(line, "VmLib: %lu", &m->vm_lib);
    }
    fclose(f);
    return 0;
}

int read_pss(pid_t pid, MemoryMetrics *m) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "Pss: %lu", &m->pss);
        sscanf(line, "Shared_Clean: %lu", &m->shared_clean);
        sscanf(line, "Shared_Dirty: %lu", &m->shared_dirty);
        sscanf(line, "Private_Clean: %lu", &m->private_clean);
        sscanf(line, "Private_Dirty: %lu", &m->private_dirty);
    }
    fclose(f);
    return 0;
}

int read_page_faults(pid_t pid, PageFaults *pf) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[4096];
    if (fgets(buf, sizeof(buf), f)) {
        unsigned long dummy;
        char *p = buf;
        int field = 0;
        while (*p && field < 10) {
            if (*p == ' ') field++;
            p++;
        }
        sscanf(p, "%lu %lu %lu", &pf->minor, &dummy, &pf->major);
    }
    fclose(f);
    return 0;
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;

    printf("\nMemory Map:\n");
    printf("Address Range       Perms  Size     Type\n");
    printf("--------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], pathname[256] = "";
        int n = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                       &start, &end, perms, pathname);

        if (n >= 3) {
            unsigned long size_kb = (end - start) / 1024;
            const char *type = "other";
            if (strstr(pathname, "[heap]")) type = "heap";
            else if (strstr(pathname, "[stack]")) type = "stack";
            else if (pathname[0] == '/') type = "lib/exec";

            printf("%08lx-%08lx %s %7lu KB %s\n", start, end, perms, size_kb, type);
        }
    }
    fclose(f);
}

void print_metrics(pid_t pid, MemoryMetrics *m, PageFaults *pf) {
    printf("\n");
    printf("Memory Profiler - PID %d\n", pid);
    printf("=================================\n\n");

    printf("VSZ (Virtual):       %8lu KB (%.1f MB)\n", m->vm_size, m->vm_size/1024.0);
    printf("RSS (Resident):      %8lu KB (%.1f MB)\n", m->vm_rss, m->vm_rss/1024.0);
    if (m->pss > 0) {
        printf("PSS (Proportional):  %8lu KB (%.1f MB)\n", m->pss, m->pss/1024.0);
    }

    unsigned long uss = m->private_clean + m->private_dirty;
    if (uss > 0) {
        printf("USS (Unique):        %8lu KB (%.1f MB)\n", uss, uss/1024.0);
    }

    printf("\nShared Memory:\n");
    unsigned long shared = m->shared_clean + m->shared_dirty;
    printf("  Total:             %8lu KB (%.1f MB)\n", shared, shared/1024.0);
    printf("  Clean:             %8lu KB\n", m->shared_clean);
    printf("  Dirty:             %8lu KB\n", m->shared_dirty);

    printf("\nSegments:\n");
    printf("  Heap:              %8lu KB (%.1f MB)\n", m->vm_data, m->vm_data/1024.0);
    printf("  Stack:             %8lu KB (%.1f MB)\n", m->vm_stk, m->vm_stk/1024.0);
    printf("  Code/Text:         %8lu KB (%.1f MB)\n", m->vm_exe, m->vm_exe/1024.0);
    printf("  Libraries:         %8lu KB (%.1f MB)\n", m->vm_lib, m->vm_lib/1024.0);

    printf("\nPage Faults:\n");
    printf("  Minor: %lu\n", pf->minor);
    printf("  Major: %lu\n", pf->major);
}

void watch_process(pid_t pid, int duration) {
    printf("Monitoring PID %d for %d seconds...\n\n", pid, duration);

    MemoryMetrics m = {0}, m_prev = {0};
    PageFaults pf = {0};

    for (int i = 0; i < duration; i++) {
        read_memory_metrics(pid, &m);
        read_pss(pid, &m);
        read_page_faults(pid, &pf);

        printf("\033[2J\033[H");
        printf("=== Dynamic Monitoring: %d/%d sec ===\n\n", i+1, duration);
        printf("VSZ: %.1f MB (delta: %+.1f MB)\n", m.vm_size/1024.0, (double)(m.vm_size - m_prev.vm_size)/1024.0);
        printf("RSS: %.1f MB (delta: %+.1f MB)\n", m.vm_rss/1024.0, (double)(m.vm_rss - m_prev.vm_rss)/1024.0);
        if (m.pss > 0) {
            printf("PSS: %.1f MB (delta: %+.1f MB)\n", m.pss/1024.0, (double)(m.pss - m_prev.pss)/1024.0);
        }
        printf("Minor faults: %lu\n", pf.minor);
        printf("Major faults: %lu\n", pf.major);

        m_prev = m;
        sleep(1);
    }
}

void compare_processes(pid_t pid1, pid_t pid2) {
    MemoryMetrics m1 = {0}, m2 = {0};
    PageFaults pf1 = {0}, pf2 = {0};

    read_memory_metrics(pid1, &m1);
    read_pss(pid1, &m1);
    read_page_faults(pid1, &pf1);

    read_memory_metrics(pid2, &m2);
    read_pss(pid2, &m2);
    read_page_faults(pid2, &pf2);

    printf("\n");
    printf("Comparing PID %d vs PID %d\n", pid1, pid2);
    printf("========================================\n\n");

    long long vsz_d = (long long)m1.vm_size - m2.vm_size;
    long long rss_d = (long long)m1.vm_rss - m2.vm_rss;

    printf("Metric               PID %d        PID %d        Diff\n", pid1, pid2);
    printf("----------------------------------------\n");
    printf("VSZ (MB)             %8.1f      %8.1f      %+8.1f\n", 
           m1.vm_size/1024.0, m2.vm_size/1024.0, vsz_d/1024.0);
    printf("RSS (MB)             %8.1f      %8.1f      %+8.1f\n", 
           m1.vm_rss/1024.0, m2.vm_rss/1024.0, rss_d/1024.0);

    if (m1.pss > 0 && m2.pss > 0) {
        long long pss_d = (long long)m1.pss - m2.pss;
        printf("PSS (MB)             %8.1f      %8.1f      %+8.1f\n", 
               m1.pss/1024.0, m2.pss/1024.0, pss_d/1024.0);
    }

    printf("Heap (MB)            %8.1f      %8.1f\n", m1.vm_data/1024.0, m2.vm_data/1024.0);
    printf("Stack (MB)           %8.1f      %8.1f\n", m1.vm_stk/1024.0, m2.vm_stk/1024.0);

    printf("\nPage Faults:\n");
    printf("Minor   : %lu vs %lu\n", pf1.minor, pf2.minor);
    printf("Major   : %lu vs %lu\n", pf1.major, pf2.major);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s PID                    - show metrics\n", argv[0]);
        printf("  %s --watch PID [seconds]  - monitor process\n", argv[0]);
        printf("  %s --compare PID1 PID2    - compare processes\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--watch") == 0) {
        if (argc < 3) {
            printf("Error: specify PID after --watch\n");
            return 1;
        }
        pid_t pid = atoi(argv[2]);
        int duration = (argc > 3) ? atoi(argv[3]) : 10;
        if (pid <= 0) {
            printf("Error: invalid PID\n");
            return 1;
        }
        watch_process(pid, duration);
        return 0;
    }

    if (strcmp(argv[1], "--compare") == 0) {
        if (argc < 4) {
            printf("Error: specify two PIDs for comparison\n");
            return 1;
        }
        pid_t pid1 = atoi(argv[2]);
        pid_t pid2 = atoi(argv[3]);
        if (pid1 <= 0 || pid2 <= 0) {
            printf("Error: invalid PIDs\n");
            return 1;
        }
        compare_processes(pid1, pid2);
        return 0;
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: invalid PID\n");
        return 1;
    }

    MemoryMetrics m = {0};
    PageFaults pf = {0};

    if (read_memory_metrics(pid, &m) < 0) {
        printf("Error: cannot read /proc/%d/status\n", pid);
        return 1;
    }

    read_pss(pid, &m);
    read_page_faults(pid, &pf);

    print_metrics(pid, &m, &pf);
    print_memory_map(pid);

    return 0;
}
