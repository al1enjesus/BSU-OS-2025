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

// Реализовать функцию замера времени
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Метод 1 - fwrite() (stdio, буферизованный)
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

// Метод 2 - write() (системный вызов, небуферизованный на уровне stdio)
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

// Метод 3 - write() с O_SYNC (синхронная запись)
double benchmark_write_sync(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with O_SYNC (buffer=%zu bytes) ===\n", buffer_size);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
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
    memset(buffer, 'C', buffer_size);

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
    printf("Note: O_SYNC forces physical write to disk - very slow!\n");

    free(buffer);
    close(fd);

    return elapsed;
}

// Метод 4 - mmap() (memory-mapped I/O)
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

    memset(data, 'D', size);

    // Принудительно записать на диск
    if (msync(data, size, MS_SYNC) == -1) {
        perror("msync failed");
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    munmap(data, size);
    close(fd);

    return elapsed;
}

// Сравнение разных размеров буфера для write()
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

        double time = benchmark_write(filename, file_size, buffer_sizes[i]);

        unlink(filename);
        sleep(1);
    }
}

// Главное сравнение всех методов
void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t optimal_buffer = 64 * 1024;

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer / 1024);

    double times[4] = {0};
    
    // Метод 1: fwrite
    times[0] = benchmark_fwrite("test_fwrite.bin", file_size, optimal_buffer);
    unlink("test_fwrite.bin");
    sleep(1);

    // Метод 2: write
    times[1] = benchmark_write("test_write.bin", file_size, optimal_buffer);
    unlink("test_write.bin");
    sleep(1);

    // Метод 3: mmap
    times[2] = benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");
    sleep(1);


    printf("\n=== Summary ===\n");
    const char *methods[] = {"fwrite", "write", "mmap", "write+sync"};
    double min_time = times[0];
    const char *fastest = methods[0];
    
    for (int i = 0; i < 3; i++) {
        printf("%s: %.3f seconds", methods[i], times[i]);
        if (times[i] > 0) {
            if (times[i] < min_time) {
                min_time = times[i];
                fastest = methods[i];
            }
        }
        printf("\n");
    }
    
    printf("Fastest method: %s (%.3f seconds)\n", fastest, min_time);
    
    printf("\nFactors affecting performance:\n");
    printf("- stdio (fwrite) has user-space buffering\n");
    printf("- write() goes directly to kernel, but still uses page cache\n");
    printf("- mmap() allows direct memory access, lazy writes\n");
    printf("- Actual disk speed depends on: HDD vs SSD, filesystem, etc.\n");
}

// Бенчмарк чтения
void benchmark_read_methods(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }

    size_t file_size = sb.st_size;
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / (1024.0 * 1024.0));

    // Метод 1 - fread()
    printf("\n--- fread() ---\n");
    FILE *f = fopen(filename, "rb");
    if (f) {
        char *buffer = malloc(65536);
        double start = get_time();
        
        while (fread(buffer, 1, 65536, f) > 0) {
            // Просто читаем данные
        }
        
        double end = get_time();
        printf("Time: %.3f seconds\n", end - start);
        printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
        
        free(buffer);
        fclose(f);
    }

    // Метод 2 - read()
    printf("\n--- read() ---\n");
    int fd = open(filename, O_RDONLY);
    if (fd != -1) {
        char *buffer = malloc(65536);
        double start = get_time();
        ssize_t bytes_read;
        
        while ((bytes_read = read(fd, buffer, 65536)) > 0) {
            // Просто читаем данные
        }
        
        double end = get_time();
        printf("Time: %.3f seconds\n", end - start);
        printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
        
        free(buffer);
        close(fd);
    }

    // Метод 3 - mmap()
    printf("\n--- mmap() ---\n");
    fd = open(filename, O_RDONLY);
    if (fd != -1) {
        void *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data != MAP_FAILED) {
            double start = get_time();
            
            // Просто обращаемся ко всем данным
            volatile char sum = 0;
            for (size_t i = 0; i < file_size; i += 4096) {
                sum += ((char*)data)[i];
            }
            
            double end = get_time();
            printf("Time: %.3f seconds\n", end - start);
            printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
            
            munmap(data, file_size);
        }
        close(fd);
    }
}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size_mb = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --size SIZE_MB   Size of test file in megabytes (default: %d)\n", DEFAULT_SIZE_MB);
            printf("\nExamples:\n");
            printf("  %s                 # Use default size (100 MB)\n", argv[0]);
            printf("  %s --size 500      # Test with 500 MB file\n", argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--read") == 0 && i + 1 < argc) {
            benchmark_read_methods(argv[i + 1]);
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