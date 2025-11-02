#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define FILE_SIZE (100 * 1024 * 1024) // 100MB
#define FILENAME "testfile.bin"

void create_test_file() {
    printf("Creating test file...\n");
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for creation failed");
        exit(1);
    }
    
    // Заполняем файл случайными данными
    char *buffer = malloc(1024 * 1024); // 1MB буфер
    for (int i = 0; i < 100; i++) { // Исправлено: 100 итераций по 1MB = 100MB
        memset(buffer, i % 256, 1024 * 1024);
        write(fd, buffer, 1024 * 1024);
    }
    free(buffer);
    close(fd);
    printf("Test file created: %s\n", FILENAME);
}

double read_with_syscalls() {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("open for read failed");
        return -1;
    }
    
    char *buffer = malloc(4096);
    unsigned long sum = 0;
    
    // Используем clock() вместо clock_gettime для простоты
    clock_t start = clock();
    
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, 4096)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += buffer[i];
        }
    }
    
    clock_t end = clock();
    
    free(buffer);
    close(fd);
    
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("read() sum: %lu, ", sum);
    return elapsed;
}

double read_with_mmap() {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("open for mmap failed");
        return -1;
    }
    
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return -1;
    }
    
    char *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }
    
    unsigned long sum = 0;
    
    clock_t start = clock();
    
    for (off_t i = 0; i < sb.st_size; i++) {
        sum += data[i];
    }
    
    clock_t end = clock();
    
    munmap(data, sb.st_size);
    close(fd);
    
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("mmap() sum: %lu, ", sum);
    return elapsed;
}

void print_page_faults(pid_t pid) {
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(stat_path, "r");
    if (f) {
        char line[1024];
        if (fgets(line, sizeof(line), f)) {
            char *token = strtok(line, " ");
            for (int i = 1; i <= 12; i++) {
                token = strtok(NULL, " ");
                if (i == 10) printf("Minor faults: %s, ", token);
                if (i == 12) printf("Major faults: %s\n", token);
            }
        }
        fclose(f);
    }
}

int main() {
    create_test_file();
    
    printf("Clearing page cache...\n");
    system("sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches");
    sleep(2);
    
    printf("\n--- Testing read() ---\n");
    print_page_faults(getpid());
    double time_read = read_with_syscalls();
    printf("Time: %.3f seconds\n", time_read);
    print_page_faults(getpid());
    
    printf("\nClearing page cache...\n");
    system("sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches");
    sleep(2);
    
    printf("\n--- Testing mmap() ---\n");
    print_page_faults(getpid());
    double time_mmap = read_with_mmap();
    printf("Time: %.3f seconds\n", time_mmap);
    print_page_faults(getpid());
    
    printf("\n--- Results ---\n");
    printf("read():  %.3f seconds\n", time_read);
    printf("mmap():  %.3f seconds\n", time_mmap);
    if (time_read > 0 && time_mmap > 0) {
        printf("Speedup: %.2fx\n", time_read / time_mmap);
    }
    
    // Удаляем тестовый файл
    unlink(FILENAME);
    
    return 0;
}
