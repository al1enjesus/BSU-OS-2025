/*
 * io_benchmark.c - Бенчмарк чтения файла через read() и mmap()
 *
 * Компиляция:
 * gcc -Wall -Wextra -O2 io_benchmark.c -o io_benchmark
 *
 * Использование:
 * ./io_benchmark testfile.bin
 *
 * Для создания файла:
 * dd if=/dev/urandom of=testfile.bin bs=1M count=100
 *
 * Очистка кэша:
 * sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 65536  // 64 КБ

// Получить текущее время с точностью до секунд (дробное)
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Чтение файла через read()
size_t read_with_syscalls(const char *filename, double* elapsed) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    unsigned char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t sum = 0;
    ssize_t bytes_read;

    double start = get_time();
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += buffer[i];
        }
    }
    double end = get_time();

    if (bytes_read < 0) {
        perror("read");
    }
    *elapsed = end - start;

    free(buffer);
    close(fd);

    printf("read_with_syscalls: byte sum = %zu\n", sum);
    return 0;
}

// Чтение файла через mmap()
size_t read_with_mmap(const char* filename, double* elapsed) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }
    size_t filesize = st.st_size;

    double start = get_time();
    unsigned char* data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t sum = 0;
    for (size_t i = 0; i < filesize; i++) {
        sum += data[i];
    }
    double end = get_time();

    if (munmap(data, filesize) == -1) {
        perror("munmap");
    }
    close(fd);

    *elapsed = end - start;

    printf("read_with_mmap: byte sum = %zu\n", sum);
    return 0;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s testfile.bin\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    double time_read = 0.0, time_mmap = 0.0;

    printf("Benchmarking reading methods on file: %s\n", filename);

    read_with_syscalls(filename, &time_read);
    read_with_mmap(filename, &time_mmap);

    printf("\nResults:\n");
    printf("read() time: %.6f seconds\n", time_read);
    printf("mmap() time: %.6f seconds\n", time_mmap);
    printf("Speedup (read()/mmap()): %.2fx\n", time_read / time_mmap);

    printf("\nBefore each run you should clear cache:\n");
    printf("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'\n");

    return 0;
}

