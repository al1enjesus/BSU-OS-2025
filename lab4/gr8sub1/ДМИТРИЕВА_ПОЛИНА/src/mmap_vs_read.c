#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET   "\x1b[0m"

// Создание тестового файла
void create_test_file(const char *filename, size_t size_mb) {
    printf(COLOR_BLUE "Создание тестового файла '%s' (%zu MB)...\n" COLOR_RESET, 
           filename, size_mb);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return;
    }
    
    size_t total_bytes = size_mb * 1024 * 1024;
    char buffer[4096];
    memset(buffer, 'X', sizeof(buffer));
    
    size_t written = 0;
    while (written < total_bytes) {
        size_t to_write = (total_bytes - written < sizeof(buffer)) ? 
                         (total_bytes - written) : sizeof(buffer);
        ssize_t result = write(fd, buffer, to_write);
        if (result == -1) {
            perror("Ошибка записи");
            break;
        }
        written += result;
    }
    
    close(fd);
    printf(COLOR_GREEN "Файл создан: %zu байт\n" COLOR_RESET, written);
}

// Метод 1: Чтение через read()
double read_with_syscalls(const char *filename) {
    printf(COLOR_BLUE "\n--- Метод 1: read() ---\n" COLOR_RESET);
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return -1;
    }
    
    struct stat sb;
    fstat(fd, &sb);
    printf("Размер файла: %ld байт (%.2f MB)\n", 
           sb.st_size, sb.st_size / (1024.0 * 1024.0));
    
    clock_t start = clock();
    
    char buffer[4096];
    ssize_t bytes_read;
    unsigned long long sum = 0;
    size_t total_read = 0;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
        total_read += bytes_read;
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Время: " COLOR_GREEN "%.3f секунд\n" COLOR_RESET, elapsed);
    printf("Прочитано: %zu байт\n", total_read);
    printf("Контрольная сумма: %llu\n", sum);
    
    close(fd);
    return elapsed;
}

// Метод 2: Чтение через mmap()
double read_with_mmap(const char *filename) {
    printf(COLOR_BLUE "\n--- Метод 2: mmap() ---\n" COLOR_RESET);
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return -1;
    }
    
    struct stat sb;
    fstat(fd, &sb);
    printf("Размер файла: %ld байт (%.2f MB)\n", 
           sb.st_size, sb.st_size / (1024.0 * 1024.0));
    
    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Ошибка mmap");
        close(fd);
        return -1;
    }
    
    clock_t start = clock();
    
    unsigned long long sum = 0;
    char *bytes = (char *)data;
    for (off_t i = 0; i < sb.st_size; i++) {
        sum += (unsigned char)bytes[i];
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Время: " COLOR_GREEN "%.3f секунд\n" COLOR_RESET, elapsed);
    printf("Контрольная сумма: %llu\n", sum);
    
    munmap(data, sb.st_size);
    close(fd);
    return elapsed;
}

int main() {
    printf(COLOR_GREEN "\n=== Сравнение mmap() vs read() ===\n" COLOR_RESET);
    printf("Дмитриева Полина, gr8sub1\n\n");
    
    const char *filename = "test_comparison.bin";
    size_t file_size_mb = 10; // 10 MB для теста
    
    // Создаем тестовый файл
    create_test_file(filename, file_size_mb);
    printf("\n");
    
    // Метод 1: read()
    double time_read = read_with_syscalls(filename);
    
    printf("\n" COLOR_BLUE "--- Пауза между тестами ---\n" COLOR_RESET);
    sleep(2);
    
    // Метод 2: mmap()
    double time_mmap = read_with_mmap(filename);
    
    // Сравнение результатов
    printf(COLOR_GREEN "\n=== Результаты сравнения ===\n" COLOR_RESET);
    printf("read():  %.3f сек\n", time_read);
    printf("mmap():  %.3f сек\n", time_mmap);
    
    if (time_mmap > 0 && time_read > 0) {
        if (time_mmap < time_read) {
            double faster = (time_read - time_mmap) / time_read * 100;
            printf(COLOR_GREEN "mmap() быстрее на %.1f%%\n" COLOR_RESET, faster);
        } else {
            double faster = (time_mmap - time_read) / time_mmap * 100;
            printf(COLOR_GREEN "read() быстрее на %.1f%%\n" COLOR_RESET, faster);
        }
    }
    
    // Удаляем тестовый файл
    unlink(filename);
    printf(COLOR_BLUE "\nТестовый файл удален\n" COLOR_RESET);
    
    return 0;
}
