#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE (4 * 1024)

typedef struct { long minor_faults; long major_faults; } PageFaultStats;

static PageFaultStats get_page_faults(void) {
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    return (PageFaultStats){ .minor_faults = u.ru_minflt, .major_faults = u.ru_majflt };
}

static double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}

static void create_test_file(const char *filename, size_t size_mb) {
    printf("Creating test file '%s' (%zu MB)...\n", filename, size_mb);
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd == -1) { perror("open"); exit(1); }
    char buf[4096]; for (int i=0;i<4096;i++) buf[i] = (char)(i & 0xff);

    size_t total = size_mb * 1024ULL * 1024ULL;
    size_t written = 0;
    while (written < total) {
        size_t chunk = (total - written) < sizeof(buf) ? (total - written) : sizeof(buf);
        if (write(fd, buf, chunk) != (ssize_t)chunk) { perror("write"); close(fd); exit(1); }
        written += chunk;
    }
    close(fd);
    printf("OK\n");
}

static unsigned long long read_with_syscalls(const char *filename) {
    printf("\n=== Method 1: read() syscall ===\n");
    int fd = open(filename, O_RDONLY); if (fd==-1){perror("open"); return 0;}
    struct stat sb; if (fstat(fd, &sb)==-1){perror("fstat"); close(fd); return 0;}
    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size/1024.0/1024.0);

    char *buf = (char*)malloc(BUFFER_SIZE); if (!buf){perror("malloc"); close(fd); return 0;}
    PageFaultStats s = get_page_faults(); double t0 = now_sec();

    unsigned long long sum = 0; ssize_t n;
    while ((n = read(fd, buf, BUFFER_SIZE)) > 0) {
        for (ssize_t i=0;i<n;i++) sum += (unsigned char)buf[i];
    }
    if (n<0) perror("read");

    double t1 = now_sec(); PageFaultStats e = get_page_faults();
    printf("Time elapsed: %.3f s\n", t1-t0);
    printf("Minor page faults: %ld\n", e.minor_faults - s.minor_faults);
    printf("Major page faults: %ld\n", e.major_faults - s.major_faults);
    printf("Checksum: %llu\n", sum);
    free(buf); close(fd); return sum;
}

static unsigned long long read_with_mmap(const char *filename) {
    printf("\n=== Method 2: mmap() ===\n");
    int fd = open(filename, O_RDONLY); if (fd==-1){perror("open"); return 0;}
    struct stat sb; if (fstat(fd, &sb)==-1){perror("fstat"); close(fd); return 0;}
    printf("File size: %ld bytes (%.2f MB)\n", sb.st_size, sb.st_size/1024.0/1024.0);

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); close(fd); return 0; }

    PageFaultStats s = get_page_faults(); double t0 = now_sec();

    unsigned long long sum = 0;
    unsigned char *b = (unsigned char*)data;
    for (off_t i=0; i<sb.st_size; i++) sum += b[i];

    double t1 = now_sec(); PageFaultStats e = get_page_faults();
    printf("Time elapsed: %.3f s\n", t1-t0);
    printf("Minor page faults: %ld\n", e.minor_faults - s.minor_faults);
    printf("Major page faults: %ld\n", e.major_faults - s.major_faults);
    printf("Checksum: %llu\n", sum);

    munmap(data, sb.st_size); close(fd); return sum;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename> [--create-file <size_mb>]\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    if (argc >= 3 && strcmp(argv[2], "--create-file")==0) {
        size_t sz = (argc >= 4) ? (size_t)atoi(argv[3]) : 100;
        create_test_file(filename, sz);
    }
    if (access(filename, F_OK)!=0) {
        fprintf(stderr, "File '%s' not found (use --create-file)\n", filename);
        return 1;
    }

    printf("Comparing I/O methods for file: %s\n", filename);
    printf("===========================================\n");
    unsigned long long s1 = read_with_syscalls(filename);
    sleep(1);
    unsigned long long s2 = read_with_mmap(filename);

    printf("\n=== Verification ===\n");
    if (s1 == s2) printf("✓ Checksums match: %llu\n", s1);
    else          printf("✗ Checksums differ! read(): %llu, mmap(): %llu\n", s1, s2);
    return 0;
}
