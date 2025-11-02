/*
 * memory_profiler.c - Профайлер памяти процесса
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_profiler.c -o memory_profiler
 * Использование: ./memory_profiler <PID>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>   // <--- ВАЖНО: для pid_t

#define MAX_PATH 256
#define MAX_LINE 512

void print_metrics(pid_t pid) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen status");
        return;
    }

    char line[MAX_LINE];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) sscanf(line + 7, "%lu", &vm_size);
        else if (strncmp(line, "VmRSS:", 6) == 0) sscanf(line + 6, "%lu", &vm_rss);
        else if (strncmp(line, "VmData:", 7) == 0) sscanf(line + 7, "%lu", &vm_data);
        else if (strncmp(line, "VmStk:", 6) == 0) sscanf(line + 6, "%lu", &vm_stk);
    }
    fclose(f);

    printf("=== Memory Metrics (PID %d) ===\n", pid);
    printf("VSZ:     %6lu KB (%.1f MB)\n", vm_size, vm_size / 1024.0);
    printf("RSS:     %6lu KB (%.1f MB)\n", vm_rss, vm_rss / 1024.0);
    printf("Data:    %6lu KB (%.1f MB)\n", vm_data, vm_data / 1024.0);
    printf("Stack:   %6lu KB (%.1f MB)\n", vm_stk, vm_stk / 1024.0);
}

void print_maps(pid_t pid) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen maps");
        return;
    }

    printf("\n=== Memory Map ===\n");
    printf("%-18s %-6s %-8s %s\n", "Range", "Perm", "Size", "Path");
    printf("------------------------------------------------------------\n");

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path[MAX_PATH] = "";
        unsigned long offset, inode;
        char dev[10];

        if (sscanf(line, "%lx-%lx %4s %lx %9s %lu %[^\n]", &start, &end, perms, &offset, dev, &inode, path) < 6)
            continue;

        unsigned long size_kb = (end - start) / 1024;
        printf("%08lx-%08lx %s %6lu KB %s\n", start, end, perms, size_kb, path);
    }
    fclose(f);
}

void print_smaps_summary(pid_t pid) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("\n[smaps not available]\n");
        return;
    }

    printf("\n=== Detailed Memory (smaps) ===\n");
    printf("%-12s %-8s %-8s %-8s\n", "Region", "RSS", "PSS", "Shared");
    printf("----------------------------------------------------\n");

    char line[MAX_LINE];
    unsigned long total_rss = 0, total_pss = 0, total_shared = 0;
    unsigned long rss = 0, pss = 0, shared_clean = 0, shared_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Rss:")) sscanf(line, "Rss: %lu kB", &rss);
        else if (strstr(line, "Pss:")) sscanf(line, "Pss: %lu kB", &pss);
        else if (strstr(line, "Shared_Clean:")) sscanf(line, "Shared_Clean: %lu kB", &shared_clean);
        else if (strstr(line, "Shared_Dirty:")) sscanf(line, "Shared_Dirty: %lu kB", &shared_dirty);
        else if (line[0] >= '0' && line[0] <= '9') {
            if (rss > 0) {
                total_rss += rss;
                total_pss += pss;
                total_shared += (shared_clean + shared_dirty);
                printf("%-12s %6lu KB %6lu KB %6lu KB\n", "[region]", rss, pss, shared_clean + shared_dirty);
            }
            rss = pss = shared_clean = shared_dirty = 0;
        }
    }
    fclose(f);

    printf("----------------------------------------------------\n");
    printf("TOTAL:       %6lu KB %6lu KB %6lu KB\n", total_rss, total_pss, total_shared);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        fprintf(stderr, "Example: %s $$\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Invalid PID\n");
        return 1;
    }

    printf("Memory Profiler for PID %d\n", pid);
    printf("================================\n");

    print_metrics(pid);
    print_maps(pid);
    print_smaps_summary(pid);

    printf("\nDone.\n");
    return 0;
}
