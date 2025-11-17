#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  // Добавляем для pid_t

// Функция для красивого вывода размеров
void print_human_readable(unsigned long bytes) {
    if (bytes < 1024)
        printf("%lu B", bytes);
    else if (bytes < 1024 * 1024)
        printf("%.1f KB", bytes / 1024.0);
    else if (bytes < 1024 * 1024 * 1024)
        printf("%.1f MB", bytes / 1024.0 / 1024.0);
    else
        printf("%.1f GB", bytes / 1024.0 / 1024.0 / 1024.0);
}

// Парсинг информации из /proc/pid/status
int parse_process_info(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("❌ Не могу открыть %s\n", path);
        return -1;
    }
    
    printf("\n📊 Информация о процессе PID %d:\n", pid);
    printf("----------------------------------------\n");
    
    char line[256];
    char name[256] = "Unknown";
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name: %255s", name);
            printf("Процесс: %s\n", name);
        }
        else if (strncmp(line, "VmSize:", 7) == 0) {
            unsigned long vmsize;
            sscanf(line, "VmSize: %lu kB", &vmsize);
            printf("VSZ: "); print_human_readable(vmsize * 1024); printf("\n");
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            unsigned long rss;
            sscanf(line, "VmRSS: %lu kB", &rss);
            printf("RSS: "); print_human_readable(rss * 1024); printf("\n");
        }
        else if (strncmp(line, "VmPeak:", 7) == 0) {
            unsigned long peak;
            sscanf(line, "VmPeak: %lu kB", &peak);
            printf("Peak: "); print_human_readable(peak * 1024); printf("\n");
        }
        else if (strncmp(line, "VmPTE:", 6) == 0) {
            unsigned long pte;
            sscanf(line, "VmPTE: %lu kB", &pte);
            printf("PTE: "); print_human_readable(pte * 1024); printf("\n");
        }
    }
    
    fclose(file);
    
    // Также выводим информацию о page faults
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file) {
        char line[1024];
        if (fgets(line, sizeof(line), stat_file)) {
            char *token = strtok(line, " ");
            for (int i = 1; i <= 12; i++) {
                token = strtok(NULL, " ");
                if (i == 10) printf("Minor faults: %s\n", token);
                if (i == 12) printf("Major faults: %s\n", token);
            }
        }
        fclose(stat_file);
    }
    
    return 0;
}

// Функция для вывода карты памяти
void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("❌ Не могу открыть карту памяти\n");
        return;
    }
    
    printf("\n🗺️ Карта памяти (первые 20 строк):\n");
    printf("----------------------------------------\n");
    
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), file) && count < 20) {
        printf("%s", line);
        count++;
    }
    
    fclose(file);
}

// Основная функция
int main(int argc, char *argv[]) {
    printf("🛠️ Memory Profiler - Анализатор памяти\n");
    printf("========================================\n");
    
    if (argc < 2) {
        printf("Использование: %s <PID>\n", argv[0]);
        printf("Пример: %s 1234\n", argv[0]);
        printf("Или для текущего процесса: %s %d\n", argv[0], getpid());
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    
    // Если PID = 0, используем текущий процесс
    if (pid == 0) {
        pid = getpid();
    }
    
    printf("Анализируем процесс с PID: %d\n", pid);
    
    if (parse_process_info(pid) == 0) {
        print_memory_map(pid);
        printf("\n✅ Анализ завершен\n");
    } else {
        printf("❌ Ошибка анализа\n");
    }
    
    return 0;
}
