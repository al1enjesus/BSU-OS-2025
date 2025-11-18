// page_faults_demo.c — анализ page faults в реальном времени
// Компиляция: gcc -Wall -Wextra -O2 page_faults_demo.c -o page_faults_demo

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

void print_page_faults(const char* phase) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("[%s] Minor faults: %ld, Major faults: %ld\n",
           phase, usage.ru_minflt, usage.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024;  // 100 MB
    char *arr = malloc(size);
    if (!arr) {
        perror("malloc");
        return 1;
    }

    srand(time(NULL));
    print_page_faults("init");

    // Последовательный доступ (по страницам)
    for (size_t i = 0; i < size; i += 4096) {
        arr[i] = 'A';
    }
    print_page_faults("sequential");

    // Случайный доступ
    for (int i = 0; i < 10000; i++) {
        size_t idx = rand() % size;
        arr[idx] = 'B';
    }
    print_page_faults("random");

    free(arr);
    print_page_faults("final");
    return 0;
}
