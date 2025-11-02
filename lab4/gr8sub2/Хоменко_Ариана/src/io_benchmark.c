#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

// Константы для работы с размерами файлов
#define DEFAULT_SIZE_MB 100          // Размер тестового файла по умолчанию
#define PAGE_SIZE 4096               // Размер страницы памяти
#define BYTES_IN_MB (1024*1024)      // Байт в мегабайте (целое)
#define BYTES_IN_MB_FLOAT (1024.0*1024.0) // Байт в мегабайте (дробное)

// Массив различных размеров буфера для тестирования
size_t buffer_sizes[] = {
    512,           // Очень маленький буфер - много системных вызовов
    1024,          // 1 KB
    4096,          // 4 KB (размер страницы)
    8192,          // 8 KB
    16384,         // 16 KB
    65536,         // 64 KB (оптимальный для многих задач)
    BYTES_IN_MB,   // 1 MB
    4*BYTES_IN_MB  // 4 MB (очень большой буфер)
};
// Количество тестируемых размеров буфера
int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);

/*
 * Функция для получения текущего времени с высокой точностью
 * Использует clock_gettime() с монотонными часами
 */
double get_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

/*
 * Бенчмарк для записи через fwrite() - буферизованный ввод-вывод
 * fwrite использует буферизацию на уровне стандартной библиотеки C
 */
double benchmark_fwrite(const char *filename, size_t size, size_t buffer_size, long long *libcall_count_out) {
    printf("\n=== fwrite() with buffer=%zu bytes ===\n", buffer_size);
    *libcall_count_out = 0;

    // Открываем файл для записи через стандартную библиотеку
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen failed");
        return -1;
    }

    // Выделяем буфер для записи данных
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return -1;
    }
    // Заполняем буфер тестовыми данными
    memset(buffer, 'A', buffer_size);

    double start = get_time();
    size_t total_written = 0;

    // Цикл записи данных в файл порциями
    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        size_t written_now = fwrite(buffer, 1, to_write, f);
        (*libcall_count_out)++;  // Считаем каждый вызов fwrite
        
        if (written_now != to_write) {
            perror("fwrite failed");
            break;
        }
        total_written += written_now;
    }

    // Принудительно сбрасываем буферы на диск
    fflush(f);
    (*libcall_count_out)++;  // Учитываем вызов fflush
    
    double end = get_time();
    double elapsed = end - start;
    
    // Проверяем корректность записи
    if (total_written != size) {
         printf("Error: Wrote %zu bytes instead of %zu\n", total_written, size);
    }

    // Выводим результаты теста
    printf("Time: %.3f seconds\n", elapsed);
    printf("Syscalls (fwrite + fflush): %lld\n", *libcall_count_out);
    printf("Throughput: %.2f MB/s\n", (total_written / (BYTES_IN_MB_FLOAT)) / elapsed);

    // Освобождаем ресурсы
    free(buffer);
    fclose(f);

    return elapsed;
}

/*
 * Бенчмарк для записи через write() - небуферизованный ввод-вывод
 * write использует прямые системные вызовы без буферизации
 */
double benchmark_write(const char *filename, size_t size, size_t buffer_size, long long *syscall_count_out) {
    printf("\n=== write() with buffer=%zu bytes ===\n", buffer_size);
    *syscall_count_out = 0;

    // Открываем файл через системный вызов
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }
    (*syscall_count_out)++;  // Учитываем вызов open

    // Выделяем буфер для записи
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc failed");
        close(fd);
        return -1;
    }
    memset(buffer, 'B', buffer_size);

    double start = get_time();
    size_t total_written = 0;

    // Цикл записи через системные вызовы
    for (size_t written = 0; written < size; written += buffer_size) {
        size_t to_write = (size - written < buffer_size) ? (size - written) : buffer_size;
        ssize_t written_now = write(fd, buffer, to_write);
        (*syscall_count_out)++;  // Считаем каждый вызов write

        if (written_now != (ssize_t)to_write) {
            perror("write failed");
            break;
        }
        total_written += written_now;
    }

    double end = get_time();
    double elapsed = end - start;

    if (total_written != size) {
         printf("Error: Wrote %zu bytes instead of %zu\n", total_written, size);
    }

    printf("Time: %.3f seconds\n", elapsed);
    
    // Закрываем файл
    close(fd);
    (*syscall_count_out)++;  // Учитываем вызов close
    
    printf("Syscalls (open + N*write + close): %lld\n", *syscall_count_out);
    printf("Throughput: %.2f MB/s\n", (total_written / (BYTES_IN_MB_FLOAT)) / elapsed);

    free(buffer);
    return elapsed;
}

/*
 * Бенчмарк для чтения через fread() - буферизованное чтение
 */
double benchmark_fread(const char *filename, size_t file_size, size_t buffer_size, long long *libcall_count_out) {
    printf("\n--- fread() (buffer=%zu) ---\n", buffer_size);
    *libcall_count_out = 0;
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen read");
        return -1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc");
        fclose(f);
        return -1;
    }

    double start = get_time();
    size_t bytes_read;
    // Читаем файл порциями до конца
    while ((bytes_read = fread(buffer, 1, buffer_size, f)) > 0) {
        (*libcall_count_out)++;
    }
    double end = get_time();
    double elapsed = end - start;
    
    printf("Time: %.3f seconds\n", elapsed);
    printf("Syscalls (N*fread): %lld\n", *libcall_count_out);
    printf("Throughput: %.2f MB/s\n", (file_size / (BYTES_IN_MB_FLOAT)) / elapsed);
    
    free(buffer);
    fclose(f);
    return elapsed;
}

/*
 * Бенчмарк для чтения через read() - небуферизованное чтение
 */
double benchmark_read(const char *filename, size_t file_size, size_t buffer_size, long long *syscall_count_out) {
    printf("\n--- read() (buffer=%zu) ---\n", buffer_size);
    *syscall_count_out = 0;
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open read");
        return -1;
    }
    (*syscall_count_out)++;  // Учитываем вызов open

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("malloc");
        close(fd);
        return -1;
    }

    double start = get_time();
    ssize_t bytes_read;
    // Читаем файл порциями через системные вызовы
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        (*syscall_count_out)++;  // Считаем каждый вызов read
    }
    double end = get_time();
    double elapsed = end - start;
    
    printf("Time: %.3f seconds\n", elapsed);
    
    close(fd);
    (*syscall_count_out)++;  // Учитываем вызов close

    printf("Syscalls (open + N*read + close): %lld\n", *syscall_count_out);
    printf("Throughput: %.2f MB/s\n", (file_size / (BYTES_IN_MB_FLOAT)) / elapsed);
    
    free(buffer);
    return elapsed;
}

/*
 * Тестирование влияния размера буфера на производительность записи
 * Запускает benchmark_write с разными размерами буфера
 */
void benchmark_buffer_sizes(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: Write Buffer Size Impact\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * BYTES_IN_MB;

    printf("\nTesting write() with different buffer sizes:\n");
    printf("File size: %zu MB\n", file_size_mb);
    
    long long syscalls = 0;

    // Тестируем все размеры буфера из массива buffer_sizes
    for (int i = 0; i < num_sizes; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test_buffer_%zu.bin", buffer_sizes[i]);

        benchmark_write(filename, file_size, buffer_sizes[i], &syscalls);
        unlink(filename);  // Удаляем временный файл после теста
        sleep(1);         // Пауза между тестами
    }
}

/*
 * Сравнение основных методов записи: fwrite vs write
 * Использует оптимальный размер буфера 64 KB
 */
void benchmark_all_methods(size_t file_size_mb) {
    printf("\n========================================\n");
    printf("Benchmark: I/O Write Methods Comparison\n");
    printf("========================================\n");

    size_t file_size = file_size_mb * BYTES_IN_MB;
    size_t optimal_buffer = 64 * 1024;  // 64 KB - оптимальный размер
    long long calls_count = 0;

    printf("\nFile size: %zu MB\n", file_size_mb);
    printf("Buffer size: %zu KB (for fwrite/write)\n", optimal_buffer / 1024);

    // Метод 1: Буферизованная запись через fwrite
    benchmark_fwrite("test_fwrite.bin", file_size, optimal_buffer, &calls_count);
    unlink("test_fwrite.bin");
    sleep(1);

    // Метод 2: Небуферизованная запись через write
    benchmark_write("test_write.bin", file_size, optimal_buffer, &calls_count);
    unlink("test_write.bin");
    sleep(1);
}

/*
 * Сравнение методов чтения: fread vs read
 */
void benchmark_read_methods(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Reading Methods Comparison\n");
    printf("========================================\n");

    // Получаем информацию о файле
    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = sb.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }
    
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", file_size / (BYTES_IN_MB_FLOAT));
    
    size_t optimal_buffer = 65536; // 64K
    long long calls_count = 0;

    // Сравниваем оба метода чтения
    benchmark_fread(filename, file_size, optimal_buffer, &calls_count);
    benchmark_read(filename, file_size, optimal_buffer, &calls_count);
}

/*
 * Тестирование влияния размера буфера на производительность чтения
 */
void benchmark_read_buffer_sizes(const char *filename) {
    printf("\n========================================\n");
    printf("Benchmark: Read Buffer Size Impact\n");
    printf("========================================\n");

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat failed");
        return;
    }
    size_t file_size = sb.st_size;
    
    if (file_size == 0) {
        printf("Error: Test file is empty!\n");
        return;
    }

    printf("\nTesting read() with different buffer sizes:\n");
    printf("File size: %.2f MB\n", file_size / (BYTES_IN_MB_FLOAT));
    
    long long syscalls = 0;

    // Тестируем все размеры буфера для чтения
    for (int i = 0; i < num_sizes; i++) {
        benchmark_read(filename, file_size, buffer_sizes[i], &syscalls);
    }
}

/*
 * Главная функция программы
 * Парсит аргументы командной строки и запускает бенчмарки
 */
int main(int argc, char *argv[]) {
    size_t size_mb = DEFAULT_SIZE_MB;

    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size_mb = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--size SIZE_MB]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --size SIZE_MB   Size of test file in megabytes (default: %d)\n", DEFAULT_SIZE_MB);
            return 0;
        }
    }

    if (size_mb == 0) {
        fprintf(stderr, "Invalid size: %zu MB\n", size_mb);
        return 1;
    }

    printf("I/O Benchmark\n");
    printf("=============\n");
    printf("Test file size: %zu MB\n", size_mb);

    // Очистка кеша страниц для чистоты эксперимента
    printf("\nAttempting to sync filesystem...\n");
    int ret;
    ret = system("sync");
    if(ret != 0) {
      perror("sync");
      return 1;
    }
    ret = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if(ret != 0) {
      perror("drop_caches");
      return 1;
    }

    // Запуск основных бенчмарков записи
    benchmark_all_methods(size_mb);
    benchmark_buffer_sizes(size_mb);
    
    // Создание тестового файла для бенчмарков чтения
    const char *read_test_file = "read_test_file.bin";
    printf("\nCreating test file '%s' for reading benchmark...\n", read_test_file);
    long long temp_calls;
    benchmark_fwrite(read_test_file, size_mb * BYTES_IN_MB, 65536, &temp_calls);
    
    // Повторная очистка кеша перед тестами чтения
    printf("\nAttempting to sync filesystem...\n");
    ret = system("sync");
    if(ret != 0) {
      perror("sync");
      return 1;
    }
    ret = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if(ret != 0) {
      perror("drop_caches");
      return 1;
    }

    // Запуск бенчмарков чтения
    benchmark_read_methods(read_test_file);
    benchmark_read_buffer_sizes(read_test_file);
    
    // Удаление временного файла
    unlink(read_test_file);

    printf("\n========================================\n");
    printf("Benchmark completed!\n");
    printf("========================================\n");

    return 0;
}
