// src/io_monitor.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

static double now_sec() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        // Фоллбек: если вдруг не получилось, используем CLOCK_REALTIME
        clock_gettime(CLOCK_REALTIME, &ts);
    }
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int write_pattern(const char* path, size_t total, size_t buf_sz, int secs) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    char* buf = (char*)malloc(buf_sz);
    if (!buf) {
        perror("malloc");
        close(fd);
        return -1;
    }
    for (size_t i = 0; i < buf_sz; i++) buf[i] = (char)(i & 0xFF);

    double t_start = now_sec();
    double t_end = t_start + (secs > 0 ? secs : 0);
    size_t bytes = 0;

    while (bytes < total) {
        if (secs > 0 && now_sec() >= t_end) break;

        size_t to = buf_sz;
        if (to > total - bytes) to = total - bytes;

        ssize_t rc = write(fd, buf, to);
        if (rc < 0) {
            if (errno == EINTR) continue;
            perror("write");
            free(buf);
            close(fd);
            return -1;
        }
        if (rc == 0) {
            // Неожиданно, но выйдем, чтобы не зависнуть
            break;
        }
        bytes += (size_t)rc;
    }

    // Зафиксируем на диск
    if (fsync(fd) < 0) {
        perror("fsync");
        // Не завершаем с ошибкой — просто логируем
    }

    if (close(fd) < 0) {
        perror("close");
        free(buf);
        return -1;
    }

    printf("Wrote %zu bytes to %s in %.2f s\n", bytes, path, now_sec() - t_start);
    free(buf);
    return 0;
}

static int read_scan(const char* path, size_t buf_sz) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    char* buf = (char*)malloc(buf_sz);
    if (!buf) {
        perror("malloc");
        close(fd);
        return -1;
    }

    double t_start = now_sec();
    size_t bytes = 0;
    while (1) {
        ssize_t rc = read(fd, buf, buf_sz);
        if (rc < 0) {
            if (errno == EINTR) continue;
            perror("read");
            free(buf);
            close(fd);
            return -1;
        }
        if (rc == 0) break; // EOF
        bytes += (size_t)rc;
    }

    if (close(fd) < 0) {
        perror("close");
        free(buf);
        return -1;
    }

    printf("Read %zu bytes from %s in %.2f s\n", bytes, path, now_sec() - t_start);
    free(buf);
    return 0;
}

int main(void) {
    const char* out = "test_io_load.bin";
    const size_t total = 500UL * 1024UL * 1024UL; // 500 MB
    const size_t buf = 128UL * 1024UL;            // 128 KB
    const int secs = 10;                           // ограничение по времени на запись

    // Выводим PID сразу — это важно для внешнего сбора метрик
    pid_t pid = getpid();
    printf("PID: %d\n", (int)pid);

    printf("Generating write load for %d s (up to %zu bytes, buf=%zu)...\n", secs, total, buf);
    if (write_pattern(out, total, buf, secs) != 0) {
        fprintf(stderr, "Write load FAILED\n");
        // Продолжим, чтобы можно было всё равно посмотреть метрики чтения, если файл уже частично создан
    }

    printf("Now reading it back (buf=%zu)...\n", buf);
    if (read_scan(out, buf) != 0) {
        fprintf(stderr, "Read scan FAILED\n");
    }

    printf("Done. Keep process alive for monitoring 5s...\n");
    sleep(30);
    return 0;
}