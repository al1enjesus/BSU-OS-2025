#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

void print_page_faults(const char *stage) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("[%s] Minor faults: %ld, Major faults: %ld\n", stage, usage.ru_minflt, usage.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024;
    char *arr = malloc(size);
    srand(time(NULL));

    print_page_faults("Start");

    for (size_t i = 0; i < size; i += 4096) {
        arr[i] = 'A';
    }
    print_page_faults("After sequential access");

    for (int i = 0; i < 10000; i++) {
        arr[rand() % size] = 'B';
    }
    print_page_faults("After random access");

    free(arr);
    return 0;
}
