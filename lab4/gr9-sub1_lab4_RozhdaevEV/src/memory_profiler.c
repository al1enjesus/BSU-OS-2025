#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINE_LENGTH 1024
#define MAX_MAPS_ENTRIES 1000
#define WATCH_INTERVAL_DEFAULT 2

typedef struct {
    unsigned long vms;    
    unsigned long rss;  
    unsigned long pss;    
    unsigned long uss;  
    unsigned long shared; 
    unsigned long text;   
    unsigned long data;  
    unsigned long lib;    
    long minor_faults;    
    long major_faults;     
} MemoryMetrics;

typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[8];
    unsigned long offset;
    char dev[16];
    unsigned long inode;
    char pathname[256];
    unsigned long size;
    unsigned long rss;
    unsigned long pss;
} MemoryMapEntry;

typedef struct {
    pid_t pid;
    char name[256];
    MemoryMetrics metrics;
    MemoryMapEntry* maps;
    int maps_count;
} ProcessInfo;

int read_memory_metrics(pid_t pid, MemoryMetrics* metrics);
int read_memory_maps(pid_t pid, MemoryMapEntry** maps, int* count);
void free_memory_maps(MemoryMapEntry* maps, int count);
int read_process_name(pid_t pid, char* name, size_t name_len);

void print_basic_info(const ProcessInfo* proc);
void print_memory_map(const ProcessInfo* proc, int limit);
void print_comparison(const ProcessInfo* proc1, const ProcessInfo* proc2);
void print_watch_header(void);

const char* format_size(unsigned long bytes);
const char* format_delta(long delta, char* buffer, size_t buffer_size);
double get_time(void);
void clear_screen(void);
int parse_arguments(int argc, char* argv[], pid_t* pid1, pid_t* pid2,
    int* watch_mode, int* interval, int* show_map);

static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
    printf("\nStopping monitoring...\n");
}

int read_memory_metrics(pid_t pid, MemoryMetrics* metrics) {
    char path[256];
    FILE* f;
    char line[MAX_LINE_LENGTH];

    memset(metrics, 0, sizeof(MemoryMetrics));

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &metrics->vms);
            metrics->vms *= 1024;  
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &metrics->rss);
            metrics->rss *= 1024;
        }
        else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &metrics->data);
            metrics->data *= 1024;
        }
        else if (strncmp(line, "VmStk:", 6) == 0) {
        }
        else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line, "VmExe: %lu kB", &metrics->text);
            metrics->text *= 1024;
        }
        else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line, "VmLib: %lu kB", &metrics->lib);
            metrics->lib *= 1024;
        }
        else if (strncmp(line, "Name:", 5) == 0) {
        }
    }
    fclose(f);

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (f) {
        for (int i = 0; i < 9; i++) {
            if (fscanf(f, "%*s") == EOF) break;
        }
        fscanf(f, "%ld %*s %ld", &metrics->minor_faults, &metrics->major_faults);
        fclose(f);
    }

    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "Pss:", 4) == 0) {
                unsigned long pss_kb;
                sscanf(line, "Pss: %lu kB", &pss_kb);
                metrics->pss = pss_kb * 1024;
            }
            else if (strncmp(line, "Private_Clean:", 14) == 0 ||
                strncmp(line, "Private_Dirty:", 14) == 0) {
                unsigned long private_kb;
                sscanf(line, "%*s %lu kB", &private_kb);
                metrics->uss += private_kb * 1024;
            }
            else if (strncmp(line, "Shared_Clean:", 13) == 0 ||
                strncmp(line, "Shared_Dirty:", 13) == 0) {
                unsigned long shared_kb;
                sscanf(line, "%*s %lu kB", &shared_kb);
                metrics->shared += shared_kb * 1024;
            }
        }
        fclose(f);
    }
    else {
        metrics->pss = metrics->rss;  
        metrics->uss = metrics->rss / 2;  
    }

    return 0;
}

int read_memory_maps(pid_t pid, MemoryMapEntry** maps, int* count) {
    char path[256];
    FILE* f;
    char line[MAX_LINE_LENGTH];
    MemoryMapEntry* entries = NULL;
    int entry_count = 0;
    int capacity = 0;

    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        if (entry_count >= capacity) {
            capacity = capacity == 0 ? 50 : capacity * 2;
            MemoryMapEntry* new_entries = realloc(entries, capacity * sizeof(MemoryMapEntry));
            if (!new_entries) {
                free(entries);
                fclose(f);
                return -1;
            }
            entries = new_entries;
        }

        MemoryMapEntry* entry = &entries[entry_count];
        memset(entry, 0, sizeof(MemoryMapEntry));

        if (sscanf(line, "%lx-%lx %7s %lx %15s %lu %255[^\n]",
            &entry->start, &entry->end, entry->perms, &entry->offset,
            entry->dev, &entry->inode, entry->pathname) >= 6) {

            entry->size = entry->end - entry->start;
            entry_count++;
        }
    }

    fclose(f);

    *maps = entries;
    *count = entry_count;
    return 0;
}

void free_memory_maps(MemoryMapEntry* maps, int count) {
    free(maps);
}

int read_process_name(pid_t pid, char* name, size_t name_len) {
    char path[256];
    FILE* f;

    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    if (fgets(name, name_len, f)) {
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') {
            name[len - 1] = '\0';
        }
    }
    else {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

const char* format_size(unsigned long bytes) {
    static char buffer[32];
    const char* units[] = { "B", "KB", "MB", "GB" };
    int unit_index = 0;
    double size = bytes;

    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }

    if (unit_index == 0) {
        snprintf(buffer, sizeof(buffer), "%lu %s", bytes, units[unit_index]);
    }
    else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit_index]);
    }

    return buffer;
}

const char* format_delta(long delta, char* buffer, size_t buffer_size) {
    if (delta == 0) {
        snprintf(buffer, buffer_size, "0");
    }
    else if (delta > 0) {
        snprintf(buffer, buffer_size, "+%s", format_size(delta));
    }
    else {
        snprintf(buffer, buffer_size, "-%s", format_size(-delta));
    }
    return buffer;
}

double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void print_basic_info(const ProcessInfo* proc) {
    printf("Process: %s (PID %d)\n", proc->name, proc->pid);
    printf("=");
    for (int i = 0; i < 60; i++) printf("=");
    printf("\n\n");

    printf("Memory Metrics:\n");
    printf("  VSZ (Virtual):     %s\n", format_size(proc->metrics.vms));
    printf("  RSS (Resident):    %s\n", format_size(proc->metrics.rss));

    if (proc->metrics.pss > 0) {
        printf("  PSS (Proportional): %s (more accurate)\n", format_size(proc->metrics.pss));
    }
    if (proc->metrics.uss > 0) {
        printf("  USS (Unique):      %s\n", format_size(proc->metrics.uss));
    }

    printf("\n");
    printf("  Shared memory:     %s\n", format_size(proc->metrics.shared));
    printf("  Text (code):       %s\n", format_size(proc->metrics.text));
    printf("  Data + Heap:       %s\n", format_size(proc->metrics.data));
    printf("  Libraries:         %s\n", format_size(proc->metrics.lib));
    printf("\n");

    printf("Page Faults:\n");
    printf("  Minor: %ld\n", proc->metrics.minor_faults);
    printf("  Major: %ld\n", proc->metrics.major_faults);
}

void print_memory_map(const ProcessInfo* proc, int limit) {
    printf("\nMemory Map:");
    if (limit > 0 && proc->maps_count > limit) {
        printf(" (showing first %d of %d entries)\n", limit, proc->maps_count);
    }
    else {
        printf(" (%d entries)\n", proc->maps_count);
    }

    printf("%-18s %-6s %-8s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("-");
    for (int i = 0; i < 78; i++) printf("-");
    printf("\n");

    int show_count = (limit > 0 && proc->maps_count > limit) ? limit : proc->maps_count;

    for (int i = 0; i < show_count; i++) {
        const MemoryMapEntry* entry = &proc->maps[i];
        printf("%08lx-%08lx %-6s %-8s %s\n",
            entry->start, entry->end, entry->perms,
            format_size(entry->size), entry->pathname);
    }

    if (proc->maps_count > show_count) {
        printf("... (%d more entries)\n", proc->maps_count - show_count);
    }
}

void print_comparison(const ProcessInfo* proc1, const ProcessInfo* proc2) {
    printf("Comparing processes: %s (PID %d) vs %s (PID %d)\n",
        proc1->name, proc1->pid, proc2->name, proc2->pid);
    printf("=");
    for (int i = 0; i < 70; i++) printf("=");
    printf("\n\n");

    printf("%-20s %15s %15s %15s\n",
        "Metric",
        proc1->name,
        proc2->name,
        "Difference");
    printf("-");
    for (int i = 0; i < 70; i++) printf("-");
    printf("\n");

    char delta_buffer[32];

    long vsz_delta = proc2->metrics.vms - proc1->metrics.vms;
    printf("%-20s %15s %15s %15s\n",
        "VSZ",
        format_size(proc1->metrics.vms),
        format_size(proc2->metrics.vms),
        format_delta(vsz_delta, delta_buffer, sizeof(delta_buffer)));

    long rss_delta = proc2->metrics.rss - proc1->metrics.rss;
    printf("%-20s %15s %15s %15s\n",
        "RSS",
        format_size(proc1->metrics.rss),
        format_size(proc2->metrics.rss),
        format_delta(rss_delta, delta_buffer, sizeof(delta_buffer)));

    if (proc1->metrics.pss > 0 && proc2->metrics.pss > 0) {
        long pss_delta = proc2->metrics.pss - proc1->metrics.pss;
        printf("%-20s %15s %15s %15s\n",
            "PSS",
            format_size(proc1->metrics.pss),
            format_size(proc2->metrics.pss),
            format_delta(pss_delta, delta_buffer, sizeof(delta_buffer)));
    }

    long minor_delta = proc2->metrics.minor_faults - proc1->metrics.minor_faults;
    printf("%-20s %15ld %15ld %15+ld\n",
        "Minor Faults",
        proc1->metrics.minor_faults,
        proc2->metrics.minor_faults,
        minor_delta);

    long major_delta = proc2->metrics.major_faults - proc1->metrics.major_faults;
    printf("%-20s %15ld %15ld %15+ld\n",
        "Major Faults",
        proc1->metrics.major_faults,
        proc2->metrics.major_faults,
        major_delta);
}

void print_watch_header(void) {
    printf("%-12s %-12s %-12s %-12s %-12s %-12s\n",
        "Time", "VSZ", "RSS", "PSS", "MinorF", "MajorF");
    printf("-");
    for (int i = 0; i < 72; i++) printf("-");
    printf("\n");
}

int parse_arguments(int argc, char* argv[], pid_t* pid1, pid_t* pid2,
    int* watch_mode, int* interval, int* show_map) {
    *pid1 = 0;
    *pid2 = 0;
    *watch_mode = 0;
    *interval = WATCH_INTERVAL_DEFAULT;
    *show_map = 0;

    if (argc < 2) {
        return -1;
    }

    *pid1 = atoi(argv[1]);
    if (*pid1 <= 0) {
        fprintf(stderr, "Error: Invalid PID '%s'\n", argv[1]);
        return -1;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            *watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                *interval = atoi(argv[i + 1]);
                if (*interval <= 0) *interval = WATCH_INTERVAL_DEFAULT;
                i++;
            }
        }
        else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            *pid2 = atoi(argv[i + 1]);
            if (*pid2 <= 0) {
                fprintf(stderr, "Error: Invalid second PID '%s'\n", argv[i + 1]);
                return -1;
            }
            i++;
        }
        else if (strcmp(argv[i], "--map") == 0) {
            *show_map = 1;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s <PID> [OPTIONS]\n\n", argv[0]);
            printf("Options:\n");
            printf("  --watch [INTERVAL]    Monitor process continuously (default: %d sec)\n", WATCH_INTERVAL_DEFAULT);
            printf("  --compare PID2        Compare with another process\n");
            printf("  --map                 Show detailed memory map\n");
            printf("  --help                Show this help message\n\n");
            printf("Examples:\n");
            printf("  %s 1234                 # Show memory info for PID 1234\n", argv[0]);
            printf("  %s 1234 --watch         # Monitor PID 1234\n", argv[0]);
            printf("  %s 1234 --watch 5       # Monitor with 5 sec interval\n", argv[0]);
            printf("  %s 1234 --compare 5678  # Compare two processes\n", argv[0]);
            printf("  %s 1234 --map           # Show memory map\n", argv[0]);
            exit(0);
        }
        else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    pid_t pid1, pid2;
    int watch_mode, interval, show_map;

    if (parse_arguments(argc, argv, &pid1, &pid2, &watch_mode, &interval, &show_map) != 0) {
        fprintf(stderr, "Usage: %s <PID> [--watch] [--interval SECONDS] [--compare PID2] [--map] [--help]\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);

    char proc_name[256];
    if (read_process_name(pid1, proc_name, sizeof(proc_name)) != 0) {
        fprintf(stderr, "Error: Process %d does not exist or access denied\n", pid1);
        return 1;
    }

    if (pid2 > 0) {
        ProcessInfo proc1, proc2;

        memset(&proc1, 0, sizeof(ProcessInfo));
        memset(&proc2, 0, sizeof(ProcessInfo));

        proc1.pid = pid1;
        proc2.pid = pid2;

        read_process_name(pid1, proc1.name, sizeof(proc1.name));
        read_process_name(pid2, proc2.name, sizeof(proc2.name));

        if (read_memory_metrics(pid1, &proc1.metrics) != 0) {
            fprintf(stderr, "Error: Cannot read memory metrics for PID %d\n", pid1);
            return 1;
        }

        if (read_memory_metrics(pid2, &proc2.metrics) != 0) {
            fprintf(stderr, "Error: Cannot read memory metrics for PID %d\n", pid2);
            return 1;
        }

        print_comparison(&proc1, &proc2);
        return 0;
    }

    if (watch_mode) {
        ProcessInfo proc;
        MemoryMetrics prev_metrics;
        int first_run = 1;

        memset(&proc, 0, sizeof(ProcessInfo));
        proc.pid = pid1;
        read_process_name(pid1, proc.name, sizeof(proc.name));

        printf("Monitoring %s (PID %d) - Update every %d seconds\n", proc.name, pid1, interval);
        printf("Press Ctrl+C to stop\n\n");

        print_watch_header();

        while (keep_running) {
            if (read_memory_metrics(pid1, &proc.metrics) != 0) {
                fprintf(stderr, "Error: Cannot read memory metrics for PID %d\n", pid1);
                break;
            }

            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

            printf("%-12s %-12s %-12s %-12s %-12ld %-12ld",
                time_str,
                format_size(proc.metrics.vms),
                format_size(proc.metrics.rss),
                proc.metrics.pss > 0 ? format_size(proc.metrics.pss) : "N/A",
                proc.metrics.minor_faults,
                proc.metrics.major_faults);

            if (!first_run) {
                char delta_buffer[32];
                printf(" | VSZ:%s RSS:%s",
                    format_delta(proc.metrics.vms - prev_metrics.vms, delta_buffer, sizeof(delta_buffer)),
                    format_delta(proc.metrics.rss - prev_metrics.rss, delta_buffer, sizeof(delta_buffer)));
            }

            printf("\n");
            fflush(stdout);

            prev_metrics = proc.metrics;
            first_run = 0;

            sleep(interval);
        }

        printf("\nMonitoring stopped.\n");
        return 0;
    }

    ProcessInfo proc;
    memset(&proc, 0, sizeof(ProcessInfo));
    proc.pid = pid1;

    if (read_process_name(pid1, proc.name, sizeof(proc.name)) != 0) {
        fprintf(stderr, "Error: Cannot read process name for PID %d\n", pid1);
        return 1;
    }

    if (read_memory_metrics(pid1, &proc.metrics) != 0) {
        fprintf(stderr, "Error: Cannot read memory metrics for PID %d\n", pid1);
        return 1;
    }

    print_basic_info(&proc);

    if (show_map) {
        if (read_memory_maps(pid1, &proc.maps, &proc.maps_count) == 0) {
            print_memory_map(&proc, 20);  
            free_memory_maps(proc.maps, proc.maps_count);
        }
        else {
            printf("\nCannot read memory map (access denied)\n");
        }
    }

    return 0;
}