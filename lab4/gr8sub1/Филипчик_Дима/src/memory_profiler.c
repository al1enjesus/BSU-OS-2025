/*
 * memory_profiler.c - Профилировщик памяти процессов
 * Вариант 1 - Полная реализация
 * 
 * Компиляция: gcc -Wall -Wextra -O2 memory_profiler.c -o memory_profiler
 * Использование: ./memory_profiler <PID> [--watch] [--compare PID2] [--map]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>

// Цвета для вывода
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// Структура для хранения метрик памяти
typedef struct {
    unsigned long vm_size;      // VSZ - виртуальный размер (KB)
    unsigned long vm_rss;       // RSS - резидентная память (KB)
    unsigned long vm_data;      // Data + Heap (KB)
    unsigned long vm_stk;       // Stack (KB)
    unsigned long vm_exe;       // Text (код) (KB)
    unsigned long vm_lib;       // Shared libraries (KB)
    unsigned long vm_pte;       // Page Table Entries (KB)
    unsigned long vm_swap;      // Swap usage (KB)
    unsigned long pss;          // Proportional Set Size (KB)
    unsigned long shared_clean; // Чистая разделяемая память (KB)
    unsigned long shared_dirty; // Грязная разделяемая память (KB)
    unsigned long private_clean;// Чистая приватная память (KB)
    unsigned long private_dirty;// Грязная приватная память (KB)
    unsigned long referenced;   // Referenced memory (KB)
    unsigned long anonymous;    // Anonymous memory (KB)
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
    unsigned long offset;
    char dev[8];
    unsigned long inode;
    char path[256];
} MemorySegment;

// Структура для статистики по типам памяти
typedef struct {
    unsigned long total_size;
    unsigned long total_rss;
    unsigned long total_pss;
    int count;
} MemoryTypeStats;

// Глобальные переменные для отслеживания сигналов
static volatile int keep_running = 1;

// Обработчик сигнала для graceful shutdown
void signal_handler(int sig) {
    keep_running = 0;
    printf("\nReceived signal %d, shutting down...\n", sig);
}

// Функция для проверки существования процесса
int process_exists(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    return access(path, F_OK) == 0;
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
    } else {
        snprintf(name, len, "unknown");
    }

    fclose(f);
    return 0;
}

// Функция для чтения метрик из /proc/[PID]/status
int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &metrics->vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &metrics->vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &metrics->vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &metrics->vm_stk);
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line, "VmExe: %lu kB", &metrics->vm_exe);
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line, "VmLib: %lu kB", &metrics->vm_lib);
        } else if (strncmp(line, "VmPTE:", 6) == 0) {
            sscanf(line, "VmPTE: %lu kB", &metrics->vm_pte);
        } else if (strncmp(line, "VmSwap:", 7) == 0) {
            sscanf(line, "VmSwap: %lu kB", &metrics->vm_swap);
        }
    }

    fclose(f);
    return 0;
}

// Функция для чтения PSS из /proc/[PID]/smaps_rollup
int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        // Если smaps_rollup нет, пробуем прочитать из smaps
        return read_pss_from_smaps(pid, metrics);
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &metrics->pss);
        } else if (strncmp(line, "Shared_Clean:", 13) == 0) {
            sscanf(line, "Shared_Clean: %lu kB", &metrics->shared_clean);
        } else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
            sscanf(line, "Shared_Dirty: %lu kB", &metrics->shared_dirty);
        } else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &metrics->private_clean);
        } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &metrics->private_dirty);
        } else if (strncmp(line, "Referenced:", 11) == 0) {
            sscanf(line, "Referenced: %lu kB", &metrics->referenced);
        } else if (strncmp(line, "Anonymous:", 10) == 0) {
            sscanf(line, "Anonymous: %lu kB", &metrics->anonymous);
        }
    }

    fclose(f);
    return 0;
}

// Альтернативная функция чтения PSS из smaps (если smaps_rollup нет)
int read_pss_from_smaps(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    char line[256];
    unsigned long total_pss = 0;
    unsigned long total_shared_clean = 0;
    unsigned long total_shared_dirty = 0;
    unsigned long total_private_clean = 0;
    unsigned long total_private_dirty = 0;
    unsigned long total_referenced = 0;
    unsigned long total_anonymous = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned long value;
        if (sscanf(line, "Pss: %lu kB", &value) == 1) {
            total_pss += value;
        } else if (sscanf(line, "Shared_Clean: %lu kB", &value) == 1) {
            total_shared_clean += value;
        } else if (sscanf(line, "Shared_Dirty: %lu kB", &value) == 1) {
            total_shared_dirty += value;
        } else if (sscanf(line, "Private_Clean: %lu kB", &value) == 1) {
            total_private_clean += value;
        } else if (sscanf(line, "Private_Dirty: %lu kB", &value) == 1) {
            total_private_dirty += value;
        } else if (sscanf(line, "Referenced: %lu kB", &value) == 1) {
            total_referenced += value;
        } else if (sscanf(line, "Anonymous: %lu kB", &value) == 1) {
            total_anonymous += value;
        }
    }

    fclose(f);

    metrics->pss = total_pss;
    metrics->shared_clean = total_shared_clean;
    metrics->shared_dirty = total_shared_dirty;
    metrics->private_clean = total_private_clean;
    metrics->private_dirty = total_private_dirty;
    metrics->referenced = total_referenced;
    metrics->anonymous = total_anonymous;

    return 0;
}

// Функция для чтения page faults из /proc/[PID]/stat
int read_page_faults(pid_t pid, PageFaults *faults) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    // Формат /proc/[PID]/stat: pid, comm, state, ppid, pgrp, session, tty_nr, ...
    // Нам нужны поля 10 (minflt) и 12 (majflt)
    unsigned long minflt, majflt;
    
    if (fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*lu %lu", 
               &minflt, &majflt) != 2) {
        fclose(f);
        return -1;
    }

    fclose(f);

    faults->minor_faults = minflt;
    faults->major_faults = majflt;
    return 0;
}

// Функция для чтения карты памяти из /proc/[PID]/maps
int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    // Первый проход: подсчёт количества сегментов
    *count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        (*count)++;
    }

    if (*count == 0) {
        fclose(f);
        *segments = NULL;
        return 0;
    }

    // Выделение памяти для сегментов
    *segments = malloc(*count * sizeof(MemorySegment));
    if (!*segments) {
        fclose(f);
        return -1;
    }

    // Второй проход: чтение данных
    rewind(f);
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < *count) {
        MemorySegment *seg = &(*segments)[i];
        char perms[5];
        unsigned long offset;
        char dev[8];
        unsigned long inode;
        char path_str[256] = "";

        if (sscanf(line, "%lx-%lx %4s %lx %7s %lu %255[^\n]",
                   &seg->start, &seg->end, perms, &offset, dev, &inode, path_str) >= 6) {
            strcpy(seg->perms, perms);
            seg->offset = offset;
            strcpy(seg->dev, dev);
            seg->inode = inode;
            strncpy(seg->path, path_str, sizeof(seg->path) - 1);
            seg->path[sizeof(seg->path) - 1] = '\0';
            i++;
        }
    }

    fclose(f);
    *count = i;  // Фактическое количество прочитанных сегментов
    return 0;
}

// Функция для красивого вывода размера
void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%4lu KB", kb);
    } else if (kb < 1024 * 1024) {
        printf("%6.1f MB", kb / 1024.0);
    } else {
        printf("%6.2f GB", kb / (1024.0 * 1024.0));
    }
}

// Функция для форматирования дельты
void print_delta(long delta) {
    if (delta > 0) {
        printf(COLOR_RED " (+"); print_size(delta); printf(")" COLOR_RESET);
    } else if (delta < 0) {
        printf(COLOR_GREEN " ("); print_size(-delta); printf(")" COLOR_RESET);
    }
}

// Функция для получения цвета типа памяти
const char* get_memory_type_color(const char* path) {
    if (strstr(path, "[heap]")) return COLOR_GREEN;
    if (strstr(path, "[stack]")) return COLOR_BLUE;
    if (strstr(path, "[vdso]") || strstr(path, "[vvar]")) return COLOR_MAGENTA;
    if (strstr(path, ".so")) return COLOR_YELLOW;
    if (strlen(path) == 0 || strstr(path, "anonymous")) return COLOR_CYAN;
    return COLOR_RESET;
}

// Функция для получения типа памяти
const char* get_memory_type(const char* path) {
    if (strstr(path, "[heap]")) return "Heap";
    if (strstr(path, "[stack]")) return "Stack";
    if (strstr(path, "[vdso]")) return "vdso";
    if (strstr(path, "[vvar]")) return "vvar";
    if (strstr(path, ".so")) return "Library";
    if (strlen(path) == 0) return "Anonymous";
    if (strstr(path, "anonymous")) return "Anonymous";
    
    // Проверяем, является ли это исполняемым файлом
    if (access(path, X_OK) == 0) return "Executable";
    
    return "File-backed";
}

// Главная функция вывода информации о процессе
void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));

    printf(COLOR_CYAN "Process: %s (PID %d)\n" COLOR_RESET, proc_name, pid);
    printf("=====================================\n\n");

    // Метрики памяти
    MemoryMetrics metrics;
    if (read_memory_metrics(pid, &metrics) == 0) {
        printf(COLOR_YELLOW "Basic Memory Metrics:\n" COLOR_RESET);
        printf("  VSZ (Virtual):     "); print_size(metrics.vm_size); printf("\n");
        printf("  RSS (Resident):    "); print_size(metrics.vm_rss); printf("\n");
        
        if (metrics.vm_swap > 0) {
            printf("  Swap:              "); print_size(metrics.vm_swap); printf(COLOR_RED " ⚠\n" COLOR_RESET);
        }

        // Расширенные метрики PSS
        if (read_pss(pid, &metrics) == 0) {
            printf(COLOR_YELLOW "\nAdvanced Memory Metrics (more accurate):\n" COLOR_RESET);
            printf("  PSS (Proportional):"); print_size(metrics.pss); printf(" 📊\n");
            
            unsigned long uss = metrics.private_clean + metrics.private_dirty;
            printf("  USS (Unique):      "); print_size(uss); printf(" 🔒\n");
            
            unsigned long shared_total = metrics.shared_clean + metrics.shared_dirty;
            printf("  Shared Total:      "); print_size(shared_total); printf(" 🔗\n");
            
            printf(COLOR_YELLOW "\nMemory Breakdown:\n" COLOR_RESET);
            printf("  Shared Clean:      "); print_size(metrics.shared_clean); printf("\n");
            printf("  Shared Dirty:      "); print_size(metrics.shared_dirty); printf("\n");
            printf("  Private Clean:     "); print_size(metrics.private_clean); printf("\n");
            printf("  Private Dirty:     "); print_size(metrics.private_dirty); printf("\n");
            
            if (metrics.referenced > 0) {
                printf("  Referenced:        "); print_size(metrics.referenced); printf("\n");
            }
            if (metrics.anonymous > 0) {
                printf("  Anonymous:         "); print_size(metrics.anonymous); printf("\n");
            }
        }

        printf(COLOR_YELLOW "\nMemory Regions:\n" COLOR_RESET);
        printf("  Text (code):       "); print_size(metrics.vm_exe); printf(" 📝\n");
        printf("  Data + Heap:       "); print_size(metrics.vm_data); printf(" 💾\n");
        printf("  Stack:             "); print_size(metrics.vm_stk); printf(" 🥞\n");
        printf("  Libraries:         "); print_size(metrics.vm_lib); printf(" 📚\n");
        if (metrics.vm_pte > 0) {
            printf("  Page Tables:       "); print_size(metrics.vm_pte); printf(" 📋\n");
        }
    } else {
        printf(COLOR_RED "Error: Failed to read memory metrics\n" COLOR_RESET);
    }

    // Page faults
    printf(COLOR_YELLOW "\nPage Faults:\n" COLOR_RESET);
    PageFaults faults;
    if (read_page_faults(pid, &faults) == 0) {
        printf("  Minor: %lu", faults.minor_faults);
        if (faults.minor_faults > 100000) printf(COLOR_YELLOW " ⚠" COLOR_RESET);
        printf("\n");
        
        printf("  Major: %lu", faults.major_faults);
        if (faults.major_faults > 100) printf(COLOR_RED " 🚨" COLOR_RESET);
        printf("\n");
    } else {
        printf(COLOR_RED "  Failed to read page faults\n" COLOR_RESET);
    }
}

// Функция для вывода суммарной статистики по карте памяти
void print_memory_map_summary(pid_t pid) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        printf(COLOR_RED "Error: Failed to read memory map\n" COLOR_RESET);
        return;
    }

    if (count == 0) {
        printf("No memory segments found.\n");
        return;
    }

    // Статистика по типам памяти
    MemoryTypeStats heap_stats = {0};
    MemoryTypeStats stack_stats = {0};
    MemoryTypeStats lib_stats = {0};
    MemoryTypeStats exec_stats = {0};
    MemoryTypeStats anon_stats = {0};
    MemoryTypeStats file_stats = {0};
    MemoryTypeStats other_stats = {0};

    for (int i = 0; i < count; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size = (seg->end - seg->end) / 1024; // KB
        
        const char* type = get_memory_type(seg->path);
        
        if (strcmp(type, "Heap") == 0) {
            heap_stats.total_size += size;
            heap_stats.count++;
        } else if (strcmp(type, "Stack") == 0) {
            stack_stats.total_size += size;
            stack_stats.count++;
        } else if (strcmp(type, "Library") == 0) {
            lib_stats.total_size += size;
            lib_stats.count++;
        } else if (strcmp(type, "Executable") == 0) {
            exec_stats.total_size += size;
            exec_stats.count++;
        } else if (strcmp(type, "Anonymous") == 0) {
            anon_stats.total_size += size;
            anon_stats.count++;
        } else if (strcmp(type, "File-backed") == 0) {
            file_stats.total_size += size;
            file_stats.count++;
        } else {
            other_stats.total_size += size;
            other_stats.count++;
        }
    }

    printf(COLOR_CYAN "\nMemory Map Summary (%d segments):\n" COLOR_RESET, count);
    printf("=====================================\n");
    
    printf(COLOR_YELLOW "%-15s %8s %8s\n" COLOR_RESET, "Type", "Size", "Segments");
    printf("--------------- -------- --------\n");
    
    if (heap_stats.count > 0) {
        printf(COLOR_GREEN "%-15s" COLOR_RESET, "Heap"); 
        print_size(heap_stats.total_size); printf(" %8d\n", heap_stats.count);
    }
    
    if (stack_stats.count > 0) {
        printf(COLOR_BLUE "%-15s" COLOR_RESET, "Stack");
        print_size(stack_stats.total_size); printf(" %8d\n", stack_stats.count);
    }
    
    if (lib_stats.count > 0) {
        printf(COLOR_YELLOW "%-15s" COLOR_RESET, "Libraries");
        print_size(lib_stats.total_size); printf(" %8d\n", lib_stats.count);
    }
    
    if (exec_stats.count > 0) {
        printf("%-15s", "Executables");
        print_size(exec_stats.total_size); printf(" %8d\n", exec_stats.count);
    }
    
    if (anon_stats.count > 0) {
        printf(COLOR_CYAN "%-15s" COLOR_RESET, "Anonymous");
        print_size(anon_stats.total_size); printf(" %8d\n", anon_stats.count);
    }
    
    if (file_stats.count > 0) {
        printf("%-15s", "File-backed");
        print_size(file_stats.total_size); printf(" %8d\n", file_stats.count);
    }
    
    if (other_stats.count > 0) {
        printf("%-15s", "Other");
        print_size(other_stats.total_size); printf(" %8d\n", other_stats.count);
    }

    free(segments);
}

// Функция для вывода детальной карты памяти
void print_detailed_memory_map(pid_t pid, int limit) {
    MemorySegment *segments = NULL;
    int count = 0;

    if (read_memory_map(pid, &segments, &count) != 0) {
        printf(COLOR_RED "Error: Failed to read memory map\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CYAN "\nDetailed Memory Map (%d segments):\n" COLOR_RESET, count);
    printf("================================================================================\n");
    printf(COLOR_YELLOW "%-18s %-6s %-8s %-12s %s\n" COLOR_RESET, 
           "Address Range", "Perms", "Size", "Type", "Path");
    printf("--------------------------------------------------------------------------------\n");

    int display_count = (limit > 0 && count > limit) ? limit : count;
    
    for (int i = 0; i < display_count; i++) {
        MemorySegment *seg = &segments[i];
        unsigned long size_kb = (seg->end - seg->start) / 1024;
        const char* type = get_memory_type(seg->path);
        const char* color = get_memory_type_color(seg->path);

        printf("%s%08lx-%08lx %-4s ", color, seg->start, seg->end, seg->perms);
        print_size(size_kb);
        printf(" %-12s %s" COLOR_RESET "\n", type, seg->path);
    }

    if (count > display_count) {
        printf(COLOR_CYAN "... (%d more segments)\n" COLOR_RESET, count - display_count);
    }

    free(segments);
}

// Режим мониторинга (--watch)
void watch_process(pid_t pid, int interval) {
    printf(COLOR_CYAN "Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n" COLOR_RESET, pid, interval);
    printf("Process: ");
    
    char proc_name[256];
    if (get_process_name(pid, proc_name, sizeof(proc_name)) == 0) {
        printf("%s", proc_name);
    }
    printf("\n\n");

    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    MemoryMetrics prev_metrics = {0};
    PageFaults prev_faults = {0};
    int first_iteration = 1;
    time_t start_time = time(NULL);

    while (keep_running) {
        if (!process_exists(pid)) {
            printf(COLOR_RED "\nProcess %d no longer exists!\n" COLOR_RESET, pid);
            break;
        }

        // Очистка экрана (кроме первой итерации)
        if (!first_iteration) {
            printf("\033[2J\033[H");  // ANSI escape codes для очистки экрана
        }

        printf(COLOR_CYAN "=== Memory Profiler - Live Monitoring ===\n" COLOR_RESET);
        printf("Time: %s", ctime(&start_time));
        printf("Elapsed: %ld seconds\n", time(NULL) - start_time);
        printf("Process: %s (PID %d)\n", proc_name, pid);
        printf("----------------------------------------\n");

        // Текущие метрики
        MemoryMetrics metrics;
        PageFaults faults;

        if (read_memory_metrics(pid, &metrics) != 0) {
            printf(COLOR_RED "Error reading memory metrics\n" COLOR_RESET);
            break;
        }

        read_pss(pid, &metrics);
        read_page_faults(pid, &faults);

        // Вывод метрик с дельтами
        printf(COLOR_YELLOW "Memory Usage:\n" COLOR_RESET);
        
        printf("VSZ:  "); print_size(metrics.vm_size);
        if (!first_iteration) {
            long delta = (long)metrics.vm_size - (long)prev_metrics.vm_size;
            print_delta(delta);
        }
        printf("\n");

        printf("RSS:  "); print_size(metrics.vm_rss);
        if (!first_iteration) {
            long delta = (long)metrics.vm_rss - (long)prev_metrics.vm_rss;
            print_delta(delta);
        }
        printf("\n");

        if (metrics.pss > 0) {
            printf("PSS:  "); print_size(metrics.pss);
            if (!first_iteration && prev_metrics.pss > 0) {
                long delta = (long)metrics.pss - (long)prev_metrics.pss;
                print_delta(delta);
            }
            printf("\n");
        }

        if (metrics.vm_swap > 0) {
            printf("Swap: "); print_size(metrics.vm_swap);
            if (!first_iteration) {
                long delta = (long)metrics.vm_swap - (long)prev_metrics.vm_swap;
                print_delta(delta);
            }
            printf(COLOR_RED " ⚠\n" COLOR_RESET);
        }

        printf(COLOR_YELLOW "\nPage Faults:\n" COLOR_RESET);
        printf("Minor: %lu", faults.minor_faults);
        if (!first_iteration) {
            long delta = faults.minor_faults - prev_faults.minor_faults;
            if (delta > 0) {
                printf(COLOR_RED " (+%ld)" COLOR_RESET, delta);
            }
        }
        printf("\n");

        printf("Major: %lu", faults.major_faults);
        if (!first_iteration) {
            long delta = faults.major_faults - prev_faults.major_faults;
            if (delta > 0) {
                printf(COLOR_RED " (+%ld) 🚨" COLOR_RESET, delta);
            }
        }
        printf("\n");

        // Простая ASCII визуализация использования памяти
        printf(COLOR_YELLOW "\nMemory Visualization:\n" COLOR_RESET);
        printf("[");
        int rss_blocks = (int)((double)metrics.vm_rss / metrics.vm_size * 50);
        for (int i = 0; i < 50; i++) {
            if (i < rss_blocks) {
                printf(COLOR_GREEN "█" COLOR_RESET);
            } else {
                printf("░");
            }
        }
        printf("] %.1f%% RSS/VSZ\n", (double)metrics.vm_rss / metrics.vm_size * 100);

        // Сохранить текущие значения
        prev_metrics = metrics;
        prev_faults = faults;
        first_iteration = 0;

        // Пауза
        for (int i = 0; i < interval && keep_running; i++) {
            sleep(1);
        }
    }

    printf(COLOR_CYAN "\nMonitoring stopped.\n" COLOR_RESET);
}

// Режим сравнения двух процессов (--compare)
void compare_processes(pid_t pid1, pid_t pid2) {
    char name1[256], name2[256];
    get_process_name(pid1, name1, sizeof(name1));
    get_process_name(pid2, name2, sizeof(name2));

    printf(COLOR_CYAN "Comparing processes:\n" COLOR_RESET);
    printf("  PID %d: %s\n", pid1, name1);
    printf("  PID %d: %s\n", pid2, name2);
    printf("=====================================\n\n");

    MemoryMetrics m1, m2;
    PageFaults f1, f2;

    if (read_memory_metrics(pid1, &m1) != 0 || read_memory_metrics(pid2, &m2) != 0) {
        printf(COLOR_RED "Error: Failed to read metrics for one or both processes\n" COLOR_RESET);
        return;
    }

    read_pss(pid1, &m1);
    read_pss(pid2, &m2);
    read_page_faults(pid1, &f1);
    read_page_faults(pid2, &f2);

    printf(COLOR_YELLOW "%-20s %15s %15s %15s\n" COLOR_RESET, 
           "Metric", "PID 1", "PID 2", "Difference");
    printf("-------------------- --------------- --------------- ---------------\n");

    // VSZ сравнение
    printf("%-20s ", "VSZ");
    print_size(m1.vm_size); printf(" ");
    print_size(m2.vm_size); printf(" ");
    long delta = (long)m2.vm_size - (long)m1.vm_size;
    print_delta(delta);
    printf("\n");

    // RSS сравнение
    printf("%-20s ", "RSS");
    print_size(m1.vm_rss); printf(" ");
    print_size(m2.vm_rss); printf(" ");
    delta = (long)m2.vm_rss - (long)m1.vm_rss;
    print_delta(delta);
    printf("\n");

    // PSS сравнение
    if (m1.pss > 0 && m2.pss > 0) {
        printf("%-20s ", "PSS");
        print_size(m1.pss); printf(" ");
        print_size(m2.pss); printf(" ");
        delta = (long)m2.pss - (long)m1.pss;
        print_delta(delta);
        printf("\n");
    }

    // USS сравнение
    unsigned long uss1 = m1.private_clean + m1.private_dirty;
    unsigned long uss2 = m2.private_clean + m2.private_dirty;
    printf("%-20s ", "USS");
    print_size(uss1); printf(" ");
    print_size(uss2); printf(" ");
    delta = (long)uss2 - (long)uss1;
    print_delta(delta);
    printf("\n");

    // Page faults сравнение
    printf("%-20s %15lu %15lu %15ld\n", "Minor Faults", f1.minor_faults, f2.minor_faults, 
           (long)f2.minor_faults - (long)f1.minor_faults);
    printf("%-20s %15lu %15lu %15ld\n", "Major Faults", f1.major_faults, f2.major_faults,
           (long)f2.major_faults - (long)f1.major_faults);

    // Вывод рекомендаций
    printf(COLOR_YELLOW "\nAnalysis:\n" COLOR_RESET);
    if (m2.vm_rss > m1.vm_rss * 1.5) {
        printf("• PID %d uses significantly more memory than PID %d\n", pid2, pid1);
    } else if (m1.vm_rss > m2.vm_rss * 1.5) {
        printf("• PID %d uses significantly more memory than PID %d\n", pid1, pid2);
    } else {
        printf("• Both processes have similar memory usage\n");
    }

    if (f2.major_faults > f1.major_faults + 10) {
        printf("• PID %d has more major page faults - possible memory pressure\n", pid2);
    }
}

// Функция для поиска процессов по имени
void find_processes_by_name(const char* name) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        printf(COLOR_RED "Error: Cannot open /proc directory\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CYAN "Searching for processes with name containing: %s\n" COLOR_RESET, name);
    printf("=====================================\n");

    struct dirent *entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Проверяем, является ли entry числом (PID)
        char *endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue; // Не число

        // Читаем имя процесса
        char proc_name[256];
        if (get_process_name(pid, proc_name, sizeof(proc_name)) == 0) {
            if (strstr(proc_name, name) != NULL) {
                printf("PID: %5d, Name: %s\n", pid, proc_name);
                found++;
            }
        }
    }

    closedir(dir);

    if (found == 0) {
        printf("No processes found matching '%s'\n", name);
    } else {
        printf("\nFound %d processes\n", found);
    }
}

// Основная функция
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(COLOR_CYAN "Memory Profiler - Variant 1\n" COLOR_RESET);
        printf("Usage: %s <PID|NAME> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --watch [interval]     Monitor process continuously (default: 1 sec)\n");
        printf("  --compare <PID2>       Compare with another process\n");
        printf("  --map [limit]          Show detailed memory map (optional limit)\n");
        printf("  --summary              Show memory map summary\n");
        printf("  --find <name>          Find processes by name\n");
        printf("\nExamples:\n");
        printf("  %s 1234                # Show info for PID 1234\n", argv[0]);
        printf("  %s 1234 --watch        # Monitor PID 1234\n", argv[0]);
        printf("  %s 1234 --watch 5      # Monitor with 5 sec interval\n", argv[0]);
        printf("  %s 1234 --compare 5678 # Compare two processes\n", argv[0]);
        printf("  %s 1234 --map          # Show detailed memory map\n", argv[0]);
        printf("  %s 1234 --map 10       # Show first 10 memory segments\n", argv[0]);
        printf("  %s 1234 --summary      # Show memory map summary\n", argv[0]);
        printf("  %s --find chrome       # Find Chrome processes\n", argv[0]);
        printf("  %s bash                # Profile process by name\n", argv[0]);
        return 1;
    }

    // Парсинг аргументов
    int watch_mode = 0;
    int watch_interval = 1;
    int compare_mode = 0;
    pid_t compare_pid = 0;
    int show_map = 0;
    int map_limit = 0;
    int show_summary = 0;
    int find_mode = 0;
    char* find_name = NULL;
    pid_t target_pid = 0;
    char* target_name = NULL;

    // Определяем, первый аргумент - PID или имя
    char *endptr;
    target_pid = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        // Это не число, значит имя процесса
        target_name = argv[1];
    }

    // Поиск процессов по имени
    if (target_name && strcmp(target_name, "--find") == 0) {
        if (argc >= 3) {
            find_mode = 1;
            find_name = argv[2];
        } else {
            printf(COLOR_RED "Error: --find requires a process name\n" COLOR_RESET);
            return 1;
        }
    }

    // Парсинг остальных опций
    for (int i = (target_name ? 2 : 1); i < argc; i++) {
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
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                map_limit = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "--summary") == 0) {
            show_summary = 1;
        } else if (strcmp(argv[i], "--find") == 0 && i + 1 < argc) {
            find_mode = 1;
            find_name = argv[i + 1];
            i++;
        }
    }

    // Режим поиска процессов
    if (find_mode) {
        find_processes_by_name(find_name);
        return 0;
    }

    // Если указано имя процесса, находим PID
    if (target_name && !find_mode) {
        DIR *dir = opendir("/proc");
        if (!dir) {
            printf(COLOR_RED "Error: Cannot open /proc directory\n" COLOR_RESET);
            return 1;
        }

        struct dirent *entry;
        pid_t found_pid = 0;

        while ((entry = readdir(dir)) != NULL) {
            char *endptr;
            pid_t pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr != '\0') continue;

            char proc_name[256];
            if (get_process_name(pid, proc_name, sizeof(proc_name)) == 0) {
                if (strcmp(proc_name, target_name) == 0) {
                    if (found_pid == 0) {
                        found_pid = pid;
                    } else {
                        printf(COLOR_YELLOW "Warning: Multiple processes with name '%s', using PID %d\n" COLOR_RESET, 
                               target_name, pid);
                    }
                }
            }
        }

        closedir(dir);

        if (found_pid == 0) {
            printf(COLOR_RED "Error: No process found with name '%s'\n" COLOR_RESET, target_name);
            return 1;
        }

        target_pid = found_pid;
        printf(COLOR_CYAN "Found process '%s' with PID %d\n" COLOR_RESET, target_name, target_pid);
    }

    // Проверка существования процесса
    if (!process_exists(target_pid)) {
        printf(COLOR_RED "Error: Process %d does not exist or not accessible.\n" COLOR_RESET, target_pid);
        return 1;
    }

    // Выполнить нужный режим
    if (compare_mode) {
        if (!process_exists(compare_pid)) {
            printf(COLOR_RED "Error: Process %d does not exist or not accessible.\n" COLOR_RESET, compare_pid);
            return 1;
        }
        compare_processes(target_pid, compare_pid);
    } else if (watch_mode) {
        watch_process(target_pid, watch_interval);
    } else {
        // Обычный режим - показать информацию один раз
        print_process_info(target_pid);

        if (show_summary) {
            print_memory_map_summary(target_pid);
        }

        if (show_map) {
            print_detailed_memory_map(target_pid, map_limit);
        }
    }

    return 0;
}
