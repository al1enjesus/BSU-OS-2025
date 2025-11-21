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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
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

double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }

    // Подготовить буфер с данными
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }
    memset(buffer, 'A', buffer_size);

    double start = get_time();

    for (size_t readen = 0; readen < size; readen += buffer_size) {
        size_t to_read = (size - readen < buffer_size) ? (size - readen) : buffer_size;
        if (fwrite(buffer, 1, to_read, f) != to_read) {
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

    // Подготовить буфер
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);

    double start = get_time();

    for (size_t readen = 0; readen < size; readen += buffer_size) {
        size_t to_read = (size - readen < buffer_size) ? (size - readen) : buffer_size;
        if (write(fd, buffer, to_read) != (ssize_t)to_read) {
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


double benchmark_write_sync(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with O_SYNC (buffer=%zu bytes) ===\n", buffer_size);


    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);

    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    // Подготовить буфер
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);


    double start = get_time();

    for (size_t readen = 0; readen < size; readen += buffer_size) {
        size_t to_read = (size - readen < buffer_size) ? (size - readen) : buffer_size;
        if (write(fd, buffer, to_read) != (ssize_t)to_read) {
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

        // Удалить файл после теста
        unlink(filename);

        // Небольшая пауза между тестами
        sleep(1);
    }
}

void benchmark_read_methods(const char *filename, size_t buffer_size);

// TODO: Главное сравнение всех методов
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
    benchmark_write_sync("test_write.bin", file_size, optimal_buffer);
    benchmark_read_methods("test_write.bin", optimal_buffer);
    unlink("test_write.bin");
    sleep(1);


    printf("\n=== Summary ===\n");
    printf("Fastest method: (compare results above)\n");
    printf("\nFactors affecting performance:\n");
    printf("- stdio (fwrite) has user-space buffering\n");
    printf("- write() goes directly to kernel, but still uses page cache\n");
    printf("- mmap() allows direct memory access, lazy writes\n");
    printf("- Actual disk speed depends on: HDD vs SSD, filesystem, etc.\n");
}


void benchmark_read_methods(const char *filename, size_t buffer_size) {
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

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        return ;
    }


    printf("\n--- fread() ---\n");
    // Реализовать чтение через fread() с замером времени

    FILE* fd = fopen(filename,"rb");

    double start = get_time();

    for (size_t readen = 0; readen < file_size; readen += buffer_size) {
        size_t to_read = (file_size - readen < buffer_size) ? (file_size - readen) : buffer_size;
        if (fread(buffer, sizeof(buffer[0]), to_read, fd) != (size_t)to_read) {
            perror("read failed");
            break;
        }
    }

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / elapsed);
    fclose(fd);



    printf("\n--- read() ---\n");
    // Реализовать чтение через read() с замером времени

    int fr = open(filename, O_RDONLY);

    start = get_time();

    for (size_t readen = 0; readen < file_size; readen += buffer_size) {
        size_t to_read = (file_size - readen < buffer_size) ? (file_size - readen) : buffer_size;
        if (read(fr, buffer, to_read) != (ssize_t)to_read) {
            perror("read failed");
            break;
        }
    }

    end = get_time();
    elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fr);


}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    // Парсинг аргументов
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

    printf("\n");

    benchmark_buffer_sizes(size_mb);

    printf("\n========================================\n");
    printf("Benchmark completed!\n");
    printf("========================================\n");

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
