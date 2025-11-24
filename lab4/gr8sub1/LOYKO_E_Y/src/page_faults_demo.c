/*
 * page_faults_demo.c - Демонстрация page faults
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

#define PAGE_SIZE 4096

void get_page_faults(long *minor, long *major) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    *minor = usage.ru_minflt;
    *major = usage.ru_majflt;
}

void print_page_fault_delta(const char *label, long start_minor, long start_major) {
    long end_minor, end_major;
    get_page_faults(&end_minor, &end_major);

    long delta_minor = end_minor - start_minor;
    long delta_major = end_major - start_major;

    printf("%s:\n", label);
    printf("  Minor faults: %ld (+%ld)\n", end_minor, delta_minor);
    printf("  Major faults: %ld (+%ld)\n", end_major, delta_major);
}

void demo_allocation_no_access() {
    printf("\n=== Demo 1: Allocation without access ===\n");

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // Выделяем 10 MB
    size_t size = 10 * 1024 * 1024;
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));
    print_page_fault_delta("After malloc (no access)", start_minor, start_major);

    free(ptr);
}

void demo_sequential_access() {
    printf("\n=== Demo 2: Sequential access ===\n");

    size_t size = 10 * 1024 * 1024;  // 10 MB
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // Заполнить память последовательно, по одной странице за раз
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        ptr[i] = 'A';
    }

    print_page_fault_delta("After sequential write (one byte per page)", start_minor, start_major);

    // Вычислить ожидаемое количество page faults
    size_t expected_faults = size / PAGE_SIZE;
    printf("Expected page faults: %zu (size / PAGE_SIZE)\n", expected_faults);

    free(ptr);
}

void demo_full_write() {
    printf("\n=== Demo 3: Full memory write ===\n");

    size_t size = 5 * 1024 * 1024;  // 5 MB
    char *ptr = malloc(size);

    if (!ptr) {
        perror("malloc failed");
        return;
    }

    printf("Allocated %zu bytes (%.1f MB)\n", size, size / (1024.0 * 1024.0));

    long start_minor, start_major;
    get_page_faults(&start_minor, &start_major);

    // Заполнить всю память
    memset(ptr, 'B', size);

    print_page_fault_delta("After memset", start_minor, start_major);

    free(ptr);
}

void demo_random_access() {
    printf("\n=== Demo 4: Random access ===\n");

    size_t size = 10 * 1024 * 1024;  // 10 MB
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

    // Сделать 1000 случайных обращений к разным страницам
    int num_accesses = 1000;

    for (int i = 0; i < num_accesses; i++) {
        size_t random_offset = (rand() % (size / PAGE_SIZE)) * PAGE_SIZE;
        ptr[random_offset] = 'C';
    }

    print_page_fault_delta("After random writes", start_minor, start_major);

    printf("Random accesses performed: %d\n", num_accesses);
    printf("Note: Some pages may have been accessed multiple times.\n");

    free(ptr);
}

void demo_rereading() {
    printf("\n=== Demo 5: Re-reading memory ===\n");

    size_t size = 5 * 1024 * 1024;  // 5 MB
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

    // Прочитать всю память (не должно быть новых page faults)
    unsigned long long sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += (unsigned char)ptr[i];
    }

    print_page_fault_delta("After second read", start_minor, start_major);
    printf("Checksum: %llu (to prevent optimization)\n", sum);
    printf("Note: No new page faults expected (pages already in memory).\n");

    free(ptr);
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

    printf("\n=== Summary ===\n");
    get_page_faults(&initial_minor, &initial_major);
    printf("Total page faults: minor=%ld, major=%ld\n", initial_minor, initial_major);

    printf("\nKey observations:\n");
    printf("1. malloc() alone doesn't cause page faults (memory is virtual)\n");
    printf("2. First write to a page causes a minor page fault\n");
    printf("3. Subsequent accesses to the same page don't cause faults\n");
    printf("4. Major faults occur only when loading from disk (swap, files)\n");

    return 0;
}
