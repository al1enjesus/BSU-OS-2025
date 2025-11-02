#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#define ARRAY_SIZE (10 * 1024 * 1024)  // 10 MB

// Функция для вывода page faults
void print_page_faults(const char* label) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    printf("%s:\n", label);
    printf("  Minor faults: %ld\n", usage.ru_minflt);
    printf("  Major faults: %ld\n", usage.ru_majflt);
}

// Функция для последовательного доступа
void sequential_access(char* arr, size_t size) {
    printf("=== Последовательный доступ ===\n");
    print_page_faults("До последовательного доступа");
    
    clock_t start = clock();
    
    // Доступ к каждой странице (4 KB)
    for (size_t i = 0; i < size; i += 4096) {
        arr[i] = (char)(i % 256);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("После последовательного доступа");
    printf("Время: %.3f секунд\n", time_spent);
}

// Функция для случайного доступа
void random_access(char* arr, size_t size) {
    printf("\n=== Случайный доступ ===\n");
    print_page_faults("До случайного доступа");
    
    // Инициализируем генератор случайных чисел
    srand(time(NULL));
    
    clock_t start = clock();
    
    // 1000 случайных обращений
    for (int i = 0; i < 1000; i++) {
        size_t index = (size_t)rand() % size;
        arr[index] = (char)(i % 256);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("После случайного доступа");
    printf("Время: %.3f секунд\n", time_spent);
}

int main() {
    size_t size = ARRAY_SIZE;
    
    printf("Выделяем %zu MB памяти...\n", size / (1024 * 1024));
    print_page_faults("Начальное состояние");
    
    // Выделяем память
    char* arr = malloc(size);
    if (!arr) {
        perror("malloc failed");
        return 1;
    }
    
    printf("\nПосле malloc (до любого доступа):\n");
    print_page_faults("После malloc");
    
    // Разные паттерны доступа
    
    // 1. Последовательный доступ
    sequential_access(arr, size);
    
    // 2. Случайный доступ
    random_access(arr, size);
    
    // 3. Повторный последовательный доступ
    printf("\n=== Повторный последовательный доступ ===\n");
    sequential_access(arr, size);
    
    // Освобождаем память
    free(arr);
    
    printf("\n=== Финальное состояние ===\n");
    print_page_faults("После освобождения памяти");
    
    return 0;
}
