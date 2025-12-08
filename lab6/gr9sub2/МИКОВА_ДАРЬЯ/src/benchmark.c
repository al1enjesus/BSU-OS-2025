#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TEST_DIR "/tmp/fuse_bench_test"
#define TEST_FILE_SIZE (1024 * 1024)      
#define NUM_SMALL_FILES 100
#define SMALL_FILE_SIZE 1024              

typedef struct {
    const char *name;
    double time_ms;
} benchmark_result_t;


static double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}


static double test_single_open() {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_open.txt", TEST_DIR);
    
   
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("open (create)");
        return -1;
    }
    write(fd, "test", 4);
    close(fd);
    
   
    double start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        fd = open(path, O_RDONLY);
        if (fd < 0) break;
        close(fd);
    }
    double elapsed = get_time_ms() - start;
    
    unlink(path);
    return elapsed / 1000.0; 
}


static double test_4kb_read() {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_read_4k.txt", TEST_DIR);
    
    
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("open (create)");
        return -1;
    }
    
    char *buf = malloc(TEST_FILE_SIZE);
    memset(buf, 'A', TEST_FILE_SIZE);
    write(fd, buf, TEST_FILE_SIZE);
    close(fd);
    
    
    double start = get_time_ms();
    int reads = 0;
    for (int i = 0; i < TEST_FILE_SIZE; i += 4096) {
        fd = open(path, O_RDONLY);
        read(fd, buf, 4096);
        close(fd);
        reads++;
    }
    double elapsed = get_time_ms() - start;
    
    free(buf);
    unlink(path);
    return elapsed / reads; 
}


static double test_4kb_write() {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_write_4k.txt", TEST_DIR);
    
    char *buf = malloc(4096);
    memset(buf, 'B', 4096);
    
    double start = get_time_ms();
    for (int i = 0; i < 256; i++) {
        int fd = open(path, O_WRONLY | O_CREAT, 0644);
        if (fd < 0) break;
        write(fd, buf, 4096);
        close(fd);
    }
    double elapsed = get_time_ms() - start;
    
    free(buf);
    unlink(path);
    return elapsed / 256.0;
}


static double test_stat() {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_stat.txt", TEST_DIR);
    

    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    write(fd, "test", 4);
    close(fd);
    
    struct stat st;
    double start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        stat(path, &st);
    }
    double elapsed = get_time_ms() - start;
    
    unlink(path);
    return elapsed / 1000.0;
}

static double test_sequential_read(size_t file_size) {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_seq.txt", TEST_DIR);
    
 
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    char *buf = malloc(file_size);
    memset(buf, 'C', file_size);
    write(fd, buf, file_size);
    close(fd);
    
     double start = get_time_ms();
    fd = open(path, O_RDONLY);
    ssize_t total = 0;
    char read_buf[65536];
    while (1) {
        ssize_t n = read(fd, read_buf, sizeof(read_buf));
        if (n <= 0) break;
        total += n;
    }
    close(fd);
    double elapsed = get_time_ms() - start;
    
    free(buf);
    unlink(path);
    
    double throughput = (total / (1024.0 * 1024.0)) / (elapsed / 1000.0); 
    return throughput;
}

static double test_small_files_iops() {
    double start = get_time_ms();
    
    for (int i = 0; i < NUM_SMALL_FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/small_%d.txt", TEST_DIR, i);
        
        int fd = open(path, O_WRONLY | O_CREAT, 0644);
        if (fd < 0) continue;
        
        char buf[SMALL_FILE_SIZE];
        memset(buf, 'D', SMALL_FILE_SIZE);
        write(fd, buf, SMALL_FILE_SIZE);
        close(fd);
    }
    
    double elapsed = get_time_ms() - start;
    
   
    for (int i = 0; i < NUM_SMALL_FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/small_%d.txt", TEST_DIR, i);
        unlink(path);
    }
    
    double iops = (NUM_SMALL_FILES * 1000.0) / elapsed;
    return iops; 
}

int main() {
    mkdir(TEST_DIR, 0755);
    
    printf("FUSE File System Benchmark\n");
    printf("============================\n\n");
    
    printf("Test 1: Single file open (1000 iterations)\n");
    double result1 = test_single_open();
    printf("  Result: %.4f ms per open\n\n", result1);
    
    printf("Test 2: 4KB sequential read\n");
    double result2 = test_4kb_read();
    printf("  Result: %.4f ms per 4KB read\n\n", result2);
    
    printf("Test 3: 4KB sequential write\n");
    double result3 = test_4kb_write();
    printf("  Result: %.4f ms per 4KB write\n\n", result3);
    
    printf("Test 4: Stat (metadata) - 1000 iterations\n");
    double result4 = test_stat();
    printf("  Result: %.4f ms per stat\n\n", result4);
    
    printf("Test 5: Sequential read throughput (1 MB file)\n");
    double result5 = test_sequential_read(1024 * 1024);
    printf("  Result: %.2f MB/s\n\n", result5);
    
    printf("Test 6: Sequential read throughput (10 MB file)\n");
    double result6 = test_sequential_read(10 * 1024 * 1024);
    printf("  Result: %.2f MB/s\n\n", result6);
    
    printf("Test 7: Small files IOPS (%d x %d bytes)\n", NUM_SMALL_FILES, SMALL_FILE_SIZE);
    double result7 = test_small_files_iops();
    printf("  Result: %.2f files/sec\n\n", result7);
    
    rmdir(TEST_DIR);
    
    return 0;
}
