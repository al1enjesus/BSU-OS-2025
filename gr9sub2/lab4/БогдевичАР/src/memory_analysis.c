#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void print_memory_info(const char* phase) {
    printf("\n=== %s ===\n", phase);
    
    // Читаем информацию из /proc/self/status
    FILE *status_file = fopen("/proc/self/status", "r");
    if (status_file) {
        char line[256];
        while (fgets(line, sizeof(line), status_file)) {
            if (strncmp(line, "VmSize:", 7) == 0 || 
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmPeak:", 7) == 0) {
                printf("%s", line);
            }
        }
        fclose(status_file);
    }
}

void print_maps_summary() {
    printf("\n--- Карта памяти (фрагмент) ---\n");
    FILE *maps_file = fopen("/proc/self/maps", "r");
    if (maps_file) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), maps_file) && count < 10) {
            printf("%s", line);
            count++;
        }
        fclose(maps_file);
    }
}

int main() {
    printf("🚀 Задание A: Анализ виртуальной памяти процесса\n");
    printf("PID: %d\n", getpid());
    
    // Фаза 1: Исходное состояние
    print_memory_info("Исходное состояние");
    
    // Выделение памяти в стеке
    printf("\n>>> Выделяем 1 KB в стеке...\n");
    char stack_var[1024];
    strcpy(stack_var, "stack memory");
    
    // Фаза 2: После стека
    print_memory_info("После выделения в стеке");
    
    // Выделение в heap
    printf("\n>>> Выделяем 1 MB в heap...\n");
    char *heap_var = malloc(1024 * 1024);
    if (heap_var) {
        strcpy(heap_var, "heap memory");
    }
    
    // Фаза 3: После heap
    print_memory_info("После выделения в heap");
    
    // Выделение через mmap
    printf("\n>>> Выделяем 1 MB через mmap...\n");
    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var != MAP_FAILED) {
        strcpy(mmap_var, "mmap memory");
    }
    
    // Фаза 4: После всех выделений
    print_memory_info("После всех выделений");
    print_maps_summary();
    
    // Ожидание для проверки
    printf("\n⏳ Программа работает. Проверьте в другом терминале:\n");
    printf("ps -o pid,comm,vsz,rss -p %d\n", getpid());
    printf("Нажмите Enter для завершения...");
    getchar();
    
    // Освобождение памяти
    if (heap_var) free(heap_var);
    if (mmap_var != MAP_FAILED) munmap(mmap_var, 1024 * 1024);
    
    printf("✅ Задание A завершено!\n");
    return 0;
}
