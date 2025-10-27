#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>


#define COLOR_RED       "\x1b[31m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"

#define COLOR_BOLD      "\x1b[1m"
#define COLOR_RESET     "\x1b[0m"

#define DEFAULT_SIZE_MB 100

// Реализовать функцию замера времени
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Метод 1 - fwrite() (stdio, буферизованный)
double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer = ");
    if (buffer_size < 1024) {
        printf(COLOR_BOLD "%lu" COLOR_RESET " B", buffer_size);
    }
    else {
        printf(COLOR_BOLD "%.1f" COLOR_RESET " KB", buffer_size/1024.0);
    }
    printf(" ===\n");

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

    // TODO: Замерить время записи
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
    printf("Throughput: " COLOR_BOLD COLOR_GREEN "%.2f" COLOR_RESET " MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    fclose(f);

    return elapsed;
}

// Метод 2 - write() (системный вызов, небуферизованный на уровне stdio)
double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with buffer = ");
    if (buffer_size < 1024) {
        printf(COLOR_BOLD "%lu" COLOR_RESET " B", buffer_size);
    }
    else {
        printf(COLOR_BOLD "%.1f" COLOR_RESET " KB", buffer_size/1024.0);
    }
    printf(" ===\n");

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

    // Замерить время записи
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
    printf("Throughput: " COLOR_BOLD COLOR_GREEN "%.2f" COLOR_RESET " MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    free(buffer);
    close(fd);

    return elapsed;
}

// Метод 3 - mmap() (memory-mapped I/O)
double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    // Установить размер файла
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        close(fd);
        return -1;
    }

    // Отобразить файл в память
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }

    // Замерить время записи
    double start = get_time();

    memset(data, 'C', size);

    double end = get_time();
    double elapsed = end - start;

    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: " COLOR_BOLD COLOR_GREEN "%.2f" COLOR_RESET " MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);

    if (data) {
        munmap(data, size);
    }
    close(fd);

    return elapsed;
}

// TODO: Сравнение разных размеров буфера для write()
void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Buffer Size Impact\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * 1024 * 1024;
    size_t buffer_sizes[] = {512, 1024, 4096, 8192, 16384, 65536, 512*1024, 1024*1024};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);

    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        /*double time = */benchmark_write(filename, file_size, buffer_sizes[i]);

        // Удалить файл после теста
        unlink(filename);

        // Небольшая пауза между тестами
        sleep(1);
    }
}

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
    benchmark_write("test_write.bin", file_size, optimal_buffer);
    unlink("test_write.bin");
    sleep(1);

    // Метод 3: mmap
    benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");

    printf("\n=== Summary ===\n");
    printf("Fastest method: (compare results above)\n");
    printf("\nFactors affecting performance:\n");
    printf("- stdio (fwrite) has user-space buffering\n");
    printf("- write() goes directly to kernel, but still uses page cache\n");
    printf("- mmap() allows direct memory access, lazy writes\n");
    printf("- Actual disk speed depends on: HDD vs SSD, filesystem, etc.\n");
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
    printf("Test file size: " COLOR_BOLD "%zu" COLOR_RESET " MB\n", size_mb);

    // TODO: Для чистоты эксперимента можно очистить page cache
    // system("sync");  // Сбросить буферы на диск
    // system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");  // Очистить cache

    // Запустить бенчмарки
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
