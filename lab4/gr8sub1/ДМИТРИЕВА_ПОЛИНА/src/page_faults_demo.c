#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_RESET   "\x1b[0m"

#define PAGE_SIZE 4096

// Получение статистики page faults
void get_page_faults(long *minor, long *major) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    *minor = usage.ru_minflt;
    *major = usage.ru_majflt;
}

// Вывод разницы в page faults
void print_page_fault_delta(const char *label, long start_minor, long start_major) {
    long end_minor, end_major;
    get_page_faults(&end_minor, &end_major);
    
    printf("%s:\n", label);
    printf("  Minor faults: " COLOR_BLUE "%ld" COLOR_RESET " (+%ld)\n", 
           end_minor, end_minor - start_minor);
    printf("  Major faults: " COLOR_BLUE "%ld" COLOR_RESET " (+%ld)\n", 
           end_major, end_major - start_major);
}

// Демонстрация 1: Выделение без обращения
void demo_allocation_only() {
    printf(COLOR_GREEN "\n=== Демо 1: Выделение памяти без обращения ===\n" COLOR_RESET);
    
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);
    
    // Выделяем 20 MB, но не обращаемся
    size_t size = 20 * 1024 * 1024;
    char *ptr = malloc(size);
    if (!ptr) {
        printf(COLOR_RED "Ошибка malloc!\n" COLOR_RESET);
        return;
    }
    
    printf("Выделено: %zu MB виртуальной памяти\n", size / (1024 * 1024));
    printf("Адрес: %p\n", ptr);
    
    print_page_fault_delta("После malloc (без доступа)", start_minor, start_major);
    printf(COLOR_YELLOW "Ожидание: 0 page faults (память только виртуальная)\n" COLOR_RESET);
    
    free(ptr);
}

// Демонстрация 2: Последовательный доступ
void demo_sequential_access() {
    printf(COLOR_GREEN "\n=== Демо 2: Последовательный доступ ===\n" COLOR_RESET);
    
    size_t size = 10 * 1024 * 1024; // 10 MB
    char *ptr = malloc(size);
    if (!ptr) {
        printf(COLOR_RED "Ошибка malloc!\n" COLOR_RESET);
        return;
    }
    
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);
    
    // Записываем по одному байту в каждую страницу
    printf("Записываем по одному байту на каждую страницу...\n");
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        ptr[i] = 'A';
    }
    
    print_page_fault_delta("После последовательной записи", start_minor, start_major);
    
    size_t expected_faults = size / PAGE_SIZE;
    printf(COLOR_YELLOW "Ожидание: ~%zu minor faults (по одному на страницу)\n" COLOR_RESET, 
           expected_faults);
    
    free(ptr);
}

// Демонстрация 3: Случайный доступ
void demo_random_access() {
    printf(COLOR_GREEN "\n=== Демо 3: Случайный доступ ===\n" COLOR_RESET);
    
    size_t size = 10 * 1024 * 1024; // 10 MB
    char *ptr = malloc(size);
    if (!ptr) {
        printf(COLOR_RED "Ошибка malloc!\n" COLOR_RESET);
        return;
    }
    
    // Инициализация ГСЧ
    srand(time(NULL));
    
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);
    
    // 1000 случайных обращений к разным страницам
    int accesses = 1000;
    printf("Выполняем %d случайных обращений...\n", accesses);
    for (int i = 0; i < accesses; i++) {
        size_t page_index = rand() % (size / PAGE_SIZE);
        size_t offset = page_index * PAGE_SIZE;
        ptr[offset] = 'B';
    }
    
    print_page_fault_delta("После случайных записей", start_minor, start_major);
    printf("Случайных обращений: %d\n", accesses);
    printf(COLOR_YELLOW "Ожидание: ~%d minor faults (по одному на новую страницу)\n" COLOR_RESET, 
           accesses);
    
    free(ptr);
}

// Демонстрация 4: Сравнение malloc vs calloc
void demo_malloc_vs_calloc() {
    printf(COLOR_GREEN "\n=== Демо 4: malloc vs calloc ===\n" COLOR_RESET);
    
    size_t size = 5 * 1024 * 1024; // 5 MB
    
    printf("--- malloc + memset ---\n");
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);
    
    char *malloc_ptr = malloc(size);
    memset(malloc_ptr, 0, size);
    
    print_page_fault_delta("malloc + memset", start_minor, start_major);
    free(malloc_ptr);
    
    printf("\n--- calloc ---\n");
    get_page_faults(&start_minor, &start_major);
    
    char *calloc_ptr = calloc(1, size);
    // calloc использует zero-page optimization
    
    print_page_fault_delta("calloc (только выделение)", start_minor, start_major);
    
    // Теперь запишем в calloc память
    get_page_faults(&start_minor, &start_major);
    printf("Записываем в calloc память...\n");
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        calloc_ptr[i] = 'C';
    }
    
    print_page_fault_delta("calloc + запись", start_minor, start_major);
    printf(COLOR_YELLOW "calloc использует Copy-on-Write оптимизацию\n" COLOR_RESET);
    
    free(calloc_ptr);
}

int main() {
    printf(COLOR_GREEN "\n=== Демонстрация Page Faults ===\n" COLOR_RESET);
    printf("Дмитриева Полина, gr8sub1\n");
    printf("Размер страницы: %d байт\n\n", PAGE_SIZE);
    
    long initial_minor, initial_major;
    get_page_faults(&initial_minor, &initial_major);
    printf("Начальные page faults: minor=%ld, major=%ld\n", 
           initial_minor, initial_major);
    
    // Запускаем все демонстрации
    demo_allocation_only();
    demo_sequential_access();
    demo_random_access();
    demo_malloc_vs_calloc();
    
    // Итоги
    long final_minor, final_major;
    get_page_faults(&final_minor, &final_major);
    
    printf(COLOR_GREEN "\n=== Итоги ===\n" COLOR_RESET);
    printf("Всего minor faults: %ld\n", final_minor - initial_minor);
    printf("Всего major faults: %ld\n", final_major - initial_major);
    
    printf(COLOR_GREEN "\n=== Выводы ===\n" COLOR_RESET);
    printf("• malloc() не вызывает page faults - память виртуальная\n");
    printf("• Первая запись в страницу вызывает minor fault\n"); 
    printf("• Последовательный доступ предсказуем по faults\n");
    printf("• calloc использует оптимизацию (zero-page)\n");
    printf("• Major faults не было - вся память в RAM\n");
    
    return 0;
}
