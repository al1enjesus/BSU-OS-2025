#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Чтение и вывод содержимого файла (с фильтром или без)
void print_file(const char *path, const char *filter) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (!filter || strstr(line, filter))
            printf("%s", line);
    }
    fclose(f);
}

// Вывод основных метрик памяти
void print_memory_info() {
    printf("\n=== /proc/self/status ===\n");
    print_file("/proc/self/status", "Vm");

    printf("\n=== /proc/self/smaps_rollup ===\n");
    print_file("/proc/self/smaps_rollup", NULL);
}

int main() {
    printf("PID процесса: %d\n", getpid());
    printf("Состояние памяти ДО выделения:\n");
    print_memory_info();

    printf("\n=== Выделяем память ===\n");
    char stack_var[1024]; // стек
    memset(stack_var, 0, sizeof(stack_var));

    char *heap_var = malloc(1024 * 1024); // 1 MB heap
    if (!heap_var) {
        perror("malloc");
        return 1;
    }
    memset(heap_var, 1, 1024 * 1024);

    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap");
        free(heap_var);
        return 1;
    }
    memset(mmap_var, 2, 1024 * 1024);

    printf("\nСостояние памяти ПОСЛЕ выделения:\n");
    print_memory_info();

    printf("\n=== Карта памяти процесса (/proc/self/maps) ===\n");
    print_file("/proc/self/maps", NULL);

    printf("\nНажмите Enter для завершения (можно проверить ps/top/htop)...");
    getchar();

    munmap(mmap_var, 1024 * 1024);
    free(heap_var);

    return 0;
}