#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

void wait_for_enter(const char* message) {
    printf("\n%s", message);
    printf("Нажмите Enter для продолжения...");
    getchar();
}

void create_disk_load() {
    printf("Генерация дисковой нагрузки...\n\n");
    
    printf("Подготовка к тесту - запустите мониторинг в других терминалах:\n");
    printf("1. iostat -x 1 10\n");
    printf("2. pidstat -d 1 10\n"); 
    printf("3. sudo iotop -b -n 10\n");
    printf("4. watch -n 1 'cat /proc/%d/io'\n\n", getpid());
    
    wait_for_enter("");
    
    // Создаем директорию для тестовых файлов
    int result = system("mkdir -p test_load");
    if (result == -1) {
        perror("Ошибка создания директории");
        return;
    }
    
    const int FILE_COUNT = 5;
    const size_t FILE_SIZE = 50 * 1024 * 1024; // Увеличил до 50MB для более долгой нагрузки
    
    printf("Создаю %d файлов по %zu MB каждый...\n", FILE_COUNT, FILE_SIZE / (1024 * 1024));
    printf("PID процесса: %d\n\n", getpid());
    
    for (int i = 0; i < FILE_COUNT; i++) {
        char filename[100];
        sprintf(filename, "test_load/file_%d.bin", i);
        
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Ошибка открытия файла");
            continue;
        }
        
        printf("Создаю файл: %s\n", filename);
        
        // Выделяем буфер для записи
        size_t buffer_size = 65536; // Увеличил буфер до 64KB для более медленной записи
        char* buffer = malloc(buffer_size);
        if (!buffer) {
            perror("Ошибка выделения памяти");
            close(fd);
            continue;
        }
        
        // Заполняем буфер случайными данными
        for (size_t j = 0; j < buffer_size; j++) {
            buffer[j] = rand() % 256;
        }
        
        // Записываем данные с паузами для мониторинга
        for (size_t written = 0; written < FILE_SIZE; written += buffer_size) {
            ssize_t bytes_written = write(fd, buffer, buffer_size);
            if (bytes_written == -1) {
                perror("Ошибка записи");
                break;
            }
            
            // Пауза каждые 5MB для возможности мониторинга
            if (written % (5 * 1024 * 1024) == 0 && written > 0) {
                printf("  Записано: %zu/%zu MB\n", written / (1024 * 1024), FILE_SIZE / (1024 * 1024));
                sleep(1); // Пауза 1 секунда
            }
        }
        
        free(buffer);
        close(fd);
        printf("Файл создан: %s\n\n", filename);
        
        // Пауза между файлами
        if (i < FILE_COUNT - 1) {
            printf("Пауза 3 секунды перед созданием следующего файла...\n");
            sleep(3);
        }
    }
    
    printf("\nНагрузка завершена!\n");
}

void analyze_io_scheduler() {
    printf("\nАнализ планировщика I/O:\n");
    
    printf("Текущий планировщик: ");
    fflush(stdout);
    
    FILE *f = popen("cat /sys/block/sda/queue/scheduler 2>/dev/null || echo 'Не удалось определить'", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            printf("%s", line);
        }
        pclose(f);
    }
    
    printf("\nДля смены планировщика (требует root):\n");
    printf("sudo bash -c 'echo \"mq-deadline\" > /sys/block/sda/queue/scheduler'\n");
    printf("sudo bash -c 'echo \"bfq\" > /sys/block/sda/queue/scheduler'\n");
    printf("sudo bash -c 'echo \"none\" > /sys/block/sda/queue/scheduler'\n");
}

void cleanup_test_files() {
    printf("\nОчистка тестовых файлов...\n");
    
    // Удаляем отдельные файлы
    for (int i = 0; i < 5; i++) {
        char filename[100];
        sprintf(filename, "test_load/file_%d.bin", i);
        remove(filename);
    }
    
    // Удаляем директорию
    rmdir("test_load");
    printf("Очистка завершена!\n");
}

int main() {
    printf("=========================================\n");
    printf("       ТЕСТ ДИСКОВОЙ НАГРУЗКИ\n");
    printf("=========================================\n");
    
    create_disk_load();
    
    wait_for_enter("\nНагрузка завершена. Проанализируйте данные мониторинга.\n");
    
    analyze_io_scheduler();
    
    wait_for_enter("\nНажмите Enter для очистки тестовых файлов...");
    
    cleanup_test_files();
    
    return 0;
}
