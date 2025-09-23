#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_LINE 256

// Функция для получения PPID из /proc/pid/status
pid_t get_ppid(pid_t pid) {
    char path[MAX_LINE];
    char line[MAX_LINE];
    FILE *file;
    pid_t ppid = 0;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 5, "%d", &ppid);
            break;
        }
    }
    
    fclose(file);
    return ppid;
}

// Функция для получения имени процесса из /proc/pid/comm
void get_process_name(pid_t pid, char *name, size_t size) {
    char path[MAX_LINE];
    FILE *file;
    
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    file = fopen(path, "r");
    if (!file) {
        snprintf(name, size, "unknown");
        return;
    }
    
    if (fgets(name, size, file)) {
        // Убираем символ новой строки
        name[strcspn(name, "\n")] = 0;
    } else {
        snprintf(name, size, "unknown");
    }
    
    fclose(file);
}

int main(int argc, char *argv[]) {
    pid_t current_pid;
    pid_t ppid;
    char process_name[MAX_LINE];
    
    if (argc == 2) {
        current_pid = atoi(argv[1]);
    } else {
        current_pid = getpid();
    }
    
    printf("Дерево процессов для PID %d:\n", current_pid);
    printf("═" * 50);
    printf("\n");
    
    while (current_pid > 1) {
        get_process_name(current_pid, process_name, sizeof(process_name));
        ppid = get_ppid(current_pid);
        
        printf("%s(%d)", process_name, current_pid);
        
        if (ppid > 0 && ppid != 1) {
            printf(" ← ");
        } else if (ppid == 1) {
            get_process_name(ppid, process_name, sizeof(process_name));
            printf(" ← %s(%d)\n", process_name, ppid);
            break;
        } else {
            printf(" (родитель недоступен)\n");
            break;
        }
        
        current_pid = ppid;
    }
    
    return 0;
}
