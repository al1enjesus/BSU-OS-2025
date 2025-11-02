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

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }

    char *buffer = (char *) malloc(buffer_size);
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

    fflush(f);

    double end = get_time();
    double elapsed = end - start;
    if (elapsed <= 0) elapsed = 1e-9;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    fclose(f);

    return elapsed;
}

static double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
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
        ssize_t w = write(fd, buffer, to_write);
        if (w < 0) {
            perror("write failed");
            break;
        }
    }

    double end = get_time();
    double elapsed = end - start;

    if (elapsed <= 0) elapsed = 1e-9;
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

static double benchmark_write_sync(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with O_SYNC (buffer=%zu bytes) ===\n", buffer_size);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    char *buffer = (char *) malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'S', buffer_size);

    double start = get_time();

    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        ssize_t w = write(fd, buffer, to_write);
        if (w < 0) {
            perror("write failed");
            break;
        }
    }

    double end = get_time();
    double elapsed = end - start;
    if (elapsed <= 0) elapsed = 1e-9;
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

static double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    if (ftruncate(fd, (off_t) size) == -1) {
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
    msync(data, size, MS_SYNC);

    double end = get_time();
    double elapsed = end - start;

    if (elapsed <= 0) elapsed = 1e-9;
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    munmap(data, size);
    close(fd);

    return elapsed;
}

static void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Buffer Size Impact\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t buffer_sizes[] = {512, 1024, 4096, 8192, 16384, 65536, 1024 * 1024};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);

    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        double t = benchmark_write(filename, file_size, buffer_sizes[i]);
        (void) t;
        unlink(filename);
        sleep(1);
    }
}

static void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t optimal_buffer = 64 * 1024;

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer / 1024);

    benchmark_fwrite("test_fwrite.bin", file_size, optimal_buffer);
    unlink("test_fwrite.bin");
    sleep(1);

    benchmark_write("test_write.bin", file_size, optimal_buffer);
    unlink("test_write.bin");
    sleep(1);

    // медленно
    benchmark_write_sync("test_write_sync.bin", file_size, optimal_buffer);
    unlink("test_write_sync.bin");
    sleep(1);

    benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");

    printf("\n=== Summary ===\n");
    printf("Смотри тайминги выше. В общем случае:\n");
    printf("- Малый буфер => много syscall => медленнее\n");
    printf("- stdio (fwrite) сглаживает мелкие записи => часто быстрее при мелких блоках\n");
    printf("- O_SYNC сильно замедляет (ждём физзаписи)\n");
    printf("- mmap позволяет писать/читать как память, ядро само флашит\n");
}

static void benchmark_read_methods(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }

    size_t file_size = (size_t) sb.st_size;
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / (1024.0 * 1024.0));

    const size_t BUF = 64 * 1024;

    // fread()
    {
        printf("\n--- fread() ---\n");
        FILE *f = fopen(filename, "rb");
        if (!f) { perror("fopen"); } else {
            char *buf = malloc(BUF);
            if (!buf) {
                perror("malloc");
                fclose(f);
            } else {
                double t0 = get_time();
                size_t rtot = 0;
                while (1) {
                    size_t r = fread(buf, 1, BUF, f);
                    rtot += r;
                    if (r < BUF) {
                        if (feof(f)) break;
                        perror("fread");
                        break;
                    }
                }
                double t1 = get_time(), dt = t1 - t0;
                if (dt <= 0) dt = 1e-9;
                printf("Time: %.3f s, Speed: %.2f MB/s\n", dt, (rtot / 1024.0 / 1024.0) / dt);
                free(buf);
                fclose(f);
            }
        }
    }

    // read()
    {
        printf("\n--- read() ---\n");
        int fd = open(filename, O_RDONLY);
        if (fd < 0) { perror("open"); } else {
            char *buf = malloc(BUF);
            if (!buf) {
                perror("malloc");
                close(fd);
            } else {
                double t0 = get_time();
                size_t rtot = 0;
                while (1) {
                    ssize_t r = read(fd, buf, BUF);
                    if (r < 0) {
                        perror("read");
                        break;
                    }
                    if (r == 0) break;
                    rtot += (size_t) r;
                }
                double t1 = get_time(), dt = t1 - t0;
                if (dt <= 0) dt = 1e-9;
                printf("Time: %.3f s, Speed: %.2f MB/s\n", dt, (rtot / 1024.0 / 1024.0) / dt);
                free(buf);
                close(fd);
            }
        }
    }

    // mmap()
    {
        printf("\n--- mmap() ---\n");
        int fd = open(filename, O_RDONLY);
        if (fd < 0) { perror("open"); } else {
            void *p = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (p == MAP_FAILED) {
                perror("mmap");
                close(fd);
            } else {
                volatile unsigned long long sum = 0;
                double t0 = get_time();
                unsigned char *b = p;
                for (size_t i = 0; i < file_size; i++) sum += b[i];
                double t1 = get_time(), dt = t1 - t0;
                if (dt <= 0) dt = 1e-9;
                printf("Time: %.3f s, Speed: %.2f MB/s (checksum=%llu)\n",
                       dt, (file_size / 1024.0 / 1024.0) / dt, sum);
                munmap(p, file_size);
                close(fd);
            }
        }
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
        }
    }

    printf("I/O Benchmark\n");
    printf("=============\n");
    printf("Test file size: %zu MB\n", size_mb);

    benchmark_all_methods(size_mb);

    benchmark_buffer_sizes(size_mb);

    {
        const char *tf = "rb_file.bin";
        size_t bytes = size_mb * 1024 * 1024;
        int fd = open(tf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            const size_t BUF = 64 * 1024;
            char *b = (char *) malloc(BUF);
            memset(b, 0x5A, BUF);
            size_t w = 0;
            while (w < bytes) {
                size_t t = (bytes - w < BUF ? bytes - w : BUF);
                if (write(fd, b, t) < 0) {
                    perror("write");
                    break;
                }
                w += t;
            }
            free(b);
            close(fd);
            benchmark_read_methods(tf);
            unlink(tf);
        }
    }

    printf(
        "\n========================================\nBenchmark completed!\n========================================\n");
    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте все TODO функции
 *
 * 2. Запустите бенчмарк с разными размерами:
 *    $ ./io_benchmark --size 100
 *    $ ./io_benchmark --size 500
 *
 * 3. Для чистого эксперимента очистите page cache перед запуском:
 *    $ sync
 *    $ sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
 *    $ ./io_benchmark
 *
 * 4. Проанализируйте результаты:
 *    - Какой метод самый быстрый для записи?
 *    - Как влияет размер буфера на производительность?
 *    - Есть ли "оптимальный" размер буфера?
 *    - Почему очень маленький буфер (512 байт) медленный?
 *    - Почему очень большой буфер (1 MB) не даёт пропорционального ускорения?
 *
 * 5. Дополнительные эксперименты:
 *    - Реализуйте benchmark_write_sync() и сравните (будет ОЧЕНЬ медленно!)
 *    - Реализуйте benchmark_read_methods() для сравнения чтения
 *    - Измерьте разницу на HDD vs SSD (если доступно)
 *    - Попробуйте O_DIRECT (прямой I/O минуя page cache)
 *    - Используйте strace для подсчёта системных вызовов:
 *      $ strace -c ./io_benchmark --size 10
 *
 * 6. Постройте графики:
 *    - Размер буфера (ось X) vs Throughput MB/s (ось Y)
 *    - Сравнение методов (столбчатая диаграмма)
 *
 * 7. В отчёте объясните:
 *    - Почему fwrite может быть быстрее write несмотря на дополнительный слой?
 *    - Что такое page cache и как он влияет на результаты?
 *    - Когда имеет смысл использовать каждый метод?
 *    - Как файловая система влияет на производительность?
 */
