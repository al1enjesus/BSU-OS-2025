/*
 * io_benchmark.c - Бенчмарк различных методов файлового I/O
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_SIZE_MB 10

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }
    memset(buffer, 'A', buffer_size);

    double start = get_time();

    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        if (fwrite(buffer, 1, to_write, f) != to_write) {
            perror("fwrite failed");
            break;
        }
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    fclose(f);

    return elapsed;
}

double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);

    double start = get_time();

    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        if (write(fd, buffer, to_write) != (ssize_t)to_write) {
            perror("write failed");
            break;
        }
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        close(fd);
        return -1;
    }

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }

    double start = get_time();

    memset(data, 'C', size);

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    munmap(data, size);
    close(fd);

    return elapsed;
}

void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t optimal_buffer = 64 * 1024;  // 64 KB

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer / 1024);

    // Метод 1: fwrite
    benchmark_fwrite("test_fwrite.bin", file_size, optimal_buffer);
    unlink("test_fwrite.bin");
    sleep(1);

    // Метод 2: write
    benchmark_write("test_write.bin", file_size, optimal_buffer);
    unlink("test_write.bin");
    sleep(1);

    // Метод 3: mmap
    benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");

    printf("\n=== Summary ===\n");
    printf("Compare the throughput values above to determine fastest method.\n");
}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    if (argc > 1 && strcmp(argv[1], "--size") == 0 && argc > 2) {
        size_mb = atoi(argv[2]);
    } else if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
        printf("Default size: %d MB\n", DEFAULT_SIZE_MB);
        return 0;
    }

    printf("I/O Benchmark\n");
    printf("=============\n");
    printf("Test file size: %zu MB\n", size_mb);

    benchmark_all_methods(size_mb);

    printf("\nBenchmark completed!\n");

    return 0;
}
