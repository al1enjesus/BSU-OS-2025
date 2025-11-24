#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

void print_page_faults(const char *stage) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    printf("\n=== %s ===\n", stage);
    printf("Minor faults: %ld\n", usage.ru_minflt);
    printf("Major faults: %ld\n", usage.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024; // 100 MB

    char *arr = malloc(size);
    if (!arr) {
        perror("malloc");
        return 1;
    }

    print_page_faults("After malloc");

    // последовательный доступ
    for (size_t i = 0; i < size; i += 4096) { // шаг = 1 страница
        arr[i] = 'A';
    }
    print_page_faults("After sequential access");

    // случайный доступ
    srand(time(NULL));
    for (int i = 0; i < 10000; i++) {
        arr[rand() % size] = 'B';
    }
    print_page_faults("After random access");

    free(arr);
    print_page_faults("After free");

    return 0;
}
