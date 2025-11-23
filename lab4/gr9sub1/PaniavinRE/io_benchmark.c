/*
 * io_benchmark.c - Бенчмарк разных методов I/O
 *
 * Компиляция: gcc -Wall -Wextra -O2 io_benchmark.c -o io_benchmark
 * Использование:
 *   ./io_benchmark --size <MB>
 *
 * Пример:
 *   ./io_benchmark --size 100
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#define BUFFER_SIZE (1024 * 1024)  // 1 MB

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Создать файл
void create_file(const char *filename, size_t size_mb) {
    printf("Creating %s (%zu MB)... ", filename, size_mb);
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) { perror("open"); exit(1); }

    char *buf = malloc(BUFFER_SIZE);
    memset(buf, 'X', BUFFER_SIZE);

    size_t total = size_mb * 1024 * 1024;
    for (size_t written = 0; written < total; written += BUFFER_SIZE) {
        size_t to_write = (written + BUFFER_SIZE > total) ? (total - written) : BUFFER_SIZE;
        if (write(fd, buf, to_write) != (ssize_t)to_write) {
            perror("write"); exit(1);
        }
    }

    free(buf);
    close(fd);
    printf("OK\n");
}

// read()
void bench_read(const char *filename, size_t size) {
    printf("read()... ");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror("open"); return; }

    char *buf = malloc(BUFFER_SIZE);
    double start = get_time();
    size_t total = 0;
    while (total < size) {
        size_t to_read = (total + BUFFER_SIZE > size) ? (size - total) : BUFFER_SIZE;
        ssize_t n = read(fd, buf, to_read);
        if (n <= 0) break;
        total += n;
    }
    double end = get_time();
    close(fd);
    free(buf);

    double sec = end - start;
    double speed = size / (1024.0 * 1024.0) / sec;
    printf("%.2f s, %.1f MB/s\n", sec, speed);
}

// mmap()
void bench_mmap(const char *filename, size_t size) {
    printf("mmap()... ");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror("open"); return; }

    void *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); close(fd); return; }

    double start = get_time();
    volatile char sink = 0;
    for (size_t i = 0; i < size; i += 4096) {
        sink += ((char*)map)[i];
    }
    double end = get_time();

    munmap(map, size);
    close(fd);

    double sec = end - start;
    double speed = size / (1024.0 * 1024.0) / sec;
    printf("%.2f s, %.1f MB/s\n", sec, speed);
}

int main(int argc, char *argv[]) {
    size_t size_mb = 100;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size_mb = atoi(argv[++i]);
        }
    }

    if (size_mb == 0) {
        fprintf(stderr, "Size must be > 0\n");
        return 1;
    }

    const char *file = "test_io.bin";
    create_file(file, size_mb);

    printf("\n--- I/O Benchmark (%zu MB) ---\n", size_mb);

    bench_read(file, size_mb * 1024 * 1024);
    bench_mmap(file, size_mb * 1024 * 1024);

    printf("\nDone. File: %s\n", file);
    return 0;
}
