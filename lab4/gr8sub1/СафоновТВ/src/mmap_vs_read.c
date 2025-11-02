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

#define BUFFER_SIZE (4 * 1024)

void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file: %s (%zu MB)\n", filename, size_mb);
    
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    size_t total_size = size_mb * 1024 * 1024;
    char *buffer = malloc(1024 * 1024);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        exit(1);
    }

    for (size_t i = 0; i < 1024 * 1024; i++) {
        buffer[i] = rand() % 256;
    }

    for (size_t written = 0; written < total_size; written += 1024 * 1024) {
        ssize_t result = write(fd, buffer, 1024 * 1024);
        if (result == -1) {
            perror("write failed");
            break;
        }
    }

    free(buffer);
    close(fd);
    printf("Test file created successfully\n");
}

typedef struct {
    long minor_faults;
    long major_faults;
} PageFaultStats;

PageFaultStats get_page_faults() {
    PageFaultStats stats = {0, 0};

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    stats.minor_faults = usage.ru_minflt;
    stats.major_faults = usage.ru_majflt;

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

    printf("Time elapsed: %.3f seconds\n", end_time - start_time);
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

    printf("Time elapsed: %.3f seconds\n", end_time - start_time);
    printf("Minor page faults: %ld\n", end_faults.minor_faults - start_faults.minor_faults);
    printf("Major page faults: %ld\n", end_faults.major_faults - start_faults.major_faults);
    printf("Checksum: %llu\n", sum);

    if (data) {
        munmap(data, sb.st_size);
    }
    close(fd);

    return sum;
}

int main(int argc, char *argv[]) {
    const char *filename;
    int create_file = 0;
    size_t file_size_mb = 100;

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

    system("sh -c 'echo 3 > /proc/sys/vm/drop_caches'");

    unsigned long long sum1 = read_with_syscalls(filename);

    sleep(1);

    unsigned long long sum2 = read_with_mmap(filename);

    printf("\n=== Verification ===\n");
    if (sum1 == sum2) {
        printf("✓ Checksums match: %llu\n", sum1);
    } else {
        printf("✗ Checksums differ! read(): %llu, mmap(): %llu\n", sum1, sum2);
    }

    return 0;
}
