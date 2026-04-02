#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define FILE_SIZE (10 * 1024 * 1024) // 10MB для быстрого теста
#define FILENAME "testfile.bin"

void create_test_file() {
    printf("Creating test file (%d MB)...\n", FILE_SIZE / (1024 * 1024));
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for creation failed");
        exit(1);
    }
    
    // Используем меньший буфер для экономии памяти
    size_t buffer_size = 64 * 1024; // 64KB
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed for buffer");
        close(fd);
        exit(1);
    }
    
    size_t total_written = 0;
    while (total_written < FILE_SIZE) {
        size_t to_write = (FILE_SIZE - total_written < buffer_size) ? 
                         FILE_SIZE - total_written : buffer_size;
        memset(buffer, total_written % 256, to_write);
        
        ssize_t written = write(fd, buffer, to_write);
        if (written == -1) {
            perror("write failed");
            free(buffer);
            close(fd);
            exit(1);
        }
        total_written += written;
    }
    
    free(buffer);
    close(fd);
    printf("Test file created: %s (%zu bytes)\n", FILENAME, total_written);
}

double read_with_syscalls() {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("open for read failed");
        return -1;
    }
    
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed for read buffer");
        close(fd);
        return -1;
    }
    
    unsigned long sum = 0;
    ssize_t bytes_read;
    
    clock_t start = clock();
    
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i]; // Беззнаковое для избежания переполнения
        }
    }
    
    clock_t end = clock();
    
    if (bytes_read == -1) {
        perror("read failed");
    }
    
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
    
    if (sb.st_size == 0) {
        printf("Error: file is empty\n");
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
    
    // Читаем с шагом для ускорения на больших файлах
    for (off_t i = 0; i < sb.st_size; i += sizeof(char)) {
        sum += (unsigned char)data[i];
    }
    
    clock_t end = clock();
    
    if (munmap(data, sb.st_size) == -1) {
        perror("munmap failed");
    }
    close(fd);
    
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("mmap() sum: %lu, ", sum);
    return elapsed;
}

void print_page_faults() {
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", getpid());
    
    FILE *f = fopen(stat_path, "r");
    if (f) {
        char line[1024];
        if (fgets(line, sizeof(line), f)) {
            char *token = strtok(line, " ");
            for (int i = 1; i <= 12 && token != NULL; i++) {
                token = strtok(NULL, " ");
                if (i == 10) printf("Minor faults: %s, ", token ? token : "N/A");
                if (i == 12) printf("Major faults: %s\n", token ? token : "N/A");
            }
        }
        fclose(f);
    } else {
        printf("Cannot access page fault statistics\n");
    }
}

int try_clear_page_cache() {
    printf("Attempting to clear page cache... ");
    
    FILE *f = fopen("/proc/sys/vm/drop_caches", "w");
    if (f) {
        fprintf(f, "3\n");
        fclose(f);
        printf("Success (root required)\n");
        return 1;
    } else {
        printf("Failed - no root access\n");
        return 0;
    }
}

int main() {
    printf("=== I/O Comparison Test (No Sudo Required) ===\n");
    
    create_test_file();
    
    printf("\nNote: Running without page cache clearing\n");
    printf("For more accurate results, run manually as root:\n");
    printf("  sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches\n\n");
    
    // Не очищаем кэш автоматически - только информируем
    int cache_cleared = 0;
    if (geteuid() == 0) {
        cache_cleared = try_clear_page_cache();
    }
    
    if (cache_cleared) {
        sleep(1); // Даем время на очистку
    }
    
    printf("\n--- Testing read() ---\n");
    print_page_faults();
    double time_read = read_with_syscalls();
    printf("Time: %.3f seconds\n", time_read);
    
    printf("\n--- Testing mmap() ---\n");
    print_page_faults();
    double time_mmap = read_with_mmap();
    printf("Time: %.3f seconds\n", time_mmap);
    
    printf("\n=== Results ===\n");
    printf("read():  %.3f seconds\n", time_read);
    printf("mmap():  %.3f seconds\n", time_mmap);
    
    if (time_read > 0 && time_mmap > 0) {
        double speedup = time_read / time_mmap;
        printf("Speedup: %.2fx\n", speedup);
        
        if (speedup > 1.0) {
            printf("mmap() is %.1f%% faster\n", (speedup - 1.0) * 100);
        } else {
            printf("read() is %.1f%% faster\n", (1.0/speedup - 1.0) * 100);
        }
    }
    
    if (!cache_cleared) {
        printf("\n⚠️  Note: Tests were run with existing page cache\n");
        printf("   Results show performance with caching. For cold cache tests,\n");
        printf("   clear cache manually as shown above.\n");
    }
    
    // Удаляем тестовый файл
    if (unlink(FILENAME) == 0) {
        printf("Test file cleaned up\n");
    }
    
    return 0;
}
