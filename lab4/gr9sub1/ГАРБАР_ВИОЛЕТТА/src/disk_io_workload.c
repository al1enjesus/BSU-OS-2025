#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#define FILE_SIZE (500 * 1024 * 1024) 
#define NUM_FILES 3

void generate_workload() {
    printf("Генерация дискового I/O workload...\n");
    printf("Создание %d файлов по %d MB каждый\n", NUM_FILES, FILE_SIZE / (1024*1024));
    
    for (int file_num = 0; file_num < NUM_FILES; file_num++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "workload_file_%d.bin", file_num);
        
        printf("Запись файла %s...\n", filename);
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open for write failed");
            continue;
        }
        
        size_t buffer_size = 64 * 1024; // 64 KB
        char *buffer = malloc(buffer_size);
        if (!buffer) {
            perror("malloc failed");
            close(fd);
            continue;
        }
        memset(buffer, file_num + 'A', buffer_size);
        
        for (size_t written = 0; written < FILE_SIZE; written += buffer_size) {
            ssize_t result = write(fd, buffer, buffer_size);
            if (result != (ssize_t)buffer_size) {
                perror("write failed");
                break;
            }
        }
        
        close(fd);
        free(buffer);
        
        printf("Чтение файла %s...\n", filename);
        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("open for read failed");
            continue;
        }
        
        buffer = malloc(buffer_size);
        if (!buffer) {
            perror("malloc failed");
            close(fd);
            continue;
        }
        
        unsigned long long sum = 0;
        lseek(fd, 0, SEEK_SET);
        
        for (size_t read_bytes = 0; read_bytes < FILE_SIZE; read_bytes += buffer_size) {
            ssize_t result = read(fd, buffer, buffer_size);
            if (result != (ssize_t)buffer_size) {
                perror("read failed");
                break;
            }
            sum += (unsigned char)buffer[0];
        }
        
        close(fd);
        free(buffer);
        
        printf("Файл %s обработан (checksum: %llu)\n", filename, sum);
    }
    
    for (int file_num = 0; file_num < NUM_FILES; file_num++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "workload_file_%d.bin", file_num);
        if (unlink(filename) == -1) {
            perror("unlink failed");
        }
    }
    
    printf("Workload завершен\n");
}

int main() {
    printf("=== Дисковый I/O Workload ===\n");
    printf("PID процесса: %d\n", getpid());
    printf("Запустите мониторинг в другом терминале:\n");
    printf("  iostat -x 1 10\n");
    printf("  pidstat -d 1 10\n");
    printf("  sudo iotop -b -n 10\n");
    printf("  cat /proc/%d/io\n", getpid());
    printf("Нажмите Enter для начала...\n");
    getchar();
    
    generate_workload();
    
    return 0;
}
