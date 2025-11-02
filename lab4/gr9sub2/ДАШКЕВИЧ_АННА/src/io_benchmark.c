// src/io_benchmark.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

static double now_sec() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}
static void fill_buf(char* b, size_t n, char v){ memset(b, v, n); }

static int write_stdio_char(size_t total_bytes, const char* path) {
    FILE* f = fopen(path, "wb");
    if(!f){ perror("fopen"); return -1; }
    for (size_t i=0;i<total_bytes;i++) {
        if (fputc('A', f)==EOF) { perror("fputc"); fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

static int write_stdio_buf(size_t total_bytes, const char* path, size_t buf_sz) {
    FILE* f = fopen(path, "wb");
    if(!f){ perror("fopen"); return -1; }
    char* buf = malloc(buf_sz);
    if(!buf){ perror("malloc"); fclose(f); return -1; }
    fill_buf(buf, buf_sz, 'B');
    size_t written = 0;
    while (written < total_bytes) {
        size_t to_write = buf_sz;
        if (to_write > total_bytes - written) to_write = total_bytes - written;
        if (fwrite(buf,1,to_write,f) != to_write) { perror("fwrite"); free(buf); fclose(f); return -1; }
        written += to_write;
    }
    free(buf);
    fclose(f);
    return 0;
}

static int write_syscall(size_t total_bytes, const char* path, size_t buf_sz) {
    int fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd<0){ perror("open"); return -1; }
    char* buf = malloc(buf_sz);
    if(!buf){ perror("malloc"); close(fd); return -1; }
    fill_buf(buf, buf_sz, 'C');
    size_t written = 0;
    while (written < total_bytes) {
        size_t to_write = buf_sz;
        if (to_write > total_bytes - written) to_write = total_bytes - written;
        ssize_t rc = write(fd, buf, to_write);
        if (rc<0){ perror("write"); free(buf); close(fd); return -1; }
        written += (size_t)rc;
    }
    free(buf);
    fsync(fd);
    close(fd);
    return 0;
}

static void bench(const char* name, int (*fn)(size_t,const char*,size_t), size_t total, const char* path, size_t buf) {
    printf("Case: %s (buf=%zu)\n", name, buf);
    double t0 = now_sec();
    int rc = fn(total, path, buf);
    double t1 = now_sec();
    if (rc==0) {
        printf("  Time: %.3f s, Throughput: %.2f MB/s\n", t1-t0, (total/1e6)/(t1-t0));
    } else {
        printf("  FAILED\n");
    }
}

int main(void){
    const size_t TOTAL = 100UL*1024UL*1024UL; // 100 MB
    printf("I/O Benchmark, total %.1f MB\n", TOTAL/1024.0/1024.0);

    bench("stdio char-by-char (fputc)", (int(*)(size_t,const char*,size_t))write_stdio_char, TOTAL, "test_stdio_char.bin", 1);
    bench("stdio fwrite buf", write_stdio_buf, TOTAL, "test_stdio_4k.bin", 4*1024);
    bench("stdio fwrite buf", write_stdio_buf, TOTAL, "test_stdio_64k.bin", 64*1024);

    bench("syscall write", write_syscall, TOTAL, "test_syscall_512.bin", 512);
    bench("syscall write", write_syscall, TOTAL, "test_syscall_4k.bin", 4*1024);
    bench("syscall write", write_syscall, TOTAL, "test_syscall_64k.bin", 64*1024);

    printf("\nUse:\n  strace -c ./bin/io_benchmark\nfor syscall stats\n");
    return 0;
}