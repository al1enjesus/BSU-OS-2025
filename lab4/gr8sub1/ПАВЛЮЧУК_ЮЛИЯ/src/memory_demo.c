#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {
    printf("=== Memory Demo Process ===\n");
    printf("PID: %d\n", getpid());
    
    // Разные типы памяти как в задании
    char stack_var[1024];                    // Стек
    char *heap_var = malloc(1024*1024);      // Heap - 1MB
    void *mmap_var = mmap(NULL, 1024*1024, PROT_READ|PROT_WRITE, 
                         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0); // mmap - 1MB
    
    printf("Memory allocated:\n");
    printf("  Stack:  %p\n", (void*)stack_var);
    printf("  Heap:   %p\n", (void*)heap_var); 
    printf("  MMAP:   %p\n", mmap_var);
    
    // Использование памяти (вызовет page faults)
    printf("Touching memory...\n");
    memset(stack_var, 'S', sizeof(stack_var));
    memset(heap_var, 'H', 1024*1024);
    memset(mmap_var, 'M', 1024*1024);
    
    printf("Ready for analysis. Run: ./memory_analyzer %d --map --libs\n", getpid());
    printf("This process will exit in 60 seconds...\n");
    
    sleep(60);  // Ждем 60 секунд для анализа
    
    // Освобождение
    free(heap_var);
    munmap(mmap_var, 1024*1024);
    
    printf("Demo finished.\n");
    return 0;
}
