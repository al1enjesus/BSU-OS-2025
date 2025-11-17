#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

#define PAGE_SIZE 4096
#define MEMORY_SIZE (100 * 1024 * 1024)
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

typedef struct {
    long minor_faults;
    long major_faults;
} PageFaults;

PageFaults get_page_faults(void) {
    PageFaults stats = {0, 0};
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    stats.minor_faults = usage.ru_minflt;
    stats.major_faults = usage.ru_majflt;
    return stats;
}

void print_bar(long value, long max_value, int width) {
    int filled = (int)((double)value / max_value * width);
    if (filled > width) filled = width;
    
    printf("[");
    for (int i = 0; i < filled; i++) printf("█");
    for (int i = filled; i < width; i++) printf("░");
    printf("]");
}

void print_page_fault_delta(const char *label, PageFaults start, PageFaults end) {
    long delta_minor = end.minor_faults - start.minor_faults;
    long delta_major = end.major_faults - start.major_faults;
    
    printf(COLOR_BOLD "%s:\n" COLOR_RESET, label);
    printf("  Минорные ошибки: " COLOR_GREEN "%ld" COLOR_RESET " (всего: %ld, дельта: " COLOR_YELLOW "+%ld" COLOR_RESET ")\n",
           end.minor_faults, end.minor_faults, delta_minor);
    printf("  Мажорные ошибки: " COLOR_CYAN "%ld" COLOR_RESET " (всего: %ld, дельта: " COLOR_YELLOW "+%ld" COLOR_RESET ")\n",
           end.major_faults, end.major_faults, delta_major);
}

void demo_allocation_no_access(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 1: malloc() без обращения к памяти ===" COLOR_RESET "\n");
    printf("Ожидается: Нет ошибок страниц (память виртуальная)\n\n");
    
    size_t size = MEMORY_SIZE;
    PageFaults start = get_page_faults();
    
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    PageFaults after_malloc = get_page_faults();
    printf("Выделено %zu байт (%.1f МБ)\n", size, size / (1024.0 * 1024.0));
    print_page_fault_delta("После malloc (без обращения)", start, after_malloc);
    
    printf("\n" COLOR_GREEN "✓" COLOR_RESET " Наблюдение: Нет существенных ошибок страниц, так как страницы виртуальные.\n");
    printf("  Физическая память выделяется только при первом обращении к страницам.\n");
    
    free(ptr);
}

void demo_sequential_access(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 2: Последовательный доступ (один байт на страницу) ===" COLOR_RESET "\n");
    printf("Ожидается: ~25600 минорных ошибок для 100 МБ (100МБ / 4КБ на ошибку)\n\n");
    
    size_t size = MEMORY_SIZE;
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    printf("Выделено %zu байт (%.1f МБ)\n", size, size / (1024.0 * 1024.0));
    
    PageFaults start = get_page_faults();
    printf("Начало последовательных записей (одна на страницу)...\n");
    fflush(stdout);
    
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        ptr[i] = 'A';
    }
    
    PageFaults end = get_page_faults();
    print_page_fault_delta("После последовательных записей", start, end);
    
    size_t expected_faults = size / PAGE_SIZE;
    printf("\nОжидаемые ошибки страниц: " COLOR_YELLOW "%zu" COLOR_RESET "\n", expected_faults);
    printf(COLOR_GREEN "✓" COLOR_RESET " Анализ: Каждая запись в новую страницу вызывает одну минорную ошибку.\n");
    printf("  Это ожидаемое поведение: ленивое выделение страниц.\n");
    
    free(ptr);
}

void demo_full_write(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 3: memset() - запись всей памяти ===" COLOR_RESET "\n");
    printf("Ожидается: Меньше ошибок чем при последовательном (memset оптимизирован)\n\n");
    
    size_t size = 50 * 1024 * 1024;
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    printf("Выделено %zu байт (%.1f МБ)\n", size, size / (1024.0 * 1024.0));
    
    PageFaults start = get_page_faults();
    printf("Запуск memset()...\n");
    fflush(stdout);
    
    memset(ptr, 'B', size);
    
    PageFaults end = get_page_faults();
    print_page_fault_delta("После memset", start, end);
    
    printf("\n" COLOR_GREEN "✓" COLOR_RESET " Анализ: memset() может быть более эффективным благодаря:\n");
    printf("  • Большим записям за итерацию\n");
    printf("  • Оптимизации кэша CPU\n");
    printf("  • Предзагрузке ядра\n");
    
    free(ptr);
}

void demo_random_access(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 4: Случайный доступ ===" COLOR_RESET "\n");
    printf("Ожидается: Много ошибок, но потенциально меньше уникальных страниц\n\n");
    
    size_t size = MEMORY_SIZE;
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    printf("Выделено %zu байт (%.1f МБ)\n", size, size / (1024.0 * 1024.0));
    
    srand(time(NULL));
    int num_accesses = 10000;
    
    PageFaults start = get_page_faults();
    printf("Начало %d случайных обращений...\n", num_accesses);
    fflush(stdout);
    
    for (int i = 0; i < num_accesses; i++) {
        size_t random_offset = (rand() % (size / PAGE_SIZE)) * PAGE_SIZE;
        ptr[random_offset] = 'C';
        if (i % 1000 == 0) printf(COLOR_GREEN "." COLOR_RESET);
        fflush(stdout);
    }
    printf("\n");
    
    PageFaults end = get_page_faults();
    print_page_fault_delta("После случайных обращений", start, end);
    
    printf("\n" COLOR_GREEN "✓" COLOR_RESET " Наблюдение: Некоторые страницы затрагиваются несколько раз.\n");
    printf("  Ошибок должно быть меньше чем при последовательном (примерно ~%d ошибок страниц).\n",
           num_accesses < (int)(size / PAGE_SIZE) ? num_accesses : (int)(size / PAGE_SIZE));
    
    free(ptr);
}

void demo_rereading(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 5: Повторное чтение памяти (НЕ должно быть новых ошибок) ===" COLOR_RESET "\n");
    printf("Ожидается: Ноль ошибок страниц (страницы уже в памяти)\n\n");
    
    size_t size = 50 * 1024 * 1024;
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    printf("Первый проход: Запись памяти...\n");
    PageFaults start1 = get_page_faults();
    memset(ptr, 'D', size);
    PageFaults after_write = get_page_faults();
    print_page_fault_delta("После первой записи", start1, after_write);
    
    printf("\nВторой проход: Чтение памяти...\n");
    PageFaults start2 = get_page_faults();
    
    unsigned long long sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += (unsigned char)ptr[i];
    }
    
    PageFaults after_read = get_page_faults();
    print_page_fault_delta("После второго чтения", start2, after_read);
    
    printf("Контрольная сумма: " COLOR_YELLOW "%llu" COLOR_RESET " (предотвращает оптимизацию компилятора)\n", sum);
    printf("\n" COLOR_GREEN "✓" COLOR_RESET " Наблюдение: Нет новых ошибок страниц, так как страницы уже в RAM.\n");
    printf("  Это демонстрирует эффективность кэширования страниц.\n");
    
    free(ptr);
}

void demo_calloc_vs_malloc(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 6: calloc против malloc+memset ===" COLOR_RESET "\n");
    printf("Ожидается: calloc может быть быстрее из-за оптимизации нулевых страниц\n\n");
    
    size_t size = 50 * 1024 * 1024;
    
    printf(COLOR_BOLD "Метод 1: malloc + memset\n" COLOR_RESET);
    PageFaults start1 = get_page_faults();
    
    char *ptr1 = malloc(size);
    if (ptr1) {
        memset(ptr1, 0, size);
        
        PageFaults end1 = get_page_faults();
        print_page_fault_delta("После malloc+memset", start1, end1);
        free(ptr1);
    }
    
    sleep(1);
    
    printf("\n" COLOR_BOLD "Метод 2: calloc\n" COLOR_RESET);
    PageFaults start2 = get_page_faults();
    
    char *ptr2 = calloc(size, 1);
    if (ptr2) {
        for (size_t i = 0; i < size; i += PAGE_SIZE) {
            ptr2[i] = 1;
        }
        
        PageFaults end2 = get_page_faults();
        print_page_fault_delta("После calloc+запись", start2, end2);
        free(ptr2);
    }
    
    printf("\n" COLOR_GREEN "✓" COLOR_RESET " Анализ: Преимущества calloc():\n");
    printf("  • Использует оптимизацию нулевых страниц изначально\n");
    printf("  • Copy-on-write для приватной памяти\n");
    printf("  • Может уменьшить начальные ошибки страниц\n");
}

void demo_page_size_impact(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Демо 7: Влияние паттерна доступа на размер страницы ===" COLOR_RESET "\n");
    printf("Размер страницы: " COLOR_YELLOW "%d" COLOR_RESET " байт\n", PAGE_SIZE);
    
    size_t size = 10 * 1024 * 1024;
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc не удался");
        return;
    }
    
    printf("Выделено %zu байт (%.1f МБ)\n", size, size / (1024.0 * 1024.0));
    printf("Ожидаемые страницы: " COLOR_YELLOW "%zu" COLOR_RESET "\n\n", size / PAGE_SIZE);
    
    printf(COLOR_BOLD "Паттерн 1: Один байт на страницу\n" COLOR_RESET);
    PageFaults start = get_page_faults();
    
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        ptr[i] = 1;
    }
    
    PageFaults end = get_page_faults();
    long pattern1_faults = end.minor_faults - start.minor_faults;
    printf("Ошибки страниц: " COLOR_GREEN "%ld" COLOR_RESET " ", pattern1_faults);
    print_bar(pattern1_faults, size / PAGE_SIZE, 30);
    printf("\n");
    printf("Ожидается: ~%zu (одна на страницу)\n", size / PAGE_SIZE);
    
    printf("\n" COLOR_BOLD "Паттерн 2: Последовательные записи (несколько на страницу)\n" COLOR_RESET);
    start = get_page_faults();
    
    for (size_t i = 0; i < size; i++) {
        ptr[i] = 2;
    }
    
    end = get_page_faults();
    long pattern2_faults = end.minor_faults - start.minor_faults;
    printf("Ошибки страниц: " COLOR_GREEN "%ld" COLOR_RESET " ", pattern2_faults);
    print_bar(pattern2_faults, pattern1_faults > 0 ? pattern1_faults : 1, 30);
    printf("\n");
    printf("Ожидается: 0 или очень мало (страницы уже загружены)\n");
    
    free(ptr);
}

int main(int argc, char *argv[]) {
    printf(COLOR_BOLD COLOR_BLUE "=== Демонстрация ошибок страниц ===" COLOR_RESET "\n");
    printf("Размер страницы: " COLOR_YELLOW "%d" COLOR_RESET " байт\n", PAGE_SIZE);
    printf("Всего памяти в демо: " COLOR_YELLOW "%.1f" COLOR_RESET " МБ\n\n", MEMORY_SIZE / (1024.0 * 1024.0));
    
    int run_all = (argc == 1);
    int demo_sequential = run_all;
    int demo_random = run_all;
    int demo_calloc = run_all;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sequential") == 0) demo_sequential = 1;
        if (strcmp(argv[i], "--random") == 0) demo_random = 1;
        if (strcmp(argv[i], "--calloc") == 0) demo_calloc = 1;
        if (strcmp(argv[i], "--help") == 0) {
            printf("Использование: %s [--sequential] [--random] [--calloc]\n", argv[0]);
            printf("Запустите без аргументов для всех демо.\n");
            return 0;
        }
    }
    
    demo_allocation_no_access();
    
    if (demo_sequential) {
        demo_sequential_access();
        demo_full_write();
    }
    
    if (demo_random) {
        demo_random_access();
    }
    
    demo_rereading();
    
    if (demo_calloc) {
        demo_calloc_vs_malloc();
    }
    
    demo_page_size_impact();
    
    printf("\n" COLOR_BOLD COLOR_BLUE "=== ИТОГИ ===" COLOR_RESET "\n");
    printf(COLOR_BOLD "Ключевые наблюдения:\n" COLOR_RESET);
    printf("1. " COLOR_GREEN "malloc()" COLOR_RESET " сам по себе: НЕТ ошибок страниц (виртуальная память)\n");
    printf("2. " COLOR_GREEN "Первая запись" COLOR_RESET " на страницу: ОДНА минорная ошибка (ленивое выделение)\n");
    printf("3. " COLOR_GREEN "Повторный доступ" COLOR_RESET " к той же странице: НЕТ новых ошибок (в памяти)\n");
    printf("4. " COLOR_GREEN "Последовательный доступ" COLOR_RESET ": ОДНА ошибка на новую страницу\n");
    printf("5. " COLOR_GREEN "Случайный доступ" COLOR_RESET ": Зависит от уникальных затронутых страниц\n");
    printf("6. " COLOR_GREEN "calloc" COLOR_RESET ": Может использовать оптимизации (нулевая страница, CoW)\n");
    printf("7. " COLOR_GREEN "Мажорные ошибки" COLOR_RESET ": Только при загрузке с диска/swap\n");
    printf("\n" COLOR_YELLOW "Ошибки страниц - это ХОРОШО!" COLOR_RESET " Они обеспечивают ленивое выделение и защиту памяти!\n");
    
    return 0;
}
