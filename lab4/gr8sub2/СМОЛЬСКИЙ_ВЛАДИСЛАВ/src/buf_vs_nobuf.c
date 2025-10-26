#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
//#include <sys/mman.h>
#include <sys/stat.h>
//#include <sys/resource.h>
#include <time.h>
#include <errno.h>

#define COLOR_RED       "\x1b[31m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"

#define COLOR_BOLD      "\x1b[1m"
#define COLOR_RESET     "\x1b[0m"

#define FILE_SIZE (100*1024*1024) // 100 MB

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int write_with_syscalls(const char *filename, const size_t BUFFER_SIZE)
{
    printf("\n=== Method 1: write() syscall ===\n");
    if (BUFFER_SIZE < 1024) {
        printf("=== Buffer size: " COLOR_BOLD "%lu" COLOR_RESET " B ===\n", BUFFER_SIZE);
    }
    else {
        printf("=== Buffer size: " COLOR_BOLD "%.1f" COLOR_RESET " KB ===\n", BUFFER_SIZE/1024.0);
    }

    double start_time = get_time();

    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }

    char buf[BUFFER_SIZE];
    memset(buf, 'A', BUFFER_SIZE);
    for (size_t written = 0; written < FILE_SIZE; written += BUFFER_SIZE) {
        int result = write(fd, buf, BUFFER_SIZE);
        if (result == -1) {
            perror("write failed");
            return 1;
        }
    }

    double end_time = get_time();

    printf("Time elapsed: " COLOR_BOLD COLOR_GREEN "%.3f" COLOR_RESET " seconds\n", end_time - start_time);
    
    close(fd);
    return 0;
}

int write_with_stdio(const char *filename)
{
    printf("\n=== Method 2: fputc() stdio ===\n");

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        perror("fopen failed");
        return 1;
    }

    double start_time = get_time();

    for (int i = 0; i < FILE_SIZE; i++) {
        fputc('A', f);
    }

    double end_time = get_time();

    printf("Time elapsed: " COLOR_BOLD COLOR_GREEN "%.3f" COLOR_RESET " seconds\n", end_time - start_time);

    fclose(f);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *filename;

    // Парсинг аргументов
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s testfile.bin\n", argv[0]);
        printf("  %s /path/to/existing/file.bin\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    // Создать тестовый файл, если нужно
    if (access(filename, F_OK) != 0) {
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open failed");
            return 1;
        }
        close(fd);
    }

    printf("Comparing Input methods for file: %s\n", filename);
    printf("===========================================\n");

    write_with_syscalls(filename, 512);
    write_with_syscalls(filename, 4*1024);
    write_with_syscalls(filename, 64*1024);
    write_with_stdio(filename);

    return 0;
}