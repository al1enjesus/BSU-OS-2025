#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>  // Добавил этот заголовок для getpid() и sleep()

#define SIZE_100MB (100 * 1024 * 1024)

void print_page_faults(const char* description) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("%s: Minor faults: %ld, Major faults: %ld\n", 
           description, usage.ru_minflt, usage.ru_majflt);
}

void sequential_access(char *arr, size_t size) {
    printf("\n--- Sequential Access ---\n");
    print_page_faults("Before");
    
    clock_t start = clock();
    
    for (size_t i = 0; i < size; i += 4096) {
        arr[i] = (char)(i % 256);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("After");
    printf("Time: %.3f seconds\n", elapsed);
}

void random_access(char *arr, size_t size, int iterations) {
    printf("\n--- Random Access (%d iterations) ---\n", iterations);
    print_page_faults("Before");
    
    srand(time(NULL));
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        size_t index = (size_t)rand() % size;
        arr[index] = (char)(i % 256);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_page_faults("After");
    printf("Time: %.3f seconds\n", elapsed);
}

int main() {
    printf("PID: %d\n", getpid());
    printf("Allocating 100MB array...\n");
    
    size_t size = SIZE_100MB;
    char *arr = malloc(size);
    if (!arr) {
        perror("malloc failed");
        return 1;
    }
    
    print_page_faults("After malloc");
    
    // Даем время посмотреть состояние до инициализации
    printf("\nArray allocated but not initialized. Waiting 5 seconds...\n");
    sleep(5);
    
    // Последовательный доступ
    sequential_access(arr, size);
    sleep(3);
    
    // Случайный доступ
    random_access(arr, size, 100000);
    sleep(3);
    
    // Еще один последовательный доступ (должно быть меньше page faults)
    printf("\n--- Second Sequential Access ---\n");
    sequential_access(arr, size);
    
    printf("\nReleasing memory...\n");
    free(arr);
    print_page_faults("After free");
    
    return 0;
}
