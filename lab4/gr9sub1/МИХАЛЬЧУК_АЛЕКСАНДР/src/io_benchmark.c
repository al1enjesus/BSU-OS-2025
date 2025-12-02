#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#define FILE_SIZE (100 * 1024 * 1024) // 100 MB

// Тест с stdio (буферизованный)
double test_stdio_write(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fopen failed");
        return -1;
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < FILE_SIZE; i++) {
        fputc('A', file);
    }
    
    clock_t end = clock();
    fclose(file);
    
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Тест с системными вызовами и разным размером буфера
double test_syscall_write(const char* filename, size_t buffer_size) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }
    
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        close(fd);
        return -1;
    }
    
    memset(buffer, 'B', buffer_size);
    
    clock_t start = clock();
    
    for (size_t written = 0; written < FILE_SIZE; written += buffer_size) {
        ssize_t bytes_written = write(fd, buffer, buffer_size);
        if (bytes_written == -1) {
            perror("write failed");
            free(buffer);
            close(fd);
            return -1;
        }
    }
    
    clock_t end = clock();
    
    free(buffer);
    close(fd);
    
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Тест с stdio и блочной записью
double test_stdio_block_write(const char* filename, size_t buffer_size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("fopen failed");
        return -1;
    }
    
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    
    memset(buffer, 'C', buffer_size);
    
    clock_t start = clock();
    
    for (size_t written = 0; written < FILE_SIZE; written += buffer_size) {
        size_t written_blocks = fwrite(buffer, 1, buffer_size, file);
        if (written_blocks != buffer_size) {
            perror("fwrite failed");
            free(buffer);
            fclose(file);
            return -1;
        }
    }
    
    clock_t end = clock();
    
    free(buffer);
    fclose(file);
    
    return (double)(end - start) / CLOCKS_PER_SEC;
}

void print_result(const char* method, double time_seconds, size_t buffer_size) {
    double speed_mbps = (FILE_SIZE / (1024.0 * 1024.0)) / time_seconds;
    printf("%-25s | %8.3f сек | %7.1f MB/s | Буфер: ", method, time_seconds, speed_mbps);
    
    if (buffer_size < 1024) {
        printf("%zu байт", buffer_size);
    } else if (buffer_size < 1024 * 1024) {
        printf("%zu KB", buffer_size / 1024);
    } else {
        printf("%zu MB", buffer_size / (1024 * 1024));
    }
    printf("\n");
}

int main() {
    printf(" Тестирование производительности I/O\n");
    printf("Размер файла: %d MB\n\n", FILE_SIZE / (1024 * 1024));
    printf("Метод                     |   Время   | Скорость | Примечание\n");
    printf("--------------------------|-----------|----------|------------\n");
    
    // Тест 1: stdio с посимвольной записью
    double time_stdio = test_stdio_write("test_stdio.bin");
    if (time_stdio > 0) {
        print_result("stdio (fputc)", time_stdio, 1);
    }
    
    // Тест 2: stdio с блочной записью
    size_t stdio_buffers[] = {512, 4096, 65536};
    for (int i = 0; i < 3; i++) {
        char name[100];
        sprintf(name, "test_stdio_block_%zu.bin", stdio_buffers[i]);
        double time = test_stdio_block_write(name, stdio_buffers[i]);
        if (time > 0) {
            char method[50];
            sprintf(method, "stdio (fwrite %zu)", stdio_buffers[i]);
            print_result(method, time, stdio_buffers[i]);
        }
    }
    
    // Тест 3: системные вызовы с разными размерами буфера
    size_t buffer_sizes[] = {1, 512, 4096, 16384, 65536, 131072};
    for (int i = 0; i < 6; i++) {
        char name[100];
        sprintf(name, "test_syscall_%zu.bin", buffer_sizes[i]);
        double time = test_syscall_write(name, buffer_sizes[i]);
        if (time > 0) {
            char method[50];
            sprintf(method, "syscall (write %zu)", buffer_sizes[i]);
            print_result(method, time, buffer_sizes[i]);
        }
    }
    
    printf("\n Анализ системных вызовов:\n");
    printf("Запустите в другом терминале для анализа:\n");
    printf("strace -c -e trace=write ./io_benchmark\n");
    printf("strace -c ./io_benchmark\n\n");
    
    // Удаляем тестовые файлы (без предупреждения)
    int ret = system("rm -f test_*.bin");
    (void)ret; // Игнорируем возвращаемое значение
    
    return 0;
}
