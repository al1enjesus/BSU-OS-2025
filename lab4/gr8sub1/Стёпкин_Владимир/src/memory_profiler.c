#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

void print_memory_info(const char *stage) {
    printf("\n=== %s ===\n", stage);
    system("grep -E 'VmSize|VmRSS' /proc/self/status");
    system("grep -E 'Pss|Private' /proc/self/smaps_rollup");
}

int main() {
    printf("PID: %d\n", getpid());
    getchar(); // Пауза для запуска 'ps' вручную

    char stack_var[1024]; // Стек
    print_memory_info("After stack allocation");

    char *heap_var = malloc(1024 * 1024); // 1 MB в куче
    if (!heap_var) {
        perror("malloc");
        return 1;
    }
    print_memory_info("After malloc (1 MB)");

    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    print_memory_info("After mmap (1 MB)");

    printf("\n=== /proc/self/maps ===\n");
    system("cat /proc/self/maps | head -n 20");

    getchar(); // Пауза перед выходом
    free(heap_var);
    munmap(mmap_var, 1024 * 1024);
    return 0;
}
