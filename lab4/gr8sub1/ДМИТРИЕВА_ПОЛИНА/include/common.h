#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>

// Цветной вывод
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// Размер страницы
#define PAGE_SIZE 4096

// Функции для замера времени
double get_time_monotonic();

// Функции для работы с памятью
void print_size_human(unsigned long bytes);

// Page faults
typedef struct {
    long minor_faults;
    long major_faults;
} PageFaultStats;

PageFaultStats get_page_faults();

// Проверка ошибок
void check_mmap_result(void *ptr, size_t size);

#endif
