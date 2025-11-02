/*
 * mmap_vs_read.c - Сравнение mmap и read для чтения файла
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>     // <--- ВАЖНО: для mmap, munmap, madvise, MADV_SEQUENTIAL
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>

#define BUFFER_SIZE (1024 * 1024)  // 1 MB

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file: %s (%zu MB)...\n", filename, size_mb);
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    size_t total_bytes = size_mb * 1024 * 1024;
    char *buf = malloc(BUFFER_SIZE);
    if (!buf) {
        perror("malloc failed");
        close(fd);
        exit(1);
    }
    memset(buf, 'A' + (rand() % 26), BUFFER_SIZE);

    for (size_t written = 0; written < total_bytes; written += BUFFER_SIZE) {
        size_t to_write = (written + BUFFER_SIZE > total_bytes) ? (total_bytes - written) : BUFFER_SIZE;
        if (write(fd, buf, to_write) != (ssize_t)to_write) {
            perror("write failed");
            free(buf);
            close(fd);
            exit(1);
        }
    }

    free(buf);
    close(fd);
    printf("File created: %zu bytes\n", total_bytes);
}

void benchmark_read(const char *filename, size_t file_size) {
    printf("\n=== Benchmark: read() ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return;
    }

    double start = get_time();
    size_t total_read = 0;
    while (total_read < file_size) {
        size_t to_read = (total_read + BUFFER_SIZE > file_size) ? (file_size - total_read) : BUFFER_SIZE;
        ssize_t n = read(fd, buffer, to_read);
        if (n <= 0) break;
        total_read += n;
    }
    double end = get_time();

    close(fd);
    free(buffer);

    double elapsed = end - start;
    double speed = file_size / (1024.0 * 1024.0) / elapsed;
    printf("read(): %.2f s, %.2f MB/s\n", elapsed, speed);
}

void benchmark_mmap(const char *filename, size_t file_size) {
    printf("\n=== Benchmark: mmap() ===\n");

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    void *map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }

    double start = get_time();
    volatile char sink = 0;
    for (size_t i = 0; i < file_size; i += 4096) {
        sink += ((char*)map)[i];
    }
    double end = get_time();

    munmap(map, file_size);
    close(fd);

    double elapsed = end - start;
    double speed = file_size / (1024.0 * 1024.0) / elapsed;
    printf("mmap(): %.2f s, %.2f MB/s (sink=%d)\n", elapsed, speed, sink);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [--create-file <MB>]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    size_t create_size_mb = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--create-file") == 0 && i + 1 < argc) {
            create_size_mb = atoi(argv[++i]);
        }
    }

    if (create_size_mb > 0) {
        create_test_file(filename, create_size_mb);
    }

    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat failed");
        return 1;
    }
    size_t file_size = st.st_size;

    if (file_size == 0) {
        fprintf(stderr, "File is empty or not created.\n");
        return 1;
    }

    printf("File: %s (%zu bytes, %.2f MB)\n", filename, file_size, file_size / (1024.0 * 1024.0));

    benchmark_read(filename, file_size);
    benchmark_mmap(filename, file_size);

    printf("\n=== Summary ===\n");
    printf("mmap() часто быстрее для:\n");
    printf("  • Последовательного доступа\n");
    printf("  • Больших файлов\n");
    printf("  • Многократного чтения\n");
    printf("read() + madvise может быть близок по скорости\n");

    return 0;
}
