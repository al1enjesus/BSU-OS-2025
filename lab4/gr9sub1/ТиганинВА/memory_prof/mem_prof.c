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

/* ========================
 * ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
 * ======================== */

// Функция для красивого вывода размера
void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%4lu KB", kb);
    } else if (kb < 1024UL * 1024UL) {
        printf("%6.1f MB", kb / 1024.0);
    } else {
        printf("%6.2f GB", kb / (1024.0 * 1024.0));
    }
}

// Функция для получения имени процесса
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

/* ============================
 * РЕАЛИЗАЦИЯ ЧТЕНИЯ ИЗ /proc
 * ============================ */

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
    while (fgets(line, sizeof(line), f)) {
        // Набор стандартных полей
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

// Фолбэк: суммирование PSS и разбиения из /proc/[PID]/smaps, если smaps_rollup недоступен
static int read_pss_from_smaps(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1; // нет доступа или слишком старое ядро/права
    }

    char line[256];
    unsigned long pss_sum = 0;
    unsigned long shared_clean_sum = 0, shared_dirty_sum = 0;
    unsigned long private_clean_sum = 0, private_dirty_sum = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned long val;
        if (sscanf(line, "Pss: %lu kB", &val) == 1) {
            pss_sum += val;
            continue;
        }
        if (sscanf(line, "Shared_Clean: %lu kB", &val) == 1) {
            shared_clean_sum += val;
            continue;
        }
        if (sscanf(line, "Shared_Dirty: %lu kB", &val) == 1) {
            shared_dirty_sum += val;
            continue;
        }
        if (sscanf(line, "Private_Clean: %lu kB", &val) == 1) {
            private_clean_sum += val;
            continue;
        }
        if (sscanf(line, "Private_Dirty: %lu kB", &val) == 1) {
            private_dirty_sum += val;
            continue;
        }
    }

    fclose(f);

    metrics->pss = pss_sum;
    metrics->shared_clean = shared_clean_sum;
    metrics->shared_dirty = shared_dirty_sum;
    metrics->private_clean = private_clean_sum;
    metrics->private_dirty = private_dirty_sum;

    return 0;
}

// Чтение PSS из /proc/[PID]/smaps_rollup
int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        // smaps_rollup может не существовать на старых ядрах — fallback
        return read_pss_from_smaps(pid, metrics);
    }

    char line[256];
    unsigned long pss = 0, shared_clean = 0, shared_dirty = 0, private_clean = 0, private_dirty = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Pss: %lu kB", &pss) == 1) continue;
        if (sscanf(line, "Shared_Clean: %lu kB", &shared_clean) == 1) continue;
        if (sscanf(line, "Shared_Dirty: %lu kB", &shared_dirty) == 1) continue;
        if (sscanf(line, "Private_Clean: %lu kB", &private_clean) == 1) continue;
        if (sscanf(line, "Private_Dirty: %lu kB", &private_dirty) == 1) continue;
    }

    fclose(f);

    metrics->pss = pss;
    metrics->shared_clean = shared_clean;
    metrics->shared_dirty = shared_dirty;
    metrics->private_clean = private_clean;
    metrics->private_dirty = private_dirty;

    return 0;
}

// Корректное чтение page faults из /proc/[PID]/stat с учётом (comm) в скобках
int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open /proc/[PID]/stat");
        return -1;
    }

    // Считаем всю строку, потом разберём
    char buf[4096];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    // Найдём конец (comm), который в круглых скобках и может содержать пробелы
    char *lparen = strchr(buf, '(');
    char *rparen = strrchr(buf, ')');
    if (!lparen || !rparen || rparen < lparen) {
        return -1;
    }

    // Поля после rparen+2 (пробел) — это начиная с state (3-е поле)
    char *p = rparen + 2; // пропустить ") "

    // Теперь парсим оставшиеся поля как токены
    // Порядок после comm:
    // 3: state, 4: ppid, 5: pgrp, 6: session, 7: tty_nr, 8: tpgid, 9: flags,
    // 10: minflt, 11: cminflt, 12: majflt, 13: cmajflt, ...
    // Возьмём 7 токенов, затем minflt, пропустим один, затем majflt.
    char *saveptr = NULL;
    int field_index = 3;
    unsigned long minflt = 0, majflt = 0;

    // state
    strtok_r(p, " ", &saveptr); // 3
    for (int i = 4; i <= 9; i++) {
        strtok_r(NULL, " ", &saveptr); // 4..9
    }
    // 10: minflt
    {
        char *tok = strtok_r(NULL, " ", &saveptr);
        if (!tok) return -1;
        minflt = strtoul(tok, NULL, 10);
    }
    // 11: cminflt
    strtok_r(NULL, " ", &saveptr);
    // 12: majflt
    {
        char *tok = strtok_r(NULL, " ", &saveptr);
        if (!tok) return -1;
        majflt = strtoul(tok, NULL, 10);
    }

    faults->minor_faults = minflt;
    faults->major_faults = majflt;
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

    char line[1024];
    int lines = 0;
    while (fgets(line, sizeof(line), f)) {
        lines++;
    }

    if (lines == 0) {
        fclose(f);
        *segments = NULL;
        *count = 0;
        return 0;
    }

    rewind(f);
    MemorySegment *arr = (MemorySegment *)malloc((size_t)lines * sizeof(MemorySegment));
    if (!arr) {
        fclose(f);
        return -1;
    }

    int i = 0;
    while (i < lines && fgets(line, sizeof(line), f)) {
        MemorySegment *seg = &arr[i];
        seg->path[0] = '\0';
        seg->perms[0] = '\0';

        // Формат: address perms offset dev inode pathname(optional)
        // Пример строки:
        // 55c7d1c8e000-55c7d1cb0000 r--p 00000000 103:00 123456 /usr/bin/cat
        // path может отсутствовать (анонимный)
        unsigned long start, end;
        char perms[5] = {0};
        char pathname[256] = {0};

        // Вытаскиваем адреса, права и путь (если есть)
        // %255[^\n] захватывает остаток строки как путь/метку
        int matched = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
                             &start, &end, perms, pathname);

        seg->start = start;
        seg->end = end;
        strncpy(seg->perms, perms, sizeof(seg->perms) - 1);
        seg->perms[sizeof(seg->perms) - 1] = '\0';

        if (matched == 4) {
            // Уберём начальные пробелы для path
            size_t off = 0;
            while (pathname[off] == ' ' || pathname[off] == '\t') off++;
            strncpy(seg->path, pathname + off, sizeof(seg->path) - 1);
            seg->path[sizeof(seg->path) - 1] = '\0';
        } else {
            seg->path[0] = '\0';
        }

        i++;
    }

    fclose(f);
    *segments = arr;
    *count = lines;
    return 0;
}

/* ===========================
 * ВЫВОД И РЕЖИМЫ РАБОТЫ
 * =========================== */

// Функция для вывода карты памяти
void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        return;
    }

    printf("Memory Map (%d segments):\n", count);
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    // Пример вывода первых 20 сегментов
    for (int i = 0; i < count && i < 20; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024UL;

        printf("%08lx-%08lx %-6s ", seg->start, seg->end, seg->perms);
        print_size(size_kb);
        printf("  %s\n", seg->path[0] ? seg->path : "(anonymous)");
    }

    if (count > 20) {
        printf("... (%d more segments)\n", count - 20);
    }

    free(segments);
}

// Главная функция вывода информации о процессе
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

        // PSS + разбиение, если возможно
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
    } else {
        printf("Failed to read memory metrics.\n");
    }

    // Page faults
    printf("\n");
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", faults.minor_faults);
        printf("  Major: %lu\n", faults.major_faults);
    } else {
        printf("Failed to read page faults.\n");
    }

    // Опционально - показать карту памяти (выводится из main при --map)
    // printf("\n");
    // print_memory_map_summary(pid);
}

// Режим мониторинга (--watch)
void watch_process(pid_t pid, int interval) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    PageFaults prev_faults = (PageFaults){0, 0};
    MemoryMetrics prev_metrics;
    memset(&prev_metrics, 0, sizeof(prev_metrics));

    int first_iteration = 1;

    while (1) {
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

        if (metrics.pss > 0) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration) {
                long delta = (long)metrics.pss - (long)prev_metrics.pss;
                if (delta != 0) {
                    printf("  (%+ld KB)", delta);
                }
            }
            printf("\n");
        }

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = (long)faults.minor_faults - (long)prev_faults.minor_faults;
            if (delta > 0) {
                printf("  (+%ld)", delta);
            }
        }
        printf("\n");

        printf("  Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = (long)faults.major_faults - (long)prev_faults.major_faults;
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

    // Табличный вывод + разница
    printf("%-20s %15s %15s %15s\n", "Metric", "PID 1", "PID 2", "Δ (PID2-PID1)");
    printf("-----------------------------------------------------------------------\n");

    // Вспомогательный макрос для печати KB и дельты
    #define PRINT_ROW(label, v1, v2) do { \
        printf("%-20s ", label); \
        char buf1[32], buf2[32], bufd[32]; \
        /* Преобразуем для печати в человекочитаемом формате */ \
        /* Используем временные FILE* для форматирования через print_size */ \
        /* Но проще — распечатать KB напрямую, чтобы избежать сложностей */ \
        printf("%15lu KB %15lu KB %15ld KB\n", (unsigned long)(v1), (unsigned long)(v2), (long)((v2) - (v1))); \
    } while(0)

    PRINT_ROW("VSZ", m1.vm_size, m2.vm_size);
    PRINT_ROW("RSS", m1.vm_rss, m2.vm_rss);
    PRINT_ROW("PSS", m1.pss, m2.pss);
    PRINT_ROW("USS", (m1.private_clean + m1.private_dirty), (m2.private_clean + m2.private_dirty));
    PRINT_ROW("Text(code)", m1.vm_exe, m2.vm_exe);
    PRINT_ROW("Data+Heap", m1.vm_data, m2.vm_data);
    PRINT_ROW("Stack", m1.vm_stk, m2.vm_stk);
    PRINT_ROW("Libraries", m1.vm_lib, m2.vm_lib);
    PRINT_ROW("Shared clean", m1.shared_clean, m2.shared_clean);
    PRINT_ROW("Shared dirty", m1.shared_dirty, m2.shared_dirty);
    PRINT_ROW("Private clean", m1.private_clean, m2.private_clean);
    PRINT_ROW("Private dirty", m1.private_dirty, m2.private_dirty);

    #undef PRINT_ROW
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

    pid_t pid = (pid_t)atoi(argv[1]);

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
                if (watch_interval <= 0) watch_interval = 1;
                i++;
            }
        } else if (strcmp(argv[i], "--compare") == 0 && i + 1 < argc) {
            compare_mode = 1;
            compare_pid = (pid_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--map") == 0) {
            show_map = 1;
        }
    }

    // Выполнить нужный режим
    if (compare_mode) {
        // Проверить второй PID
        snprintf(path, sizeof(path), "/proc/%d", compare_pid);
        if (access(path, F_OK) != 0) {
            fprintf(stderr, "Error: Process %d does not exist or not accessible.\n", compare_pid);
            return 1;
        }
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
