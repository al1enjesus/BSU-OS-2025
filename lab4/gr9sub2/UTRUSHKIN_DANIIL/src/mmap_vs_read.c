#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define FILE_SIZE (10 * 1024 * 1024)  // 10 MB (меньше для теста)

// Создание тестового файла
void create_test_file(const char* filename) {
    printf("Создаем тестовый файл: %s (%d MB)\n", filename, FILE_SIZE / (1024 * 1024));
    
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    // Заполняем файл данными
    char* buffer = malloc(1024 * 1024);  // 1 MB буфер
    for (int i = 0; i < 1024 * 1024; i++) {
        buffer[i] = (i % 256);  // Простые данные
    }
    
    for (int i = 0; i < 10; i++) {
        if (write(fd, buffer, 1024 * 1024) != 1024 * 1024) {
            perror("write");
            break;
        }
    }
    
    free(buffer);
    close(fd);
    printf("Тестовый файл создан успешно\n");
}

// Подсчет суммы байтов через read()
unsigned long long read_with_syscalls(const char* filename) {
    printf("Чтение через syscalls (read())...\n");
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 0;
    }
    
    unsigned long long sum = 0;
    char buffer[4096];  // 4 KB буфер
    ssize_t bytes_read;
    
    clock_t start = clock();
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    close(fd);
    
    printf("Время с read(): %.3f секунд\n", time_spent);
    printf("Сумма байтов: %llu\n", sum);
    
    return sum;
}

// Подсчет суммы байтов через mmap()
unsigned long long read_with_mmap(const char* filename) {
    printf("Чтение через mmap()...\n");
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 0;
    }
    
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return 0;
    }
    
    clock_t start = clock();
    
    char* data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 0;
    }
    
    unsigned long long sum = 0;
    for (off_t i = 0; i < sb.st_size; i++) {
        sum += (unsigned char)data[i];
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    munmap(data, sb.st_size);
    close(fd);
    
    printf("Время с mmap(): %.3f секунд\n", time_spent);
    printf("Сумма байтов: %llu\n", sum);
    
    return sum;
}

int main() {
    const char* filename = "testfile.bin";
    
    printf("=== Сравнение mmap() и read() ===\n");
    
    // Создаем тестовый файл
    create_test_file(filename);
    
    printf("\n=== Первый запуск (холодный кеш) ===\n");
    
    // Тестируем read()
    printf("\n--- Тестируем read() ---\n");
    unsigned long long sum1 = read_with_syscalls(filename);
    
    // Даем системе время
    sleep(1);
    
    // Тестируем mmap()
    printf("\n--- Тестируем mmap() ---\n");
    unsigned long long sum2 = read_with_mmap(filename);
    
    printf("\n=== Второй запуск (теплый кеш) ===\n");
    
    // Повторяем тесты (данные уже в кеше)
    printf("\n--- Тестируем read() (теплый кеш) ---\n");
    sum1 = read_with_syscalls(filename);
    
    printf("\n--- Тестируем mmap() (теплый кеш) ---\n");
    sum2 = read_with_mmap(filename);
    
    // Проверяем, что суммы совпадают
    printf("\n=== Проверка ===\n");
    printf("Суммы совпадают: %s\n", (sum1 == sum2) ? "ДА" : "НЕТ");
    
    // Удаляем временный файл
    unlink(filename);
    
    return 0;
}
