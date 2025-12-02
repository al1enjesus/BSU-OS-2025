#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

void print_memory_info(const char* stage) {
    printf("\n=== %s ===\n", stage);
    
    // Чтение информации о памяти из /proc/self/status
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmSize:", 7) == 0 || 
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmData:", 7) == 0 ||
                strncmp(line, "VmStk:", 6) == 0 ||
                strncmp(line, "VmExe:", 6) == 0 ||
                strncmp(line, "VmLib:", 6) == 0) {
                printf("%s", line);
            }
        }
        fclose(status);
    }
    
    // Чтение сводной информации из smaps_rollup (если доступно)
    printf("\nДетальная информация (smaps_rollup):\n");
    FILE* smaps = fopen("/proc/self/smaps_rollup", "r");
    if (smaps) {
        char line[512];
        while (fgets(line, sizeof(line), smaps)) {
            if (strncmp(line, "Pss:", 4) == 0 ||
                strncmp(line, "Private_Clean:", 14) == 0 ||
                strncmp(line, "Private_Dirty:", 14) == 0 ||
                strncmp(line, "Swap:", 5) == 0) {
                printf("%s", line);
            }
        }
        fclose(smaps);
    }
}

void print_memory_map_sample() {
    printf("\nКарта памяти (первые 15 строк):\n");
    printf("Адрес           Права Смещение Устр. Inode Путь\n");
    printf("------------------------------------------------\n");
    
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), maps) && count < 15) {
            printf("%s", line);
            count++;
        }
        fclose(maps);
    }
}

int main() {
    printf("Анализ виртуальной памяти процесса\n");
    printf("PID: %d\n", getpid());
    
    print_memory_info("Начальное состояние");
    print_memory_map_sample();
    
    // === 1. Выделение памяти в стеке ===
    printf("\n==================================================\n");
    printf("1. ВЫДЕЛЕНИЕ ПАМЯТИ В СТЕКЕ (64 KB)\n");
    char stack_array[1024 * 64]; // 64 KB в стеке
    strcpy(stack_array, "Тестовая строка в стеке");
    
    print_memory_info("После выделения в стеке");
    
    // === 2. Выделение в куче через malloc ===
    printf("\n==================================================\n");
    printf("2. ВЫДЕЛЕНИЕ В КУЧЕ ЧЕРЕЗ MALLOC (1 MB)\n");
    char* heap_memory = malloc(1024 * 1024); // 1 MB в куче
    if (heap_memory) {
        // Инициализируем только часть памяти (ленивое выделение)
        strcpy(heap_memory, "Тестовая строка в куче");
        for (int i = 0; i < 4096; i++) { // Записываем в первые 4 KB
            heap_memory[i] = 'A' + (i % 26);
        }
        print_memory_info("После malloc(1MB) + частичная инициализация");
    }
    
    // === 3. Выделение через mmap ===
    printf("\n==================================================\n");
    printf("3. ВЫДЕЛЕНИЕ ЧЕРЕЗ MMAP (1 MB)\n");
    char* mmap_memory = mmap(NULL, 1024 * 1024, 
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_memory != MAP_FAILED) {
        strcpy(mmap_memory, "Тестовая строка в mmap");
        // Инициализируем всю mmap-память
        for (int i = 0; i < 1024 * 1024; i++) {
            mmap_memory[i] = 'B' + (i % 26);
        }
        print_memory_info("После mmap(1MB) + полная инициализация");
        print_memory_map_sample();
    }
    
    // === 4. Еще одно выделение в куче ===
    printf("\n==================================================\n");
    printf("4. ДОПОЛНИТЕЛЬНОЕ ВЫДЕЛЕНИЕ В КУЧЕ (2 MB)\n");
    char* more_heap = malloc(2 * 1024 * 1024); // 2 MB
    if (more_heap) {
        strcpy(more_heap, "Дополнительная куча");
        print_memory_info("После дополнительного malloc(2MB)");
    }
    
    // === Освобождение памяти ===
    printf("\n==================================================\n");
    printf("5. ОСВОБОЖДЕНИЕ ПАМЯТИ\n");
    
    if (heap_memory) {
        free(heap_memory);
    }
    if (more_heap) {
        free(more_heap);
    }
    if (mmap_memory != MAP_FAILED) {
        munmap(mmap_memory, 1024 * 1024);
    }
    
    print_memory_info("После освобождения всей памяти");
    
    // === Пауза для анализа ===
    printf("\n==================================================\n");
    printf("Ожидание 20 секунд для анализа извне...\n");
    printf("Запустите в другом терминале:\n");
    printf("ps -o pid,comm,vsz,rss,pmem -p %d\n", getpid());
    printf("cat /proc/%d/maps | head -20\n", getpid());
    sleep(20);
    
    printf("\nПрограмма завершена.\n");
    return 0;
}
