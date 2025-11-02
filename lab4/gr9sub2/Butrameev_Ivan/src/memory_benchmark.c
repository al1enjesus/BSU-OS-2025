#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define FILE_PATH "testfile.bin"
#define RUNS 5

unsigned long long read_with_syscalls(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open failed"); return 0; }
    unsigned char *buffer = malloc(4096);
    if (!buffer) { perror("malloc failed"); close(fd); return 0; }
    unsigned long long sum = 0; ssize_t r;
    while ((r = read(fd, buffer, 4096)) > 0)
        for (ssize_t i = 0; i < r; i++) sum += buffer[i];
    free(buffer); close(fd); return sum;
}

unsigned long long read_with_mmap(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open failed"); return 0; }
    struct stat st; fstat(fd, &st);
    size_t len = st.st_size;
    unsigned char *addr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) { perror("mmap failed"); close(fd); return 0; }
    unsigned long long sum = 0;
    for (size_t i = 0; i < len; i++) sum += addr[i];
    munmap(addr, len); close(fd); return sum;
}

void print_pagefaults() {
    FILE *f = fopen("/proc/self/stat", "r");
    if (!f) return;
    char buf[4096];
    if (fgets(buf, sizeof(buf), f)) {
        unsigned long dummy; int pid;
        char comm[64], state;
        sscanf(buf, "%d %63s %c", &pid, comm, &state);
        char *p = buf;
        int field = 1;
        unsigned long min_flt = 0, maj_flt = 0;
        while (field < 10) { // skip first 9 fields
            if (*p == ' ') field++; p++;
        }
        sscanf(p, "%lu", &min_flt); // minor
        while (*p && *p != ' ') p++; while (*p == ' ') p++;
        sscanf(p, "%lu", &maj_flt); // major
        printf("min_flt (minor page faults): %lu\n", min_flt);
        printf("maj_flt (major page faults): %lu\n", maj_flt);
    }
    fclose(f);
}

int main() {
    printf("PID: %d\n", getpid());
    printf("Benchmarking: %s (%d runs)\n", FILE_PATH, RUNS);
    double read_t = 0, mmap_t = 0; unsigned long long sum_r = 0, sum_m = 0;
    for (int i = 0; i < RUNS; ++i) {
        clock_t s = clock(); sum_r = read_with_syscalls(FILE_PATH); clock_t e = clock();
        double t = (e-s)/(double)CLOCKS_PER_SEC; read_t += t;
        printf("read() sum=%llu, time=%.4f s\n", sum_r, t);
        s = clock(); sum_m = read_with_mmap(FILE_PATH); e = clock();
        t = (e-s)/(double)CLOCKS_PER_SEC; mmap_t += t;
        printf("mmap() sum=%llu, time=%.4f s\n", sum_m, t);
        if (sum_r != sum_m) printf("[!] Warning: sums differ!\n");
    }
    printf("Avg read() : %.4f s; mmap(): %.4f s\n", read_t/RUNS, mmap_t/RUNS);
    print_pagefaults();
    printf("Press Enter to exit...\n"); getchar();
}
