#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define DEFAULT_SIZE_MB 100

static double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}

static double benchmark_fwrite(const char *filename, size_t size, size_t buf_sz) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buf_sz);
    FILE *f = fopen(filename, "wb"); if (!f){perror("fopen"); return -1;}
    char *buf = (char*)malloc(buf_sz); if (!buf){perror("malloc"); fclose(f); return -1;}
    memset(buf, 'A', buf_sz);

    double t0 = now_sec();
    size_t written = 0;
    while (written < size) {
        size_t chunk = (size - written < buf_sz) ? (size - written) : buf_sz;
        if (fwrite(buf, 1, chunk, f) != chunk) { perror("fwrite"); break; }
        written += chunk;
    }
    fflush(f);
    double t1 = now_sec();
    double sec = t1 - t0;
    printf("Time: %.3f s, Throughput: %.2f MB/s\n", sec, (size/1024.0/1024.0)/sec);
    free(buf); fclose(f); return sec;
}

static double benchmark_write(const char *filename, size_t size, size_t buf_sz) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buf_sz);
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644); if (fd==-1){perror("open"); return -1;}
    char *buf = (char*)malloc(buf_sz); if (!buf){perror("malloc"); close(fd); return -1;}
    memset(buf, 'B', buf_sz);

    double t0 = now_sec();
    size_t written = 0;
    while (written < size) {
        size_t chunk = (size - written < buf_sz) ? (size - written) : buf_sz;
        ssize_t n = write(fd, buf, chunk);
        if (n != (ssize_t)chunk) { perror("write"); break; }
        written += chunk;
    }
    double t1 = now_sec();
    double sec = t1 - t0;
    printf("Time: %.3f s, Throughput: %.2f MB/s\n", sec, (size/1024.0/1024.0)/sec);
    free(buf); close(fd); return sec;
}

static double benchmark_write_sync(const char *filename, size_t size, size_t buf_sz) {
    printf("\n=== write() with O_SYNC (buffer=%zu bytes) ===\n", buf_sz);
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC, 0644); if (fd==-1){perror("open"); return -1;}
    char *buf = (char*)malloc(buf_sz); if (!buf){perror("malloc"); close(fd); return -1;}
    memset(buf, 'S', buf_sz);

    double t0 = now_sec();
    size_t written = 0;
    while (written < size) {
        size_t chunk = (size - written < buf_sz) ? (size - written) : buf_sz;
        ssize_t n = write(fd, buf, chunk);
        if (n != (ssize_t)chunk) { perror("write"); break; }
        written += chunk;
    }
    double t1 = now_sec();
    double sec = t1 - t0;
    printf("Time: %.3f s, Throughput: %.2f MB/s (expected slow)\n", sec, (size/1024.0/1024.0)/sec);
    free(buf); close(fd); return sec;
}

static double benchmark_mmap(const char *filename, size_t size) {
    printf("\n=== mmap() ===\n");
    int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644); if (fd==-1){perror("open"); return -1;}
    if (ftruncate(fd, size)==-1){perror("ftruncate"); close(fd); return -1;}
    void *p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); return -1; }

    double t0 = now_sec();
    memset(p, 'C', size);
    // msync(p, size, MS_SYNC); // по желанию
    double t1 = now_sec();
    double sec = t1 - t0;
    printf("Time: %.3f s, Throughput: %.2f MB/s\n", sec, (size/1024.0/1024.0)/sec);
    munmap(p, size); close(fd); return sec;
}

static void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\nBenchmark: Buffer Size Impact\n========================================\n");
    size_t size = file_size_mb * 1024ULL * 1024ULL;
    size_t bufs[] = {512, 1024, 4096, 8192, 16384, 65536, 1024*1024};
    int n = (int)(sizeof(bufs)/sizeof(bufs[0]));
    for (int i=0;i<n;i++) {
        char name[64]; snprintf(name, sizeof(name), "test_buffer_%zu.bin", bufs[i]);
        benchmark_write(name, size, bufs[i]);
        unlink(name);
        sleep(1);
    }
}

static void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\nBenchmark: I/O Methods Comparison\n========================================\n");
    size_t size = file_size_mb * 1024ULL * 1024ULL;
    size_t optimal = 64 * 1024;
    printf("\nFile size: %zu MB\nBuffer size: %zu KB\n", file_size_mb, optimal/1024);

    benchmark_fwrite("test_fwrite.bin", size, optimal); unlink("test_fwrite.bin"); sleep(1);
    benchmark_write ("test_write.bin",  size, optimal); unlink("test_write.bin");  sleep(1);
    benchmark_mmap  ("test_mmap.bin",   size);          unlink("test_mmap.bin");

    printf("\n--- O_SYNC (slow) ---\n");
    benchmark_write_sync("test_osync.bin", size, optimal); unlink("test_osync.bin");

    printf("\nSummary: compare times above.\n");
}

int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"--size")==0 && i+1<argc) size_mb = (size_t)atoi(argv[++i]);

    printf("I/O Benchmark\n=============\nTest file size: %zu MB\n", size_mb);

    benchmark_all_methods(size_mb);
    benchmark_buffer_sizes(size_mb);

    printf("\n========================================\nBenchmark completed!\n========================================\n");
    return 0;
}
