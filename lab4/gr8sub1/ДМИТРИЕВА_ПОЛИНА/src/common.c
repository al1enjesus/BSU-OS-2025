#include "../include/common.h"

// Замер монотонного времени
double get_time_monotonic() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Замер CPU времени
double get_time_cpu() {
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Человеко-читаемый вывод размера
void print_size_human(unsigned long bytes) {
    if (bytes < 1024) {
        printf("%4lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        printf("%6.1f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        printf("%6.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        printf("%6.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

void print_size_human_fd(unsigned long bytes, FILE *stream) {
    if (bytes < 1024) {
        fprintf(stream, "%4lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        fprintf(stream, "%6.1f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        fprintf(stream, "%6.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        fprintf(stream, "%6.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// Получение статистики page faults
PageFaultStats get_page_faults() {
    PageFaultStats stats = {0, 0};
    struct rusage usage;
    
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats.minor_faults = usage.ru_minflt;
        stats.major_faults = usage.ru_majflt;
    }
    
    return stats;
}

// Вывод разницы в page faults
void print_page_fault_delta(const char *label, PageFaultStats start) {
    PageFaultStats end = get_page_faults();
    
    long delta_minor = end.minor_faults - start.minor_faults;
    long delta_major = end.major_faults - start.major_faults;
    
    printf("%s:\n", label);
    printf("  Minor faults: " COLOR_BLUE "%ld" COLOR_RESET " (+%ld)\n", 
           end.minor_faults, delta_minor);
    printf("  Major faults: " COLOR_BLUE "%ld" COLOR_RESET " (+%ld)\n", 
           end.major_faults, delta_major);
}

// Проверка результатов mmap
void check_mmap_result(void *ptr, size_t size) {
    if (ptr == MAP_FAILED) {
        fprintf(stderr, COLOR_RED "mmap failed for size %zu: %s\n" COLOR_RESET, 
                size, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

// Проверка результатов malloc
void check_malloc_result(void *ptr, size_t size) {
    if (!ptr) {
        fprintf(stderr, COLOR_RED "malloc failed for size %zu: %s\n" COLOR_RESET, 
                size, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
