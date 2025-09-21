#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h> 
#include <errno.h> 
#include <limits.h>

#define MAX_LINE 256
#define MAX_CHAIN 100

pid_t get_ppid(pid_t pid) {
    char path[64];
    char line[MAX_LINE];
    FILE *file;
    pid_t ppid = -1;
    char *endptr;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            long val = strtol(line + 5, &endptr, 10);
            
            if (endptr == line + 5) {
                fprintf(stderr, "Ошибка: не удалось преобразовать PPid для процесса %d\n", pid);
                ppid = -1;
            } else if (val < 0 || val > INT_MAX) {
                fprintf(stderr, "Ошибка: некорректное значение PPid %ld для процесса %d\n", val, pid);
                ppid = -1;
            } else {
                ppid = (pid_t)val;
            }
            break;
        }
    }
    
    fclose(file);
    return ppid;
}

void get_process_name(pid_t pid, char *buffer, size_t size) {
    char path[64];
    FILE *file;
    
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    
    file = fopen(path, "r");
    if (file == NULL) {
        snprintf(buffer, size, "[unknown]");
        return;
    }
    
    if (fgets(buffer, size, file)) {
        buffer[strcspn(buffer, "\n")] = '\0';
    } else {
        snprintf(buffer, size, "[unknown]");
    }
    
    fclose(file);
}

int main() {
    pid_t chain[MAX_CHAIN];
    int chain_length = 0;
    pid_t current_pid = getpid();
    
    printf("Цепочка родителей от текущего процесса (%d) до init/systemd (PID 1):\n", current_pid);
    printf("──────────────────────────────────────────────────────────────\n");
    
    chain[chain_length++] = current_pid;
    
    while (current_pid != 1 && chain_length < MAX_CHAIN) {
        current_pid = get_ppid(current_pid);
        if (current_pid <= 0) {
            break;
        }
        chain[chain_length++] = current_pid;
    }
    
    for (int i = 0; i < chain_length; i++) {
        char name[64];
        get_process_name(chain[i], name, sizeof(name));
        
        for (int j = 0; j < i; j++) {
            printf("  ");
        }
        printf("└─ %s (%d)\n", name, chain[i]);
    }
    return 0;
}
