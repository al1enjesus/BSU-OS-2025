#define _GNU_SOURCE

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
#define FILE_PERMS 0644
#define READ_BUFFER_SIZE 65536
#define OPTIMAL_BUFFER_SIZE (64 * 1024)
#define DIRECT_BUFFER_SIZE (64 * 1024)

#ifndef O_DIRECT
#define O_DIRECT 040000
#endif

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);
    FILE *f = fopen(filename, "wbx");
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

double benchmark_write(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, FILE_PERMS);
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
        ssize_t result = write(fd, buffer, to_write);
        if (result != (ssize_t)to_write) {
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
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC | O_SYNC, FILE_PERMS);
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
        ssize_t result = write(fd, buffer, to_write);
        if (result != (ssize_t)to_write) {
            perror("write failed");
            break;
        }
    }
    double end = get_time();
    double elapsed = end - start;
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);
    printf("Note: O_SYNC forces physical write to disk on each write() call\n");
    free(buffer);
    close(fd);
    return elapsed;
}

double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");
    int fd = open(filename, O_RDWR | O_CREAT | O_EXCL | O_TRUNC, FILE_PERMS);
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
    memset(data, 'C', size);
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

double benchmark_write_direct(const char *filename, size_t size, size_t buffer_size) {
    printf("\n=== write() with O_DIRECT (buffer=%zu bytes) ===\n", buffer_size);
#ifdef O_DIRECT
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC | O_DIRECT, FILE_PERMS);
    if (fd == -1) {
        perror("open with O_DIRECT failed");
        printf("Note: O_DIRECT may require root privileges or specific filesystem\n");
        return -1;
    }
    char *buffer;
    long page_size = sysconf(_SC_PAGESIZE);
    if (posix_memalign((void**)&buffer, page_size, buffer_size) != 0) {
        perror("posix_memalign failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'D', buffer_size);
    if (buffer_size % page_size != 0) {
        printf("Warning: buffer size %zu not multiple of page size %ld\n", buffer_size, page_size);
    }
    double start = get_time();
    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        if (to_write % 512 != 0) {
            to_write = (to_write / 512) * 512;
        }
        if (to_write == 0) break;
        ssize_t result = write(fd, buffer, to_write);
        if (result != (ssize_t)to_write) {
            perror("write with O_DIRECT failed");
            break;
        }
    }
    double end = get_time();
    double elapsed = end - start;
    printf("Time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", (size / (1024.0 * 1024.0)) / elapsed);
    printf("Note: O_DIRECT bypasses page cache (direct to disk)\n");
    printf("      Requires aligned memory and buffer size\n");
    free(buffer);
    close(fd);
    return elapsed;
#else
    printf("O_DIRECT not supported on this system\n");
    return -1;
#endif
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

void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Methods Comparison\n");
    printf("========================================\n");
    size_t file_size = file_size_mb * 1024 * 1024;
    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %u KB (for fwrite/write)\n", (unsigned int)(OPTIMAL_BUFFER_SIZE / 1024));
    benchmark_fwrite("test_fwrite.bin", file_size, OPTIMAL_BUFFER_SIZE);
    unlink("test_fwrite.bin");
    sleep(1);
    benchmark_write("test_write.bin", file_size, OPTIMAL_BUFFER_SIZE);
    unlink("test_write.bin");
    sleep(1);
    benchmark_write_sync("test_write_sync.bin", file_size, OPTIMAL_BUFFER_SIZE);
    unlink("test_write_sync.bin");
    sleep(1);
    benchmark_mmap("test_mmap.bin", file_size);
    unlink("test_mmap.bin");
    sleep(1);
    benchmark_write_direct("test_direct.bin", file_size, DIRECT_BUFFER_SIZE);
    unlink("test_direct.bin");
    printf("\n=== Summary ===\n");
    printf("Fastest method: (compare results above)\n");
    printf("\nFactors affecting performance:\n");
    printf("- stdio (fwrite) has user-space buffering\n");
    printf("- write() goes directly to kernel, but still uses page cache\n");
    printf("- mmap() allows direct memory access, lazy writes\n");
    printf("- O_SYNC forces physical write to disk on each call (VERY slow)\n");
#ifdef O_DIRECT
    printf("- O_DIRECT bypasses page cache entirely (direct to disk)\n");
#endif
    printf("- Actual disk speed depends on: HDD vs SSD, filesystem, etc.\n");
}

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
    printf("\n--- fread() ---\n");
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen failed");
        return;
    }
    char *buffer = malloc(READ_BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return;
    }
    double start = get_time();
    size_t total_read = 0;
    while (total_read < file_size) {
        size_t to_read = (file_size - total_read < READ_BUFFER_SIZE) ? (file_size - total_read) : READ_BUFFER_SIZE;
        size_t bytes_read = fread(buffer, 1, to_read, f);
        if (bytes_read == 0) {
            if (feof(f)) break;
            perror("fread failed");
            break;
        }
        total_read += bytes_read;
    }
    double end = get_time();
    printf("Time: %.3f seconds\n", end - start);
    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
    fclose(f);
    free(buffer);
    printf("\n--- read() ---\n");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }
    buffer = malloc(READ_BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return;
    }
    start = get_time();
    total_read = 0;
    while (total_read < file_size) {
        size_t to_read = (file_size - total_read < READ_BUFFER_SIZE) ? (file_size - total_read) : READ_BUFFER_SIZE;
        ssize_t bytes_read = read(fd, buffer, to_read);
        if (bytes_read <= 0) {
            if (bytes_read == 0) break;
            perror("read failed");
            break;
        }
        total_read += bytes_read;
    }
    end = get_time();
    printf("Time: %.3f seconds\n", end - start);
    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
    close(fd);
    free(buffer);
    printf("\n--- mmap() ---\n");
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }
    void *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
    start = get_time();
    volatile char *ptr = (char*)data;
    for (size_t i = 0; i < file_size; i += 4096) {
        char c = ptr[i];
        (void)c;
    }
    end = get_time();
    printf("Time: %.3f seconds\n", end - start);
    printf("Throughput: %.2f MB/s\n", (file_size / (1024.0 * 1024.0)) / (end - start));
    munmap(data, file_size);
    close(fd);
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
    printf("Note: For clean results, run manually before:\n");
    printf("  sync && sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'\n\n");
    benchmark_all_methods(size_mb);
    printf("\n");
    benchmark_buffer_sizes(size_mb);
    printf("\nCreating file for read benchmarks\n");
    benchmark_write("test_read.bin", size_mb * 1024 * 1024, READ_BUFFER_SIZE);
    printf("\n");
    benchmark_read_methods("test_read.bin");
    unlink("test_read.bin");
    printf("\n========================================\n");
    printf("Benchmark completed\n");
    printf("========================================\n");
    return 0;
}
