#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

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
            sscanf(line, "VmSize:%lu", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS:%lu", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData:%lu", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk:%lu", &vm_stk);
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
    printf("%-18s %-6s %-10s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("---------------------------------------------------------------------\n");

    char line[512];
    
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";
        char addr_range[20], size_str[32];

        int fields = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                            &start, &end, perms, path_str);

        if (fields < 3) {
            continue;
        }

        unsigned long size_bytes = end - start;
        if (size_bytes == 0) {
             snprintf(size_str, sizeof(size_str), "0 B");
        } else if (size_bytes < 1024) {
            snprintf(size_str, sizeof(size_str), "%lu B", size_bytes);
        } else if (size_bytes < (1024 * 1024)) {
            snprintf(size_str, sizeof(size_str), "%.1f KB", size_bytes / 1024.0);
        } else if (size_bytes < (1024ULL * 1024 * 1024)) {
             snprintf(size_str, sizeof(size_str), "%.1f MB", size_bytes / (1024.0 * 1024.0));
        } else {
             snprintf(size_str, sizeof(size_str), "%.1f GB", size_bytes / (1024.0 * 1024.0 * 1024.0));
        }

        snprintf(addr_range, sizeof(addr_range), "%lx-%lx", start, end);
        
        printf("%-18s %-6s %-10s %s\n", addr_range, perms, size_str, path_str);
    }

    printf("\n");
    fclose(f);
}

void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");
    
    printf("Before allocation:\n");
    print_memory_metrics(getpid());
    print_memory_map(getpid());

    // 1. Стек (stack)
    char stack_var[1024]; 
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    // 2. Heap (через malloc)
    size_t heap_size = 10 * 1024 * 1024;  // 10 MB
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }

    printf("\n   Touching heap memory (10 MB) to trigger a page fault...");
    memset(heap_var, 'H', heap_size);
    printf(" heap touch complete.\n");

    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    // 3. Anonymous mmap (аналог malloc для больших блоков)
    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    
    printf("\n   Touching anonymous mmap (50 MB) to trigger a page fault...");
    memset(mmap_var, 'M', mmap_size);
    printf(" mmap touch complete.\n");

    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    // file-backed mmap
    const char *filename = "mmap_file.txt";
    size_t file_mmap_size = 4096; // 1 страница
    void *file_map = MAP_FAILED;
    int fd = -1;

    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for mmap");
    } else {
        if (ftruncate(fd, file_mmap_size) == -1) {
            perror("ftruncate");
        } else {
            file_map = mmap(NULL, file_mmap_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
            
            if (file_map != MAP_FAILED) {
                printf("4. File-backed mmap: %lu bytes at %p\n", file_mmap_size, file_map);
                strcpy((char*)file_map, "hello from file-backed mmap!");
                printf("   Mapped content: '%s'\n", (char*)file_map);
            } else {
                perror("file mmap failed");
            }
        }
        close(fd);
    }


    printf("After allocation:\n");
    print_memory_metrics(getpid());
    print_memory_map(getpid());

    // Освободить ресурсы
    free(heap_var);
    munmap(mmap_var, mmap_size);
    
    if (file_map != MAP_FAILED) {
        munmap(file_map, file_mmap_size);
    }
    remove(filename); // Удаляем временный файл

    printf("Memory freed. Exiting.\n");
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        // Режим: анализ заданного PID
        pid_t pid = atoi(argv[1]);
        if (pid <= 0) {
            fprintf(stderr, "Invalid PID: %s\n", argv[1]);
            return 1;
        }
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        // Режим: демонстрация на себе
        printf("Memory Info Demo\n");
        printf("================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }

    return 0;
}