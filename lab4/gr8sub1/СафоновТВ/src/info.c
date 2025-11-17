#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &vm_stk);
        }
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk/1024.0);
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    printf("\nMemory Map:\n");
    printf("%-18s %-6s %-8s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        int fields = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                           &start, &end, perms, path_str);

        unsigned long size_bytes = end - start;
        char size_str[32];
        
        if (size_bytes >= 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1fMB", size_bytes / (1024.0 * 1024.0));
        } else if (size_bytes >= 1024) {
            snprintf(size_str, sizeof(size_str), "%.1fKB", size_bytes / 1024.0);
        } else {
            snprintf(size_str, sizeof(size_str), "%luB", size_bytes);
        }

        if (fields < 4 || path_str[0] == '\0') {
            if (strstr(line, "[heap]")) {
                strcpy(path_str, "[heap]");
            } else if (strstr(line, "[stack]")) {
                strcpy(path_str, "[stack]");
            } else if (strstr(line, "[vdso]")) {
                strcpy(path_str, "[vdso]");
            } else if (strstr(line, "[vsyscall]")) {
                strcpy(path_str, "[vsyscall]");
            } else {
                strcpy(path_str, "[anonymous]");
            }
        }

        printf("%08lx-%08lx %-6s %-8s %s\n", start, end, perms, size_str, path_str);
    }

    fclose(f);
}

int create_temp_file(const char* filename, size_t size) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }
    
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        close(fd);
        return -1;
    }
    
    return fd;
}

void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    size_t heap_size = 10 * 1024 * 1024;
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }

    memset(heap_var, 'H', heap_size);
    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    size_t mmap_size = 50 * 1024 * 1024;
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }

    memset(mmap_var, 'M', mmap_size);
    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    const char* temp_filename = "/tmp/memory_info_demo.dat";
    size_t file_size = 5 * 1024 * 1024;
    void *file_mmap = MAP_FAILED;
    int fd = -1;
    
    fd = create_temp_file(temp_filename, file_size);
    if (fd != -1) {
        file_mmap = mmap(NULL, file_size, PROT_READ | PROT_WRITE, 
                              MAP_SHARED, fd, 0);
        if (file_mmap != MAP_FAILED) {
            memset(file_mmap, 'F', file_size);
            printf("4. File-backed mmap: 5 MB at %p (file: %s)\n", file_mmap, temp_filename);
        } else {
            perror("file mmap failed");
        }
        close(fd);
    }

    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    print_memory_metrics(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    free(heap_var);
    
    if (munmap(mmap_var, mmap_size) == -1) {
        perror("munmap anonymous failed");
    }
    
    if (file_mmap != MAP_FAILED) {
        if (munmap(file_mmap, file_size) == -1) {
            perror("munmap file-backed failed");
        }
    }
}

void print_detailed_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen smaps failed");
        return;
    }

    char line[256];
    unsigned long pss = 0, private_clean = 0, private_dirty = 0;
    unsigned long current_pss = 0, current_private_clean = 0, current_private_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &current_pss);
            pss += current_pss;
        } else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &current_private_clean);
            private_clean += current_private_clean;
        } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &current_private_dirty);
            private_dirty += current_private_dirty;
        }
    }

    fclose(f);

    unsigned long uss = private_clean + private_dirty;
    
    printf("  PSS (Proportional): %lu KB (%.1f MB)\n", pss, pss/1024.0);
    printf("  USS (Unique):       %lu KB (%.1f MB)\n", uss, uss/1024.0);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_detailed_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        printf("Memory Info Demo\n");
        printf("================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }

    return 0;
}