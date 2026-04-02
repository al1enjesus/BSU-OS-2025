#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/resource.h>

#define FILE_NAME "testfile.bin"
#define FILE_SIZE (100 * 1024 * 1024) // 100 MB

void create_test_file() {
    printf("Creating 100MB test file...\n");
    char command[256];
    snprintf(command, sizeof(command), 
             "dd if=/dev/urandom of=%s bs=1M count=100 status=none", FILE_NAME);
    if (system(command) != 0) {
        fprintf(stderr, "Failed to create test file\n");
        exit(1);
    }
    printf("Test file created successfully\n");
}

void print_page_faults(const char* stage) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Page faults - %s: Minor: %ld, Major: %ld\n",
           stage, usage.ru_minflt, usage.ru_majflt);
}

unsigned long long read_with_syscalls(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    unsigned char buffer[8192];
    ssize_t bytes;
    unsigned long long sum = 0;

    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes; i++)
            sum += buffer[i];
    }

    close(fd);
    return sum;
}

unsigned long long read_with_mmap(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); exit(1); }
    size_t filesize = st.st_size;

    unsigned char *data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); exit(1); }

    unsigned long long sum = 0;
    for (size_t i = 0; i < filesize; i++)
        sum += data[i];

    munmap(data, filesize);
    close(fd);
    return sum;
}

void run_test(const char* test_name, unsigned long long (*func)(const char *), 
              const char *filename, int clear_cache) {
    struct rusage start_usage, end_usage;
    struct timespec start_time, end_time;
    
    printf("\n=== %s ===\n", test_name);
    
    if (clear_cache) {
        printf("Clearing page cache...\n");
        system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' 2>/dev/null");
        sleep(1);
    }
    
    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    unsigned long long sum = func(filename);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    getrusage(RUSAGE_SELF, &end_usage);
    
    double time_spent = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    long minor_faults = end_usage.ru_minflt - start_usage.ru_minflt;
    long major_faults = end_usage.ru_majflt - start_usage.ru_majflt;
    
    printf("Checksum: %llu\n", sum);
    printf("Time: %.3f seconds\n", time_spent);
    printf("Page faults - Minor: %ld, Major: %ld\n", minor_faults, major_faults);
}

int main() {
    printf("=== File I/O Performance Comparison ===\n");
    printf("Comparing read() syscalls vs mmap() for 100MB file\n\n");
    
    create_test_file();
    
    // Тест 1: read() системные вызовы
    run_test("TRADITIONAL: read() syscalls", read_with_syscalls, FILE_NAME, 0);
    
    // Тест 2: mmap() с очисткой кеша
    run_test("MEMORY MAPPING: mmap()", read_with_mmap, FILE_NAME, 1);
    
    // Уборка
    unlink(FILE_NAME);
    printf("\nTest file removed.\n");
    
    return 0;
}
