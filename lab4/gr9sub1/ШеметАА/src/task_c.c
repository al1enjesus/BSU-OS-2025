#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

void print_faults() {
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    printf("Minor: %ld, Major: %ld\n", u.ru_minflt, u.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024;
    char *arr = malloc(size);
    print_faults();

    printf("Sequential access...\n");
    for (size_t i = 0; i < size; i += 4096) arr[i] = 'A';
    print_faults();

    printf("Random access...\n");
    srand(time(NULL));
    for (int i = 0; i < 10000; i++) arr[rand() % size] = 'B';
    print_faults();

    free(arr);
    return 0;
}
