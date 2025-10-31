#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

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
    unsigned long size;
} MemorySegment;

typedef struct {
    char path[256];
    unsigned long size;
    int sharing_count;
    unsigned long shared_size;
} LibraryInfo;

typedef struct {
    pid_t pid;
    char name[256];
    unsigned long rss;
} ProcessInfo;

#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

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
            unsigned long value;
            if (sscanf(line, "Pss: %lu kB", &value) == 1) {
                total_pss += value;
            }
            else if (sscanf(line, "Shared_Clean: %lu kB", &value) == 1) {
                total_shared_clean += value;
            }
            else if (sscanf(line, "Shared_Dirty: %lu kB", &value) == 1) {
                total_shared_dirty += value;
            }
            else if (sscanf(line, "Private_Clean: %lu kB", &value) == 1) {
                total_private_clean += value;
            }
            else if (sscanf(line, "Private_Dirty: %lu kB", &value) == 1) {
                total_private_dirty += value;
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
    char *token = strtok(line, " ");
    int field = 1;
    unsigned long minflt = 0, majflt = 0;
    while (token != NULL) {
        switch (field) {
            case 10:
                minflt = strtoul(token, NULL, 10);
                break;
            case 12:
                majflt = strtoul(token, NULL, 10);
                break;
        }
        token = strtok(NULL, " ");
        field++;
    }
    if (field < 13) {
        return -1;
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
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
               &seg->start, &seg->end, seg->perms, seg->path);
        seg->size = (seg->end - seg->start) / 1024;
        i++;
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

const char* get_segment_color(const char* path) {
    if (strstr(path, "[heap]")) return COLOR_GREEN;
    if (strstr(path, "[stack]")) return COLOR_BLUE;
    if (strstr(path, ".so") || strstr(path, "/lib/")) return COLOR_YELLOW;
    if (strstr(path, "[vdso]") || strstr(path, "[vvar]")) return COLOR_MAGENTA;
    if (strlen(path) == 0 || strstr(path, "//anon")) return COLOR_CYAN;
    return COLOR_RESET;
}

const char* get_segment_type(const char* path) {
    if (strstr(path, "[heap]")) return "Heap";
    if (strstr(path, "[stack]")) return "Stack";
    if (strstr(path, ".so") || strstr(path, "/lib/")) return "Library";
    if (strstr(path, "[vdso]") || strstr(path, "[vvar]")) return "Kernel";
    if (strlen(path) == 0 || strstr(path, "//anon")) return "Anonymous";
    if (strchr(path, '/') && !strstr(path, ".so")) return "Executable";
    return "Other";
}

int compare_segments(const void *a, const void *b) {
    const MemorySegment *seg1 = (const MemorySegment *)a;
    const MemorySegment *seg2 = (const MemorySegment *)b;
    if (seg1->size > seg2->size) return -1;
    if (seg1->size < seg2->size) return 1;
    return 0;
}

int analyze_libraries(pid_t pid, LibraryInfo **libraries, int *count) {
    MemorySegment *segments = NULL;
    int seg_count = 0;
    if (read_memory_map(pid, &segments, &seg_count) != 0) {
        return -1;
    }
    *count = 0;
    *libraries = malloc(seg_count * sizeof(LibraryInfo));
    for (int i = 0; i < seg_count; i++) {
        MemorySegment *seg = &segments[i];
        if (strstr(seg->path, ".so")) {
            int found = 0;
            for (int j = 0; j < *count; j++) {
                if (strcmp((*libraries)[j].path, seg->path) == 0) {
                    (*libraries)[j].size += seg->size;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                LibraryInfo *lib = &(*libraries)[*count];
                strncpy(lib->path, seg->path, sizeof(lib->path) - 1);
                lib->path[sizeof(lib->path) - 1] = '\0';
                lib->size = seg->size;
                lib->sharing_count = 1;
                lib->shared_size = 0;
                (*count)++;
            }
        }
    }
    for (int i = 0; i < *count; i++) {
        LibraryInfo *lib = &(*libraries)[i];
        DIR *dir = opendir("/proc");
        if (!dir) {
            continue;
        }
        struct dirent *entry;
        int sharing_count = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (isdigit(entry->d_name[0])) {
                pid_t other_pid = atoi(entry->d_name);
                if (other_pid == pid) continue;
                char maps_path[256];
                snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", other_pid);
                FILE *f = fopen(maps_path, "r");
                if (f) {
                    char line[512];
                    while (fgets(line, sizeof(line), f)) {
                        if (strstr(line, lib->path)) {
                            sharing_count++;
                            break;
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(dir);
        lib->sharing_count = sharing_count + 1;
        lib->shared_size = lib->size / lib->sharing_count;
    }
    free(segments);
    return 0;
}

int find_pid_by_name(const char *name, pid_t *pids, int max_pids) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        return -1;
    }
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < max_pids) {
        if (isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            char proc_name[256];
            if (get_process_name(pid, proc_name, sizeof(proc_name)) == 0) {
                if (strstr(proc_name, name)) {
                    pids[count++] = pid;
                }
            }
        }
    }
    closedir(dir);
    return count;
}

int get_all_processes(ProcessInfo **processes, int *count) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        return -1;
    }
    *count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            (*count)++;
        }
    }
    rewinddir(dir);
    *processes = malloc(*count * sizeof(ProcessInfo));
    if (!*processes) {
        closedir(dir);
        return -1;
    }
    int i = 0;
    while ((entry = readdir(dir)) != NULL && i < *count) {
        if (isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            ProcessInfo *proc = &(*processes)[i];
            proc->pid = pid;
            if (get_process_name(pid, proc->name, sizeof(proc->name)) == 0) {
                MemoryMetrics metrics;
                if (read_memory_metrics(pid, &metrics) == 0) {
                    proc->rss = metrics.vm_rss;
                } else {
                    proc->rss = 0;
                }
                i++;
            }
        }
    }
    closedir(dir);
    *count = i;
    return 0;
}

int compare_processes_rss(const void *a, const void *b) {
    const ProcessInfo *p1 = (const ProcessInfo *)a;
    const ProcessInfo *p2 = (const ProcessInfo *)b;
    if (p1->rss > p2->rss) return -1;
    if (p1->rss < p2->rss) return 1;
    return 0;
}

pid_t interactive_process_selection() {
    ProcessInfo *processes = NULL;
    int count = 0;
    if (get_all_processes(&processes, &count) != 0) {
        printf("Failed to get process list\n");
        return -1;
    }
    qsort(processes, count, sizeof(ProcessInfo), compare_processes_rss);
    printf("\nSelect a process (PID and name):\n");
    printf("=====================================\n");
    printf("%-8s %-8s %-20s %s\n", "Index", "PID", "Name", "RSS");
    printf("-------------------------------------\n");
    for (int i = 0; i < count && i < 50; i++) {
        printf("%-8d %-8d %-20s ", i + 1, processes[i].pid, processes[i].name);
        print_size(processes[i].rss);
        printf("\n");
    }
    printf("\nEnter index (1-%d) or 0 to exit: ", (count < 50) ? count : 50);
    int choice;
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > ((count < 50) ? count : 50)) {
        free(processes);
        return -1;
    }
    pid_t selected_pid = (choice == 0) ? -1 : processes[choice - 1].pid;
    free(processes);
    return selected_pid;
}

void show_segment_detail(const MemorySegment *seg) {
    printf("\n=== Memory Segment Detail ===\n");
    printf("Address Range: %08lx-%08lx\n", seg->start, seg->end);
    printf("Size:          "); print_size(seg->size); printf("\n");
    printf("Permissions:   %s\n", seg->perms);
    printf("Type:          %s\n", get_segment_type(seg->path));
    printf("Path:          %s\n", seg->path[0] ? seg->path : "(anonymous)");
    printf("\nPermissions Analysis:\n");
    printf("  Read:    %s\n", (seg->perms[0] == 'r') ? "Yes" : "No");
    printf("  Write:   %s\n", (seg->perms[1] == 'w') ? "Yes" : "No");
    printf("  Execute: %s\n", (seg->perms[2] == 'x') ? "Yes" : "No");
    printf("  Shared:  %s\n", (seg->perms[3] == 's') ? "Yes" : "No");
    printf("  Private: %s\n", (seg->perms[3] == 'p') ? "Yes" : "No");
}

void interactive_memory_map(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;
    if (read_memory_map(pid, &segments, &count) != 0) {
        printf("Failed to read memory map\n");
        return;
    }
    int current_index = 0;
    int page_size = 20;
    while (1) {
        printf("\033[2J\033[H");
        printf("Interactive Memory Map - PID %d (%d segments total)\n", pid, count);
        printf("==================================================\n\n");
        int start = current_index;
        int end = (current_index + page_size < count) ? current_index + page_size : count;
        for (int i = start; i < end; i++) {
            MemorySegment *seg = &segments[i];
            const char *color = get_segment_color(seg->path);
            printf("%s%3d. %08lx-%08lx %-6s ", color, i + 1, seg->start, seg->end, seg->perms);
            print_size(seg->size);
            printf("  %s%s\n", seg->path[0] ? seg->path : "(anonymous)", COLOR_RESET);
        }
        printf("\nCommands: n-next, p-previous, d-detail, q-quit, g-goto: ");
        char command[10];
        if (scanf("%s", command) != 1) break;
        if (strcmp(command, "n") == 0 || strcmp(command, "next") == 0) {
            if (current_index + page_size < count) {
                current_index += page_size;
            }
        } else if (strcmp(command, "p") == 0 || strcmp(command, "prev") == 0) {
            if (current_index - page_size >= 0) {
                current_index -= page_size;
            }
        } else if (strcmp(command, "d") == 0 || strcmp(command, "detail") == 0) {
            printf("Enter segment number: ");
            int seg_num;
            if (scanf("%d", &seg_num) == 1 && seg_num >= 1 && seg_num <= count) {
                show_segment_detail(&segments[seg_num - 1]);
                printf("\nPress Enter to continue");
                getchar(); getchar();
            }
        } else if (strcmp(command, "g") == 0 || strcmp(command, "goto") == 0) {
            printf("Enter segment number: ");
            int seg_num;
            if (scanf("%d", &seg_num) == 1 && seg_num >= 1 && seg_num <= count) {
                current_index = ((seg_num - 1) / page_size) * page_size;
            }
        } else if (strcmp(command, "q") == 0 || strcmp(command, "quit") == 0) {
            break;
        }
    }
    free(segments);
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
        if (read_pss(pid, &metrics) == 0) {
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(" (more accurate)\n");
            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf("\n");
            printf("\n");
            printf("Memory Breakdown:\n");
            printf("  Shared (clean):    "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared (dirty):    "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private (clean):   "); print_size(metrics.private_clean); printf("\n");
            printf("  Private (dirty):   "); print_size(metrics.private_dirty); printf("\n");
        }
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
    }
}

void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;
    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }
    qsort(segments, count, sizeof(MemorySegment), compare_segments);
    printf("Memory Map (%d segments)\n", count);
    printf("Top 10 largest segments:\n");
    printf("%-18s %-6s %10s  %-20s %s\n", "Address Range", "Perms", "Size", "Type", "Path");
    printf("----------------------------------------------------------------\n");
    for (int i = 0; i < count && i < 10; i++) {
        MemorySegment *seg = &segments[i];
        const char *color = get_segment_color(seg->path);
        const char *type = get_segment_type(seg->path);
        printf("%s%08lx-%08lx %-6s ", color, seg->start, seg->end, seg->perms);
        print_size(seg->size);
        printf("  %-20s %s%s\n", type, seg->path[0] ? seg->path : "(anonymous)", COLOR_RESET);
    }
    unsigned long heap_size = 0, stack_size = 0, lib_size = 0, anon_size = 0, exe_size = 0, other_size = 0;
    for (int i = 0; i < count; i++) {
        MemorySegment *seg = &segments[i];
        const char *type = get_segment_type(seg->path);
        if (strcmp(type, "Heap") == 0) heap_size += seg->size;
        else if (strcmp(type, "Stack") == 0) stack_size += seg->size;
        else if (strcmp(type, "Library") == 0) lib_size += seg->size;
        else if (strcmp(type, "Anonymous") == 0) anon_size += seg->size;
        else if (strcmp(type, "Executable") == 0) exe_size += seg->size;
        else other_size += seg->size;
    }
    printf("\nMemory Summary by Type:\n");
    printf("  %sHeap:              ", COLOR_GREEN); print_size(heap_size); printf("%s\n", COLOR_RESET);
    printf("  %sStack:             ", COLOR_BLUE); print_size(stack_size); printf("%s\n", COLOR_RESET);
    printf("  %sLibraries:         ", COLOR_YELLOW); print_size(lib_size); printf("%s\n", COLOR_RESET);
    printf("  %sAnonymous:         ", COLOR_CYAN); print_size(anon_size); printf("%s\n", COLOR_RESET);
    printf("  %sExecutable:        ", COLOR_RESET); print_size(exe_size); printf("\n");
    printf("  Other:              "); print_size(other_size); printf("\n");
    free(segments);
}

void print_library_analysis(pid_t pid) {
    LibraryInfo *libraries = NULL;
    int count = 0;
    printf("\nShared Library Analysis:\n");
    printf("========================\n");
    if (analyze_libraries(pid, &libraries, &count) != 0) {
        printf("Failed to analyze libraries\n");
        return;
    }
    if (count == 0) {
        printf("No shared libraries found.\n");
        free(libraries);
        return;
    }
    printf("%-40s %10s %8s %12s %12s\n", "Library", "Size", "Sharing", "Shared Size", "Savings");
    printf("----------------------------------------------------------------------------\n");
    unsigned long total_savings = 0;
    for (int i = 0; i < count; i++) {
        LibraryInfo *lib = &libraries[i];
        unsigned long savings = lib->size - lib->shared_size;
        total_savings += savings;
        char short_path[40];
        strncpy(short_path, lib->path, sizeof(short_path) - 1);
        short_path[sizeof(short_path) - 1] = '\0';
        if (strlen(lib->path) > 39) {
            strcpy(short_path + 36, "...");
        }
        printf("%-40s ", short_path);
        print_size(lib->size); printf(" ");
        printf("%8d ", lib->sharing_count);
        print_size(lib->shared_size); printf(" ");
        print_size(savings); printf("\n");
    }
    printf("\nTotal memory savings from sharing: ");
    print_size(total_savings);
    printf("\n");
    free(libraries);
}

void watch_process(pid_t pid, int interval) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);
    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    int first_iteration = 1;
    int iteration_count = 0;
    unsigned long rss_history[100] = {0};
    FILE *csv_file = NULL;
    char csv_filename[256];
    snprintf(csv_filename, sizeof(csv_filename), "memory_pid%d.csv", pid);
    csv_file = fopen(csv_filename, "w");
    if (csv_file) {
        fprintf(csv_file, "timestamp,VSZ,RSS,PSS,minor_faults,major_faults\n");
    }
    while (1) {
        if (iteration_count % 20 == 0) {
            printf("\033[2J\033[H");
        }
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
        if (iteration_count < 100) {
            rss_history[iteration_count] = metrics.vm_rss;
        }
        if (csv_file) {
            fprintf(csv_file, "%ld,%lu,%lu,%lu,%lu,%lu\n", 
                    now, metrics.vm_size, metrics.vm_rss, metrics.pss,
                    faults.minor_faults, faults.major_faults);
            fflush(csv_file);
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
        if (metrics.pss > 0) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration) {
                unsigned long prev_pss = prev_metrics.pss;
                long delta = (long)metrics.pss - (long)prev_pss;
                if (delta != 0) {
                    printf("  (%+ld KB)", delta);
                }
            }
            printf("\n");
        }
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
        if (iteration_count > 5) {
            printf("\nRSS History (last %d samples):\n", (iteration_count < 20) ? iteration_count : 20);
            int start = (iteration_count < 20) ? 0 : iteration_count - 20;
            int samples = (iteration_count < 20) ? iteration_count : 20;
            unsigned long min_rss = rss_history[start];
            unsigned long max_rss = rss_history[start];
            for (int i = start; i < start + samples; i++) {
                if (rss_history[i] < min_rss) min_rss = rss_history[i];
                if (rss_history[i] > max_rss) max_rss = rss_history[i];
            }
            if (max_rss > min_rss) {
                int graph_height = 8;
                for (int h = graph_height; h >= 0; h--) {
                    printf("  ");
                    for (int i = start; i < start + samples; i++) {
                        double ratio = (double)(rss_history[i] - min_rss) / (max_rss - min_rss);
                        if (ratio * graph_height >= h) {
                            printf("=");
                        } else {
                            printf(" ");
                        }
                    }
                    printf("\n");
                }
                printf("  ");
                for (int i = 0; i < samples; i++) printf("─");
                printf("\n");
                printf("  Min: "); print_size(min_rss); 
                printf("  Max: "); print_size(max_rss); printf("\n");
            }
        }
        if (csv_file) {
            printf("\nData saved to: %s\n", csv_filename);
        }
        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;
        iteration_count++;
        sleep(interval);
    }
    if (csv_file) {
        fclose(csv_file);
    }
}

void compare_processes(pid_t pid1, pid_t pid2) {
    printf("Comparing processes: %d vs %d\n", pid1, pid2);
    printf("=====================================\n\n");
    MemoryMetrics m1, m2;
    char name1[256], name2[256];
    get_process_name(pid1, name1, sizeof(name1));
    get_process_name(pid2, name2, sizeof(name2));
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
    printf("%-20s %15s %15s %15s\n", "Metric", name1, name2, "Difference");
    printf("----------------------------------------------------------------\n");
    printf("%-20s ", "VSZ"); 
    print_size(m1.vm_size); printf(" "); 
    print_size(m2.vm_size); printf(" ");
    long diff_vsz = (long)m1.vm_size - (long)m2.vm_size;
    if (diff_vsz != 0) printf("(%+ld KB)", diff_vsz);
    printf("\n");
    printf("%-20s ", "RSS"); 
    print_size(m1.vm_rss); printf(" "); 
    print_size(m2.vm_rss); printf(" ");
    long diff_rss = (long)m1.vm_rss - (long)m2.vm_rss;
    if (diff_rss != 0) printf("(%+ld KB)", diff_rss);
    printf("\n");
    if (m1.pss > 0 && m2.pss > 0) {
        printf("%-20s ", "PSS"); 
        print_size(m1.pss); printf(" "); 
        print_size(m2.pss); printf(" ");
        long diff_pss = (long)m1.pss - (long)m2.pss;
        if (diff_pss != 0) printf("(%+ld KB)", diff_pss);
        printf("\n");
        unsigned long uss1 = m1.private_clean + m1.private_dirty;
        unsigned long uss2 = m2.private_clean + m2.private_dirty;
        printf("%-20s ", "USS"); 
        print_size(uss1); printf(" "); 
        print_size(uss2); printf(" ");
        long diff_uss = (long)uss1 - (long)uss2;
        if (diff_uss != 0) printf("(%+ld KB)", diff_uss);
        printf("\n");
    }
    PageFaults f1, f2;
    if (read_page_faults(pid1, &f1) == 0 && read_page_faults(pid2, &f2) == 0) {
        printf("\nPage Faults:\n");
        printf("%-20s %15lu %15lu %15ld\n", "Minor", f1.minor_faults, f2.minor_faults, 
               (long)f1.minor_faults - (long)f2.minor_faults);
        printf("%-20s %15lu %15lu %15ld\n", "Major", f1.major_faults, f2.major_faults,
               (long)f1.major_faults - (long)f2.major_faults);
    }
    printf("\nMemory Map Comparison:\n");
    MemorySegment *seg1 = NULL, *seg2 = NULL;
    int count1 = 0, count2 = 0;
    if (read_memory_map(pid1, &seg1, &count1) == 0 && 
        read_memory_map(pid2, &seg2, &count2) == 0) {
        printf("  %s: %d segments\n", name1, count1);
        printf("  %s: %d segments\n", name2, count2);
        free(seg1);
        free(seg2);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID|process_name> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --watch [interval]     Monitor process continuously (default: 1 sec)\n");
        printf("  --compare <PID2>       Compare two processes\n");
        printf("  --map                  Show detailed memory map\n");
        printf("  --libs                 Show shared library analysis\n");
        printf("  --interactive          Interactive process selection\n");
        printf("  --imap                 Interactive memory map viewer\n");
        printf("\nExamples:\n");
        printf("  %s 1234                # Show info for PID 1234\n", argv[0]);
        printf("  %s firefox             # Find and analyze firefox process\n", argv[0]);
        printf("  %s 1234 --watch        # Monitor PID 1234\n", argv[0]);
        printf("  %s 1234 --watch 5      # Monitor with 5 sec interval\n", argv[0]);
        printf("  %s 1234 --compare 5678 # Compare two processes\n", argv[0]);
        printf("  %s 1234 --map          # Show memory map\n", argv[0]);
        printf("  %s 1234 --libs         # Show library analysis\n", argv[0]);
        printf("  %s --interactive       # Interactive process selection\n", argv[0]);
        printf("  %s 1234 --imap         # Interactive memory map\n", argv[0]);
        return 1;
    }
    pid_t pid = 0;
    int interactive_mode = 0;
    int interactive_map = 0;
    if (isdigit(argv[1][0])) {
        pid = atoi(argv[1]);
    } else if (strcmp(argv[1], "--interactive") == 0) {
        interactive_mode = 1;
    } else {
        pid_t pids[10];
        int found = find_pid_by_name(argv[1], pids, 10);
        if (found <= 0) {
            fprintf(stderr, "Error: No processes found with name '%s'\n", argv[1]);
            return 1;
        }
        if (found == 1) {
            pid = pids[0];
            printf("Found process: %s (PID %d)\n", argv[1], pid);
        } else {
            printf("Found %d processes with name '%s':\n", found, argv[1]);
            for (int i = 0; i < found; i++) {
                char name[256];
                get_process_name(pids[i], name, sizeof(name));
                printf("  %d. PID %d: %s\n", i + 1, pids[i], name);
            }
            printf("Using first one (PID %d)\n", pids[0]);
            pid = pids[0];
        }
    }
    int watch_mode = 0;
    int watch_interval = 1;
    int compare_mode = 0;
    pid_t compare_pid = 0;
    int show_map = 0;
    int show_libs = 0;
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
        } else if (strcmp(argv[i], "--libs") == 0) {
            show_libs = 1;
        } else if (strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = 1;
        } else if (strcmp(argv[i], "--imap") == 0) {
            interactive_map = 1;
        }
    }
    if (interactive_mode) {
        pid = interactive_process_selection();
        if (pid == -1) {
            return 0;
        }
    }
    if (!interactive_mode && pid != 0) {
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d", pid);
        if (access(path, F_OK) != 0) {
            fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
            return 1;
        }
    }
    if (compare_mode) {
        compare_processes(pid, compare_pid);
    } else if (watch_mode) {
        watch_process(pid, watch_interval);
    } else if (interactive_map) {
        interactive_memory_map(pid);
    } else if (pid != 0) {
        print_process_info(pid);
        if (show_map) {
            printf("\n");
            print_memory_map_summary(pid);
        }
        if (show_libs) {
            print_library_analysis(pid);
        }
    } else {
        fprintf(stderr, "Error: No process specified or found.\n");
        return 1;
    }
    return 0;
}
