#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>

#define FILE_SIZE_MB 500
#define BUFFER_SIZE (64 * 1024)

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void intensive_io_work(const char* filename) {
    printf("Starting intensive I/O work on %s\n", filename);
    
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return;
    }
    
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return;
    }
    
    // Fill buffer with pattern
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = (char)(i % 256);
    }
    
    size_t total_size = FILE_SIZE_MB * 1024 * 1024;
    size_t written = 0;
    
    double start_time = get_time();
    
    // Write phase
    printf("Writing %d MB of data...\n", FILE_SIZE_MB);
    while (written < total_size) {
        size_t to_write = (total_size - written < BUFFER_SIZE) ? 
                         (total_size - written) : BUFFER_SIZE;
        ssize_t result = write(fd, buffer, to_write);
        if (result == -1) {
            perror("write failed");
            break;
        }
        written += result;
    }
    
    fsync(fd);
    double write_time = get_time() - start_time;
    
    // Read phase
    printf("Reading back data...\n");
    lseek(fd, 0, SEEK_SET);
    
    start_time = get_time();
    size_t read_total = 0;
    while (read_total < total_size) {
        size_t to_read = (total_size - read_total < BUFFER_SIZE) ?
                        (total_size - read_total) : BUFFER_SIZE;
        ssize_t result = read(fd, buffer, to_read);
        if (result == -1) {
            perror("read failed");
            break;
        }
        read_total += result;
    }
    
    double read_time = get_time() - start_time;
    
    printf("\nI/O Performance Results:\n");
    printf("Write time: %.2f seconds, Throughput: %.2f MB/s\n", 
           write_time, FILE_SIZE_MB / write_time);
    printf("Read time: %.2f seconds, Throughput: %.2f MB/s\n", 
           read_time, FILE_SIZE_MB / read_time);
    
    free(buffer);
    close(fd);
}

void print_io_stats(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Cannot read /proc/%d/io (try running as root)\n", pid);
        return;
    }
    
    printf("\nI/O Statistics for PID %d:\n", pid);
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("  %s", line);
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    printf("Disk I/O Monitor - Variant 2\n");
    printf("=============================\n\n");
    
    const char *testfile = "io_stress_test.bin";
    pid_t my_pid = getpid();
    
    printf("My PID: %d\n", my_pid);
    printf("Test file: %s\n", testfile);
    
    printf("\n=== Starting I/O intensive work ===\n");
    intensive_io_work(testfile);
    
    printf("\n=== I/O Statistics ===\n");
    print_io_stats(my_pid);
    
    // Show system I/O stats
    printf("\n=== System I/O Statistics ===\n");
    printf("Running iostat...\n");
    system("iostat -x 1 1");
    
    printf("\n=== Process I/O Statistics ===\n");
    system("pidstat -d 1 1 2>/dev/null | head -10");
    
    // I/O scheduler info
    printf("\n=== I/O Scheduler Info ===\n");
    system("cat /sys/block/sda/queue/scheduler 2>/dev/null || echo 'Cannot read scheduler'");
    
    // Cleanup
    unlink(testfile);
    
    printf("\n=== Instructions for further analysis ===\n");
    printf("1. Monitor real-time I/O:\n");
    printf("   $ iostat -x 1\n");
    printf("   $ sudo iotop\n");
    printf("\n2. Change I/O scheduler (requires root):\n");
    printf("   $ echo bfq | sudo tee /sys/block/sda/queue/scheduler\n");
    printf("\n3. Check disk stats:\n");
    printf("   $ cat /proc/diskstats\n");
    
    return 0;
}
