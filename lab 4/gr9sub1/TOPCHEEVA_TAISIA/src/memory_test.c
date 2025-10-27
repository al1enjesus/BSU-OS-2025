
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

void print_memory_info(pid_t pid) {
    printf(" Анализ памяти процесса PID: %d \n\n", pid);
    
    char command[256];
    
    // Basic process info
    snprintf(command, sizeof(command), "ps -o pid,comm,vsz,rss,pmem -p %d", pid);
    printf("Информация процесса:\n");
    system(command);
    
    printf("\nДетальная информация \n");
    snprintf(command, sizeof(command), "cat /proc/%d/status | grep -E '^(Vm|Pid)'", pid);
    system(command);
    
    printf("\n=== Page Faults ===\n");
    snprintf(command, sizeof(command), "cat /proc/%d/stat | awk '{print \"Minor:\" $10 \" Major:\" $12}'", pid);
    system(command);
    
    printf("\n=== Карта памяти (первые 15 строк) ===\n");
    snprintf(command, sizeof(command), "cat /proc/%d/maps | head -15", pid);
    system(command);
}

int main() {
    printf("Задание A: Анализ виртуальной памяти процесса\n\n");
    
    pid_t pid = getpid();
    printf("Текущий PID: %d\n\n", pid);
    
    printf("1. Выделение памяти на стеке (1 KB)...\n");
    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));

    printf("2. Выделение памяти в куче (1 MB)...\n");
    char *heap_var = malloc(1024 * 1024);
    if (!heap_var) {
        fprintf(stderr, "Error: malloc failed: %s\n", strerror(errno));
        return 1;
    }
    memset(heap_var, 'H', 1024 * 1024);
   
    printf("3. Выделение через mmap (1 MB)...\n");
    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed: %s\n", strerror(errno));
        free(heap_var);
        return 1;
    }
    memset(mmap_var, 'M', 1024 * 1024);
    
    printf("\n Память выделена тремя способами:\n");
    printf("   - Стек: 1 KB локальной переменной\n");
    printf("   - Куча: 1 MB через malloc()\n");
    printf("   - MMAP: 1 MB через mmap()\n");
    
    // Show memory info after allocation
    print_memory_info(pid);
    
    printf("\n Для дополнительного анализа выполните в другом терминале:\n");
    printf("   ps -o pid,comm,vsz,rss -p %d\n", pid);
    printf("   cat /proc/%d/status | grep -E '^(Vm|Pid)'\n", pid);
    printf("   cat /proc/%d/maps | head -20\n", pid);
    
    printf("\n Нажмите Enter для освобождения памяти и выхода...\n");
    getchar();
    
    // Освобождение памяти
    printf("Освобождение памяти...\n");
    free(heap_var);
    
    if (munmap(mmap_var, 1024 * 1024) == -1) {
        fprintf(stderr, "Warning: munmap failed: %s\n", strerror(errno));
    }
    
    printf(" Память освобождена. Программа завершена.\n");
    return 0;
}

