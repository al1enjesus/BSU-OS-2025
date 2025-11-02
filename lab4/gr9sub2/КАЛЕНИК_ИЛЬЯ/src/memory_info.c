// memory_info.c - Анализ виртуальной памяти процесса с выделением памяти разными способами
// Компиляция: gcc -Wall -Wextra -O2 memory_info.c -o memory_info
// Запуск: ./memory_info [PID]

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen /proc/[PID]/status");
        return;
    }
    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &vm_stk) == 1) continue;
    }
    fclose(f);
    printf("Memory metrics for PID %d:\n", pid);
    printf("  VSZ (VmSize) : %lu MB\n", vm_size / 1024);
    printf("  RSS (VmRSS)  : %lu MB\n", vm_rss / 1024);
    printf("  Data (heap)  : %lu MB\n", vm_data / 1024);
    printf("  Stack        : %lu MB\n", vm_stk / 1024);
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen /proc/[PID]/maps");
        return;
    }
    char line[512];
    printf("Memory map for PID %d:\n", pid);
    printf("Address range        Permissions  Size (KB)  Path\n");
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5];
        char path[256] = "";
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %[^\n]", &start, &end, perms, path);
        printf("%08lx-%08lx  %s  %8lu  %s\n", start, end, perms, (end - start)/1024, path);
    }
    fclose(f);
}

void demonstrate_memory_types() {
    pid_t pid = getpid();

    // Локальная переменная на стеке
    char stack_var[1024] = {0};
    volatile char use_var = stack_var[0];
    (void)use_var; // Явно помечаем как использованную

    // Куча: выделяем 10 МБ, заполняем чтобы реально выделились страницы
    char *heap_var = malloc(10 * 1024 * 1024);
    if (!heap_var) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(heap_var, 1, 10 * 1024 * 1024);

    // mmap: выделяем 50 МБ анонимной памяти
    char *mmap_var = mmap(NULL, 50 * 1024 * 1024, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    memset(mmap_var, 2, 50 * 1024 * 1024);

    printf("After memory allocations:\n");
    print_memory_metrics(pid);
    print_memory_map(pid);

    // Пауза для просмотра процессов извне, можно запустить htop отдельно
    printf("Press Enter to free memory and exit...\n");
    getchar();

    free(heap_var);
    munmap(mmap_var, 50 * 1024 * 1024);
}

int main(int argc, char **argv) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        demonstrate_memory_types();
    }
    return 0;
}
