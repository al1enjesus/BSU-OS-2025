#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#define MB (1024 * 1024)

// Функция для вывода содержимого файла
void print_file(const char* filename) {
    printf("=== %s ===\n", filename);
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("Не могу открыть %s\n", filename);
        return;
    }
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }
    fclose(f);
    printf("\n");
}

// Функция для получения метрик памяти
void print_memory_metrics() {
    printf("=== Memory Metrics from /proc/self/status ===\n");
    
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) {
        printf("Не могу открыть /proc/self/status\n");
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "VmSize") || strstr(line, "VmRSS") || 
            strstr(line, "VmPeak") || strstr(line, "VmHWM")) {
            printf("%s", line);
        }
    }
    fclose(f);
}

// Функция для вывода карты памяти (первые строки)
void print_memory_map() {
    printf("=== Memory Map (первые 20 строк) ===\n");
    
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) {
        printf("Не могу открыть /proc/self/maps\n");
        return;
    }
    
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 20) {
        printf("%s", line);
        count++;
    }
    fclose(f);
    
    if (count == 20) {
        printf("... (выведено только первые 20 строк)\n");
    }
    printf("\n");
}

int main() {
    printf("PID: %d\n", getpid());
    printf("=== Начальное состояние ===\n");
    print_memory_metrics();
    
    printf("\nНажмите Enter чтобы продолжить...");
    getchar();
    
    // Выделяем память разными способами
    
    printf("1. Выделяем 1 MB в стеке...\n");
    char stack_array[1 * MB];
    memset(stack_array, 0xAA, sizeof(stack_array));
    
    printf("2. Выделяем 2 MB в куче (malloc)...\n");
    char* heap_array = malloc(2 * MB);
    if (heap_array) {
        memset(heap_array, 0xBB, 2 * MB);
    }
    
    printf("3. Выделяем 3 MB через mmap (анонимный)...\n");
    char* mmap_array = mmap(NULL, 3 * MB, PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_array != MAP_FAILED) {
        memset(mmap_array, 0xCC, 3 * MB);
    }
    
    printf("\n=== После выделения памяти ===\n");
    print_memory_metrics();
    
    printf("\nНажмите Enter чтобы увидеть карту памяти...");
    getchar();
    
    print_memory_map();
    
    printf("\nНажмите Enter чтобы завершить...");
    getchar();
    
    // Освобождаем ресурсы
    if (heap_array) free(heap_array);
    if (mmap_array != MAP_FAILED) munmap(mmap_array, 3 * MB);
    
    return 0;
}
