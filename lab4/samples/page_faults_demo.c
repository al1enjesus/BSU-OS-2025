/*
 * page_faults_demo.c - Демонстрация page faults
 *
 * Компиляция: gcc -Wall -Wextra -O2 page_faults_demo.c -o page_faults_demo
 * Использование: ./page_faults_demo
 *
 * Демонстрирует:
 * - Minor page faults при первом обращении к выделенной памяти
 * - Разницу между последовательным и случайным доступом
 * - Влияние размера памяти на количество page faults
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

#define PAGE_SIZE 4096

// TODO: Реализовать функцию для получения текущих page faults
void get_page_faults(long *minor, long *major) {
    struct rusage usage;
    // TODO: Использовать getrusage(RUSAGE_SELF, &usage)
    // *minor = usage.ru_minflt;
    // *major = usage.ru_majflt;

    *minor = 0;
    *major = 0;
}

// TODO: Реализовать функцию для вывода разницы в page faults
void print_page_fault_delta(const char *label, long start_minor, long start_major) {
    long end_minor, end_major;
    get_page_faults(&end_minor, &end_major);

    long delta_minor = end_minor - start_minor;
    long delta_major = end_major - start_major;

    printf("%s:\n", label);
    printf("  Minor faults: %ld (+%ld)\n", end_minor, delta_minor);
    printf("  Major faults: %ld (+%ld)\n", end_major, delta_major);
}

// TODO: Демонстрация 1 - базовое выделение без обращения
void demo_allocation_no_access() {
    printf("\n=== Demo 1: Allocation without access ===\n");

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // Выделяем 100 MB
    size_t size = 100 * 1024 * 1024;
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    // TODO: Вывести page faults сразу после malloc
    // Ожидание: почти нет новых faults (память виртуальная, страницы не выделены)
    print_page_fault_delta("After malloc (no access)", start_minor, start_major);

    free(ptr);
}

// TODO: Демонстрация 2 - последовательный доступ
void demo_sequential_access() {
    printf("\n=== Demo 2: Sequential access ===\n");

    size_t size = 100 * 1024 * 1024;  // 100 MB
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // TODO: Заполнить память последовательно, по одной странице за раз
    // Это вызовет minor page fault для каждой новой страницы
    //
    // for (size_t i = 0; i < size; i += PAGE_SIZE) {
    //     ptr[i] = 'A';
    // }

    print_page_fault_delta("After sequential write (one byte per page)", start_minor, start_major);

    // TODO: Вычислить ожидаемое количество page faults
    size_t expected_faults = size / PAGE_SIZE;
    printf("Expected page faults: %zu (size / PAGE_SIZE)\n", expected_faults);

    free(ptr);
}

// TODO: Демонстрация 3 - заполнение всей памяти
void demo_full_write() {
    printf("\n=== Demo 3: Full memory write ===\n");

    size_t size = 50 * 1024 * 1024;  // 50 MB (меньше, чтобы быстрее)
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // TODO: Заполнить всю память (memset вызовет page faults)
    // memset(ptr, 'B', size);

    print_page_fault_delta("After memset", start_minor, start_major);

    free(ptr);
}

// TODO: Демонстрация 4 - случайный доступ
void demo_random_access() {
    printf("\n=== Demo 4: Random access ===\n");

    size_t size = 100 * 1024 * 1024;  // 100 MB
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    // Инициализировать генератор случайных чисел
    srand(time(NULL));

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // TODO: Сделать 10000 случайных обращений к разным страницам
    int num_accesses = 10000;
    int pages_touched = 0;

    // for (int i = 0; i < num_accesses; i++) {
    //     size_t random_offset = (rand() % (size / PAGE_SIZE)) * PAGE_SIZE;
    //     ptr[random_offset] = 'C';
    //     // Примечание: некоторые страницы могут повториться
    // }

    print_page_fault_delta("After random writes", start_minor, start_major);

    printf("Random accesses performed: %d\n", num_accesses);
    printf("Note: Some pages may have been accessed multiple times.\n");

    free(ptr);
}

// TODO: Демонстрация 5 - повторное чтение (без новых faults)
void demo_rereading() {
    printf("\n=== Demo 5: Re-reading memory ===\n");

    size_t size = 50 * 1024 * 1024;  // 50 MB
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    // Первое заполнение
    printf("First pass: writing memory...\n");
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    memset(ptr, 'D', size);

    print_page_fault_delta("After first write", start_minor, start_major);

    // Второе чтение
    printf("\nSecond pass: reading memory...\n");
    get_page_faults(&start_minor, &start_major);

    // TODO: Прочитать всю память (не должно быть новых page faults)
    unsigned long long sum = 0;
    // for (size_t i = 0; i < size; i++) {
    //     sum += (unsigned char)ptr[i];
    // }

    print_page_fault_delta("After second read", start_minor, start_major);
    printf("Checksum: %llu (to prevent optimization)\n", sum);
    printf("Note: No new page faults expected (pages already in memory).\n");

    free(ptr);
}

// TODO: Демонстрация 6 - сравнение с calloc
void demo_calloc_vs_malloc() {
    printf("\n=== Demo 6: calloc vs malloc ===\n");

    size_t size = 50 * 1024 * 1024;  // 50 MB

    // malloc + memset
    printf("Method 1: malloc + memset\n");
    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    char *ptr1 = malloc(size);
    if (ptr1) {
        memset(ptr1, 0, size);
        print_page_fault_delta("After malloc + memset", start_minor, start_major);
        free(ptr1);
    }

    printf("\n");

    // calloc (выделяет обнулённую память)
    printf("Method 2: calloc\n");
    get_page_faults(&start_minor, &start_major);

    char *ptr2 = calloc(size, 1);
    if (ptr2) {
        // TODO: Обратиться ко всей памяти, чтобы вызвать page faults
        // Подсказка: calloc может использовать zero page optimization
        // (все страницы изначально указывают на одну zero-page в read-only)

        // for (size_t i = 0; i < size; i += PAGE_SIZE) {
        //     volatile char tmp = ptr2[i];  // Чтение
        // }

        print_page_fault_delta("After calloc + read", start_minor, start_major);

        // Теперь запишем в память
        get_page_faults(&start_minor, &start_major);

        // for (size_t i = 0; i < size; i += PAGE_SIZE) {
        //     ptr2[i] = 1;  // Запись (вызовет copy-on-write)
        // }

        print_page_fault_delta("After calloc + write", start_minor, start_major);

        free(ptr2);
    }
}

int main() {
    printf("Page Faults Demonstration\n");
    printf("==========================\n");
    printf("Page size: %d bytes\n", PAGE_SIZE);

    long initial_minor, initial_major;
    get_page_faults(&initial_minor, &initial_major);
    printf("Initial page faults: minor=%ld, major=%ld\n", initial_minor, initial_major);

    // Запустить все демонстрации
    demo_allocation_no_access();
    demo_sequential_access();
    demo_full_write();
    demo_random_access();
    demo_rereading();
    demo_calloc_vs_malloc();

    printf("\n=== Summary ===\n");
    get_page_faults(&initial_minor, &initial_major);
    printf("Total page faults: minor=%ld, major=%ld\n", initial_minor, initial_major);

    printf("\nKey observations:\n");
    printf("1. malloc() alone doesn't cause page faults (memory is virtual)\n");
    printf("2. First write to a page causes a minor page fault\n");
    printf("3. Subsequent accesses to the same page don't cause faults\n");
    printf("4. Major faults occur only when loading from disk (swap, files)\n");
    printf("5. calloc() may use optimizations (zero page, CoW)\n");

    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте все TODO функции
 *
 * 2. Запустите программу и проанализируйте вывод:
 *    $ ./page_faults_demo
 *
 * 3. Проверьте результаты системными средствами:
 *    $ /usr/bin/time -v ./page_faults_demo
 *    (смотрите "Minor page faults" и "Major page faults")
 *
 * 4. Ответьте на вопросы:
 *    - Почему malloc без записи не вызывает page faults?
 *    - Сколько page faults происходит при заполнении 100 MB памяти?
 *    - Почему повторное чтение не вызывает новых faults?
 *    - В чём разница между calloc и malloc с точки зрения page faults?
 *
 * 5. Дополнительные эксперименты:
 *    - Увеличьте размер памяти до 1 GB (если есть достаточно RAM)
 *    - Запустите под strace и посмотрите системные вызовы:
 *      $ strace -e trace=mmap,brk,mprotect ./page_faults_demo 2>&1 | less
 *    - Измените доступ к памяти (только чтение vs запись)
 *    - Попробуйте вызвать major page faults (создайте swap pressure)
 *
 * 6. В отчёте:
 *    - Постройте график: размер памяти vs количество page faults
 *    - Объясните zero page optimization для calloc
 *    - Опишите, как ОС управляет page faults
 */
