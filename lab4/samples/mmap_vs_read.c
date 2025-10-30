/*
 * mmap_vs_read.c - Сравнение производительности mmap() vs read()
 *
 * Компиляция: gcc -Wall -Wextra -O2 mmap_vs_read.c -o mmap_vs_read
 * Использование: ./mmap_vs_read <filename>
 *
 * Демонстрирует:
 * - Традиционный I/O через read()
 * - Memory-mapped I/O через mmap()
 * - Замер времени и page faults для обоих методов
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>

// Размер буфера для read()
#define BUFFER_SIZE (4 * 1024)  // 4 KB

// Структура для хранения статистики page faults
typedef struct {
    long minor_faults;
    long major_faults;
} PageFaultStats;

// TODO: Реализовать функцию для получения текущего количества page faults
PageFaultStats get_page_faults() {
    PageFaultStats stats = {0, 0};

    // TODO: Использовать getrusage() для получения статистики
    // struct rusage usage;
    // getrusage(RUSAGE_SELF, &usage);
    // stats.minor_faults = usage.ru_minflt;
    // stats.major_faults = usage.ru_majflt;

    return stats;
}

// TODO: Реализовать функцию для замера времени (возвращает секунды)
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// TODO: Реализовать метод 1 - чтение через read()
unsigned long long read_with_syscalls(const char *filename) {
    printf("\n=== Method 1: read() syscall ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    // Получить размер файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    // TODO: Выделить буфер для чтения
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return 0;
    }

    // Начать замер
    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    // TODO: Читать файл блоками и считать сумму всех байтов
    unsigned long long sum = 0;
    ssize_t bytes_read;

    // while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
    //     for (ssize_t i = 0; i < bytes_read; i++) {
    //         sum += (unsigned char)buffer[i];
    //     }
    // }

    // if (bytes_read == -1) {
    //     perror("read failed");
    // }

    // Закончить замер
    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    // Вывести результаты
    printf("Time elapsed: %.3f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    // Освободить ресурсы
    free(buffer);
    close(fd);

    return sum;
}

// TODO: Реализовать метод 2 - чтение через mmap()
unsigned long long read_with_mmap(const char *filename) {
    printf("\n=== Method 2: mmap() ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    // Получить размер файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    // TODO: Отобразить файл в память
    // void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    // if (data == MAP_FAILED) {
    //     perror("mmap failed");
    //     close(fd);
    //     return 0;
    // }

    void *data = NULL;  // TODO: убрать после реализации

    // Начать замер
    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    // TODO: Прочитать весь файл как массив байтов
    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;

    // for (off_t i = 0; i < sb.st_size; i++) {
    //     sum += bytes[i];
    // }

    // Закончить замер
    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    // Вывести результаты
    printf("Time elapsed: %.3f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    // Освободить ресурсы
    if (data) {
        // munmap(data, sb.st_size);
    }
    close(fd);

    return sum;
}

// TODO: Опционально - метод 3: mmap() с madvise
unsigned long long read_with_mmap_sequential(const char *filename) {
    printf("\n=== Method 3: mmap() + madvise(SEQUENTIAL) ===\n");

    // TODO: То же, что read_with_mmap(), но после mmap() вызвать:
    // madvise(data, sb.st_size, MADV_SEQUENTIAL);
    //
    // Это подсказка ядру, что мы будем читать файл последовательно.
    // Ядро может начать prefetch следующих страниц.

    printf("(Not implemented yet)\n");
    return 0;
}

// TODO: Создать тестовый файл заданного размера
void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file '%s' (%zu MB)...\n", filename, size_mb);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    size_t total_bytes = size_mb * 1024 * 1024;
    char buffer[4096];

    // Заполнить буфер псевдослучайными данными
    for (int i = 0; i < 4096; i++) {
        buffer[i] = (char)(i % 256);
    }

    // TODO: Записать данные в файл блоками
    // for (size_t written = 0; written < total_bytes; written += sizeof(buffer)) {
    //     size_t to_write = (total_bytes - written < sizeof(buffer)) ?
    //                       (total_bytes - written) : sizeof(buffer);
    //     if (write(fd, buffer, to_write) == -1) {
    //         perror("write failed");
    //         break;
    //     }
    // }

    close(fd);
    printf("Test file created successfully.\n");
}

int main(int argc, char *argv[]) {
    const char *filename;
    int create_file = 0;
    size_t file_size_mb = 100;

    // Парсинг аргументов
    if (argc < 2) {
        printf("Usage: %s <filename> [--create-file <size_mb>]\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s testfile.bin --create-file 100\n", argv[0]);
        printf("  %s /path/to/existing/file.bin\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    if (argc >= 3 && strcmp(argv[2], "--create-file") == 0) {
        create_file = 1;
        if (argc >= 4) {
            file_size_mb = atoi(argv[3]);
        }
    }

    // Создать тестовый файл, если нужно
    if (create_file) {
        create_test_file(filename, file_size_mb);
        printf("\n");
    }

    // Проверить существование файла
    if (access(filename, F_OK) != 0) {
        fprintf(stderr, "Error: File '%s' does not exist.\n", filename);
        fprintf(stderr, "Use --create-file option to create a test file.\n");
        return 1;
    }

    printf("Comparing I/O methods for file: %s\n", filename);
    printf("===========================================\n");

    // TODO: Для чистоты эксперимента можно очистить page cache (требует root):
    // system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");

    // Метод 1: read()
    unsigned long long sum1 = read_with_syscalls(filename);

    // Небольшая пауза
    sleep(1);

    // Метод 2: mmap()
    unsigned long long sum2 = read_with_mmap(filename);

    // Проверка корректности (checksums должны совпадать)
    printf("\n=== Verification ===\n");
    if (sum1 == sum2) {
        printf("✓ Checksums match: %llu\n", sum1);
    } else {
        printf("✗ Checksums differ! read(): %llu, mmap(): %llu\n", sum1, sum2);
    }

    // TODO: Опционально - метод 3
    // unsigned long long sum3 = read_with_mmap_sequential(filename);

    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте все функции, помеченные TODO
 *
 * 2. Создайте тестовый файл и запустите программу:
 *    $ ./mmap_vs_read testfile.bin --create-file 100
 *
 * 3. Для чистого эксперимента перед каждым запуском очищайте page cache:
 *    $ sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
 *    $ ./mmap_vs_read testfile.bin
 *
 * 4. Запустите несколько раз и усредните результаты
 *
 * 5. Проанализируйте:
 *    - Какой метод быстрее? Почему?
 *    - Сколько page faults у каждого метода?
 *    - Как влияет размер файла на разницу?
 *    - Что происходит при повторном запуске (когда файл в page cache)?
 *
 * 6. Дополнительные эксперименты:
 *    - Измените BUFFER_SIZE (512 bytes, 64 KB, 1 MB) и сравните
 *    - Реализуйте метод 3 с madvise(MADV_SEQUENTIAL)
 *    - Попробуйте случайное чтение вместо последовательного
 *    - Сравните на HDD vs SSD (если есть доступ к разным дискам)
 *
 * 7. Постройте график: размер файла vs время выполнения для обоих методов
 *
 * 8. В отчёте объясните:
 *    - Что такое page cache и как он влияет на результаты
 *    - Почему mmap может быть быстрее/медленнее read()
 *    - Когда стоит использовать каждый метод
 */
