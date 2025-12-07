/*
 * memory_profiler.c - Профилировщик памяти процессов
 *
 * Компиляция: gcc -Wall -Wextra -O2 memory_profiler.c -o memory_profiler
 * Использование: ./memory_profiler <PID> [--watch]
 *
 * Основная практическая утилита для Lab 4.
 * Анализирует использование памяти процессом, показывает карту памяти,
 * отслеживает page faults и динамически мониторит изменения.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

// Структура для хранения метрик памяти
typedef struct {
    unsigned long vm_size;      // VSZ - виртуальный размер (KB)
    unsigned long vm_rss;       // RSS - резидентная память (KB)
    unsigned long vm_data;      // Data + Heap (KB)
    unsigned long vm_stk;       // Stack (KB)
    unsigned long vm_exe;       // Text (код) (KB)
    unsigned long vm_lib;       // Shared libraries (KB)
    unsigned long pss;          // Proportional Set Size (KB)
    unsigned long shared_clean; // Чистая разделяемая память (KB)
    unsigned long shared_dirty; // Грязная разделяемая память (KB)
    unsigned long private_clean;// Чистая приватная память (KB)
    unsigned long private_dirty;// Грязная приватная память (KB)
} MemoryMetrics;

// Структура для page faults
typedef struct {
    unsigned long minor_faults;
    unsigned long major_faults;
} PageFaults;

// Структура для сегмента памяти
typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[5];
    char path[256];
} MemorySegment;

// Чтение метрик из /proc/[PID]/status
int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/status");
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));

    char line[256];
    // TODO: Читать файл построчно и извлечь:
    // VmSize, VmRSS, VmData, VmStk, VmExe, VmLib
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &metrics->vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &metrics->vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &metrics->vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &metrics->vm_stk) == 1) continue;
        if (sscanf(line, "VmExe: %lu kB", &metrics->vm_exe) == 1) continue;
        if (sscanf(line, "VmLib: %lu kB", &metrics->vm_lib) == 1) continue;
    }

    fclose(f);
    return 0;
}

// Чтение PSS из /proc/[PID]/smaps_rollup
int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        // smaps_rollup может не существовать на старых ядрах
        // В этом случае нужно парсить /proc/[PID]/smaps и суммировать
        perror("Failed to open /proc/[PID]/smaps_rollup");
        return -1;
    }

    char line[256];
    // TODO: Извлечь Pss, Shared_Clean, Shared_Dirty, Private_Clean, Private_Dirty
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Pss: %lu kB", &metrics->pss) == 1) continue;
        if (sscanf(line, "Shared_Clean: %lu kB", &metrics->shared_clean) == 1) continue;
        if (sscanf(line, "Shared_Dirty: %lu kB", &metrics->shared_dirty) == 1) continue;
        if (sscanf(line, "Private_Clean: %lu kB", &metrics->private_clean) == 1) continue;
        if (sscanf(line, "Private_Dirty: %lu kB", &metrics->private_dirty) == 1) continue;
    }

    fclose(f);
    return 0;
}

// Чтение page faults из /proc/[PID]/stat
int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/stat");
        return -1;
    }

    // Формат /proc/[PID]/stat довольно сложный
    // Поля разделены пробелами, но comm может содержать пробелы
    // Нужные поля:
    // 10 - minflt (minor page faults)
    // 12 - majflt (major page faults)

    // TODO: Прочитать и распарсить
    // Подсказка: можно использовать fscanf с форматом, пропуская ненужные поля
    
    unsigned long minflt, majflt;
    int res = fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*u %lu", &minflt, &majflt);
    if (res == -1) {
        perror("Failed to fscanf /proc/[PID]/stat");
        fclose(f);
        return -1;
    }

    faults->minor_faults = minflt;
    faults->major_faults = majflt;

    fclose(f);
    return 0;
}

// Чтение карты памяти из /proc/[PID]/maps
int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/maps");
        return -1;
    }

    // TODO: Подсчитать количество строк (сегментов)
    // Выделить память для массива сегментов
    // Прочитать и распарсить каждую строку

    *segments = NULL;
    *count = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        (*count)++;
    }
    
    rewind(f);
    
    *segments = malloc(*count * sizeof(MemorySegment));
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < *count) {
        MemorySegment *seg = &(*segments)[i];
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
               &seg->start, &seg->end, seg->perms, seg->path);
        i++;
    }

    fclose(f);
    return 0;
}

// TODO: Функция для красивого вывода размера
void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%4lu KB", kb);
    } else if (kb < 1024 * 1024) {
        printf("%6.1f MB", kb / 1024.0);
    } else {
        printf("%6.2f GB", kb / (1024.0 * 1024.0));
    }
}

char* get_formated_size(unsigned long kb, char* str, size_t len) {
    memset(str, 0, len);

    if (kb < 1024) {
        snprintf(str, len, "%4lu KB", kb);
    } else if (kb < 1024 * 1024) {
        snprintf(str, len, "%6.1f MB", kb / 1024.0);
    } else {
        snprintf(str, len, "%6.2f GB", kb / (1024.0 * 1024.0));
    }

    return str;
}

// TODO: Функция для получения имени процесса
int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(name, len, "unknown");
        return -1;
    }

    if (fgets(name, len, f)) {
        // Убрать \n в конце
        name[strcspn(name, "\n")] = 0;
    }

    fclose(f);
    return 0;
}

// TODO: Главная функция вывода информации о процессе
void print_process_info(pid_t pid)
{
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    char size_str[16];

    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    // Метрики памяти
    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0)
    {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):      %10s\n", get_formated_size(metrics.vm_size, size_str, sizeof(size_str)));
        printf("  RSS (Resident):     %10s\n", get_formated_size(metrics.vm_rss, size_str, sizeof(size_str)));

        // TODO: Если удалось прочитать PSS
        if (read_pss(pid, &metrics) == 0)
        {
            printf("  PSS (Proportional): %10s\n", get_formated_size(metrics.pss, size_str, sizeof(size_str)));

            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):       %10s\n", get_formated_size(uss, size_str, sizeof(size_str)));

            printf("\n");
            printf("Memory Breakdown:\n");
            printf("  Shared (clean):     %10s\n", get_formated_size(metrics.shared_clean, size_str, sizeof(size_str)));
            printf("  Shared (dirty):     %10s\n", get_formated_size(metrics.shared_dirty, size_str, sizeof(size_str)));
            printf("  Private (clean):    %10s\n", get_formated_size(metrics.private_clean, size_str, sizeof(size_str)));
            printf("  Private (dirty):    %10s\n", get_formated_size(metrics.private_dirty, size_str, sizeof(size_str)));
        }

        printf("\n");
        printf("Memory Regions:\n");
        printf("  Text (code):        %10s\n", get_formated_size(metrics.vm_exe, size_str, sizeof(size_str)));
        printf("  Data + Heap:        %10s\n", get_formated_size(metrics.vm_data, size_str, sizeof(size_str)));
        printf("  Stack:              %10s\n", get_formated_size(metrics.vm_stk, size_str, sizeof(size_str)));
        printf("  Libraries:          %10s\n", get_formated_size(metrics.vm_lib, size_str, sizeof(size_str)));
    }

    // Page faults
    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0)
    {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    }

    // TODO: Опционально - показать карту памяти
    // printf("\n");
    // print_memory_map_summary(pid);
}

// TODO: Функция для вывода карты памяти
void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }

    printf("Memory Map (%d segments):\n", count);
    printf("%-25s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    // TODO: Группировать сегменты по типу
    // - [heap]
    // - [stack]
    // - [vdso], [vvar]
    // - библиотеки (.so)
    // - anonymous
    // - исполняемый файл

    // Пример вывода первых 20 сегментов    
    for (int i = 0; i < count; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;
        char size_str[16];

        printf("%012lx-%012lx %-6s %10s", seg->start, seg->end, seg->perms, 
            get_formated_size(size_kb, size_str, sizeof(size_str)));
        printf("  %s\n", seg->path[0] ? seg->path : "(anonymous)");
    }

    // if (count > 20) {
    //     printf("... (%d more segments)\n", count - 20);
    // }

    free(segments);
}

// Режим мониторинга (--watch)
void watch_process(pid_t pid, int interval) {
    
    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};
    
    int first_iteration = 1;
    
    while (1) {
        // Очистить экран (опционально)
        printf("\033[2J\033[H");  // ANSI escape codes
        
        printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);
        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));

        // Текущие метрики
        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf("Process no longer exists or not accessible.\n");
            break;
        }

        read_pss(pid, &metrics);
        read_page_faults(pid, &faults);

        // Вывести метрики
        char proc_name[256];
        get_process_name(pid, proc_name, sizeof(proc_name));
        printf("Process: %s (PID %d)\n\n", proc_name, pid);

        char size_str[16];

        printf("VSZ:  %10s", get_formated_size(metrics.vm_size, size_str, sizeof(size_str)));
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        printf("RSS:  %10s", get_formated_size(metrics.vm_rss, size_str, sizeof(size_str)));
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        // TODO: Вывести PSS, если доступно
        printf("PSS:  %10s", get_formated_size(metrics.pss, size_str, sizeof(size_str)));
        if (!first_iteration) {
            long delta = (long)metrics.pss - (long)prev_metrics.pss;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = faults.minor_faults - prev_faults.minor_faults;
            if (delta > 0) {
                printf("  (+%ld)", delta);
            }
        }
        printf("\n");

        printf("  Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = faults.major_faults - prev_faults.major_faults;
            if (delta > 0) {
                printf("  (+%ld)", delta);
            }
        }
        printf("\n");

        // Сохранить текущие значения
        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        sleep(interval);
    }
}

// Режим сравнения двух процессов (--compare)
void compare_processes(pid_t pid1, pid_t pid2) {
    printf("Comparing processes: %d vs %d\n", pid1, pid2);
    printf("=====================================\n\n");

    MemoryMetrics m1, m2;

    if (read_memory_metrics(pid1, &m1) != 0) {
        printf("Failed to read metrics for PID %d\n", pid1);
        return;
    }

    if (read_memory_metrics(pid2, &m2) != 0) {
        printf("Failed to read metrics for PID %d\n", pid2);
        return;
    }

    read_pss(pid1, &m1);
    read_pss(pid2, &m2);

    // TODO: Вывести сравнительную таблицу
    printf("%-20s %15s %15s\n", "Metric", "PID 1", "PID 2");
    printf("--------------------------------------------------------\n");

    char fmsz1[16];
    char fmsz2[16];

    printf("%-20s %15s %15s\n", "VSZ", 
        get_formated_size(m1.vm_size, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_size, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "RSS", 
        get_formated_size(m1.vm_rss, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_rss, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Data + Heap", 
        get_formated_size(m1.vm_data, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_data, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Stack", 
        get_formated_size(m1.vm_stk, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_stk, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Text", 
        get_formated_size(m1.vm_exe, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_exe, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Shared libraries", 
        get_formated_size(m1.vm_lib, fmsz1, sizeof(fmsz1)), get_formated_size(m2.vm_lib, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "PSS", 
        get_formated_size(m1.pss, fmsz1, sizeof(fmsz1)), get_formated_size(m2.pss, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Shared clean", 
        get_formated_size(m1.shared_clean, fmsz1, sizeof(fmsz1)), get_formated_size(m2.shared_clean, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Shared dirty", 
        get_formated_size(m1.shared_dirty, fmsz1, sizeof(fmsz1)), get_formated_size(m2.shared_dirty, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Private clean", 
        get_formated_size(m1.private_clean, fmsz1, sizeof(fmsz1)), get_formated_size(m2.private_clean, fmsz2, sizeof(fmsz2)));
    printf("%-20s %15s %15s\n", "Private dirty", 
        get_formated_size(m1.private_dirty, fmsz1, sizeof(fmsz1)), get_formated_size(m2.private_dirty, fmsz2, sizeof(fmsz2)));
    
    // printf("%-20s ", "VSZ"); print_size(m1.vm_size); printf(" "); print_size(m2.vm_size); printf("\n");
    // printf("%-20s ", "RSS"); print_size(m1.vm_rss); printf(" "); print_size(m2.vm_rss); printf("\n");
    // printf("%-20s ", "Data + Heap"); print_size(m1.vm_data); printf(" "); print_size(m2.vm_data); printf("\n");
    // printf("%-20s ", "Stack"); print_size(m1.vm_stk); printf(" "); print_size(m2.vm_stk); printf("\n");
    // printf("%-20s ", "Text"); print_size(m1.vm_exe); printf(" "); print_size(m2.vm_exe); printf("\n");
    // printf("%-20s ", "Shared libraries"); print_size(m1.vm_lib); printf(" "); print_size(m2.vm_lib); printf("\n");
    // printf("%-20s ", "PSS"); print_size(m1.pss); printf(" "); print_size(m2.pss); printf("\n");
    // printf("%-20s ", "Shared clean"); print_size(m1.shared_clean); printf(" "); print_size(m2.shared_clean); printf("\n");
    // printf("%-20s ", "Shared dirty"); print_size(m1.shared_dirty); printf(" "); print_size(m2.shared_dirty); printf("\n");
    // printf("%-20s ", "Private clean"); print_size(m1.private_clean); printf(" "); print_size(m2.private_clean); printf("\n");
    // printf("%-20s ", "Private dirty"); print_size(m1.private_dirty); printf(" "); print_size(m2.private_dirty); printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --watch [interval]     Monitor process continuously (default: 1 sec)\n");
        printf("  --compare <PID2>       Compare two processes\n");
        printf("  --map                  Show detailed memory map\n");
        printf("\nExamples:\n");
        printf("  %s 1234                # Show info for PID 1234\n", argv[0]);
        printf("  %s 1234 --watch        # Monitor PID 1234\n", argv[0]);
        printf("  %s 1234 --watch 5      # Monitor with 5 sec interval\n", argv[0]);
        printf("  %s 1234 --compare 5678 # Compare two processes\n", argv[0]);
        printf("  %s 1234 --map          # Show memory map\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    // Проверить существование процесса
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", pid);
        return 1;
    }

    // Парсинг опций
    int watch_mode = 0;
    int watch_interval = 1;
    int compare_mode = 0;
    pid_t compare_pid = 0;
    int show_map = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                watch_interval = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            compare_mode = 1;
            compare_pid = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--map") == 0) {
            show_map = 1;
        }
    }

    // Выполнить нужный режим
    if (compare_mode) {
        compare_processes(pid, compare_pid);
    } else if (watch_mode) {
        watch_process(pid, watch_interval);
    } else {
        // Обычный режим - показать информацию один раз
        print_process_info(pid);

        if (show_map) {
            printf("\n");
            print_memory_map_summary(pid);
        }
    }

    return 0;
}
