#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

static double now_sec(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec / 1e9; }

static size_t parse_size(const char* s) {
    char* end; double v = strtod(s, &end);
    if (*end == 'G' || *end == 'g') v *= 1024 * 1024 * 1024.0;
    else if (*end == 'M' || *end == 'm') v *= 1024 * 1024.0;
    else if (*end == 'K' || *end == 'k') v *= 1024.0;
    return (size_t)v;
}

static void do_write(const char* path, size_t total) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); exit(1); }
    size_t buf_sz = 1 << 20;
    unsigned char* buf = malloc(buf_sz); if (!buf) { perror("malloc"); exit(1); } memset(buf, 0xAB, buf_sz);

    double t0 = now_sec(); size_t done = 0; while (done < total) { size_t chunk = (total - done < buf_sz) ? (total - done) : buf_sz; if (write(fd, buf, chunk) < 0) { perror("write"); exit(1); } done += chunk; }
    fsync(fd); close(fd); double t1 = now_sec();
    printf("[WRITE] %.2f MB in %.3f s (%.2f MB/s)\n", total / 1048576.0, t1 - t0, (total / 1048576.0) / (t1 - t0));
}

static void do_read(const char* path) {
    int fd = open(path, O_RDONLY); if (fd < 0) { perror("open"); exit(1); }
    size_t buf_sz = 1 << 20; unsigned char* buf = malloc(buf_sz); if (!buf) { perror("malloc"); exit(1); }
    double t0 = now_sec(); ssize_t rc; size_t total = 0; while ((rc = read(fd, buf, buf_sz)) > 0) { total += (size_t)rc; }
    close(fd); double t1 = now_sec();
    printf("[READ]  %.2f MB in %.3f s (%.2f MB/s)\n", total / 1048576.0, t1 - t0, (total / 1048576.0) / (t1 - t0));
}

int main(int argc, char** argv) {
    const char* mode = "write-read"; const char* path = "./stress.bin"; size_t size = 200 * 1024 * 1024;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) path = argv[++i];
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) size = parse_size(argv[++i]);
    }
    printf("PID: %d\n", getpid()); fflush(stdout);
    if (strcmp(mode, "write") == 0 || strcmp(mode, "write-read") == 0) do_write(path, size);
    if (strcmp(mode, "read") == 0 || strcmp(mode, "write-read") == 0) do_read(path);
    return 0;
}