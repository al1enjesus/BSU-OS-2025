
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "fopen failed: %s\n", strerror(errno));
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;
    unsigned long vm_exe = 0, vm_lib = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &vm_stk);
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line, "VmExe: %lu kB", &vm_exe);
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line, "VmLib: %lu kB", &vm_lib);
        }
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk/1024.0);
    printf("  Text (code):    %lu KB (%.1f MB)\n", vm_exe, vm_exe/1024.0);
    printf("  Libraries:      %lu KB (%.1f MB)\n", vm_lib, vm_lib/1024.0);
}

void print_pss_info(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "PSS info not available: %s\n", strerror(errno));
        return;
    }

    char line[256];
    unsigned long pss = 0, shared_clean = 0, shared_dirty = 0;
    unsigned long private_clean = 0, private_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &pss);
        } else if (strncmp(line, "Shared_Clean:", 13) == 0) {
            sscanf(line, "Shared_Clean: %lu kB", &shared_clean);
        } else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
            sscanf(line, "Shared_Dirty: %lu kB", &shared_dirty);
        } else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &private_clean);
        } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &private_dirty);
        }
    }

    fclose(f);

    unsigned long uss = private_clean + private_dirty;
    unsigned long shared_total = shared_clean + shared_dirty;

    printf("\nAdvanced Memory Metrics (PSS/USS):\n");
    printf("  PSS (Proportional): %lu KB (%.1f MB)\n", pss, pss/1024.0);
    printf("  USS (Unique):       %lu KB (%.1f MB)\n", uss, uss/1024.0);
    printf("  Shared Total:       %lu KB (%.1f MB)\n", shared_total, shared_total/1024.0);
    printf("  Private Clean:      %lu KB\n", private_clean);
    printf("  Private Dirty:      %lu KB\n", private_dirty);
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "fopen failed: %s\n", strerror(errno));
        return;
    }

    printf("\nMemory Map (first 20 segments):\n");
    printf("%-18s %-6s %-8s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    char line[512];
    int count = 0;
    
    while (fgets(line, sizeof(line), f) && count < 20) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, path_str) >= 3) {
            
            unsigned long size_kb = (end - start) / 1024;
            const char *display_path = path_str[0] ? path_str : "[anonymous]";
            
            if (strstr(display_path, "[heap]")) {
                printf("\033[32m");
            } else if (strstr(display_path, "[stack]")) {
                printf("\033[34m");
            } else if (strstr(display_path, ".so")) {
                printf("\033[33m");
            }
            
            printf("%08lx-%08lx %-4s %6lu KB  %s\033[0m\n", 
                   start, end, perms, size_kb, display_path);
            count++;
        }
    }

    if (count == 20) {
        printf("... (more segments available, use 'cat /proc/%d/maps' to see all)\n", pid);
    }

    fclose(f);
}

void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    size_t heap_size = 10 * 1024 * 1024;
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        return;
    }
    
    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);
    printf("   Filling heap memory to trigger page allocation...\n");
    for (size_t i = 0; i < heap_size; i += 4096) {
        heap_var[i] = 'H';
    }

    size_t mmap_size = 50 * 1024 * 1024;
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        free(heap_var);
        return;
    }

    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);
    printf("   Filling mmap memory to trigger page allocation...\n");
    for (size_t i = 0; i < mmap_size; i += 4096) {
        ((char*)mmap_var)[i] = 'M';
    }

    printf("4. Creating file-backed mmap...\n");
    int fd = open("test_mmap_file.bin", O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, 1024 * 1024) == -1) {
            fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
            close(fd);
        } else {
            void *file_mmap = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
            if (file_mmap != MAP_FAILED) {
                printf("   File-backed mmap: 1 MB at %p\n", file_mmap);
                strncpy((char*)file_mmap, "Hello from file-backed mmap!", 1024 * 1024 - 1);
                ((char*)file_mmap)[1024 * 1024 - 1] = '\0';
                munmap(file_mmap, 1024 * 1024);
            } else {
                fprintf(stderr, "file mmap failed: %s\n", strerror(errno));
            }
            close(fd);
        }
    } else {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
    }

    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    print_memory_metrics(getpid());
    print_pss_info(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    free(heap_var);
    munmap(mmap_var, mmap_size);
    printf("Memory freed.\n");
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_pss_info(pid);
        print_memory_map(pid);
    } else {
        printf("Memory Info Demo - Variant 1\n");
        printf("=============================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }

    return 0;
}
