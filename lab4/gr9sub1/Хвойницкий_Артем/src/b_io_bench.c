#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

static double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void ensure_unlink(const char* p) { unlink(p); }

static void bench_stdio_fputc(size_t total_bytes, const char* path) {
    ensure_unlink(path);
    FILE* f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }

    double t0 = now_sec();
    for (size_t i = 0; i < total_bytes; ++i) fputc('A', f);
    fflush(f);
    fclose(f);
    double t1 = now_sec();
    double mb = total_bytes / (1024.0 * 1024.0);
    printf("stdio fputc: %.2f MB written in %.3f s (%.2f MB/s)\n", mb, t1 - t0, mb / (t1 - t0));
}

static void bench_syswrite(size_t total_bytes, size_t buf_sz, const char* path) {
    ensure_unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); exit(1); }

    unsigned char* buf = malloc(buf_sz);
    if (!buf) { perror("malloc"); exit(1); }
    memset(buf, 'A', buf_sz);

    double t0 = now_sec();
    size_t written = 0;
    while (written < total_bytes) {
        size_t chunk = buf_sz;
        if (total_bytes - written < chunk) chunk = total_bytes - written;
        ssize_t rc = write(fd, buf, chunk);
        if (rc < 0) { perror("write"); exit(1); }
        written += (size_t)rc;
    }
    fsync(fd);
    close(fd);
    double t1 = now_sec();

    double mb = total_bytes / (1024.0 * 1024.0);
    double calls = (double)((total_bytes + buf_sz - 1) / buf_sz);
    printf("write(%zu B buf): %.2f MB in %.3f s (%.2f MB/s), ~%.0f write() calls\n",
        buf_sz, mb, t1 - t0, mb / (t1 - t0), calls);
    free(buf);
}

int main(int argc, char** argv) {
    const size_t TOTAL = 100ull * 1024 * 1024;
    int run_all = 0;

    if (argc == 1) {
        fprintf(stderr, "Usage: %s [--all | --stdio-fputc | --syswrite BUF]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--all") == 0) run_all = 1;
    }

    if (run_all || (argc > 1 && strcmp(argv[1], "--stdio-fputc") == 0))
        bench_stdio_fputc(TOTAL, "test_stdio.bin");

    if (run_all) {
        bench_syswrite(TOTAL, 512, "test_syscall_512.bin");
        bench_syswrite(TOTAL, 4096, "test_syscall_4k.bin");
        bench_syswrite(TOTAL, 65536, "test_syscall_64k.bin");
    }
    else if (argc > 2 && strcmp(argv[1], "--syswrite") == 0) {
        size_t b = strtoull(argv[2], NULL, 10);
        bench_syswrite(TOTAL, b, "test_syscall_custom.bin");
    }

    return 0;
}