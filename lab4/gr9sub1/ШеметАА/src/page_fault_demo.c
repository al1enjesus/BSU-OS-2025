// page_fault_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>

#define SIZE (100 * 1024 * 1024)  // 100 MB

void print_faults(const char *label) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("%s: Minor: %ld, Major: %ld\n", label, usage.ru_minflt, usage.ru_majflt);
}

int main() {
    printf("Выделяем 100 МБ...\n");
    char *arr = malloc(SIZE);
    if (!arr) { perror("malloc"); return 1; }

    print_faults("После malloc");

    printf("Последовательный доступ...\n");
    for (size_t i = 0; i < SIZE; i += 4096) {
        arr[i] = 1;
    }
    print_faults("После последовательного");

    printf("Случайный доступ (10000 точек)...\n");
    srand(time(NULL));
    for (int i = 0; i < 10000; i++) {
        arr[rand() % SIZE] = 2;
    }
    print_faults("После случайного");

    free(arr);
    return 0;
}
