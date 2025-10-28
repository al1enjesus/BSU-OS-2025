#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#define FILE_SIZE (100 * 1024 * 1024)

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void cleanup_file(const char* filename) {
    if (access(filename, F_OK) != -1) {
        unlink(filename);
    }
}

double benchmark_fwrite(size_t buffer_size) {
    char filename[256];
    snprintf(filename, sizeof(filename), "test_fwrite_%zu.bin", buffer_size);
    cleanup_file(filename);
    
    double start = get_time();
    
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }
    
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }
    memset(buffer, 'A', buffer_size);
    
    size_t total_written = 0;
    while (total_written < FILE_SIZE) {
        size_t to_write = (FILE_SIZE - total_written < buffer_size) ? 
                         (FILE_SIZE - total_written) : buffer_size;
        size_t written = fwrite(buffer, 1, to_write, f);
        if (written != to_write) {
            perror("fwrite failed");
            break;
        }
        total_written += written;
    }
    
    fclose(f);
    free(buffer);
    
    double end = get_time();
    double elapsed = end - start;
    
    printf("fwrite (buffer=%5zu bytes): %6.3f sec, %6.2f MB/s\n", 
           buffer_size, elapsed, (FILE_SIZE / (1024.0 * 1024.0)) / elapsed);
    
    cleanup_file(filename);
    return elapsed;
}


double benchmark_write(size_t buffer_size) {
    char filename[256];
    snprintf(filename, sizeof(filename), "test_write_%zu.bin", buffer_size);
    cleanup_file(filename);
    
    double start = get_time();
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }
    
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'A', buffer_size);
    
    size_t total_written = 0;
    while (total_written < FILE_SIZE) {
        size_t to_write = (FILE_SIZE - total_written < buffer_size) ? 
                         (FILE_SIZE - total_written) : buffer_size;
        ssize_t written = write(fd, buffer, to_write);
        if (written != (ssize_t)to_write) {
            perror("write failed");
            break;
        }
        total_written += written;
    }
    
    close(fd);
    free(buffer);
    
    double end = get_time();
    double elapsed = end - start;
    
    printf("write  (buffer=%5zu bytes): %6.3f sec, %6.2f MB/s\n", 
           buffer_size, elapsed, (FILE_SIZE / (1024.0 * 1024.0)) / elapsed);
    
    cleanup_file(filename);
    return elapsed;
}


void benchmark_buffer_sizes() {
    printf("\n=== Сравнение размеров буфера ===\n");
    printf("Запись 100 MB данных\n\n");
    
    size_t buffer_sizes[] = {512, 1024, 4096, 8192, 16384, 32768, 65536, 131072, 1024*1024};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);
    
    printf("Размер буфера |   fwrite время   |   write время    | fwrite скорость | write скорость | Кол-во вызовов\n");
    printf("--------------|------------------|------------------|-----------------|----------------|----------------\n");
    
    for (int i = 0; i < num_sizes; i++) {
        size_t buffer_size = buffer_sizes[i];
        int num_calls = FILE_SIZE / buffer_size + (FILE_SIZE % buffer_size ? 1 : 0);
        
        double time_fwrite = benchmark_fwrite(buffer_size);
        double time_write = benchmark_write(buffer_size);
        
        printf("%11zu B | %8.3f сек     | %8.3f сек     | %7.2f MB/s   | %6.2f MB/s   | %9d\n",
               buffer_size, time_fwrite, time_write,
               (FILE_SIZE / (1024.0 * 1024.0)) / time_fwrite,
               (FILE_SIZE / (1024.0 * 1024.0)) / time_write,
               num_calls);
        
        sleep(1);
    }
}


void benchmark_methods() {
    printf("\n=== Сравнение методов I/O ===\n");
    printf("Запись 100 MB данных с буфером 64 KB\n\n");
    
    size_t optimal_buffer = 65536; 
    
    printf("Метод          | Время (сек) | Скорость (MB/s) | Системные вызовы\n");
    printf("----------------|-------------|-----------------|------------------\n");
    
    double time_fwrite = benchmark_fwrite(optimal_buffer);
    double time_write = benchmark_write(optimal_buffer);
    
    printf("fwrite (stdio)  |    %6.3f    |      %6.2f      |       ~%d\n", 
           time_fwrite, (FILE_SIZE / (1024.0 * 1024.0)) / time_fwrite, 
           FILE_SIZE / optimal_buffer / 8); 
    
    printf("write (syscall) |    %6.3f    |      %6.2f      |       %d\n", 
           time_write, (FILE_SIZE / (1024.0 * 1024.0)) / time_write,
           FILE_SIZE / optimal_buffer);
}

int main() {
    printf("Бенчмарк методов файлового I/O\n");
    printf("===============================\n");
    
    
    benchmark_methods();
    benchmark_buffer_sizes();
    
    printf("\n=== Анализ системных вызовов ===\n");
    printf("Для детального анализа запустите:\n");
    printf("  strace -c ./program  # статистика вызовов\n");
    printf("  strace -e trace=write ./program  # трассировка write\n");
    
    return 0;
}
