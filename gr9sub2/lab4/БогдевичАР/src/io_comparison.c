#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define FILE_SIZE (100 * 1024 * 1024) // 100 MB
#define FILENAME "testfile.bin"

// Создание тестового файла
void create_test_file() {
    printf("📁 Создаем тестовый файл 100 MB...\n");
    char command[256];
    snprintf(command, sizeof(command), 
             "dd if=/dev/urandom of=%s bs=1M count=100 status=progress 2>/dev/null", 
             FILENAME);
    int result = system(command);
    if (result != 0) {
        printf("❌ Ошибка создания файла\n");
        exit(1);
    }
    printf("✅ Файл создан\n");
}

// Чтение через системные вызовы
long long read_with_syscalls() {
    printf("\n🔹 Метод: read()/write()\n");
    
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("❌ Ошибка открытия файла");
        return -1;
    }
    
    char *buffer = malloc(4096);
    if (!buffer) {
        close(fd);
        return -1;
    }
    
    long long sum = 0;
    ssize_t bytes_read;
    
    clock_t start = clock();
    
    while ((bytes_read = read(fd, buffer, 4096)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Время: %.3f секунд\n", time_spent);
    printf("Сумма байтов: %lld\n", sum);
    
    free(buffer);
    close(fd);
    return sum;
}

// Чтение через mmap
long long read_with_mmap() {
    printf("\n🔹 Метод: mmap()\n");
    
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("❌ Ошибка открытия файла");
        return -1;
    }
    
    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("❌ Ошибка mmap");
        close(fd);
        return -1;
    }
    
    long long sum = 0;
    
    clock_t start = clock();
    
    // Читаем файл через mmap
    for (int i = 0; i < FILE_SIZE; i++) {
        sum += (unsigned char)mapped[i];
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Время: %.3f секунд\n", time_spent);
    printf("Сумма байтов: %lld\n", sum);
    
    munmap(mapped, FILE_SIZE);
    close(fd);
    return sum;
}

int main() {
    printf("🚀 Задание B: Сравнение I/O методов\n");
    
    create_test_file();
    
    printf("\n=== ТЕСТ 1: Системные вызовы ===\n");
    long long sum1 = read_with_syscalls();
    
    printf("\n=== ТЕСТ 2: Memory Mapping ===\n");
    long long sum2 = read_with_mmap();
    
    // Проверка корректности
    if (sum1 == sum2) {
        printf("\n✅ Результаты совпадают: оба метода дали одинаковую сумму\n");
    } else {
        printf("\n❌ Результаты не совпадают!\n");
    }
    
    // Удаление тестового файла
    remove(FILENAME);
    printf("\n✅ Задание B завершено!\n");
    
    return 0;
}
