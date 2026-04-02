#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#define SIZE_100MB (100 * 1024 * 1024)
#define PAGE_SIZE 4096

void print_page_faults(const char* stage) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("%s\n", stage);
    printf("  Minor faults: %ld\n", usage.ru_minflt);
    printf("  Major faults: %ld\n", usage.ru_majflt);
    if (usage.ru_minflt > 0 || usage.ru_majflt > 0) {
        printf("  Total faults: %ld\n", usage.ru_minflt + usage.ru_majflt);
    }
    printf("\n");
}

void print_memory_info(size_t size, size_t total_pages) {
    printf("=== Page Fault Analysis ===\n");
    printf("Memory Configuration:\n");
    printf("  Total allocation: %zu MB\n", size / (1024 * 1024));
    printf("  Page size: %d bytes\n", PAGE_SIZE);
    printf("  Total pages: %zu\n", total_pages);
    printf("  Sequential accesses: %zu\n", total_pages);
    printf("  Random accesses: 10000\n\n");
}

int main() {
    size_t size = SIZE_100MB;
    size_t page_size = PAGE_SIZE;
    size_t total_pages = size / page_size;
    
    print_memory_info(size, total_pages);
    print_page_faults("1. Initial state (before any allocation)");
    
    // Выделение виртуальной памяти
    printf("2. Allocating virtual memory...\n");
    char *arr = malloc(size);
    if (!arr) {
        perror("malloc failed");
        return 1;
    }
    print_page_faults("   After malloc (virtual memory reserved)");
    
    // Последовательный доступ - касаемся каждой страницы
    printf("3. Sequential access - touching every page...\n");
    printf("   (This should generate ~%zu minor faults)\n", total_pages);
    for (size_t i = 0; i < size; i += page_size) {
        arr[i] = 'A';  // Запись в каждую страницу
    }
    print_page_faults("   After sequential access (all pages touched)");
    
    // Случайный доступ - можем попасть на уже использованные страницы
    printf("4. Random access - 10,000 operations...\n");
    printf("   (Most should hit already-allocated pages)\n");
    srand(42);  // Фиксированный seed для воспроизводимости
    for (int i = 0; i < 10000; i++) {
        size_t index = (rand() % total_pages) * page_size;
        arr[index] = 'B';
    }
    print_page_faults("   After random access");
    
    // Доступ к тем же страницам - не должно вызывать faults
    printf("5. Repeat access - modifying existing pages...\n");
    printf("   (Should generate minimal additional faults)\n");
    for (size_t i = 0; i < size; i += page_size * 2) {  // Каждая вторая страница
        arr[i] = 'C';
    }
    print_page_faults("   After repeat access");
    
    // Еще один тест - доступ с другим шагом
    printf("6. Strided access - every 3rd page...\n");
    for (size_t i = 0; i < size; i += page_size * 3) {
        arr[i] = 'D';
    }
    print_page_faults("   After strided access");
    
    // Освобождение памяти
    printf("7. Freeing memory...\n");
    free(arr);
    print_page_faults("   After free");
    
    printf("=== Analysis Complete ===\n");
    printf("Key observations:\n");
    printf("• Most page faults occur during first touch (sequential access)\n");
    printf("• Subsequent accesses to same pages generate minimal faults\n");
    printf("• This demonstrates Linux 'lazy allocation' strategy\n");
    
    return 0;
}
