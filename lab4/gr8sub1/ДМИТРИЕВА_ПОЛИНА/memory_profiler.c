#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_RESET   "\x1b[0m"

// Простая функция проверки malloc
void check_malloc_result(void *ptr) {
    if (!ptr) {
        fprintf(stderr, COLOR_RED "malloc failed\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }
}

// Человеко-читаемый вывод размера
void print_size_human(unsigned long bytes) {
    if (bytes < 1024) {
        printf("%4lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        printf("%6.1f KB", bytes / 1024.0);
    } else {
        printf("%6.1f MB", bytes / (1024.0 * 1024.0));
    }
}

// Чтение основных метрик из /proc/self/status
void read_memory_info() {
    printf(COLOR_BLUE "\n=== Информация о памяти ===\n" COLOR_RESET);
    
    FILE *f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmSize:", 7) == 0 ||
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmData:", 7) == 0) {
                printf("%s", line);
            }
        }
        fclose(f);
    }
}

// Демонстрация разных типов памяти (УПРОЩЕННАЯ)
void demonstrate_memory_types() {
    printf(COLOR_BLUE "\n=== Демонстрация выделения памяти ===\n" COLOR_RESET);
    
    // 1. Stack
    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. " COLOR_GREEN "Stack:" COLOR_RESET " 1 KB at %p\n", (void*)stack_var);

    // 2. Heap (malloc)
    size_t heap_size = 2 * 1024 * 1024;  // 2 MB
    char *heap_var = malloc(heap_size);
    check_malloc_result(heap_var);
    
    // Заполняем для реального выделения страниц
    printf("2. " COLOR_GREEN "Heap:" COLOR_RESET " 2 MB at %p\n", (void*)heap_var);
    printf("   Заполняем память...\n");
    for (size_t i = 0; i < heap_size; i += 4096) {
        heap_var[i] = 'H';
    }

    // 3. Anonymous mmap (БЕЗ файла - проще!)
    size_t mmap_size = 5 * 1024 * 1024;  // 5 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        printf(COLOR_RED "3. mmap failed!\n" COLOR_RESET);
    } else {
        printf("3. " COLOR_GREEN "Anonymous mmap:" COLOR_RESET " 5 MB at %p\n", mmap_var);
        printf("   Заполняем память...\n");
        memset(mmap_var, 'M', mmap_size);
    }

    printf(COLOR_YELLOW "\nПамять выделена. Проверяем метрики:\n" COLOR_RESET);
    
    // Показать метрики
    read_memory_info();
    
    printf(COLOR_YELLOW "\nОсвобождаем память...\n" COLOR_RESET);
    
    // Освобождение
    free(heap_var);
    if (mmap_var != MAP_FAILED) {
        munmap(mmap_var, mmap_size);
    }
    
    printf(COLOR_GREEN "Память освобождена!\n" COLOR_RESET);
}

int main() {
    printf(COLOR_GREEN "Memory Profiler - Лабораторная работа №4\n" COLOR_RESET);
    printf("Дмитриева Полина, gr8sub1\n");
    
    demonstrate_memory_types();
    
    printf(COLOR_GREEN "\nПрограмма завершена успешно!\n" COLOR_RESET);
    return 0;
}
