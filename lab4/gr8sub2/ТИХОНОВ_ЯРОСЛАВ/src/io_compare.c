//
// Created by jarik on 12.11.2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define FILE_SIZE (100 * 1024 * 1024) // 100 MB

// Измерение времени (в секундах)
double now_sec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Вариант 1 — стандартная библиотека stdio (fopen/fwrite)
void test_stdio() {
    printf("\n=== Тест 1: stdio (fopen/fwrite) ===\n");
    FILE *f = fopen("test_stdio.bin", "wb");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    char *buf = malloc(64 * 1024);
	if (!buf) { perror("malloc"); exit(1); }
    memset(buf, 'A', 64 * 1024);

    double start = now_sec();
    size_t written = 0;
    while (written < FILE_SIZE) {
        size_t chunk = fwrite(buf, 1, 64 * 1024, f);
        written += chunk;
    }
    fflush(f);
    fclose(f);
    double end = now_sec();
    printf("Время: %.3f секунд\n", end - start);
    free(buf);
}

// Вариант 2 — системные вызовы (open/write) с настраиваемым буфером
void test_syscall(size_t buf_size) {
    printf("\n=== Тест 2: syscall write(), буфер %zu байт ===\n", buf_size);
    int fd = open("test_syscall.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char *buf = malloc(buf_size);
	if (!buf) { perror("malloc"); exit(1); }
    memset(buf, 'A', buf_size);

    double start = now_sec();
    size_t written = 0;
    while (written < FILE_SIZE) {
        ssize_t chunk = write(fd, buf, buf_size);
        if (chunk < 0) {
            perror("write");
            close(fd);
            exit(1);
        }
        written += chunk;
    }
    fsync(fd);
    close(fd);
    double end = now_sec();
    printf("Время: %.3f секунд\n", end - start);
    free(buf);
}

int main() {
    printf("PID: %d\n", getpid());
    printf("Сравнение буферизованного и небуферизованного ввода-вывода\n");

    test_stdio();
    test_syscall(512);
    test_syscall(4096);
    test_syscall(65536);

    printf("\nГотово. Файлы: test_stdio.bin, test_syscall.bin\n");
    return 0;
}