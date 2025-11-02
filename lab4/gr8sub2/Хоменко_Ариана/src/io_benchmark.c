#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

// Константы для работы с размерами
#define DEFAULT_SIZE_MB 100
#define PAGE_SIZE 4096
#define BYTES_IN_MB (1024*1024)
#define BYTES_IN_MB_FLOAT (1024.0*1024.0)

// Различные размеры буферов для тестирования
size_t buffer_sizes[] = {
    512,       // Очень маленький буфер
    1024,      // 1 KB
    4096,      // 4 KB (размер страницы)
    8192,      // 8 KB
    16384,     // 16 KB
    65536,     // 64 KB (оптимальный для многих задач)
    BYTES_IN_MB,       // 1 MB
    4*BYTES_IN_MB      // 4 MB (очень большой буфер)
};
int num_buffer_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

// Функция для получения текущего времени с высокой точностью
double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Тестирование записи через fwrite() (буферизованный ввод-вывод)
double benchmark_fwrite(const char *filename, size_t total_size, size_t buffer_size, long long *function_call_count) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);
    *function_call_count = 0;

    // Открываем файл для записи
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen failed");
        return -1;
    }

    // Выделяем буфер для записи
    char *write_buffer = malloc(buffer_size);
    if (!write_buffer) {
        perror("malloc failed");
        fclose(file);
        return -1;
    }
    memset(write_buffer, 'A', buffer_size);  // Заполняем буфер тестовыми данными

    double start_time = get_current_time();
    size_t total_bytes_written = 0;

    // Цикл записи данных в файл
    for (size_t bytes_written = 0; bytes_written < total_size; bytes_written += buffer_size) {
        size_t bytes_to_write = (total_size - bytes_written < buffer_size) ? 
                               (total_size - bytes_written) : buffer_size;
        size_t bytes_written_now = fwrite(write_buffer, 1, bytes_to_write, file);
        (*function_call_count)++;  // Считаем вызовы fwrite
        
        if (bytes_written_now != bytes_to_write) {
            perror("fwrite failed");
            break;
        }
        total_bytes_written += bytes_written_now;
    }

    // Сбрасываем буферы на диск
    fflush(file);
    (*function_call_count)++;  // Учитываем вызов fflush
    
    double end_time = get_current_time();
    double elapsed_time = end_time - start_time;
    
    // Проверяем, что записано правильное количество данных
    if (total_bytes_written != total_size) {
         printf("Warning: Wrote %zu bytes instead of %zu\n", total_bytes_written, total_size);
    }

    // Выводим результаты
    printf("Time: %.3f seconds\n", elapsed_time);
    printf("Function calls (fwrite + fflush): %lld\n", *function_call_count);
    printf("Throughput: %.2f MB/s\n", (total_bytes_written / BYTES_IN_MB_FLOAT) / elapsed_time);

    // Освобождаем ресурсы
    free(write_buffer);
    fclose(file);

    return elapsed_time;
}

// Тестирование записи через write() (системные вызовы)
double benchmark_write(const char *filename, size_t total_size, size_t buffer_size, long long *syscall_count) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);
    *syscall_count = 0;

    // Открываем файл через системный вызов
    int file_descriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_descriptor == -1) {
        perror("open failed");
        return -1;
    }
    (*syscall_count)++;  // Учитываем вызов open

    // Выделяем буфер для записи
    char *write_buffer = malloc(buffer_size);
    if (!write_buffer) {
        perror("malloc failed");
        close(file_descriptor);
        return -1;
    }
    memset(write_buffer, 'B', buffer_size);

    double start_time = get_current_time();
    size_t total_bytes_written = 0;

    // Цикл записи через системные вызовы
    for (size_t bytes_written = 0; bytes_written < total_size; bytes_written += buffer_size) {
        size_t bytes_to_write = (total_size - bytes_written < buffer_size) ? 
                               (total_size - bytes_written) : buffer_size;
        ssize_t bytes_written_now = write(file_descriptor, write_buffer, bytes_to_write);
        (*syscall_count)++;  // Считаем каждый вызов write

        if (bytes_written_now != (ssize_t)bytes_to_write) {
            perror("write failed");
            break;
        }
        total_bytes_written += bytes_written_now;
    }

    double end_time = get_current_time();
    double elapsed_time = end_time - start_time;

    // Проверяем корректность записи
    if (total_bytes_written != total_size) {
         printf("Warning: Wrote %zu bytes instead of %zu\n", total_bytes_written, total_size);
    }

    printf("Time: %.3f seconds\n", elapsed_time);
    
    // Закрываем файл
    close(file_descriptor);
    (*syscall_count)++;  // Учитываем вызов close
    
    printf("System calls (open + N*write + close): %lld\n", *syscall_count);
    printf("Throughput: %.2f MB/s\n", (total_bytes_written / BYTES_IN_MB_FLOAT) / elapsed_time);

    free(write_buffer);
    return elapsed_time;
}

// Тестирование чтения через fread() (буферизованное чтение)
double benchmark_fread(const char *filename, size_t file_size, size_t buffer_size, long long *function_call_count) {
    printf("\n--- fread() (buffer=%zu bytes) ---\n", buffer_size);
    *function_call_count = 0;
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen for reading failed");
        return -1;
    }

    char *read_buffer = malloc(buffer_size);
    if (!read_buffer) {
        perror("malloc failed");
        fclose(file);
        return -1;
    }

    double start_time = get_current_time();
    size_t bytes_read;
    
    // Читаем файл порциями через fread
    while ((bytes_read = fread(read_buffer, 1, buffer_size, file)) > 0) {
        (*function_call_count)++;  // Считаем вызовы fread
    }
    
    double end_time = get_current_time();
    double elapsed_time = end_time - start_time;
    
    printf("Time: %.3f seconds\n", elapsed_time);
    printf("Function calls (N*fread): %lld\n", *function_call_count);
    printf("Throughput: %.2f MB/s\n", (file_size / BYTES_IN_MB_FLOAT) / elapsed_time);
    
    free(read_buffer);
    fclose(file);
    return elapsed_time;
}

// Тестирование чтения через read() (системные вызовы)
double benchmark_read(const char *filename, size_t file_size, size_t buffer_size, long long *syscall_count) {
    printf("\n--- read() (buffer=%zu bytes) ---\n", buffer_size);
    *syscall_count = 0;
    
    int file_descriptor = open(filename, O_RDONLY);
    if (file_descriptor == -1) {
        perror("open for reading failed");
        return -1;
    }
    (*syscall_count)++;  // Учитываем вызов open

    char *read_buffer = malloc(buffer_size);
    if (!read_buffer) {
        perror("malloc failed");
        close(file_descriptor);
        return -1;
    }

    double start_time = get_current_time();
    ssize_t bytes_read;
    
    // Читаем файл порциями через read
    while ((bytes_read = read(file_descriptor, read_buffer, buffer_size)) > 0) {
        (*syscall_count)++;  // Считаем каждый вызов read
    }
    
    double end_time = get_current_time();
    double elapsed_time = end_time - start_time;
    
    printf("Time: %.3f seconds\n", elapsed_time);
    
    close(file_descriptor);
    (*syscall_count)++;  // Учитываем вызов close

    printf("System calls (open + N*read + close): %lld\n", *syscall_count);
    printf("Throughput: %.2f MB/s\n", (file_size / BYTES_IN_MB_FLOAT) / elapsed_time);
    
    free(read_buffer);
    return elapsed_time;
}

// Бенчмарк влияния размера буфера на производительность записи
void benchmark_buffer_sizes_write(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Write Buffer Size Impact\n");
    printf("========================================\n");

    size_t total_file_size = file_size_mb * BYTES_IN_MB;

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);
    
    long long syscall_counter = 0;

    for (int i = 0; i < num_buffer_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        benchmark_write(filename, total_file_size, buffer_sizes[i], &syscall_counter);
        unlink(filename);  // Удаляем временный файл
        sleep(1);  // Пауза между тестами
    }
}

// Сравнение всех методов записи
void benchmark_all_write_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Write Methods Comparison\n");
    printf("========================================\n");

    size_t total_file_size = file_size_mb * BYTES_IN_MB;
    size_t optimal_buffer_size = 64 * 1024;  // 64 KB - оптимальный размер для многих задач
    long long call_counter = 0;

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer_size / 1024);

    // Метод 1: fwrite (буферизованная запись)
    benchmark_fwrite("test_fwrite.bin", total_file_size, optimal_buffer_size, &call_counter);
    unlink("test_fwrite.bin");
    sleep(1);

    // Метод 2: write (системные вызовы)
    benchmark_write("test_write.bin", total_file_size, optimal_buffer_size, &call_counter);
    unlink("test_write.bin");
    sleep(1);
}

// Сравнение методов чтения
void benchmark_read_methods_comparison(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods Comparison\n");
    printf("========================================\n");

    struct stat file_stat;
    if (stat(filename, &file_stat) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = file_stat.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }
    
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / BYTES_IN_MB_FLOAT);
    
    size_t optimal_buffer_size = 65536; // 64K
    long long call_counter = 0;

    benchmark_fread(filename, file_size, optimal_buffer_size, &call_counter);
    benchmark_read(filename, file_size, optimal_buffer_size, &call_counter);
}

// Бенчмарк влияния размера буфера на чтение
void benchmark_buffer_sizes_read(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Read Buffer Size Impact\n");
    printf("========================================\n");

    struct stat file_stat;
    if (stat(filename, &file_stat) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = file_stat.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }

    printf("\nTesting read() with different buffer sizes:\n");
    printf("File size: %.2f MB\n", file_size / BYTES_IN_MB_FLOAT);
    
    long long syscall_counter = 0;

    for (int i = 0; i < num_buffer_sizes; i++) {
        benchmark_read(filename, file_size, buffer_sizes[i], &syscall_counter);
    }
}

int main(int argc, char *argv[]) {
    size_t file_size_mb = DEFAULT_SIZE_MB;

    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            file_size_mb = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --size SIZE_MB   Size of test file in megabytes (default: %d)\n", DEFAULT_SIZE_MB);
            return 0;
        }
    }

    if (file_size_mb == 0) {
        fprintf(stderr, "Invalid size: %zu MB\n", file_size_mb);
        return 1;
    }

    printf("I/O Benchmark - Вариант 2\n");
    printf("==========================\n");
    printf("Test file size: %zu MB\n", file_size_mb);

    // Очистка кеша страниц для чистоты эксперимента
    printf("\nClearing page cache for accurate results...\n");
    system("sync");  // Синхронизируем файловые системы
    system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");  // Очищаем кеш

    // Запуск бенчмарков записи
    benchmark_all_write_methods(file_size_mb);
    benchmark_buffer_sizes_write(file_size_mb);
    
    // Создание тестового файла для бенчмарков чтения
    const char *read_test_filename = "read_test_file.bin";
    printf("\nCreating test file '%s' for reading benchmarks...\n", read_test_filename);
    long long temp_call_counter;
    benchmark_fwrite(read_test_filename, file_size_mb * BYTES_IN_MB, 65536, &temp_call_counter);
    
    // Повторная очистка кеша перед тестами чтения
    printf("\nClearing page cache before read benchmarks...\n");
    system("sync");
    system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");

    // Запуск бенчмарков чтения
    benchmark_read_methods_comparison(read_test_filename);
    benchmark_buffer_sizes_read(read_test_filename);
    
    // Удаление временного файла
    unlink(read_test_filename);

    printf("\n========================================\n");
    printf("Benchmark completed successfully!\n");
    printf("========================================\n");

    return 0;
}
