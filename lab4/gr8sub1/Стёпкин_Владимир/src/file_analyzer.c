#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define FILE_NAME "testfile.bin"

unsigned long long read_with_syscalls(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    unsigned char buffer[8192]; // 8 KB буфер
    ssize_t bytes;
    unsigned long long sum = 0;

    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes; i++)
            sum += buffer[i];
    }

    close(fd);
    return sum;
}

unsigned long long read_with_mmap(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); exit(1); }
    size_t filesize = st.st_size;

    unsigned char *data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); exit(1); }

    unsigned long long sum = 0;
    for (size_t i = 0; i < filesize; i++)
        sum += data[i];

    munmap(data, filesize);
    close(fd);
    return sum;
}

double measure_time(unsigned long long (*func)(const char *), const char *filename) {
    clock_t start = clock();
    unsigned long long sum = func(filename);
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Sum = %llu\n", sum); // чтобы оптимизатор не выкинул цикл
    return time_spent;
}

int main() {
    double time_syscalls = measure_time(read_with_syscalls, FILE_NAME);
    printf("Time with read()/write(): %.6f seconds\n", time_syscalls);

    double time_mmap = measure_time(read_with_mmap, FILE_NAME);
    printf("Time with mmap(): %.6f seconds\n", time_mmap);

    return 0;
}
