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


void print_memory_map_summary(pid_t pid) ;

// TODO: Реализовать чтение метрик из /proc/[PID]/status
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
    //
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

// TODO: Реализовать чтение PSS из /proc/[PID]/smaps_rollup
int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        // smaps_rollup может не существовать на старых ядрах
        // В этом случае нужно парсить /proc/[PID]/smaps и суммировать
        return -1;
    }

    char line[256];
    // TODO: Извлечь Pss, Shared_Clean, Shared_Dirty, Private_Clean, Private_Dirty
    //
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

// TODO: Реализовать чтение page faults из /proc/[PID]/stat
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
    //
    unsigned long minflt, majflt;
    int e = fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*u %lu", &minflt, &majflt);
    if(e == 0)
    {
        perror("Failed to read page faults");
        return -1;
    }
    faults->minor_faults = minflt;
    faults->major_faults = majflt;

    fclose(f);
    return 0;
}

// TODO: Реализовать чтение карты памяти из /proc/[PID]/maps
int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/maps");
        return -1;
    }

    *segments = NULL;
    *count = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        (*count)++;
    }

    rewind(f);

    *segments = malloc(*count * sizeof(MemorySegment));
    if (!*segments) {
        perror("Failed to allocate memory for segments");
        fclose(f);
        return -1;
    }

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
void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    // Метрики памяти
    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");

        // TODO: Если удалось прочитать PSS
        if (read_pss(pid, &metrics) == 0) {
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(" (more accurate)\n");

            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf("\n");

            printf("\n");
            printf("Memory Breakdown:\n");
            printf("  Shared (clean):    "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared (dirty):    "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private (clean):   "); print_size(metrics.private_clean); printf("\n");
            printf("  Private (dirty):   "); print_size(metrics.private_dirty); printf("\n");
        }

        printf("\n");
        printf("Memory Regions:\n");
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf("\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf("\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf("\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf("\n");
    }

    // Page faults
    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    }

    // TODO: Опционально - показать карту памяти
    printf("\n");
    print_memory_map_summary(pid);
}

// TODO: Функция для вывода карты памяти
void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }

    typedef enum {
        GROUP_EXEC,
        GROUP_LIB,
        GROUP_HEAP,
        GROUP_STACK,
        GROUP_KERNEL,
        GROUP_ANON,
        NUM_GROUPS
    } GroupType;

    struct {
        const char *name;
        int count;
        unsigned long total_size_kb;
    } groups[NUM_GROUPS] = {
        {"Executable & mapped files", 0, 0},
        {"Shared libraries", 0, 0},
        {"Heap", 0, 0},
        {"Stack", 0, 0},
        {"Kernel mappings", 0, 0},
        {"Anonymous", 0, 0}
    };

    for (int i = 0; i < count; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;
        const char *path = seg->path[0] ? seg->path : NULL;
        GroupType g;

        if (path && strcmp(path, "[heap]") == 0) g = GROUP_HEAP;
        else if (path && strcmp(path, "[stack]") == 0) g = GROUP_STACK;
        else if (path && (strcmp(path, "[vdso]") == 0 || strcmp(path, "[vvar]") == 0 || strcmp(path, "[vsyscall]") == 0)) g = GROUP_KERNEL;
        else if (path && strstr(path, ".so") != NULL) g = GROUP_LIB;
        else if (!path) g = GROUP_ANON;
        else g = GROUP_EXEC;

        groups[g].count++;
        groups[g].total_size_kb += size_kb;
    }

    printf("Memory Map Summary (%d segments):\n", count);
    printf("%-25s %-10s %-15s\n", "Group", "Segments", "Total Size");
    printf("------------------------------------------------------------\n");

    for (int g = 0; g < NUM_GROUPS; g++) {
        if (groups[g].count > 0) {
            printf("%-25s %-10d ", groups[g].name, groups[g].count);
            print_size(groups[g].total_size_kb);
            printf("\n");
        }
    }

    free(segments);
}

// TODO: Режим мониторинга (--watch)
void watch_process(pid_t pid, int interval) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = {0, 0};
    MemoryMetrics prev_metrics = {0};

    int first_iteration = 1;

    while (1) {
        // Очистить экран (опционально)
        printf("\033[2J\033[H");  // ANSI escape codes

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

        printf("VSZ:  "); print_size(metrics.vm_size);
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        printf("RSS:  "); print_size(metrics.vm_rss);
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            if (delta != 0) {
                printf("  (%+ld KB)", delta);
            }
        }
        printf("\n");

        // TODO: Вывести PSS, если доступно
        printf("PSS:  "); print_size(metrics.pss);
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

// TODO: Режим сравнения двух процессов (--compare)
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

    // printf("%-20s ", "VSZ"); print_size(m1.vm_size); printf(" "); print_size(m2.vm_size); printf("\n");
    // ... и так далее
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
    }

    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * ============ ОБЯЗАТЕЛЬНАЯ ЧАСТЬ ============
 *
 * 1. Реализуйте базовые функции (TODO):
 *    - read_memory_metrics()
 *    - read_pss()
 *    - read_page_faults()
 *    - read_memory_map()
 *    - print_process_info()
 *
 * 2. Протестируйте на разных процессах:
 *    $ ./memory_profiler $$           # Текущая оболочка
 *    $ ./memory_profiler 1            # init/systemd
 *    $ firefox &
 *    $ ./memory_profiler $(pgrep firefox)
 *
 * 3. Реализуйте режим мониторинга (--watch):
 *    $ ./memory_profiler <PID> --watch
 *    Запустите в одном терминале, в другом нагрузите процесс
 *
 * 4. Сравните результаты с системными утилитами:
 *    $ ps -o pid,vsz,rss,comm -p <PID>
 *    $ cat /proc/<PID>/status | grep ^Vm
 *    $ sudo smem -p | grep <PROCESS>
 *
 * ============ ДОПОЛНИТЕЛЬНАЯ ЧАСТЬ (*) ============
 *
 * 5. Реализуйте режим сравнения (--compare):
 *    - Сравнение двух процессов
 *    - Вывод разницы в использовании памяти
 *
 * 6. Улучшите вывод карты памяти:
 *    - Группировка сегментов по типу
 *    - Цветной вывод (heap - зелёный, stack - синий, библиотеки - жёлтый)
 *    - Вывод топ-10 самых больших сегментов
 *
 * 7. Добавьте ASCII-график изменения памяти:
 *    - В режиме --watch показывать график RSS за последние N итераций
 *    - Или сохранять данные в CSV для построения графика
 *
 * 8. Реализуйте анализ разделяемых библиотек:
 *    - Какие .so используются
 *    - Сколько процессов разделяют каждую библиотеку
 *    - Реальная экономия памяти от sharing
 *
 * 9. Добавьте поиск процессов по имени:
 *    $ ./memory_profiler firefox --watch
 *    (автоматически найти PID по имени)
 *
 * 10. Создайте интерактивный режим:
 *     - Выбор процесса из списка
 *     - Навигация по карте памяти
 *     - Детальный просмотр каждого сегмента
 *
 * ============ ВОПРОСЫ ДЛЯ ОТЧЁТА ============
 *
 * 1. Почему VSZ обычно намного больше RSS?
 * 2. Что показывает PSS и чем он лучше RSS?
 * 3. Что означает USS и когда он полезен?
 * 4. Почему shared_dirty может увеличиваться?
 * 5. Как интерпретировать разные права доступа (r-x, rw-, r--)?
 * 6. Где в адресном пространстве находятся heap и stack?
 * 7. Почему количество minor page faults постоянно растёт?
 * 8. В каких случаях возникают major page faults?
 * 9. Как разделяемые библиотеки экономят память?
 * 10. Что произойдёт с памятью процесса после fork()?
 */
