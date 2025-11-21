/*
 * memory_info.c - Базовый пример чтения информации о памяти процесса
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_info.c -o memory_info
 * Использование: ./memory_info [PID]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// Функция для чтения метрик из /proc/[PID]/status
void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0, vm_exe = 0, vm_lib = 0;

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

// Функция для вывода карты памяти
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

        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, path_str) >= 3) {
            
            unsigned long size_kb = (end - start) / 1024;
            const char *size_unit = "KB";
            double size_display = size_kb;
            
            if (size_kb >= 1024) {
                size_display = size_kb / 1024.0;
                size_unit = "MB";
            }
            
            printf("%08lx-%08lx %-4s %7.1f %-2s  %s\n", 
                   start, end, perms, size_display, size_unit, 
                   path_str[0] ? path_str : "(anonymous)");
        }
    }

    fclose(f);
}

// Функция для чтения PSS из smaps_rollup
void print_pss_info(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("PSS info not available (smaps_rollup not supported)\n");
        return;
    }

    char line[256];
    unsigned long pss = 0, private_clean = 0, private_dirty = 0;
    unsigned long shared_clean = 0, shared_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &pss);
        } else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &private_clean);
        } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &private_dirty);
        } else if (strncmp(line, "Shared_Clean:", 13) == 0) {
            sscanf(line, "Shared_Clean: %lu kB", &shared_clean);
        } else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
            sscanf(line, "Shared_Dirty: %lu kB", &shared_dirty);
        }
    }

    fclose(f);

    unsigned long uss = private_clean + private_dirty;
    
    printf("\nAdvanced Memory Metrics:\n");
    printf("  PSS (Proportional): %lu KB (%.1f MB)\n", pss, pss/1024.0);
    printf("  USS (Unique):       %lu KB (%.1f MB)\n", uss, uss/1024.0);
    printf("  Shared Clean:       %lu KB\n", shared_clean);
    printf("  Shared Dirty:       %lu KB\n", shared_dirty);
    printf("  Private Clean:      %lu KB\n", private_clean);
    printf("  Private Dirty:      %lu KB\n", private_dirty);
}

// Функция, демонстрирующая разные типы памяти
void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

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
    // Заполняем память, чтобы вызвать page faults
    for (size_t i = 0; i < heap_size; i += 4096) {
        heap_var[i] = 'H';
    }
    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    // 3. Anonymous mmap
    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    // Заполняем часть памяти
    memset(mmap_var, 'M', mmap_size / 10);  // Заполняем только 10%
    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    // 4. File-backed mmap
    int fd = open("test_mmap_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        size_t file_size = 5 * 1024 * 1024;  // 5 MB
        // Проверяем результат ftruncate
        if (ftruncate(fd, file_size) == -1) {
            perror("ftruncate failed");
        } else {
            void *file_mmap = mmap(NULL, file_size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
            if (file_mmap != MAP_FAILED) {
                memset(file_mmap, 'F', file_size);
                printf("4. File-backed mmap: 5 MB at %p\n", file_mmap);
                munmap(file_mmap, file_size);
            }
        }
        close(fd);
        unlink("test_mmap_file.bin");
    }

    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    // Вывести информацию о текущем процессе
    print_memory_metrics(getpid());
    print_pss_info(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    // Освободить ресурсы
    free(heap_var);
    munmap(mmap_var, mmap_size);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        // Режим: анализ заданного PID
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_pss_info(pid);
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