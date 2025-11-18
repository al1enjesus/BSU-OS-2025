#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define FILE_SIZE (500 * 1024 * 1024)
#define NUM_FILES 3

int check_disk_space() {
    struct statvfs vfs;
    if (statvfs(".", &vfs) == 0) {
        unsigned long long free_space = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
        unsigned long long required_space = (unsigned long long)FILE_SIZE * NUM_FILES * 2;
        
        printf("Доступное место: %.2f GB\n", free_space / (1024.0 * 1024 * 1024));
        printf("Требуется места: %.2f GB\n", required_space / (1024.0 * 1024 * 1024));
        
        if (free_space < required_space) {
            printf("Ошибка: Недостаточно места на диске\n");
            printf("Нужно освободить еще %.2f GB\n", 
                   (required_space - free_space) / (1024.0 * 1024 * 1024));
            return -1;
        }
        return 0;
    } else {
        perror("Ошибка проверки места на диске");
        return -1;
    }
}

int write_file(const char* filename, int file_num) {
    printf("Запись файла %s...\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Ошибка открытия файла для записи");
        return -1;
    }
    
    size_t buffer_size = 64 * 1024;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Ошибка выделения памяти");
        close(fd);
        return -1;
    }
    
    memset(buffer, file_num + 'A', buffer_size);
    
    size_t total_written = 0;
    for (size_t written = 0; written < FILE_SIZE; written += buffer_size) {
        ssize_t result = write(fd, buffer, buffer_size);
        if (result != (ssize_t)buffer_size) {
            if (result == -1) {
                perror("Ошибка записи");
            } else {
                printf("Предупреждение: записано %zd из %zu байт\n", result, buffer_size);
            }
            break;
        }
        total_written += result;
    }
    
    close(fd);
    free(buffer);
    
    if (total_written < FILE_SIZE) {
        printf("Ошибка: файл %s записан не полностью\n", filename);
        return -1;
    }
    
    return 0;
}

int read_file(const char* filename, int file_num) {
    (void)file_num; 
    
    printf("Чтение файла %s...\n", filename);
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла для чтения");
        return -1;
    }
    
    size_t buffer_size = 64 * 1024;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Ошибка выделения памяти");
        close(fd);
        return -1;
    }
    
    unsigned long long sum = 0;
    lseek(fd, 0, SEEK_SET);
    
    for (size_t read_bytes = 0; read_bytes < FILE_SIZE; read_bytes += buffer_size) {
        ssize_t result = read(fd, buffer, buffer_size);
        if (result != (ssize_t)buffer_size) {
            if (result == -1) {
                perror("Ошибка чтения");
            }
            break;
        }
        sum += (unsigned char)buffer[0];
    }
    
    close(fd);
    free(buffer);
    
    printf("Файл %s обработан (checksum: %llu)\n", filename, sum);
    return 0;
}

void cleanup_files() {
    for (int file_num = 0; file_num < NUM_FILES; file_num++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "workload_file_%d.bin", file_num);
        if (unlink(filename) == -1) {
            perror("Ошибка удаления файла");
        }
    }
}

void generate_workload() {
    printf("Генерация дискового I/O workload...\n");
    printf("Создание %d файлов по %d MB каждый\n", NUM_FILES, FILE_SIZE / (1024*1024));
    
    if (check_disk_space() != 0) {
        return;
    }
    
    for (int file_num = 0; file_num < NUM_FILES; file_num++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "workload_file_%d.bin", file_num);
        
        if (write_file(filename, file_num) != 0) {
            continue;
        }
        
        if (read_file(filename, file_num) != 0) {
            continue;
        }
    }
    
    cleanup_files();
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
