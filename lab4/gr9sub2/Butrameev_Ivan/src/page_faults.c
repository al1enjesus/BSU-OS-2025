#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <stddef.h>  // для size_t
#include <time.h>

void print_page_faults() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Minor faults: %ld, Major faults: %ld\n", usage.ru_minflt, usage.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024;  // 100 MB
    char *arr = malloc(size);
    if (!arr) {
        perror("malloc failed");
        return 1;
    }

    print_page_faults();

    // Последовательный доступ по одной странице
    for (size_t i = 0; i < size; i += 4096) {
        arr[i] = 'A';
    }
    print_page_faults();

    // Случайный доступ
    for (int i = 0; i < 10000; i++) {
        arr[rand() % size] = 'B';
    }
    print_page_faults();

    free(arr);
    return 0;
}
