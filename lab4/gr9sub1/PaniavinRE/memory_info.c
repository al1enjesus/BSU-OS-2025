/*
 * memory_info.c - Базовый пример чтения информации о памяти процесса
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_info.c -o memory_info
 * Использование: ./memory_info [PID]
 *
 * Демонстрирует:
 * - Чтение VSZ, RSS из /proc/[PID]/status
 * - Разные типы выделения памяти (stack, heap, mmap)
 * - Отображение карты памяти из /proc/[PID]/maps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
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
            sscanf(line + 7, "%lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line + 7, "%lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line + 6, "%lu kB", &vm_stk);
        }
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size / 1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss / 1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data / 1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk / 1024.0);
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
        unsigned long start, end, offset, inode;
        char perms[5], dev[6], path_str[256] = "";
        if (sscanf(line, "%lx-%lx %4s %lx %5s %lu %[^\n]", &start, &end, perms, &offset, dev, &inode, path_str) < 6) {
            continue;
        }
        unsigned long size_kb = (end - start) / 1024;
        printf("%08lx-%08lx %s %8lu KB %s\n", start, end, perms, size_kb, path_str);
    }

    fclose(f);
}

void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    /* 1. Стек (stack) */
    char stack_var[1024];  // Локальная переменная
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    /* 2. Heap (через malloc) */
    size_t heap_size = 10 * 1024 * 1024;  // 10 MB
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }
    memset(heap_var, 'H', heap_size);  // Заполняем, чтобы вызвать page faults
    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    /* 3. Anonymous mmap */
    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    memset(mmap_var, 'M', mmap_size);  // Заполняем
    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    /* 4. File-backed mmap (опционально) */
    int fd = open("testfile.bin", O_CREAT | O_RDWR, 0666);
    if (fd != -1) {
        if (ftruncate(fd, 1024 * 1024) == 0) {  // 1 MB файл
            void *file_mmap = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (file_mmap != MAP_FAILED) {
                memset(file_mmap, 'F', 1024 * 1024);
                printf("4. File-backed mmap: 1 MB at %p\n", file_mmap);
                munmap(file_mmap, 1024 * 1024);
            } else {
                perror("mmap failed");
            }
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
    munmap(mmap_var, mmap_size);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
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
