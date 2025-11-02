#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_SIZE_MB 100

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Метод 1 - fwrite() (stdio, буферизованный)
double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("=== fwrite() with buffer=%zu bytes ===\n", buffer_size);

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

    size_t written = 0;
    while (written < size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        if (fwrite(buffer, 1, to_write, f) != to_write) {
            perror("fwrite failed");
            break;
        }
        written += to_write;
    }

    fflush(f);
    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);
    printf("Operations: %zu\n", (size + buffer_size - 1) / buffer_size);

    free(buffer);
    fclose(f);

    return elapsed;
}

// Метод 2 - write() (системный вызов)
double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
    printf("=== write() with buffer=%zu bytes ===\n", buffer_size);

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

    size_t written = 0;
    while (written < size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        ssize_t result = write(fd, buffer, to_write);
        if (result == -1) {
            perror("write failed");
            break;
        }
        written += result;
    }

    fsync(fd);
    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);
    printf("Syscalls: %zu\n", (size + buffer_size - 1) / buffer_size);

    free(buffer);
    close(fd);

    return elapsed;
}

// Метод 3 - fread() для чтения
double benchmark_fread(const char *filename) {
    printf("=== fread() ===\n");

    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        fclose(f);
        return -1;
    }

    // Переменная size больше не используется, но оставлена для ясности
    // size_t size = sb.st_size;
    
    char *buffer = malloc(65536); // 64KB buffer
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }

    double start = get_time();
    size_t total_read = 0;
    unsigned long long checksum = 0;

    while (!feof(f)) {
        size_t bytes_read = fread(buffer, 1, 65536, f);
        if (bytes_read == 0) break;
        
        for (size_t i = 0; i < bytes_read; i++) {
            checksum += (unsigned char)buffer[i];
        }
        total_read += bytes_read;
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (total_read / (1024.0 * 1024.0)) / elapsed);
    printf("Checksum: %llu\n", checksum);

    free(buffer);
    fclose(f);
    return elapsed;
}

// Метод 4 - read() для чтения
double benchmark_read(const char *filename) {
    printf("=== read() ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return -1;
    }

    // Переменная size больше не используется, но оставлена для ясности
    // size_t size = sb.st_size;
    
    char *buffer = malloc(65536);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }

    double start = get_time();
    size_t total_read = 0;
    unsigned long long checksum = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, 65536)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            checksum += (unsigned char)buffer[i];
        }
        total_read += bytes_read;
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (total_read / (1024.0 * 1024.0)) / elapsed);
    printf("Checksum: %llu\n", checksum);

    free(buffer);
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

    printf("Testing write() with different buffer sizes:\n");
    printf("File size: %zu MB\n\n", file_size_mb);

    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        printf("Buffer: %7zu bytes -> ", buffer_sizes[i]);
        // Результат benchmark_write игнорируется намеренно
        (void)benchmark_write(filename, file_size, buffer_sizes[i]);

        unlink(filename);
        sleep(1);
    }
}

void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t buffer_sizes[] = {512, 4096, 65536};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

    printf("File size: %zu MB\n\n", file_size_mb);

    // Тестируем запись с разными размерами буфера
    for (int i = 0; i < num_sizes; i++) {
        printf("--- Testing with %zu byte buffer ---\n", buffer_sizes[i]);
        
        benchmark_fwrite("test_fwrite.bin", file_size, buffer_sizes[i]);
        unlink("test_fwrite.bin");
        sleep(1);

        benchmark_write("test_write.bin", file_size, buffer_sizes[i]);
        unlink("test_write.bin");
        sleep(1);
        
        printf("\n");
    }

    // Тестируем чтение
    printf("--- Testing Read Methods ---\n");
    // Создаем файл для чтения
    benchmark_write("test_read.bin", file_size, 65536);
    
    benchmark_fread("test_read.bin");
    benchmark_read("test_read.bin");
    
    unlink("test_read.bin");
}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size_mb = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
            return 0;
        }
    }

    printf("I/O Benchmark - Variant 2\n");
    printf("==========================\n");
    printf("Test file size: %zu MB\n", size_mb);

    // Очистка кеша для чистоты эксперимента
    int ret1 = system("sync");
    int ret2 = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' 2>/dev/null");
    
    // Игнорируем возвращаемые значения system() намеренно
    (void)ret1;
    (void)ret2;

    benchmark_all_methods(size_mb);
    benchmark_buffer_sizes(size_mb);

    printf("\n========================================\n");
    printf("Benchmark completed!\n");
    printf("========================================\n");

    return 0;
}
