/*
 * memory_profiler.c - Профилировщик памяти процессов
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_profiler.c -o memory_profiler
 * Использование: ./memory_profiler <PID> [--watch]
 *
 * Анализирует использование памяти процессом, показывает карту памяти,
 * отслеживает page faults и динамически мониторит изменения.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

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

/* read_memory_metrics: parse /proc/[pid]/status for Vm* entries */
int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long val;
        if (sscanf(line, "VmSize: %lu kB", &val) == 1) {
            metrics->vm_size = val;
            continue;
        }
        if (sscanf(line, "VmRSS: %lu kB", &val) == 1) {
            metrics->vm_rss = val;
            continue;
        }
        if (sscanf(line, "VmData: %lu kB", &val) == 1) {
            metrics->vm_data = val;
            continue;
        }
        if (sscanf(line, "VmStk: %lu kB", &val) == 1) {
            metrics->vm_stk = val;
            continue;
        }
        if (sscanf(line, "VmExe: %lu kB", &val) == 1) {
            metrics->vm_exe = val;
            continue;
        }
        if (sscanf(line, "VmLib: %lu kB", &val) == 1) {
            metrics->vm_lib = val;
            continue;
        }
    }

    fclose(f);
    return 0;
}

/* read_pss: try /proc/[pid]/smaps_rollup, fallback to /proc/[pid]/smaps accumulate */
int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            unsigned long val;
            if (sscanf(line, "Pss: %lu kB", &val) == 1) {
                metrics->pss = val;
                continue;
            }
            if (sscanf(line, "Shared_Clean: %lu kB", &val) == 1) {
                metrics->shared_clean = val;
                continue;
            }
            if (sscanf(line, "Shared_Dirty: %lu kB", &val) == 1) {
                metrics->shared_dirty = val;
                continue;
            }
            if (sscanf(line, "Private_Clean: %lu kB", &val) == 1) {
                metrics->private_clean = val;
                continue;
            }
            if (sscanf(line, "Private_Dirty: %lu kB", &val) == 1) {
                metrics->private_dirty = val;
                continue;
            }
        }
        fclose(f);
        return 0;
    }

    /* Fallback: parse /proc/[pid]/smaps and sum fields (slower) */
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    char line[512];
    unsigned long sum_pss = 0;
    unsigned long sum_shared_clean = 0;
    unsigned long sum_shared_dirty = 0;
    unsigned long sum_private_clean = 0;
    unsigned long sum_private_dirty = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long val;
        if (sscanf(line, "Pss: %lu kB", &val) == 1) {
            sum_pss += val;
            continue;
        }
        if (sscanf(line, "Shared_Clean: %lu kB", &val) == 1) {
            sum_shared_clean += val;
            continue;
        }
        if (sscanf(line, "Shared_Dirty: %lu kB", &val) == 1) {
            sum_shared_dirty += val;
            continue;
        }
        if (sscanf(line, "Private_Clean: %lu kB", &val) == 1) {
            sum_private_clean += val;
            continue;
        }
        if (sscanf(line, "Private_Dirty: %lu kB", &val) == 1) {
            sum_private_dirty += val;
            continue;
        }
    }
    fclose(f);

    metrics->pss = sum_pss;
    metrics->shared_clean = sum_shared_clean;
    metrics->shared_dirty = sum_shared_dirty;
    metrics->private_clean = sum_private_clean;
    metrics->private_dirty = sum_private_dirty;
    return 0;
}

/* read_page_faults: parse /proc/[pid]/stat
 * strategy: read whole line, find closing ')', then tokenize the rest; tokens[0] is state (field 3)
 * minflt is field 10 => index = 10 - 3 = 7  (0-based in tokens array)
 * majflt is field 12 => index = 12 - 3 = 9
 */
int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    char buf[8192];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    char *p = strrchr(buf, ')');
    if (!p) {
        return -1;
    }
    p += 2; // skip ") "
    // tokenize p into tokens
    char *saveptr = NULL;
    char *tok;
    const int MAXTOK = 64;
    char *tokens[MAXTOK];
    int idx = 0;
    tok = strtok_r(p, " ", &saveptr);
    while (tok && idx < MAXTOK) {
        tokens[idx++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }
    unsigned long minflt = 0, majflt = 0;
    if (idx > 7) {
        minflt = strtoul(tokens[7], NULL, 10);
    }
    if (idx > 9) {
        majflt = strtoul(tokens[9], NULL, 10);
    }
    faults->minor_faults = minflt;
    faults->major_faults = majflt;
    return 0;
}

/* read_memory_map: parse /proc/[pid]/maps and allocate segments */
int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    // first pass: count lines
    char line[1024];
    int n = 0;
    while (fgets(line, sizeof(line), f)) n++;
    if (n == 0) {
        fclose(f);
        *segments = NULL;
        *count = 0;
        return 0;
    }
    rewind(f);

    MemorySegment *arr = malloc(sizeof(MemorySegment) * n);
    if (!arr) {
        fclose(f);
        return -1;
    }
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < n) {
        unsigned long start = 0, end = 0;
        char perms[5] = "";
        char pathbuf[256] = "";
        // Try to parse pathname; it's optional
        // example line:
        // 00400000-0040b000 r-xp 00000000 fd:01 12345 /usr/bin/...
        int matched = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", &start, &end, perms, pathbuf);
        if (matched < 3) {
            // fallback simpler parse
            sscanf(line, "%lx-%lx %4s", &start, &end, perms);
            pathbuf[0] = '\0';
        }
        arr[i].start = start;
        arr[i].end = end;
        strncpy(arr[i].perms, perms, sizeof(arr[i].perms)-1);
        arr[i].perms[4] = '\0';
        if (strlen(pathbuf)) {
            // trim leading spaces
            char *p = pathbuf;
            while (*p && isspace((unsigned char)*p)) p++;
            strncpy(arr[i].path, p, sizeof(arr[i].path)-1);
            arr[i].path[sizeof(arr[i].path)-1] = '\0';
        } else {
            arr[i].path[0] = '\0';
        }
        i++;
    }
    fclose(f);

    *segments = arr;
    *count = i;
    return 0;
}

/* print_size expects KB as input */
void print_size(unsigned long kb) {
    if (kb == 0) {
        printf("%8s", "0 B");
        return;
    }
    if (kb < 1024) {
        printf("%8lu KB", kb);
    } else if (kb < 1024UL * 1024UL) {
        printf("%8.1f MB", kb / 1024.0);
    } else {
        printf("%8.2f GB", kb / (1024.0 * 1024.0));
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

void print_memory_map_summary(pid_t pid);

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

        if (read_pss(pid, &metrics) == 0 && metrics.pss) {
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(" (more accurate)\n");
            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf("\n");

            printf("\nMemory Breakdown:\n");
            printf("  Shared (clean):    "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared (dirty):    "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private (clean):   "); print_size(metrics.private_clean); printf("\n");
            printf("  Private (dirty):   "); print_size(metrics.private_dirty); printf("\n");
        } else {
            printf("  PSS/USS:            N/A (smaps_rollup/smaps not accessible)\n");
        }

        printf("\n");
        printf("Memory Regions:\n");
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf("\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
    } else {
        printf("Failed to read /proc/%d/status\n", pid);
    }

    PageFaults faults = {0,0};
    if (read_page_faults(pid, &faults) == 0) {
        printf("\nPage Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    } else {
        printf("\nPage Faults: N/A\n");
    }
}

void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;
    if (read_memory_map(pid, &segments, &count) != 0) {
        printf("Memory map not available (permission or file missing)\n");
        return;
    }

    printf("Memory Map (%d segments):\n", count);
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    for (int i = 0; i < count && i < 20; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;
        printf("%08lx-%08lx %-6s ", seg->start, seg->end, seg->perms);
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

    PageFaults prev_faults = {0,0};
    MemoryMetrics prev_metrics;
    memset(&prev_metrics, 0, sizeof(prev_metrics));
    int first_iteration = 1;

    while (1) {
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
        printf("Process: %s (PID %d)\n\n", proc_name, pid);

        printf("VSZ:  "); print_size(metrics.vm_size);
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            if (delta != 0) printf("  (%+ld KB)", delta);
        }
        printf("\n");

        printf("RSS:  "); print_size(metrics.vm_rss);
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            if (delta != 0) printf("  (%+ld KB)", delta);
        }
        printf("\n");

        if (metrics.pss) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration) {
                long delta = (long)metrics.pss - (long)prev_metrics.pss;
                if (delta != 0) printf("  (%+ld KB)", delta);
            }
            printf("\n");
            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("USS:  "); print_size(uss);
            if (!first_iteration) {
                unsigned long prev_uss = prev_metrics.private_clean + prev_metrics.private_dirty;
                long delta = (long)uss - (long)prev_uss;
                if (delta != 0) printf("  (%+ld KB)", delta);
            }
            printf("\n");
        }

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = (long)faults.minor_faults - (long)prev_faults.minor_faults;
            if (delta > 0) printf("  (+%ld)", delta);
        }
        printf("\n");
        printf("  Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = (long)faults.major_faults - (long)prev_faults.major_faults;
            if (delta > 0) printf("  (+%ld)", delta);
        }
        printf("\n");

        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }
}

void compare_processes(pid_t pid1, pid_t pid2) {
    printf("Comparing processes: %d vs %d\n", pid1, pid2);
    printf("=====================================\n\n");

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

    printf("%-20s %15s %15s %15s\n", "Metric", "PID1", "PID2", "Diff(PID2-PID1)");
    printf("----------------------------------------------------------------------------\n");
    #define PR(x) printf("%-20s %15lu %15lu %15ld\n", x, (unsigned long)( (x== (char*)"VSZ") ? m1.vm_size : 0), 0, 0)
    // Print selected metrics manually:
    long diff;
    printf("%-20s %15s %15s %15s\n", "Metric", "PID1", "PID2", "Diff");
    printf("----------------------------------------------------------------------------\n");
    printf("%-20s %15lu %15lu %15ld\n", "VSZ (KB)", m1.vm_size, m2.vm_size, (long)m2.vm_size - (long)m1.vm_size);
    printf("%-20s %15lu %15lu %15ld\n", "RSS (KB)", m1.vm_rss, m2.vm_rss, (long)m2.vm_rss - (long)m1.vm_rss);
    printf("%-20s %15lu %15lu %15ld\n", "PSS (KB)", m1.pss, m2.pss, (long)m2.pss - (long)m1.pss);
    unsigned long uss1 = m1.private_clean + m1.private_dirty;
    unsigned long uss2 = m2.private_clean + m2.private_dirty;
    printf("%-20s %15lu %15lu %15ld\n", "USS (KB)", uss1, uss2, (long)uss2 - (long)uss1);
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
