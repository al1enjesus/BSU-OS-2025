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

// Функция для красивого вывода размера
void print_size_kb(unsigned long kb) {
    if (kb < 1024) {
        printf("%lu KB", kb);
    } else if (kb < 1024 * 1024) {
        printf("%.1f MB", kb / 1024.0);
    } else {
        printf("%.1f GB", kb / (1024.0 * 1024.0));
    }
}

// TODO: Реализовать функцию для чтения метрик из /proc/[PID]/status
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
    unsigned long pss = 0, private_clean = 0, private_dirty = 0;

    // Прочитать файл построчно и извлечь метрики:
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &vm_stk) == 1) continue;
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk/1024.0);

    // Добавим PSS/USS из smaps_rollup (Задание А)
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Pss: %lu kB", &pss) == 1) continue;
            if (sscanf(line, "Private_Clean: %lu kB", &private_clean) == 1) continue;
            if (sscanf(line, "Private_Dirty: %lu kB", &private_dirty) == 1) continue;
        }
        fclose(f);

        unsigned long uss = private_clean + private_dirty;
        printf("  PSS (Proportional): %lu KB (%.1f MB)\n", pss, pss/1024.0);
        printf("  USS (Unique):   %lu KB (%.1f MB)\n", uss, uss/1024.0);
    } else {
        printf("  (Could not open smaps_rollup for PSS/USS)\n");
    }
}

// TODO: Реализовать функцию для вывода карты памяти
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
    printf("----------------------------------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        // Распарсить строку
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
               &start, &end, perms, path_str);

        // Вычислить размер
        unsigned long size_kb = (end - start) / 1024;

        // Вывести информацию
        printf("%012lx-%012lx %-6s ", start, end, perms);
        print_size_kb(size_kb);
        // Добавим отступ для выравнивания
        if (size_kb < 1024) printf("    ");
        else if (size_kb < 1024*1024) printf("  ");

        printf("   %s\n", path_str[0] ? path_str : "(anonymous)");
    }

    fclose(f);
}

// TODO: Реализовать функцию, демонстрирующую разные типы памяти
void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    // 1. Стек (stack)
    char stack_var[1024];  // Локальная переменная
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    // 2. Heap (через malloc)
    size_t heap_size = 10 * 1024 * 1024;  // 10 MB
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }
    // Заполним память, чтобы вызвать page faults и занять RSS
    memset(heap_var, 'H', heap_size);
    printf("2. Heap allocated and touched: 10 MB at %p\n", (void*)heap_var);

    // 3. Anonymous mmap (аналог malloc для больших блоков)
    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    // Также заполним
    memset(mmap_var, 'M', mmap_size);
    printf("3. Anonymous mmap allocated and touched: 50 MB at %p\n", mmap_var);


    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    // Вывести информацию о текущем процессе
    print_memory_metrics(getpid());
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
        print_memory_map(pid);
    } else {
        // Режим: демонстрация на себе
        printf("Memory Info Demo (Variant 1, Task A)\n");
        printf("======================================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }

    return 0;
}