#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE_1MB (1024 * 1024)

void print_memory_info(const char* phase) {
    printf("\n=== %s ===\n", phase);
    
    // Чтение /proc/self/status
    FILE *status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmSize:", 7) == 0 || 
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmPeak:", 7) == 0) {
                printf("%s", line);
            }
        }
        fclose(status);
    }
    
    // Чтение /proc/self/smaps_rollup для PSS
    FILE *smaps = fopen("/proc/self/smaps_rollup", "r");
    if (smaps) {
        char line[256];
        while (fgets(line, sizeof(line), smaps)) {
            if (strstr(line, "Pss:") || strstr(line, "Private")) {
                printf("%s", line);
            }
        }
        fclose(smaps);
    }
}

void print_memory_map() {
    printf("\n--- Memory Map ---\n");
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), maps) && count < 20) {
            printf("%s", line);
            count++;
        }
        fclose(maps);
    }
}

int main() {
    printf("PID: %d\n", getpid());
    
    // Начальное состояние
    print_memory_info("Initial State");
    
    // Память в стеке - ИСПОЛЬЗУЕМ переменную чтобы убрать warning
    char stack_array[1024 * 1024]; // 1MB в стеке
    stack_array[0] = 'X'; // Используем переменную
    print_memory_info("After stack allocation");
    
    // Память в куче через malloc
    char *heap_array = malloc(SIZE_1MB);
    print_memory_info("After malloc");
    
    // Инициализация памяти в куче (вызовет page faults)
    memset(heap_array, 'A', SIZE_1MB);
    print_memory_info("After memset malloc memory");
    
    // Память через mmap
    char *mmap_array = mmap(NULL, SIZE_1MB, PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_array == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    print_memory_info("After mmap");
    
    // Инициализация mmap памяти
    memset(mmap_array, 'B', SIZE_1MB);
    print_memory_info("After memset mmap memory");
    
    // Вывод карты памяти
    print_memory_map();
    
    // Ожидание для анализа
    printf("\nProcess waiting for 30 seconds... Check with:\n");
    printf("ps -o pid,comm,vsz,rss -p %d\n", getpid());
    printf("cat /proc/%d/smaps_rollup | grep -E 'Pss|Private'\n", getpid());
    
    printf("Press Enter to continue...");
    getchar(); // Ждем нажатия Enter вместо sleep
    
    // Освобождение памяти
    free(heap_array);
    munmap(mmap_array, SIZE_1MB);
    
    return 0;
}
