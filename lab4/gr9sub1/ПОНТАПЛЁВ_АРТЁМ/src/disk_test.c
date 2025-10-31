#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FILE_SIZE_MB 300
#define BUFFER_SIZE (32 * 1024)
#define OPERATIONS 3000
#define DURATION_SECONDS 20

void generate_random_data(char *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }
}

void intensive_write(const char *filename, char *buffer, size_t buffer_size, size_t file_size) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Ошибка открытия файла для записи");
        exit(1);
    }
    size_t total_written = 0;
    size_t blocks = file_size / buffer_size;
    int sync_counter = 0;
    printf("Интенсивная запись %s (%zu MB)...\n", filename, file_size / (1024 * 1024));
    for (size_t i = 0; i < blocks; i++) {
        generate_random_data(buffer, buffer_size);
        ssize_t written = write(fd, buffer, buffer_size);
        if ((size_t)written != buffer_size) {
            printf("Ошибка записи: записано %zd из %zu байт\n", written, buffer_size);
            break;
        }
        total_written += written;
        sync_counter++;
        if (sync_counter % 10 == 0) {
            fsync(fd);
        }
        if (i % 100 == 0) {
            printf("Записано: %.2f MB\r", (double)total_written / (1024 * 1024));
            fflush(stdout);
        }
    }
    fsync(fd);
    close(fd);
    printf("\nЗапись завершена: %zu MB\n", total_written / (1024 * 1024));
}

void intensive_read(const char *filename, char *buffer, size_t buffer_size, size_t file_size) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла для чтения");
        exit(1);
    }
    size_t total_read = 0;
    size_t blocks = file_size / buffer_size;
    printf("Интенсивное чтение %s...\n", filename);
    for (size_t i = 0; i < blocks; i++) {
        ssize_t read_bytes = read(fd, buffer, buffer_size);
        if ((size_t)read_bytes != buffer_size) {
            printf("Ошибка чтения: прочитано %zd из %zu байт\n", read_bytes, buffer_size);
            break;
        }
        total_read += read_bytes;
        if (i % 100 == 0) {
            printf("Прочитано: %.2f MB\r", (double)total_read / (1024 * 1024));
            fflush(stdout);
        }
    }
    close(fd);
    printf("\nЧтение завершено: %zu MB\n", total_read / (1024 * 1024));
}

void mixed_workload(const char *filename, char *buffer, size_t buffer_size, size_t file_size) {
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Ошибка открытия файла для смешанной нагрузки");
        exit(1);
    }
    printf("Смешанная нагрузка %s...\n", filename);
    for (size_t i = 0; i < file_size / buffer_size; i++) {
        generate_random_data(buffer, buffer_size);
        ssize_t result = write(fd, buffer, buffer_size);
        if (result == -1) {
            perror("Ошибка записи при заполнении");
            break;
        }
    }
    fsync(fd);
    for (int i = 0; i < OPERATIONS; i++) {
        long position = (rand() % (file_size / buffer_size)) * buffer_size;
        if (lseek(fd, position, SEEK_SET) == -1) {
            perror("Ошибка позиционирования");
            break;
        }
        if (i % 3 == 0) {
            ssize_t result = read(fd, buffer, buffer_size);
            if (result == -1) {
                perror("Ошибка чтения");
            }
        } else {
            generate_random_data(buffer, buffer_size);
            ssize_t result = write(fd, buffer, buffer_size);
            if (result == -1) {
                perror("Ошибка записи");
            }
            if (i % 10 == 0) fsync(fd);
        }
        if (i % 200 == 0) {
            printf("Смешанных операций: %d/%d\r", i, OPERATIONS);
            fflush(stdout);
        }
    }
    fsync(fd);
    close(fd);
    printf("\nСмешанная нагрузка завершена: %d операций\n", OPERATIONS);
}

void create_many_small_files(char *buffer, size_t buffer_size, int num_files, size_t file_size) {
    printf("Создание %d маленьких файлов по %zu KB...\n", num_files, file_size / 1024);
    for (int i = 0; i < num_files; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "small_file_%d.dat", i);
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            for (size_t j = 0; j < file_size / buffer_size; j++) {
                generate_random_data(buffer, buffer_size);
                ssize_t result = write(fd, buffer, buffer_size);
                if (result == -1) {
                    perror("Ошибка записи маленького файла");
                    break;
                }
            }
            fsync(fd);
            close(fd);
            remove(filename);
        }
        if (i % 50 == 0) {
            printf("Создано файлов: %d/%d\r", i, num_files);
            fflush(stdout);
        }
    }
    printf("\nСоздание маленьких файлов завершено\n");
}

int main() {
    printf("=== ИНТЕНСИВНОЕ ТЕСТИРОВАНИЕ ДИСКА ДЛЯ МОНИТОРИНГА ===\n");
    printf("PID: %d\n", getpid());
    printf("Запустите мониторинг в отдельных терминалах:\n");
    printf("1. iostat -x 1 10\n");
    printf("2. pidstat -d 1 10\n");
    printf("3. sudo iotop -b -n 10\n");
    printf("4. cat /proc/%d/io\n", getpid());
    printf("\nНажмите Enter для начала тестирования");
    getchar();
    const char *main_file = "disk_stress_test.dat";
    size_t file_size = FILE_SIZE_MB * 1024 * 1024;
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Ошибка выделения памяти");
        return 1;
    }
    srand(time(NULL));
    struct timeval start_time, end_time;
    printf("\nТЕСТ 1: ИНТЕНСИВНАЯ ЗАПИСЬ\n");
    gettimeofday(&start_time, NULL);
    intensive_write(main_file, buffer, BUFFER_SIZE, file_size);
    gettimeofday(&end_time, NULL);
    double write_time = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("Скорость записи: %.2f MB/s\n", FILE_SIZE_MB / write_time);
    printf("\nТЕСТ 2: ИНТЕНСИВНОЕ ЧТЕНИЕ\n");
    gettimeofday(&start_time, NULL);
    intensive_read(main_file, buffer, BUFFER_SIZE, file_size);
    gettimeofday(&end_time, NULL);
    double read_time = (end_time.tv_sec - start_time.tv_sec) + 
                      (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("Скорость чтения: %.2f MB/s\n", FILE_SIZE_MB / read_time);
    printf("\nТЕСТ 3: МНОЖЕСТВО МАЛЕНЬКИХ ФАЙЛОВ\n");
    gettimeofday(&start_time, NULL);
    create_many_small_files(buffer, BUFFER_SIZE, 200, 1024 * 1024);
    gettimeofday(&end_time, NULL);
    double small_files_time = (end_time.tv_sec - start_time.tv_sec) + 
                             (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("Файлов в секунду: %.2f\n", 200 / small_files_time);
    printf("\nТЕСТ 4: СМЕШАННАЯ НАГРУЗКА\n");
    gettimeofday(&start_time, NULL);
    mixed_workload("mixed_workload.dat", buffer, BUFFER_SIZE, 150 * 1024 * 1024);
    gettimeofday(&end_time, NULL);
    double mixed_time = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("Операций в секунду: %.2f\n", OPERATIONS / mixed_time);
    printf("\nТЕСТ 5: ДЛИТЕЛЬНАЯ НАГРУЗКА (%d сек)\n", DURATION_SECONDS);
    gettimeofday(&start_time, NULL);
    int iterations = 0;
    while (1) {
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                        (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        if (elapsed >= DURATION_SECONDS) break;
        switch (iterations % 3) {
            case 0:
                intensive_write("temp_stress.dat", buffer, BUFFER_SIZE, 50 * 1024 * 1024);
                remove("temp_stress.dat");
                break;
            case 1:
                create_many_small_files(buffer, 8192, 50, 512 * 1024);
                break;
            case 2:
                mixed_workload("temp_mixed.dat", buffer, BUFFER_SIZE, 80 * 1024 * 1024);
                remove("temp_mixed.dat");
                break;
        }
        iterations++;
        printf("Итерация длительной нагрузки: %d\r", iterations);
        fflush(stdout);
    }
    printf("\nДлительная нагрузка завершена. Итераций: %d\n", iterations);
    free(buffer);
    remove(main_file);
    remove("mixed_workload.dat");
    printf("\nТЕСТИРОВАНИЕ ЗАВЕРШЕНО\n");
    return 0;
}
