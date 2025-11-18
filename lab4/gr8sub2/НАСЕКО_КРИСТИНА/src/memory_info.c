/*
 * memory_info.c - Базовый пример чтения информации о памяти процесса
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_info.c -o memory_info
 * Использование: ./memory_info [PID]
 *
 * Демонстрирует:
 * - Чтение VSZ, RSS из /proc/[PID]/status
 * - Разные типы выделения памяти (stack, heap, mmap)
 * - Отображение карты памяти из /proc/[PID]/maps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

// Функция для красивого вывода размера
void print_human_readable(unsigned long bytes) {
    if (bytes < 1024) {
        printf("%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        printf("%.1f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        printf("%.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        printf("%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// Безопасное создание временного файла для mmap
int create_temp_mmap_file(char *filename, size_t filename_size) {
    // Генерация уникального имени файла с временной меткой и PID
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    // Создаем уникальное имя с временной меткой и PID процесса
    snprintf(filename, filename_size, "test_mmap_file_%s_%d.bin", timestamp, getpid());
    
    // Проверяем, не существует ли уже файл (маловероятно, но для безопасности)
    int counter = 0;
    char unique_filename[256];
    strcpy(unique_filename, filename);
    
    while (access(unique_filename, F_OK) == 0 && counter < 100) {
        // Если файл существует, добавляем суффикс
        snprintf(unique_filename, sizeof(unique_filename), 
                 "test_mmap_file_%s_%d_%d.bin", timestamp, getpid(), counter);
        counter++;
    }
    
    if (counter >= 100) {
        fprintf(stderr, "Error: Could not create unique temporary file name\n");
        return -1;
    }
    
    // Копируем окончательное имя обратно
    strncpy(filename, unique_filename, filename_size - 1);
    filename[filename_size - 1] = '\0';
    
    printf("Creating temporary file: %s\n", filename);
    
    // Создаем файл с проверкой на существование (O_EXCL)
    int fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0644);
    
    if (fd == -1) {
        perror("Failed to create temporary file");
        return -1;
    }
    
    return fd;
}

// Реализовать функцию для чтения метрик из /proc/[PID]/status
void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;
    unsigned long vm_exe = 0, vm_lib = 0;

    // Прочитать файл построчно и извлечь метрики
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &vm_stk);
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line, "VmExe: %lu kB", &vm_exe);
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line, "VmLib: %lu kB", &vm_lib);
        }
    }

    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):     "); print_human_readable(vm_size * 1024); printf("\n");
    printf("  RSS (Resident):    "); print_human_readable(vm_rss * 1024); printf("\n");
    printf("  Data/Heap:         "); print_human_readable(vm_data * 1024); printf("\n");
    printf("  Stack:             "); print_human_readable(vm_stk * 1024); printf("\n");
    printf("  Text (code):       "); print_human_readable(vm_exe * 1024); printf("\n");
    printf("  Libraries:         "); print_human_readable(vm_lib * 1024); printf("\n");
    if (vm_size > 0) {
        printf("  Ratio RSS/VSZ:     %.1f%%\n", (vm_rss * 100.0) / vm_size);
    } else {
        printf("  Ratio RSS/VSZ:     N/A\n");
    }
}

// Реализовать функцию для вывода карты памяти
void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    printf("\nMemory Map:\n");
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    char line[512];
    int segment_count = 0;
    unsigned long total_size = 0;

    // Прочитать и распарсить каждую строку
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        // Распарсить строку
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, path_str) >= 3) {
            
            // Вычислить размер
            unsigned long size = end - start;
            total_size += size;

            // Вывести информацию в красивом виде
            printf("%08lx-%08lx %-6s ", start, end, perms);
            print_human_readable(size);
            printf("  %s\n", path_str[0] ? path_str : "(anonymous)");
            
            segment_count++;
            if (segment_count >= 20) { // Ограничим вывод
                printf("... (showing first 20 segments)\n");
                break;
            }
        }
    }

    fclose(f);
    
    printf("\nTotal mapped memory: ");
    print_human_readable(total_size);
    printf(" in %d segments\n", segment_count);
}

// Реализовать функцию, демонстрирующую разные типы памяти
void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    // 1. Стек (stack)
    char stack_var[1024];  // Локальная переменная
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    // 2. Heap (через malloc)
    size_t heap_size = 10 * 1024 * 1024;  // 10 MB
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }

    // Заполнить память, чтобы вызвать page faults
    memset(heap_var, 'H', heap_size);
    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    // 3. Anonymous mmap (аналог malloc для больших блоков)
    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    memset(mmap_var, 'M', mmap_size);
    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

    // 4. File-backed mmap с безопасным созданием файла
    char temp_filename[256];
    int fd = create_temp_mmap_file(temp_filename, sizeof(temp_filename));
    if (fd == -1) {
        fprintf(stderr, "Failed to create temporary file for mmap\n");
        free(heap_var);
        munmap(mmap_var, mmap_size);
        return;
    }

    // Установить размер файла
    if (ftruncate(fd, 5 * 1024 * 1024) == -1) {
        perror("ftruncate failed");
        close(fd);
        // Удаляем временный файл при ошибке
        unlink(temp_filename);
        free(heap_var);
        munmap(mmap_var, mmap_size);
        return;
    }

    void *file_mmap = mmap(NULL, 5 * 1024 * 1024, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);
    if (file_mmap == MAP_FAILED) {
        perror("file mmap failed");
        close(fd);
        // Удаляем временный файл при ошибке
        unlink(temp_filename);
        free(heap_var);
        munmap(mmap_var, mmap_size);
        return;
    }
    memset(file_mmap, 'F', 5 * 1024 * 1024);
    printf("4. File-backed mmap: 5 MB at %p (file: %s)\n", file_mmap, temp_filename);

    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    // Вывести информацию о текущем процессе
    print_memory_metrics(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    // Освободить ресурсы
    free(heap_var);
    munmap(mmap_var, mmap_size);
    munmap(file_mmap, 5 * 1024 * 1024);
    close(fd);
    
    // Удаляем временный файл
    if (unlink(temp_filename) == -1) {
        perror("Failed to delete temporary file");
    } else {
        printf("Temporary file deleted: %s\n", temp_filename);
    }
}

int main(int argc, char *argv[]) {
    printf("Memory Info Demo\n");
    printf("================\n");

    if (argc == 2) {
        // Режим: анализ заданного PID
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else if (argc == 1) {
        // Режим: демонстрация на себе
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");
        demonstrate_memory_types();
    } else {
        printf("Usage: %s [PID]\n", argv[0]);
        printf("  Without PID: run demonstration mode\n");
        printf("  With PID: analyze specified process\n");
        return 1;
    }

    return 0;
}
