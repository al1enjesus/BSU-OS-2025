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
#include <sys/types.h>

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = -1, vm_rss = -1, vm_data = -1, vm_stk = -1, pss = -1, pc = -1, pd = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        }
        else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &vm_data);
        }
        else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &vm_stk);
        }
    }

    fclose(f);

    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &pss);
        }
        else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &pc);
        }
        else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &pd);
        }
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk/1024.0);
    printf("  PSS:            %lu KB (%.1f MB)\n", pss, pss/1024.0);
    printf("  USS:            %lu KB (%.1f MB)\n", pc + pd, (pc + pd)/1024.0);
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
    printf("%-25s %-6s %-8s %s\n", "Address Range", "Perms", "Size(KB)", "Path");
    printf("----------------------------------------------------------------\n");

    char line[512];

    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        sscanf(line, "%lx-%lx %s %*s %*s %*s %[^\n]",
           &start, &end, perms, path_str);

        unsigned long size_kb = (end - start) / 1024;

        printf("%lx-%lx %-6s %lu %s\n", start, end, perms, size_kb, path_str);
    }

    fclose(f);
}

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

    // Исправление: заполняем весь выделенный блок, а не sizeof(pointer)
    memset(heap_var, 'H', heap_size);  // 'H' для heap, чтобы вызвать page faults
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

    // Добавление: заполняем весь mmap блок, чтобы выделить физическую память
    memset(mmap_var, 'M', mmap_size);  // 'M' для mmap, вызывает page faults
    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    // TODO: Опционально - попробовать file-backed mmap
    // Создать файл, открыть, отобразить в память

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
        printf("Memory Info Demo\n");
        printf("================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        printf("Process PID=%d. Press Enter to allocate memory...\n", getpid());
        getchar();

        demonstrate_memory_types();
    }

    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте TODO в функциях print_memory_metrics() и print_memory_map()
 *
 * 2. Добейтесь корректного вывода метрик и карты памяти
 *
 * 3. Дополните demonstrate_memory_types():
 *    - Заполните heap_var данными (чтобы страницы реально выделились)
 *    - Создайте file-backed mmap (откройте файл и отобразите его)
 *
 * 4. Запустите программу и сравните вывод с системными утилитами:
 *    $ ./memory_info &
 *    $ PID=$!
 *    $ ps -o pid,vsz,rss -p $PID
 *    $ cat /proc/$PID/status | grep ^Vm
 *
 * 5. Проанализируйте:
 *    - Почему VSZ больше RSS?
 *    - Где находятся stack, heap, mmap в адресном пространстве?
 *    - Что означают разные права доступа (r-xp, rw-p)?
 *
 * 6. Дополнительно (*):
 *    - Добавьте вывод PSS из /proc/[PID]/smaps_rollup
 *    - Добавьте цветной вывод для разных типов памяти
 *    - Реализуйте группировку сегментов (все библиотеки вместе)
 */
