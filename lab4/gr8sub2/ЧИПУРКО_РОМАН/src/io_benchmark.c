/*
 * io_benchmark.c - Бенчмарк различных методов файлового I/O
 *
 * Компиляция: gcc -Wall -Wextra -O2 io_benchmark.c -o io_benchmark
 * Использование: ./io_benchmark [--size SIZE_MB]
 *
 * Демонстрирует:
 * - Сравнение fwrite() vs write() vs mmap()
 * - Влияние размера буфера на производительность
 * - Буферизованный vs небуферизованный I/O
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
#include <stdint.h>

#define DEFAULT_SIZE_MB 100

double get_time() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        return 0.0;
    }
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static inline size_t min_size(size_t a, size_t b) { return (a < b) ? a : b; }

// Метод 1 - fwrite() (stdio, буферизованный)
double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "fopen('%s') failed: %s\n", filename, strerror(errno));
        return -1;
    }


    if (buffer_size > 0) {
        if (setvbuf(f, NULL, _IOFBF, buffer_size) != 0) {

        }
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }
    memset(buffer, 'A', buffer_size);

    size_t written = 0;
    double start = get_time();

    while (written < size) {
        size_t to_write = min_size(buffer_size, size - written);
        size_t w = fwrite(buffer, 1, to_write, f);
        if (w != to_write) {
            if (ferror(f)) {
                fprintf(stderr, "fwrite failed after %zu bytes: %s\n", written, strerror(errno));
            }
            break;
        }
        written += w;
    }

    fflush(f);

    double end = get_time();
    double elapsed = end - start;

    if (elapsed <= 0.0) elapsed = 1e-9;

    printf("Written: %zu bytes (requested %zu)\n", written, size);
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (written / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    fclose(f);

    return elapsed;
}

// Метод 2 - write() (системный вызов, небуферизованный на уровне stdio)
double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "open('%s') failed: %s\n", filename, strerror(errno));
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);

    size_t written = 0;
    double start = get_time();

    while (written < size) {
        size_t to_write = min_size(buffer_size, size - written);
        ssize_t w = write(fd, buffer, to_write);
        if (w == -1) {
            fprintf(stderr, "write failed after %zu bytes: %s\n", written, strerror(errno));
            break;
        }
        written += (size_t)w;
    }


    double end = get_time();
    double elapsed = end - start;
    if (elapsed <= 0.0) elapsed = 1e-9;

    printf("Written: %zu bytes (requested %zu)\n", written, size);
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (written / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

// Метод 3 - write() с O_SYNC (синхронная запись)
double benchmark_write_sync(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with O_SYNC (buffer=%zu bytes) ===\n", buffer_size);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "open('%s', O_SYNC) failed: %s\n", filename, strerror(errno));
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'S', buffer_size);

    size_t written = 0;
    double start = get_time();

    while (written < size) {
        size_t to_write = min_size(buffer_size, size - written);
        ssize_t w = write(fd, buffer, to_write);
        if (w == -1) {
            fprintf(stderr, "write(O_SYNC) failed after %zu bytes: %s\n", written, strerror(errno));
            break;
        }
        written += (size_t)w;
    }


    double end = get_time();
    double elapsed = end - start;
    if (elapsed <= 0.0) elapsed = 1e-9;

    printf("Written: %zu bytes (requested %zu)\n", written, size);
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (written / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

// Метод 4 - mmap() (memory-mapped I/O)
double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "open('%s') failed: %s\n", filename, strerror(errno));
        return -1;
    }

    if (size == 0) {
        close(fd);
        return -1;
    }

    if (ftruncate(fd, (off_t)size) == -1) {
        fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    double start = get_time();

    memset(data, 'C', size);

    if (msync(data, size, MS_SYNC) != 0) {
        fprintf(stderr, "msync failed: %s\n", strerror(errno));
    }

    double end = get_time();
    double elapsed = end - start;
    if (elapsed <= 0.0) elapsed = 1e-9;

    printf("Touched+msync: %zu bytes\n", size);
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    if (munmap(data, size) != 0) {
        fprintf(stderr, "munmap failed: %s\n", strerror(errno));
    }
    close(fd);

    return elapsed;
}

void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Buffer Size Impact\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t buffer_sizes[] = {512, 1024, 4096, 8192, 16384, 65536, 1024*1024};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);

    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        benchmark_write(filename, file_size, buffer_sizes[i]);

        unlink(filename);

        sleep(1);
    }
}

void benchmark_read_methods(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        fprintf(stderr, "stat('%s') failed: %s\n", filename, strerror(errno));
        return;
    }

    size_t file_size = sb.st_size;
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / (1024.0 * 1024.0));

    // Метод 1 - fread()
    printf("\n--- fread() ---\n");
    {
        FILE *f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "fopen('%s') for read failed: %s\n", filename, strerror(errno));
        } else {
            size_t buf_size = 64 * 1024;
            char *buffer = malloc(buf_size);
            if (!buffer) {
                perror("malloc");
            } else {
                size_t total = 0;
                double start = get_time();
                while (1) {
                    size_t r = fread(buffer, 1, buf_size, f);
                    total += r;
                    if (r == 0) break;
                }
                double end = get_time();
                double elapsed = end - start;
                if (elapsed <= 0.0) elapsed = 1e-9;
                printf("Read: %zu bytes\n", total);
                printf("Time: %.3f s\n", elapsed);
                printf("Throughput: %.2f MB/s\n", (total / (1024.0 * 1024.0)) / elapsed);
                free(buffer);
            }
            fclose(f);
        }
    }

    // Метод 2 - read()
    printf("\n--- read() ---\n");
    {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "open('%s') for read failed: %s\n", filename, strerror(errno));
        } else {
            size_t buf_size = 64 * 1024;
            char *buffer = malloc(buf_size);
            if (!buffer) {
                perror("malloc");
            } else {
                size_t total = 0;
                double start = get_time();
                while (1) {
                    ssize_t r = read(fd, buffer, buf_size);
                    if (r == -1) {
                        fprintf(stderr, "read error: %s\n", strerror(errno));
                        break;
                    }
                    if (r == 0) break;
                    total += (size_t)r;
                }
                double end = get_time();
                double elapsed = end - start;
                if (elapsed <= 0.0) elapsed = 1e-9;
                printf("Read: %zu bytes\n", total);
                printf("Time: %.3f s\n", elapsed);
                printf("Throughput: %.2f MB/s\n", (total / (1024.0 * 1024.0)) / elapsed);
                free(buffer);
            }
            close(fd);
        }
    }

    // Метод 3 - mmap()
    printf("\n--- mmap() ---\n");
    {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "open('%s') for mmap failed: %s\n", filename, strerror(errno));
        } else {
            if (file_size == 0) {
                close(fd);
            } else {
                void *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (data == MAP_FAILED) {
                    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
                } else {
                    volatile uint64_t sum = 0;
                    double start = get_time();
                    unsigned char *p = (unsigned char *)data;
                    for (size_t i = 0; i < file_size; i++) {
                        sum += p[i];
                    }
                    double end = get_time();
                    double elapsed = end - start;
                    if (elapsed <= 0.0) elapsed = 1e-9;
                    printf("Touched: %zu bytes (checksum=%llu)\n", file_size, (unsigned long long)sum);
                    printf("Time: %.3f s\n", elapsed);
                    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / elapsed);

                    if (munmap(data, file_size) != 0) {
                        fprintf(stderr, "munmap failed: %s\n", strerror(errno));
                    }
                }
                close(fd);
            }
        }
    }
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

    // Метод 3: write + O_SYNC (опционально, медленно)
    benchmark_write_sync("test_write_sync.bin", file_size, optimal_buffer);
    unlink("test_write_sync.bin");
    sleep(1);

    // Метод 4: mmap
    benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");

    printf("\n=== Summary ===\n");
    printf("Fastest method: (compare results above)\n");
    printf("\nFactors affecting performance:\n");
    printf("- stdio (fwrite) has user-space buffering\n");
    printf("- write() goes directly to kernel, but still uses page cache\n");
    printf("- mmap() allows direct memory access, lazy writes (msync forces write)\n");
    printf("- Actual disk speed depends on: HDD vs SSD, filesystem, etc.\n");
}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    // Парсинг аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size_mb = (size_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --size SIZE_MB   Size of test file in megabytes (default: %d)\n", DEFAULT_SIZE_MB);
            printf("\nExamples:\n");
            printf("  %s                 # Use default size (100 MB)\n", argv[0]);
            printf("  %s --size 500      # Test with 500 MB file\n", argv[0]);
            return 0;
        }
    }

    printf("I/O Benchmark\n");
    printf("=============\n");
    printf("Test file size: %zu MB\n", size_mb);

    benchmark_all_methods(size_mb);

    printf("\n");

    benchmark_buffer_sizes(size_mb);

    printf("\n========================================\n");
    printf("Benchmark completed!\n");
    printf("========================================\n");

    return 0;
}
