#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== Memory Profiler ===\n");
    printf("Дмитриева Полина, gr8sub1\n\n");
    
    // 1. Память на стеке
    printf("1. Выделяем память на стеке\n");
    int stack_var[1000];
    
    // 2. Память в куче
    printf("2. Выделяем память в куче (malloc)\n");
    int *heap_var = malloc(1000000); // 1 MB
    
    if (heap_var) {
        printf("   Выделено: 1 MB\n");
        // Используем память
        for (int i = 0; i < 1000; i++) {
            heap_var[i] = i;
        }
    }
    
    // 3. Покажем адреса памяти
    printf("\nАдреса памяти:\n");
    printf("Stack: %p\n", stack_var);
    printf("Heap:  %p\n", heap_var);
    
    // 4. Информация из /proc
    printf("\nИнформация о памяти:\n");
    system("grep -E 'VmSize|VmRSS' /proc/self/status");
    
    // Освобождаем память
    if (heap_var) {
        free(heap_var);
        printf("\nПамять освобождена\n");
    }
    
    return 0;
}
