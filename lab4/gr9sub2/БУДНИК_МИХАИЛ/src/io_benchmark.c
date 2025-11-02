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
#define PAGE_SIZE 4096
#define BYTES_IN_MB 1024*1024
#define BYTES_IN_MB_FLOAT 1024.0*1024.0

size_t buffer_sizes[] = {512, 1024, 4096, 8192, 16384, 65536, BYTES_IN_MB, 4*BYTES_IN_MB};
int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size, long long *libcall_count_out) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);
    *libcall_count_out = 0;

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
    size_t total_written = 0;

    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        size_t written_now = fwrite(buffer, 1, to_write, f);
        (*libcall_count_out)++;
        
        if (written_now != to_write) {
            perror("fwrite failed");
            break;
        }
        total_written += written_now;
    }

    fflush(f);
    (*libcall_count_out)++;
    
    double end = get_time();
    double elapsed = end - start;
    
    if (total_written != size) {
         printf("Error: Wrote %zu bytes instead of %zu\n", total_written, size);
    }

    printf("Time: %.3f seconds\n", elapsed);
    printf("Syscalls (fwrite + fflush): %lld\n", *libcall_count_out);
    printf("Throughput: %.2f MB/s\n", (total_written / (BYTES_IN_MB_FLOAT)) / elapsed);

    free(buffer);
    fclose(f);

    return elapsed;
}

double benchmark_write(const char *filename, size_t size, size_t buffer_size, long long *syscall_count_out) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);
    *syscall_count_out = 0;

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }
    (*syscall_count_out)++;

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);

    double start = get_time();
    size_t total_written = 0;

    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        ssize_t written_now = write(fd, buffer, to_write);
        (*syscall_count_out)++;

        if (written_now != (ssize_t)to_write) {
            perror("write failed");
            break;
        }
        total_written += written_now;
    }

    double end = get_time();
    double elapsed = end - start;

    if (total_written != size) {
         printf("Error: Wrote %zu bytes instead of %zu\n", total_written, size);
    }

    printf("Time: %.3f seconds\n", elapsed);
    
    close(fd);
    (*syscall_count_out)++;
    
    printf("Syscalls (open + N*write + close): %lld\n", *syscall_count_out);
    printf("Throughput: %.2f MB/s\n", (total_written / (BYTES_IN_MB_FLOAT)) / elapsed);

    free(buffer);
    return elapsed;
}

double benchmark_fread(const char *filename, size_t file_size, size_t buffer_size, long long *libcall_count_out) {
    printf("\n--- fread() (buffer=%zu) ---\n", buffer_size);
    *libcall_count_out = 0;
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen read");
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc");
        fclose(f);
        return -1;
    }

    double start = get_time();
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, buffer_size, f)) > 0) {
        (*libcall_count_out)++;
    }
    double end = get_time();
    double elapsed = end - start;
    
    printf("Time: %.3f seconds\n", elapsed);
    printf("Syscalls (N*fread): %lld\n", *libcall_count_out);
    printf("Throughput: %.2f MB/s\n", (file_size / (BYTES_IN_MB_FLOAT)) / elapsed);
    
    free(buffer);
    fclose(f);
    return elapsed;
}

double benchmark_read(const char *filename, size_t file_size, size_t buffer_size, long long *syscall_count_out) {
    printf("\n--- read() (buffer=%zu) ---\n", buffer_size);
    *syscall_count_out = 0;
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open read");
        return -1;
    }
    (*syscall_count_out)++;

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc");
        close(fd);
        return -1;
    }

    double start = get_time();
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        (*syscall_count_out)++;
    }
    double end = get_time();
    double elapsed = end - start;
    
    printf("Time: %.3f seconds\n", elapsed);
    
    close(fd);
    (*syscall_count_out)++;

    printf("Syscalls (open + N*read + close): %lld\n", *syscall_count_out);
    printf("Throughput: %.2f MB/s\n", (file_size / (BYTES_IN_MB_FLOAT)) / elapsed);
    
    free(buffer);
    return elapsed;
}

void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Write Buffer Size Impact\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * BYTES_IN_MB;

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);
    
    long long syscalls = 0;

    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        benchmark_write(filename, file_size, buffer_sizes[i], &syscalls);
        unlink(filename);
        sleep(1);
    }
}

void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Write Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * BYTES_IN_MB;
    size_t optimal_buffer = 64 * 1024;  // 64 KB
    long long calls_count = 0;

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer / 1024);

    // Метод 1: fwrite
    benchmark_fwrite("test_fwrite.bin", file_size, optimal_buffer, &calls_count);
    unlink("test_fwrite.bin");
    sleep(1);

    // Метод 2: write
    benchmark_write("test_write.bin", file_size, optimal_buffer, &calls_count);
    unlink("test_write.bin");
    sleep(1);
}

void benchmark_read_methods(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods Comparison\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = sb.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }
    
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / (BYTES_IN_MB_FLOAT));
    
    size_t optimal_buffer = 65536; // 64K
    long long calls_count = 0;

    benchmark_fread(filename, file_size, optimal_buffer, &calls_count);
    benchmark_read(filename, file_size, optimal_buffer, &calls_count);
}

void benchmark_read_buffer_sizes(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Read Buffer Size Impact\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = sb.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }

    printf("\nTesting read() with different buffer sizes:\n");
    printf("File size: %.2f MB\n", file_size / (BYTES_IN_MB_FLOAT));
    
    long long syscalls = 0;

    for (int i = 0; i < num_sizes; i++) {
        benchmark_read(filename, file_size, buffer_sizes[i], &syscalls);
    }
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
            return 0;
        }
    }

    if (size_mb == 0) {
        fprintf(stderr, "Invalid size: %zu MB\n", size_mb);
        return 1;
    }

    printf("I/O Benchmark\n");
    printf("=============\n");
    printf("Test file size: %zu MB\n", size_mb);

    printf("\nAttempting to sync filesystem...\n");
    int ret;
    ret = system("sync");
    if(ret != 0) {
      perror("sync");
      return 1;
    }
    ret = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if(ret != 0) {
      perror("drop_caches");
      return 1;
    }

    benchmark_all_methods(size_mb);
    benchmark_buffer_sizes(size_mb);
    
    const char *read_test_file = "read_test_file.bin";
    printf("\nCreating test file '%s' for reading benchmark...\n", read_test_file);
    long long temp_calls;
    benchmark_fwrite(read_test_file, size_mb * BYTES_IN_MB, 65536, &temp_calls);
    
    printf("\nAttempting to sync filesystem...\n");
    ret = system("sync");
    if(ret != 0) {
      perror("sync");
      return 1;
    }
    ret = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if(ret != 0) {
      perror("drop_caches");
      return 1;
    }

    benchmark_read_methods(read_test_file);
    benchmark_read_buffer_sizes(read_test_file);
    
    unlink(read_test_file);

    printf("\n========================================\n");
    printf("Benchmark completed!\n");
    printf("========================================\n");

    return 0;
}