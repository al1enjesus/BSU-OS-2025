#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>  // Добавлен для getpid() и sleep()
#include <sys/types.h> // Добавлен для pid_t

#define SIZE (100 * 1024 * 1024) // 100 MB
#define PAGE_SIZE 4096

// Функция для вывода page faults
void print_page_faults(const char* description) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    printf("%s:\n", description);
    printf("  Минорные page faults: %ld\n", usage.ru_minflt);
    printf("  Мажорные page faults: %ld\n", usage.ru_majflt);
    printf("  Всего page faults: %ld\n", usage.ru_minflt + usage.ru_majflt);
}

// Функция для чтения page faults из /proc/self/stat
void print_proc_page_faults(const char* description) {
    char line[1024];
    FILE *stat_file = fopen("/proc/self/stat", "r");
    
    printf("%s:\n", description);
    
    if (stat_file && fgets(line, sizeof(line), stat_file)) {
        char *token = strtok(line, " ");
        for (int i = 1; i <= 12; i++) {
            token = strtok(NULL, " ");
            if (i == 10) printf("  Minor faults (from /proc): %s\n", token);
            if (i == 12) printf("  Major faults (from /proc): %s\n", token);
        }
        fclose(stat_file);
    }
}

// Последовательный доступ к памяти
void sequential_access(char *arr, size_t size) {
    printf("\n--- ПОСЛЕДОВАТЕЛЬНЫЙ ДОСТУП ---\n");
    printf("Обращаемся к каждой странице (4KB) последовательно...\n");
    
    print_page_faults("До последовательного доступа");
    print_proc_page_faults("До последовательного доступа (/proc)");
    
    clock_t start = clock();
    
    // Доступ к каждой странице
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        arr[i] = (char)(i % 256); // Записываем данные
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("После последовательного доступа");
    print_proc_page_faults("После последовательного доступа (/proc)");
    printf("Время выполнения: %.3f секунд\n", time_spent);
}

// Случайный доступ к памяти
void random_access(char *arr, size_t size) {
    printf("\n--- СЛУЧАЙНЫЙ ДОСТУП ---\n");
    printf("Обращаемся к случайным страницам...\n");
    
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    print_page_faults("До случайного доступа");
    print_proc_page_faults("До случайного доступа (/proc)");
    
    clock_t start = clock();
    
    // 100,000 случайных обращений
    int accesses = 100000;
    for (int i = 0; i < accesses; i++) {
        size_t index = (size_t)rand() % size;
        arr[index] = (char)(rand() % 256);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("После случайного доступа");
    print_proc_page_faults("После случайного доступа (/proc)");
    printf("Время выполнения: %.3f секунд\n", time_spent);
    printf("Количество обращений: %d\n", accesses);
}

// Доступ с разным шагом
void strided_access(char *arr, size_t size) {
    printf("\n--- ДОСТУП С РАЗНЫМ ШАГОМ ---\n");
    
    int strides[] = {PAGE_SIZE, PAGE_SIZE * 2, PAGE_SIZE * 4, PAGE_SIZE * 16};
    int num_strides = sizeof(strides) / sizeof(strides[0]);
    
    for (int s = 0; s < num_strides; s++) {
        printf("\nШаг: %d байт (%d страниц)\n", strides[s], strides[s] / PAGE_SIZE);
        
        print_page_faults("До доступа");
        
        clock_t start = clock();
        
        for (size_t i = 0; i < size; i += strides[s]) {
            arr[i] = (char)(i % 256);
        }
        
        clock_t end = clock();
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        
        print_page_faults("После доступа");
        printf("Время выполнения: %.3f секунд\n", time_spent);
    }
}

// Функция для создания нагрузки на память
void memory_stress_test(char *arr, size_t size) {
    printf("\n--- ТЕСТ НАГРУЗКИ НА ПАМЯТЬ ---\n");
    
    print_page_faults("До теста нагрузки");
    
    clock_t start = clock();
    
    // Интенсивная работа с памятью
    for (int cycle = 0; cycle < 3; cycle++) {
        printf("Цикл %d...\n", cycle + 1);
        
        // Запись во всю память
        for (size_t i = 0; i < size; i += PAGE_SIZE) {
            arr[i] = (char)((i + cycle) % 256);
        }
        
        // Чтение из памяти
        char sum = 0;
        for (size_t i = 0; i < size; i += PAGE_SIZE) {
            sum += arr[i];
        }
        // Используем sum чтобы компилятор не оптимизировал цикл
        if (sum == 0) { /* ничего не делаем */ }
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("После теста нагрузки");
    printf("Общее время теста: %.3f секунд\n", time_spent);
}

// Функция для вывода информации о системе
void print_system_info() {
    printf("=== ИНФОРМАЦИЯ О СИСТЕМЕ ===\n");
    printf("PID: %d\n", getpid());
    printf("Размер страницы: %d байт\n", PAGE_SIZE);
    printf("Выделяемая память: %d MB\n", SIZE / (1024 * 1024));
    printf("Количество страниц: %d\n", (int)(SIZE / PAGE_SIZE));
    
    // Информация о памяти системы
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        printf("\n--- Информация о памяти системы ---\n");
        while (fgets(line, sizeof(line), meminfo)) {
            if (strstr(line, "MemTotal") || strstr(line, "MemFree") || 
                strstr(line, "SwapTotal") || strstr(line, "SwapFree")) {
                printf("%s", line);
            }
        }
        fclose(meminfo);
    }
}

int main() {
    printf("🚀 ЗАДАНИЕ C: АНАЛИЗ PAGE FAULTS\n");
    printf("================================\n");
    
    // Выводим информацию о системе
    print_system_info();
    
    // Исходное состояние
    printf("\n=== НАЧАЛЬНОЕ СОСТОЯНИЕ ===\n");
    print_page_faults("Исходное состояние");
    print_proc_page_faults("Исходное состояние (/proc)");
    
    // Выделение памяти
    printf("\n>>> ВЫДЕЛЯЕМ %d MB ПАМЯТИ...\n", SIZE / (1024 * 1024));
    char *arr = malloc(SIZE);
    if (!arr) {
        perror("❌ Ошибка выделения памяти с помощью malloc()");
        return 1;
    }
    
    printf("✅ Память успешно выделена\n");
    print_page_faults("После malloc()");
    print_proc_page_faults("После malloc() (/proc)");
    
    // Ждём немного
    printf("\n⏳ Ожидание 2 секунды...\n");
    sleep(2);
    
    // Разные паттерны доступа к памяти
    sequential_access(arr, SIZE);
    
    // Ожидание между тестами
    sleep(1);
    
    random_access(arr, SIZE);
    
    // Ожидание между тестами  
    sleep(1);
    
    strided_access(arr, SIZE);
    
    // Ожидание между тестами
    sleep(1);
    
    memory_stress_test(arr, SIZE);
    
    // Освобождение памяти
    printf("\n>>> ОСВОБОЖДАЕМ ПАМЯТЬ...\n");
    free(arr);
    
    printf("\n=== ФИНАЛЬНОЕ СОСТОЯНИЕ ===\n");
    print_page_faults("После free()");
    print_proc_page_faults("После free() (/proc)");
    
    // Анализ результатов
    printf("\n=== АНАЛИЗ РЕЗУЛЬТАТОВ ===\n");
    printf("Page faults происходят при:\n");
    printf("1. Первом обращении к странице (minor fault)\n");
    printf("2. Необходимости подкачки с диска (major fault)\n");
    printf("3. Защите памяти (protection fault)\n");
    
    printf("\n📈 ВЫВОДЫ:\n");
    printf("- Последовательный доступ эффективнее случайного\n");
    printf("- Minor faults - страницы выделяются в RAM\n");
    printf("- Major faults - требуется обращение к диску\n");
    printf("- Локальность данных важна для производительности\n");
    
    printf("\n✅ ЗАДАНИЕ C ЗАВЕРШЕНО!\n");
    printf("Нажмите Enter для выхода...");
    getchar();
    
    return 0;
}
