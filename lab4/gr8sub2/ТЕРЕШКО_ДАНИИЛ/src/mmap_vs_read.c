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

#define BUFFER_SIZE (64 * 1024)

typedef struct {
    long minor_faults;
    long major_faults;
} PageFaultStats;

typedef struct {
    double time_elapsed;
    long minor_faults;
    long major_faults;
    unsigned long long checksum;
} TestResult;

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
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        return 0;
    }
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

TestResult read_with_syscalls(const char *filename) {
    printf("\n=== Method 1: read() syscall ===\n");

    TestResult result = {0};
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "fstat failed: %s\n", strerror(errno));
        close(fd);
        return result;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        close(fd);
        return result;
    }

    PageFaultStats start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    ssize_t bytes_read;
    size_t total_read = 0;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
        total_read += bytes_read;
    }

    if (bytes_read == -1) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
    }

    double end_time = get_time();
    PageFaultStats end_faults = get_page_faults();

    result.time_elapsed = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf("Time elapsed: %.3f seconds\n", result.time_elapsed);
    printf("Throughput: %.2f MB/s\n", (sb.st_size / (1024.0 * 1024.0)) / result.time_elapsed);
    printf("Minor page faults: %ld\n", result.minor_faults);
    printf("Major page faults: %ld\n", result.major_faults);
    printf("Checksum: %llu\n", result.checksum);
    printf("Total bytes read: %zu\n", total_read);

    free(buffer);
    close(fd);
    return result;
}

TestResult read_with_mmap(const char *filename) {
    printf("\n=== Method 2: mmap() ===\n");

    TestResult result = {0};
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "fstat failed: %s\n", strerror(errno));
        close(fd);
        return result;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(fd);
        return result;
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

    result.time_elapsed = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf("Time elapsed: %.3f seconds\n", result.time_elapsed);
    printf("Throughput: %.2f MB/s\n", (sb.st_size / (1024.0 * 1024.0)) / result.time_elapsed);
    printf("Minor page faults: %ld\n", result.minor_faults);
    printf("Major page faults: %ld\n", result.major_faults);
    printf("Checksum: %llu\n", result.checksum);

    if (munmap(data, sb.st_size) == -1) {
        fprintf(stderr, "munmap failed: %s\n", strerror(errno));
    }
    close(fd);
    return result;
}

TestResult read_with_mmap_sequential(const char *filename) {
    printf("\n=== Method 3: mmap() + madvise(SEQUENTIAL) ===\n");

    TestResult result = {0};
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "fstat failed: %s\n", strerror(errno));
        close(fd);
        return result;
    }

    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(fd);
        return result;
    }

    if (madvise(data, sb.st_size, POSIX_MADV_SEQUENTIAL) == -1) {
        fprintf(stderr, "madvise failed: %s\n", strerror(errno));
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

    result.time_elapsed = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf("Time elapsed: %.3f seconds\n", result.time_elapsed);
    printf("Throughput: %.2f MB/s\n", (sb.st_size / (1024.0 * 1024.0)) / result.time_elapsed);
    printf("Minor page faults: %ld\n", result.minor_faults);
    printf("Major page faults: %ld\n", result.major_faults);
    printf("Checksum: %llu\n", result.checksum);

    if (munmap(data, sb.st_size) == -1) {
        fprintf(stderr, "munmap failed: %s\n", strerror(errno));
    }
    close(fd);
    return result;
}

void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file '%s' (%zu MB)...\n", filename, size_mb);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return;
    }

    size_t total_bytes = size_mb * 1024 * 1024;
    char buffer[4096];

    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (char)((i * 7) % 256);
    }

    size_t written = 0;
    while (written < total_bytes) {
        size_t to_write = (total_bytes - written < sizeof(buffer)) ?
                          (total_bytes - written) : sizeof(buffer);
        ssize_t result = write(fd, buffer, to_write);
        if (result == -1) {
            fprintf(stderr, "write failed: %s\n", strerror(errno));
            break;
        }
        written += result;
    }

    if (fsync(fd) == -1) {
        fprintf(stderr, "fsync failed: %s\n", strerror(errno));
    }

    close(fd);
    printf("Test file created successfully: %zu bytes written.\n", written);
}

void print_comparison(TestResult read_result, TestResult mmap_result, TestResult mmap_seq_result) {
    printf("\n=== Performance Comparison ===\n");
    printf("%-25s %12s %12s %12s\n", "Method", "Time (s)", "Minor Faults", "Major Faults");
    printf("%-25s %12s %12s %12s\n", "-----------------", "----------", "------------", "------------");
    
    printf("%-25s %12.3f %12ld %12ld\n", 
           "read()", read_result.time_elapsed, read_result.minor_faults, read_result.major_faults);
    
    printf("%-25s %12.3f %12ld %12ld\n", 
           "mmap()", mmap_result.time_elapsed, mmap_result.minor_faults, mmap_result.major_faults);
    
    if (mmap_seq_result.time_elapsed > 0) {
        printf("%-25s %12.3f %12ld %12ld\n", 
               "mmap()+madvise", mmap_seq_result.time_elapsed, 
               mmap_seq_result.minor_faults, mmap_seq_result.major_faults);
    }
    
    const char *fastest_method = "read()";
    double fastest_time = read_result.time_elapsed;
    
    if (mmap_result.time_elapsed < fastest_time) {
        fastest_method = "mmap()";
        fastest_time = mmap_result.time_elapsed;
    }
    
    if (mmap_seq_result.time_elapsed > 0 && mmap_seq_result.time_elapsed < fastest_time) {
        fastest_method = "mmap()+madvise";
    }
    
    printf("\nFastest method: %s (%.3f seconds)\n", fastest_method, fastest_time);
}

int main(int argc, char *argv[]) {
    const char *filename;
    int create_file = 0;
    size_t file_size_mb = 100;
    int clean_cache = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [--create-file <size_mb>] [--clean-cache]\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s testfile.bin --create-file 100\n", argv[0]);
        fprintf(stderr, "  %s /path/to/existing/file.bin --clean-cache\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--create-file") == 0 && i + 1 < argc) {
            create_file = 1;
            file_size_mb = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--clean-cache") == 0) {
            clean_cache = 1;
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

    if (clean_cache) {
        printf("Clearing page cache (requires root privileges)...\n");
        printf("Warning: This command requires sudo access. You may be prompted for password.\n");
        if (system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'") != 0) {
            fprintf(stderr, "Failed to clear page cache. Run with sudo for accurate results.\n");
            fprintf(stderr, "You can also run manually: sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'\n");
        }
        sleep(1);
    }

    printf("Comparing I/O methods for file: %s\n", filename);
    printf("File size: %zu MB\n", file_size_mb);
    printf("Clean cache: %s\n", clean_cache ? "yes" : "no");
    printf("===========================================\n");

    TestResult read_result = read_with_syscalls(filename);
    sleep(2);
    TestResult mmap_result = read_with_mmap(filename);
    sleep(2);
    TestResult mmap_seq_result = read_with_mmap_sequential(filename);

    printf("\n=== Verification ===\n");
    if (read_result.checksum == mmap_result.checksum && 
        (mmap_seq_result.time_elapsed == 0 || read_result.checksum == mmap_seq_result.checksum)) {
        printf("✓ All checksums match: %llu\n", read_result.checksum);
    } else {
        printf("✗ Checksums differ!\n");
        printf("  read(): %llu\n", read_result.checksum);
        printf("  mmap(): %llu\n", mmap_result.checksum);
        if (mmap_seq_result.time_elapsed > 0) {
            printf("  mmap()+madvise: %llu\n", mmap_seq_result.checksum);
        }
    }

    print_comparison(read_result, mmap_result, mmap_seq_result);

    printf("\n=== Recommendations ===\n");
    printf("Use read() when:\n");
    printf("  - Working with small files\n");
    printf("  - Sequential access patterns\n");
    printf("  - Need predictable memory usage\n\n");
    
    printf("Use mmap() when:\n");
    printf("  - Working with large files\n");
    printf("  - Random access patterns\n");
    printf("  - Multiple processes need to share file data\n");
    printf("  - File fits in available memory\n");

    return 0;
}