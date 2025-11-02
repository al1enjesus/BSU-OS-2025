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

#define BUFFER_SIZE (4 * 1024)  // 4 KB

typedef struct {
    long minor_faults;
    long major_faults;
} PageFaultStats;

PageFaultStats get_page_faults() {
    PageFaultStats stats = {0, 0};
    struct rusage usage;
    
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats.minor_faults = usage.ru_minflt;
        stats.major_faults = usage.ru_majflt;
    }
    
    return stats;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

unsigned long long read_with_syscalls(const char *filename) {
    printf("\n=== Method 1: read() syscall ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return 0;
    }

    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
    }

    if (bytes_read == -1) {
        perror("read failed");
    }

    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    printf("Time elapsed: %.6f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    free(buffer);
    close(fd);

    return sum;
}

unsigned long long read_with_mmap(const char *filename) {
    printf("\n=== Method 2: mmap() ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 0;
    }

    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;

    for (off_t i = 0; i < sb.st_size; i++) {
        sum += bytes[i];
    }

    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    printf("Time elapsed: %.6f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    if (data != MAP_FAILED) {
        munmap(data, sb.st_size);
    }
    close(fd);

    return sum;
}

unsigned long long read_with_mmap_sequential(const char *filename) {
    printf("\n=== Method 3: mmap() + madvise(SEQUENTIAL) ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    if (sb.st_size == 0) {
        printf("File is empty\n");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 0;
    }

    // Подсказка ядру, что мы будем читать файл последовательно
    // Ядро может начать prefetch следующих страниц
    if (madvise(data, sb.st_size, MADV_SEQUENTIAL) == -1) {
        perror("madvise failed");
        // Продолжаем выполнение даже если madvise не сработал
    }

    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;

    for (off_t i = 0; i < sb.st_size; i++) {
        sum += bytes[i];
    }

    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    printf("Time elapsed: %.6f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    if (data != MAP_FAILED) {
        munmap(data, sb.st_size);
    }
    close(fd);

    return sum;
}

unsigned long long read_with_mmap_random(const char *filename) {
    printf("\n=== Method 4: mmap() + random access ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }

    if (sb.st_size == 0) {
        printf("File is empty\n");
        close(fd);
        return 0;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 0;
    }

    // Подсказка ядру, что доступ будет случайным
    if (madvise(data, sb.st_size, MADV_RANDOM) == -1) {
        perror("madvise failed");
    }

    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;

    // Случайный доступ к данным
    srand(time(NULL));
    int num_accesses = 100000;  // Количество случайных обращений
    for (int i = 0; i < num_accesses; i++) {
        off_t random_offset = rand() % sb.st_size;
        sum += bytes[random_offset];
    }

    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    printf("Time elapsed: %.6f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Random accesses: %d\n", num_accesses);
    printf("Checksum: %llu\n", sum);

    if (data != MAP_FAILED) {
        munmap(data, sb.st_size);
    }
    close(fd);

    return sum;
}

void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file '%s' (%zu MB)...\n", filename, size_mb);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    size_t total_bytes = size_mb * 1024 * 1024;
    char buffer[4096];

    // Заполняем буфер псевдослучайными данными
    for (int i = 0; i < 4096; i++) {
        buffer[i] = (char)(i % 256);
    }

    size_t written = 0;
    while (written < total_bytes) {
        size_t to_write = (total_bytes - written < sizeof(buffer)) ?
                          (total_bytes - written) : sizeof(buffer);
        ssize_t result = write(fd, buffer, to_write);
        if (result == -1) {
            perror("write failed");
            break;
        }
        written += result;
    }

    close(fd);
    printf("Test file created successfully (%zu bytes).\n", written);
}

void clear_page_cache() {
    printf("Clearing page cache...\n");
    system("sync");
    if (system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'") != 0) {
        printf("Warning: Failed to clear page cache (need root privileges)\n");
    }
}

int main(int argc, char *argv[]) {
    const char *filename;
    int create_file = 0;
    size_t file_size_mb = 100;
    int clear_cache = 0;

    // Парсинг аргументов
    if (argc < 2) {
        printf("Usage: %s <filename> [--create-file <size_mb>] [--clear-cache]\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s testfile.bin --create-file 100\n", argv[0]);
        printf("  %s /path/to/existing/file.bin --clear-cache\n", argv[0]);
        printf("  %s testfile.bin --create-file 50 --clear-cache\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--create-file") == 0 && i + 1 < argc) {
            create_file = 1;
            file_size_mb = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--clear-cache") == 0) {
            clear_cache = 1;
        }
    }

    if (create_file) {
        create_test_file(filename, file_size_mb);
        printf("\n");
    }

    if (access(filename, F_OK) != 0) {
        fprintf(stderr, "Error: File '%s' does not exist.\n", filename);
        fprintf(stderr, "Use --create-file option to create a test file.\n");
        return 1;
    }

    printf("Comparing I/O methods for file: %s\n", filename);
    printf("===========================================\n");

    if (clear_cache) {
        clear_page_cache();
        printf("\n");
    }

    // Метод 1: read()
    unsigned long long sum1 = read_with_syscalls(filename);

    sleep(1);  // Небольшая пауза

    if (clear_cache) {
        clear_page_cache();
    }

    // Метод 2: mmap()
    unsigned long long sum2 = read_with_mmap(filename);

    sleep(1);

    if (clear_cache) {
        clear_page_cache();
    }

    // Метод 3: mmap() + madvise(SEQUENTIAL)
    unsigned long long sum3 = read_with_mmap_sequential(filename);

    sleep(1);

    if (clear_cache) {
        clear_page_cache();
    }

    // Метод 4: mmap() + random access
    unsigned long long sum4 = read_with_mmap_random(filename);

    printf("\n=== Verification ===\n");
  
    int all_match = 1;
    
    // Для методов с последовательным чтением суммы должны совпадать
    if (sum1 == sum2 && sum2 == sum3) {
        printf("✓ Methods 1-3 (sequential reads) checksums match: %llu\n", sum1);
    } else {
        printf("✗ Sequential read checksums differ! read(): %llu, mmap: %llu, mmap+seq: %llu\n", 
               sum1, sum2, sum3);
        all_match = 0;
    }
    
    // Для случайного доступа сумма будет другой, но мы проверяем что он работает
    if (sum4 > 0) {
        printf("✓ Method 4 (random access) completed successfully: %llu\n", sum4);
    } else {
        printf("✗ Method 4 (random access) failed\n");
        all_match = 0;
    }
    
    if (all_match) {
        printf("All methods completed successfully!\n");
    }

    printf("\n=== Performance Summary ===\n");
    printf("Key observations:\n");
    printf("1. read() uses system calls but has predictable memory usage\n");
    printf("2. mmap() has fewer system calls but may cause more page faults\n");
    printf("3. madvise() can optimize access patterns\n");
    printf("4. Random access favors mmap() due to on-demand loading\n");
    printf("5. Results depend on file size, cache state, and storage type\n");

    return 0;
}
