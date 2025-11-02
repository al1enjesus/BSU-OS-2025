#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>

typedef struct {
    unsigned long vsz;
    unsigned long rss;
    unsigned long pss;
    unsigned long uss;
    unsigned long min_flt;
    unsigned long maj_flt;
    char comm[256];
} ProcessInfo;

void print_human_readable(unsigned long bytes) {
    if (bytes < 1024)
        printf("%lu B", bytes);
    else if (bytes < 1024 * 1024)
        printf("%.1f KB", bytes / 1024.0);
    else if (bytes < 1024 * 1024 * 1024)
        printf("%.1f MB", bytes / (1024.0 * 1024.0));
    else
        printf("%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

int get_process_info(pid_t pid, ProcessInfo *info) {
    char path[256];
    
    // Чтение статуса
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *status = fopen(path, "r");
    if (!status) return 0;
    
    char line[256];
    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line + 5, "%255s", info->comm);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line + 7, "%lu", &info->vsz);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", &info->rss);
        } else if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
            // Пропускаем
        }
    }
    fclose(status);
    
    // Чтение статистики page faults
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *stat = fopen(path, "r");
    if (stat) {
        char line[1024];
        if (fgets(line, sizeof(line), stat)) {
            char *token = strtok(line, " ");
            for (int i = 1; i <= 12; i++) {
                token = strtok(NULL, " ");
                if (i == 10) info->min_flt = strtoul(token, NULL, 10);
                if (i == 12) info->maj_flt = strtoul(token, NULL, 10);
            }
        }
        fclose(stat);
    }
    
    // Чтение PSS из smaps_rollup
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *smaps = fopen(path, "r");
    if (smaps) {
        char line[256];
        while (fgets(line, sizeof(line), smaps)) {
            if (strstr(line, "Pss:")) {
                sscanf(line, "Pss: %lu", &info->pss);
            } else if (strstr(line, "Private_Clean:") || strstr(line, "Private_Dirty:")) {
                unsigned long private;
                sscanf(line, "%*s %lu", &private);
                info->uss += private;
            }
        }
        fclose(smaps);
    }
    
    return 1;
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    printf("\n--- Memory Map (first 20 segments) ---\n");
    FILE *maps = fopen(path, "r");
    if (maps) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), maps) && count < 20) {
            printf("%s", line);
            count++;
        }
        fclose(maps);
    }
}

void analyze_memory_segments(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    printf("\n--- Memory Segments Analysis ---\n");
    
    FILE *maps = fopen(path, "r");
    if (!maps) return;
    
    char line[512];
    unsigned long heap_size = 0, stack_size = 0, lib_size = 0, anon_size = 0;
    
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[8], pathname[256];
        
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, pathname) >= 3) {
            unsigned long size = end - start;
            
            if (strstr(pathname, "[heap]")) {
                heap_size += size;
            } else if (strstr(pathname, "[stack]")) {
                stack_size += size;
            } else if (strstr(pathname, ".so")) {
                lib_size += size;
            } else if (strlen(pathname) == 0 || strstr(pathname, "//")) {
                anon_size += size;
            }
        }
    }
    fclose(maps);
    
    printf("Heap:    "); print_human_readable(heap_size); printf("\n");
    printf("Stack:   "); print_human_readable(stack_size); printf("\n");
    printf("Libraries:"); print_human_readable(lib_size); printf("\n");
    printf("Anonymous:"); print_human_readable(anon_size); printf("\n");
}

void print_usage() {
    printf("Usage: memory_profiler <PID> [--watch] [--compare PID2]\n");
    printf("Options:\n");
    printf("  --watch    Monitor memory every second\n");
    printf("  --compare  Compare with another process\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    int watch_mode = 0;
    pid_t compare_pid = 0;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
        } else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            compare_pid = atoi(argv[++i]);
        }
    }
    
    do {
        ProcessInfo info = {0};
        if (!get_process_info(pid, &info)) {
            printf("Process %d not found\n", pid);
            break;
        }
        
        printf("\n=== Process: %s (PID %d) ===\n", info.comm, pid);
        printf("Virtual Size (VSZ):  "); print_human_readable(info.vsz * 1024); printf("\n");
        printf("Resident Set (RSS):  "); print_human_readable(info.rss * 1024); printf("\n");
        printf("Proportional (PSS):  "); print_human_readable(info.pss * 1024); printf("\n");
        printf("Unique Set (USS):    "); print_human_readable(info.uss * 1024); printf("\n");
        
        printf("\nPage Faults:\n");
        printf("  Minor: %lu\n", info.min_flt);
        printf("  Major: %lu\n", info.maj_flt);
        
        analyze_memory_segments(pid);
        
        if (compare_pid) {
            ProcessInfo info2 = {0};
            if (get_process_info(compare_pid, &info2)) {
                printf("\n=== Comparison with PID %d ===\n", compare_pid);
                printf("VSZ:  PID%d=", pid); print_human_readable(info.vsz * 1024);
                printf(" vs PID%d=", compare_pid); print_human_readable(info2.vsz * 1024);
                printf("\n");
                
                printf("RSS:  PID%d=", pid); print_human_readable(info.rss * 1024);
                printf(" vs PID%d=", compare_pid); print_human_readable(info2.rss * 1024);
                printf("\n");
            }
        }
        
        if (argc == 2) { // Только если не в watch mode
            print_memory_map(pid);
        }
        
        if (watch_mode) {
            sleep(1);
            printf("\n==================================================\n");
        }
    } while (watch_mode);
    
    return 0;
}
