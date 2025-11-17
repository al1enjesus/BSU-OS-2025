

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_FILE_COUNT 5
#define DEFAULT_FILE_SIZE_MB 2
#define DEFAULT_BUFFER_SIZE_KB 4

typedef struct {
    int file_count;
    size_t file_size_mb;
    size_t buffer_size;
    int show_monitor_commands;
} DiskStressConfig;

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  --count NUM          Number of files (default: %d)\n", DEFAULT_FILE_COUNT);
    printf("  --size SIZE_MB       File size in MB (default: %d)\n", DEFAULT_FILE_SIZE_MB);
    printf("  --buffer SIZE_KB     Buffer size in KB (default: %d)\n", DEFAULT_BUFFER_SIZE_KB);
    printf("  --no-monitor         Don't show monitor commands\n");
    printf("  --help               Show this help\n");
    printf("\nExamples:\n");
    printf("  %s                   # Default settings\n", program_name);
    printf("  %s --count 10        # Create 10 files\n", program_name);
    printf("  %s --size 10         # 10 MB files\n", program_name);
    printf("  %s --count 5 --size 20\n", program_name);
}

int parse_args(int argc, char *argv[], DiskStressConfig *config) {
    // Set defaults
    config->file_count = DEFAULT_FILE_COUNT;
    config->file_size_mb = DEFAULT_FILE_SIZE_MB;
    config->buffer_size = DEFAULT_BUFFER_SIZE_KB * 1024;
    config->show_monitor_commands = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            config->file_count = atoi(argv[i + 1]);
            if (config->file_count <= 0) {
                fprintf(stderr, "Error: Invalid file count '%s'\n", argv[i + 1]);
                return -1;
            }
            i++;
        } else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            config->file_size_mb = atoi(argv[i + 1]);
            if (config->file_size_mb <= 0) {
                fprintf(stderr, "Error: Invalid file size '%s'\n", argv[i + 1]);
                return -1;
            }
            i++;
        } else if (strcmp(argv[i], "--buffer") == 0 && i + 1 < argc) {
            size_t buffer_kb = atoi(argv[i + 1]);
            if (buffer_kb <= 0) {
                fprintf(stderr, "Error: Invalid buffer size '%s'\n", argv[i + 1]);
                return -1;
            }
            config->buffer_size = buffer_kb * 1024;
            i++;
        } else if (strcmp(argv[i], "--no-monitor") == 0) {
            config->show_monitor_commands = 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

void create_disk_load(const DiskStressConfig *config) {
    printf("Задание C: Создание дисковый нагрузки\n\n");
    printf("Конфигурация:\n");
    printf("  Количество файлов: %d\n", config->file_count);
    printf("  Размер файла: %zu MB\n", config->file_size_mb);
    printf("  Размер буфера: %zu байт\n", config->buffer_size);
    printf("  PID: %d\n\n", getpid());
    
    size_t file_size = config->file_size_mb * 1024 * 1024;
    int success_count = 0;
    
    for (int i = 0; i < config->file_count; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "disk_stress_%d.bin", i);
        
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            fprintf(stderr, "Error: Cannot open %s: %s\n", filename, strerror(errno));
            continue;
        }
        
        char *buffer = malloc(config->buffer_size);
        if (!buffer) {
            fprintf(stderr, "Error: Cannot allocate buffer: %s\n", strerror(errno));
            close(fd);
            continue;
        }
        
        // Fill buffer with pattern
        for (size_t j = 0; j < config->buffer_size; j++) {
            buffer[j] = (char)((i + j) % 256);
        }
        
        printf("Запись файла %s... ", filename);
        fflush(stdout);
        
        clock_t start = clock();
        size_t total_written = 0;
        
        for (size_t written = 0; written < file_size; written += config->buffer_size) {
            size_t to_write = (file_size - written < config->buffer_size) ? 
                             (file_size - written) : config->buffer_size;
            ssize_t result = write(fd, buffer, to_write);
            if (result != (ssize_t)to_write) {
                fprintf(stderr, "Error: write failed at offset %zu: %s\n", 
                        written, strerror(errno));
                break;
            }
            total_written += result;
        }
        
        if (fsync(fd) == -1) {
            fprintf(stderr, "Warning: fsync failed: %s\n", strerror(errno));
        }
        
        clock_t end = clock();
        double time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        if (time_used > 0) {
            double speed_mbs = (total_written / (1024.0 * 1024.0)) / time_used;
            printf("%.2f сек, %.1f MB/с\n", time_used, speed_mbs);
            success_count++;
        } else {
            printf("завершено (время < 0.01 сек)\n");
            success_count++;
        }
        
        close(fd);
        free(buffer);
    }
    
    printf("\nУспешно создано файлов: %d/%d\n", success_count, config->file_count);
    printf("Файлы: disk_stress_*.bin\n");
}

void monitor_commands(int pid) {
    printf("\n=== Команды для мониторинга ===\n\n");
    printf("1. Общая статистика диска:\n");
    printf("   iostat -x 2 5\n\n");
    
    printf("2. I/O по процессам:\n");
    printf("   pidstat -d 2 5\n\n");
    
    printf("3. Статистика текущего процесса:\n");
    printf("   cat /proc/%d/io\n", pid);
    printf("\n");
}

int main(int argc, char *argv[]) {
    DiskStressConfig config;
    int parse_result = parse_args(argc, argv, &config);
    if (parse_result != 0) {
        return parse_result == -1 ? 1 : 0;
    }
    
    create_disk_load(&config);
    
    if (config.show_monitor_commands) {
        printf("\nНажми Enter для показа команд мониторинга...\n");
        getchar();
        monitor_commands(getpid());
    }
    
    printf("Нажми Enter для очистки тестовых файлов...\n");
    getchar();
    
    // Cleanup
    int removed_count = 0;
    for (int i = 0; i < config.file_count; i++) {
        char filename[100];
        snprintf(filename, sizeof(filename), "disk_stress_%d.bin", i);
        if (remove(filename) == 0) {
            removed_count++;
        }
    }
    
    printf("Удалено тестовых файлов: %d/%d\n", removed_count, config.file_count);
    
    return 0;
}

