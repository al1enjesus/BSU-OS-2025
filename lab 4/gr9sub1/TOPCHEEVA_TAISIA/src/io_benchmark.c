
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_FILE_SIZE_MB 10
#define DEFAULT_BUFFER_SIZE_KB 64

typedef struct {
    size_t file_size_mb;
    size_t buffer_size;
    int cleanup_files;
} BenchmarkConfig;

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  --size SIZE_MB        File size in MB (default: %d)\n", DEFAULT_FILE_SIZE_MB);
    printf("  --buffer SIZE_KB      Buffer size in KB (default: %d)\n", DEFAULT_BUFFER_SIZE_KB);
    printf("  --keep-files          Keep test files after benchmark\n");
    printf("  --help                Show this help\n");
    printf("\nExamples:\n");
    printf("  %s                    # Default settings\n", program_name);
    printf("  %s --size 50          # 50 MB files\n", program_name);
    printf("  %s --buffer 16        # 16 KB buffer\n", program_name);
    printf("  %s --size 100 --buffer 32\n", program_name);
}

int parse_args(int argc, char *argv[], BenchmarkConfig *config) {
    // Set defaults
    config->file_size_mb = DEFAULT_FILE_SIZE_MB;
    config->buffer_size = DEFAULT_BUFFER_SIZE_KB * 1024;
    config->cleanup_files = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
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
        } else if (strcmp(argv[i], "--keep-files") == 0) {
            config->cleanup_files = 0;
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

void test_fwrite(const char* filename, size_t buffer_size, size_t total_size) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s for writing: %s\n", filename, strerror(errno));
        return;
    }
    
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error: Cannot allocate buffer of size %zu: %s\n", buffer_size, strerror(errno));
        fclose(f);
        return;
    }
    
    memset(buffer, 'A', buffer_size);
    
    clock_t start = clock();
    
    for (size_t written = 0; written < total_size; written += buffer_size) {
        size_t to_write = (total_size - written < buffer_size) ? 
                         (total_size - written) : buffer_size;
        if (fwrite(buffer, 1, to_write, f) != to_write) {
            fprintf(stderr, "Error: fwrite failed at offset %zu: %s\n", written, strerror(errno));
            break;
        }
    }
    
    if (fflush(f) != 0) {
        fprintf(stderr, "Error: fflush failed: %s\n", strerror(errno));
    }
    
    clock_t end = clock();
    double time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (time_used > 0) {
        double speed_mbs = (total_size / (1024.0 * 1024.0)) / time_used;
        printf("fwrite (буфер %6zu байт): %6.3f сек, %6.1f MB/с\n", 
               buffer_size, time_used, speed_mbs);
    }
    
    fclose(f);
    free(buffer);
}

void test_write(const char* filename, size_t buffer_size, size_t total_size) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "Error: Cannot open %s for writing: %s\n", filename, strerror(errno));
        return;
    }
    
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Error: Cannot allocate buffer of size %zu: %s\n", buffer_size, strerror(errno));
        close(fd);
        return;
    }
    
    memset(buffer, 'B', buffer_size);
    
    clock_t start = clock();
    
    for (size_t written = 0; written < total_size; written += buffer_size) {
        size_t to_write = (total_size - written < buffer_size) ? 
                         (total_size - written) : buffer_size;
        ssize_t result = write(fd, buffer, to_write);
        if (result != (ssize_t)to_write) {
            fprintf(stderr, "Error: write failed at offset %zu: %s\n", written, strerror(errno));
            break;
        }
    }
    
    if (fsync(fd) == -1) {
        fprintf(stderr, "Warning: fsync failed: %s\n", strerror(errno));
    }
    
    clock_t end = clock();
    double time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (time_used > 0) {
        double speed_mbs = (total_size / (1024.0 * 1024.0)) / time_used;
        printf("write  (буфер %6zu байт): %6.3f сек, %6.1f MB/с\n", 
               buffer_size, time_used, speed_mbs);
    }
    
    close(fd);
    free(buffer);
}

int main(int argc, char *argv[]) {
    BenchmarkConfig config;
    int parse_result = parse_args(argc, argv, &config);
    if (parse_result != 0) {
        return parse_result == -1 ? 1 : 0;
    }
    
    printf("Задание B: Сравнение методов I/O\n\n");
    printf("Конфигурация:\n");
    printf("  Размер файла: %zu MB\n", config.file_size_mb);
    printf("  Размер буфера: %zu байт (%.1f KB)\n", 
           config.buffer_size, config.buffer_size / 1024.0);
    printf("  Очистка файлов: %s\n\n", config.cleanup_files ? "да" : "нет");
    
    size_t total_size = config.file_size_mb * 1024 * 1024;
    
  
    size_t buffer_sizes[] = {512, 4096, 16384, 65536, config.buffer_size};
    int num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);
    
    
    for (int i = 0; i < num_sizes - 1; i++) {
        if (buffer_sizes[i] == config.buffer_size) {
            num_sizes--; 
            break;
        }
    }
    
    for (int i = 0; i < num_sizes; i++) {
        char fname1[100], fname2[100];
        snprintf(fname1, sizeof(fname1), "test_fwrite_%zu.bin", buffer_sizes[i]);
        snprintf(fname2, sizeof(fname2), "test_write_%zu.bin", buffer_sizes[i]);
        
        test_fwrite(fname1, buffer_sizes[i], total_size);
        test_write(fname2, buffer_sizes[i], total_size);
        printf("---\n");
        
        if (config.cleanup_files) {
            remove(fname1);
            remove(fname2);
        }
        
        sleep(1); 
    }
    
    if (!config.cleanup_files) {
        printf("Тестовые файлы сохранены в текущей директории.\n");
    }
    
    return 0;
}

