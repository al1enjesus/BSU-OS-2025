#include <stdio.h>       // printf()
#include <stdlib.h>      // malloc(), free(), rand()
#include <sys/resource.h> // getrusage()
#include <stddef.h>      // size_t

void print_page_faults() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Minor faults: %ld, Major faults: %ld\n",
           usage.ru_minflt, usage.ru_majflt);
}

int main() {
    srand(time(NULL));
    size_t size = 100 * 1024 * 1024;  // 100 MB
    char *arr = malloc(size);

    print_page_faults();

    // Последовательный доступ
    for (size_t i = 0; i < size; i += 4096) {  // по одной странице
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
