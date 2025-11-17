
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

void print_proc_info(pid_t pid) {
    char path[256];
    char line[512];
    
    printf("=== Анализ памяти процесса PID: %d ===\n\n", pid);
    
    
    printf("Информация процесса:\n");
    printf("  PID    COMMAND         VSZ    RSS\n");
    
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (f) {
        printf("\n=== Детальная информация ===\n");
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmSize:", 7) == 0 || 
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmData:", 7) == 0 ||
                strncmp(line, "VmStk:", 6) == 0 ||
                strncmp(line, "Pid:", 4) == 0) {
                printf("  %s", line);
            }
        }
        fclose(f);
    }
    
  
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (f) {
        printf("\n=== Page Faults ===\n");
        unsigned long minflt, majflt;
        if (fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*lu %lu", &minflt, &majflt) == 2) {
            printf("  Minor: %lu\n", minflt);
            printf("  Major: %lu\n", majflt);
        }
        fclose(f);
    }
    
    
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    f = fopen(path, "r");
    if (f) {
        printf("\n=== Карта памяти (первые 10 строк) ===\n");
        int lines = 0;
        while (fgets(line, sizeof(line), f) && lines < 10) {
            printf("  %s", line);
            lines++;
        }
        fclose(f);
    }
}

int main() {
    printf("Задание A: Анализ виртуальной памяти процесса\n\n");
    
    pid_t pid = getpid();
    printf("Текущий PID: %d\n\n", pid);
    
    printf("1. Выделение памяти на стеке (1 KB)...\n");
    char stack_var[1024];
    if (memset(stack_var, 'S', sizeof(stack_var)) != stack_var) {
        fprintf(stderr, "Error: memset failed for stack\n");
        return 1;
    }

    printf("2. Выделение памяти в куче (1 MB)...\n");
    char *heap_var = malloc(1024 * 1024);
    if (!heap_var) {
        fprintf(stderr, "Error: malloc failed: %s\n", strerror(errno));
        return 1;
    }
    if (memset(heap_var, 'H', 1024 * 1024) != heap_var) {
        fprintf(stderr, "Error: memset failed for heap\n");
        free(heap_var);
        return 1;
    }
   
    printf("3. Выделение через mmap (1 MB)...\n");
    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed: %s\n", strerror(errno));
        free(heap_var);
        return 1;
    }
    if (memset(mmap_var, 'M', 1024 * 1024) != mmap_var) {
        fprintf(stderr, "Error: memset failed for mmap\n");
        free(heap_var);
        munmap(mmap_var, 1024 * 1024);
        return 1;
    }
    
    printf("\n Память выделена тремя способами:\n");
    printf("   - Стек: 1 KB локальной переменной\n");
    printf("   - Куча: 1 MB через malloc()\n");
    printf("   - MMAP: 1 MB через mmap()\n");
    
    // Show memory info after allocation
    print_proc_info(pid);
    
    printf("\n Нажмите Enter для освобождения памяти и выхода...\n");
    getchar();
    
    // Освобождение памяти
    printf("Освобождение памяти...\n");
    free(heap_var);
    
    if (munmap(mmap_var, 1024 * 1024) == -1) {
        fprintf(stderr, "Warning: munmap failed: %s\n", strerror(errno));
    }
    
    printf("Память освобождена. Программа завершена.\n");
    return 0;
}

