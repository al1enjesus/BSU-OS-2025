#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

typedef struct {
    unsigned long vm_size;
    unsigned long vm_rss;
    unsigned long vm_data;
    unsigned long vm_stk;
    unsigned long vm_exe;
    unsigned long vm_lib;
    unsigned long pss;
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
} MemoryMetrics;

int read_memory_metrics(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Не удалось открыть /proc/[PID]/status");
        return -1;
    }

    memset(metrics, 0, sizeof(MemoryMetrics));
    char line[256];

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

int read_pss(pid_t pid, MemoryMetrics *metrics) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    char line[256];
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

void print_size(unsigned long kb) {
    if (kb < 1024) {
        printf("%8lu КБ", kb);
    } else if (kb < 1024 * 1024) {
        printf("%8.1f МБ", kb / 1024.0);
    } else {
        printf("%8.2f ГБ", kb / (1024.0 * 1024.0));
    }
}

void print_colored_size(unsigned long kb, const char* label) {
    printf("%s%-20s" COLOR_RESET, COLOR_CYAN, label);
    
    if (kb < 10 * 1024) {
        printf(COLOR_GREEN);
    } else if (kb < 100 * 1024) {
        printf(COLOR_YELLOW);
    } else {
        printf(COLOR_RED);
    }
    
    print_size(kb);
    printf(COLOR_RESET "\n");
}

void print_bar_chart(unsigned long value, unsigned long max_value, int width) {
    int filled = (int)((double)value / max_value * width);
    if (filled > width) filled = width;
    
    printf("[");
    for (int i = 0; i < filled; i++) {
        printf("█");
    }
    for (int i = filled; i < width; i++) {
        printf("░");
    }
    printf("]");
}

void print_memory_breakdown(MemoryMetrics *metrics) {
    printf("\n" COLOR_BOLD "=== График использования памяти ===" COLOR_RESET "\n\n");
    
    unsigned long total = metrics->vm_rss;
    if (total == 0) total = 1;
    
    printf("%-15s ", "Данные/Куча:");
    print_bar_chart(metrics->vm_data, total, 40);
    printf(" "); print_size(metrics->vm_data); printf("\n");
    
    printf("%-15s ", "Стек:");
    print_bar_chart(metrics->vm_stk, total, 40);
    printf(" "); print_size(metrics->vm_stk); printf("\n");
    
    printf("%-15s ", "Код:");
    print_bar_chart(metrics->vm_exe, total, 40);
    printf(" "); print_size(metrics->vm_exe); printf("\n");
    
    printf("%-15s ", "Библиотеки:");
    print_bar_chart(metrics->vm_lib, total, 40);
    printf(" "); print_size(metrics->vm_lib); printf("\n");
}

void print_memory_map(pid_t pid, int limit) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Не удалось открыть /proc/[PID]/maps");
        return;
    }

    printf("\n" COLOR_BOLD "Карта памяти (первые %d сегментов):" COLOR_RESET "\n", limit);
    printf("%-20s %-6s %-12s %s\n", "Адрес", "Права", "Размер", "Путь");
    printf("-------------------------------------------------------------\n");

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < limit) {
        unsigned long start, end;
        char perms[5], pathname[256] = "";
        
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, pathname) >= 3) {
            unsigned long size_kb = (end - start) / 1024;
            
            if (strstr(perms, "x")) {
                printf(COLOR_GREEN);
            } else if (strstr(perms, "w")) {
                printf(COLOR_YELLOW);
            } else {
                printf(COLOR_RESET);
            }
            
            printf("%08lx-%08lx %-6s ", start, end, perms);
            print_size(size_kb);
            printf(" %s" COLOR_RESET "\n", pathname[0] ? pathname : "[анонимная]");
            count++;
        }
    }

    fclose(f);
}

void analyze_shared_libraries(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;

    printf("\n" COLOR_BOLD "=== Анализ разделяемых библиотек ===" COLOR_RESET "\n\n");
    
    char line[512];
    char libs[100][256];
    unsigned long lib_sizes[100];
    int lib_count = 0;
    
    while (fgets(line, sizeof(line), f) && lib_count < 100) {
        unsigned long start, end;
        char perms[5], pathname[256] = "";
        
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, pathname) >= 3) {
            if (pathname[0] && strstr(pathname, ".so")) {
                int found = 0;
                for (int i = 0; i < lib_count; i++) {
                    if (strcmp(libs[i], pathname) == 0) {
                        lib_sizes[i] += (end - start) / 1024;
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strncpy(libs[lib_count], pathname, 255);
                    lib_sizes[lib_count] = (end - start) / 1024;
                    lib_count++;
                }
            }
        }
    }
    
    for (int i = 0; i < lib_count && i < 10; i++) {
        printf("%-50s ", libs[i]);
        print_size(lib_sizes[i]);
        printf("\n");
    }
    
    fclose(f);
}

void watch_mode(pid_t pid) {
    printf(COLOR_BOLD "=== Режим мониторинга PID %d ===" COLOR_RESET "\n", pid);
    printf("(Нажмите Ctrl+C для выхода)\n\n");
    
    MemoryMetrics prev = {0};
    
    while (1) {
        MemoryMetrics current;
        if (read_memory_metrics(pid, &current) != 0) {
            printf(COLOR_RED "Процесс завершён\n" COLOR_RESET);
            break;
        }
        read_pss(pid, &current);
        
        printf("\r");
        printf(COLOR_CYAN "RSS: " COLOR_RESET);
        print_size(current.vm_rss);
        
        if (prev.vm_rss > 0) {
            long delta = (long)current.vm_rss - (long)prev.vm_rss;
            if (delta > 0) {
                printf(" " COLOR_GREEN "(+%ld КБ)" COLOR_RESET, delta);
            } else if (delta < 0) {
                printf(" " COLOR_RED "(%ld КБ)" COLOR_RESET, delta);
            }
        }
        
        printf("  |  ");
        printf(COLOR_CYAN "VSZ: " COLOR_RESET);
        print_size(current.vm_size);
        
        fflush(stdout);
        prev = current;
        sleep(1);
    }
}

void compare_mode(pid_t pid1, pid_t pid2) {
    MemoryMetrics m1, m2;
    
    if (read_memory_metrics(pid1, &m1) != 0 || read_memory_metrics(pid2, &m2) != 0) {
        printf(COLOR_RED "Ошибка чтения метрик\n" COLOR_RESET);
        return;
    }
    read_pss(pid1, &m1);
    read_pss(pid2, &m2);
    
    printf(COLOR_BOLD "=== Сравнение процессов ===" COLOR_RESET "\n\n");
    printf("%-20s %15s %15s %15s\n", "Метрика", "PID 1", "PID 2", "Разница");
    printf("----------------------------------------------------------------\n");
    
    printf("%-20s ", "VSZ:");
    print_size(m1.vm_size); printf("  ");
    print_size(m2.vm_size); printf("  ");
    long delta = (long)m1.vm_size - (long)m2.vm_size;
    if (delta > 0) printf(COLOR_GREEN "+");
    else if (delta < 0) printf(COLOR_RED);
    printf("%ld КБ" COLOR_RESET "\n", delta);
    
    printf("%-20s ", "RSS:");
    print_size(m1.vm_rss); printf("  ");
    print_size(m2.vm_rss); printf("  ");
    delta = (long)m1.vm_rss - (long)m2.vm_rss;
    if (delta > 0) printf(COLOR_GREEN "+");
    else if (delta < 0) printf(COLOR_RED);
    printf("%ld КБ" COLOR_RESET "\n", delta);
    
    unsigned long uss1 = m1.private_clean + m1.private_dirty;
    unsigned long uss2 = m2.private_clean + m2.private_dirty;
    printf("%-20s ", "USS:");
    print_size(uss1); printf("  ");
    print_size(uss2); printf("  ");
    delta = (long)uss1 - (long)uss2;
    if (delta > 0) printf(COLOR_GREEN "+");
    else if (delta < 0) printf(COLOR_RED);
    printf("%ld КБ" COLOR_RESET "\n", delta);
}

void demonstrate_memory_types(void) {
    printf("\n" COLOR_BOLD "=== Демонстрация выделения памяти ===" COLOR_RESET "\n\n");
    
    MemoryMetrics before, after;
    read_memory_metrics(getpid(), &before);
    
    printf(COLOR_CYAN "ДО выделения:\n" COLOR_RESET);
    print_colored_size(before.vm_size, "VSZ:");
    print_colored_size(before.vm_rss, "RSS:");
    printf("\n");

    char stack_var[1024 * 1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf(COLOR_GREEN "✓" COLOR_RESET " Стек: Выделено и затронуто 1 МБ\n");

    size_t heap_size = 10 * 1024 * 1024;
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc не удался");
        return;
    }
    memset(heap_var, 'H', heap_size);
    printf(COLOR_GREEN "✓" COLOR_RESET " Куча: Выделено и затронуто 10 МБ через malloc\n");

    size_t mmap_size = 50 * 1024 * 1024;
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap не удался");
        free(heap_var);
        return;
    }
    memset(mmap_var, 'M', mmap_size);
    printf(COLOR_GREEN "✓" COLOR_RESET " Mmap: Выделено и затронуто 50 МБ через mmap\n");

    read_memory_metrics(getpid(), &after);
    
    printf("\n" COLOR_CYAN "ПОСЛЕ выделения:\n" COLOR_RESET);
    print_colored_size(after.vm_size, "VSZ:");
    printf("                     " COLOR_YELLOW "(дельта: %+ld КБ)" COLOR_RESET "\n", 
           (long)after.vm_size - (long)before.vm_size);
    print_colored_size(after.vm_rss, "RSS:");
    printf("                     " COLOR_YELLOW "(дельта: %+ld КБ)" COLOR_RESET "\n", 
           (long)after.vm_rss - (long)before.vm_rss);

    print_memory_breakdown(&after);

    if (read_pss(getpid(), &after) == 0) {
        printf("\n" COLOR_BOLD "=== Детальная разбивка памяти (PSS) ===" COLOR_RESET "\n\n");
        print_colored_size(after.pss, "PSS (пропорциональная):");
        unsigned long uss = after.private_clean + after.private_dirty;
        print_colored_size(uss, "USS (уникальная):");
        print_colored_size(after.shared_clean + after.shared_dirty, "Разделяемая:");
        print_colored_size(after.private_clean + after.private_dirty, "Приватная:");
    }

    print_memory_map(getpid(), 15);
    analyze_shared_libraries(getpid());

    free(heap_var);
    munmap(mmap_var, mmap_size);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--watch") == 0 && argc > 2) {
            watch_mode(atoi(argv[2]));
            return 0;
        }
        
        if (strcmp(argv[1], "--compare") == 0 && argc > 3) {
            compare_mode(atoi(argv[2]), atoi(argv[3]));
            return 0;
        }
        
        pid_t pid = atoi(argv[1]);
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        
        if (access(path, F_OK) != 0) {
            fprintf(stderr, COLOR_RED "Ошибка: Процесс %d не существует\n" COLOR_RESET, pid);
            return 1;
        }

        MemoryMetrics metrics;
        read_memory_metrics(pid, &metrics);
        read_pss(pid, &metrics);

        printf(COLOR_BOLD "=== Анализ процесса: PID %d ===" COLOR_RESET "\n\n", pid);
        print_colored_size(metrics.vm_size, "VSZ (виртуальная):");
        print_colored_size(metrics.vm_rss, "RSS (резидентная):");
        print_colored_size(metrics.vm_data, "Данные+Куча:");
        print_colored_size(metrics.vm_stk, "Стек:");
        print_colored_size(metrics.vm_exe, "Текст (код):");
        print_colored_size(metrics.vm_lib, "Библиотеки:");
        
        if (metrics.pss > 0) {
            print_colored_size(metrics.pss, "PSS:");
        }

        print_memory_breakdown(&metrics);
        print_memory_map(pid, 20);
        analyze_shared_libraries(pid);
    } else {
        printf(COLOR_BOLD COLOR_CYAN "Профайлер памяти - Лаб 4 Задание A (Расширенная версия)\n" COLOR_RESET);
        printf("=================================================================\n\n");
        printf("Использование:\n");
        printf("  %s                    # Демонстрация\n", argv[0]);
        printf("  %s <PID>              # Анализ процесса\n", argv[0]);
        printf("  %s --watch <PID>      # Мониторинг в реальном времени\n", argv[0]);
        printf("  %s --compare PID1 PID2 # Сравнение двух процессов\n\n", argv[0]);
        
        demonstrate_memory_types();
    }

    return 0;
}
