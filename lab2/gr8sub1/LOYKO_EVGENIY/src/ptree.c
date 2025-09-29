#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_PATH 1024
#define MAX_LINE 256

char* get_process_name(pid_t pid) {
    static char name[256];
    char path[MAX_PATH];
    FILE* file;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    
    if (file == NULL) {
        snprintf(name, sizeof(name), "unknown(%d)", pid);
        return name;
    }
    
    if (fgets(name, sizeof(name), file) != NULL) {
        char* colon = strchr(name, ':');
        if (colon != NULL) {
            char* process_name = colon + 1;
            while (*process_name == ' ' || *process_name == '\t') {
                process_name++;
            }
            char* newline = strchr(process_name, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
            fclose(file);
            return process_name;
        }
    }
    
    fclose(file);
    snprintf(name, sizeof(name), "process(%d)", pid);
    return name;
}

pid_t get_parent_pid(pid_t pid) {
    char path[MAX_PATH];
    FILE* file;
    char line[MAX_LINE];
    pid_t ppid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    
    if (file == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 5, "%d", &ppid);
            break;
        }
    }
    
    fclose(file);
    return ppid;
}

int main(int argc, char* argv[]) {
    pid_t current_pid;
    pid_t parent_pid;
    
    if (argc == 2) {
        current_pid = atoi(argv[1]);
    } else {
        current_pid = getpid();
    }
    
    printf("Process tree for PID %d:\n", current_pid);
    
    while (current_pid > 1) {
        printf("%s(%d)", get_process_name(current_pid), current_pid);
        
        parent_pid = get_parent_pid(current_pid);
        if (parent_pid > 0 && parent_pid != current_pid) {
            printf(" ← ");
            current_pid = parent_pid;
        } else {
            break;
        }
    }
    
    printf("%s(%d)\n", get_process_name(current_pid), current_pid);
    
    return 0;
}
