#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 1024
#define MAX_LINE 256

pid_t get_ppid(pid_t pid) {
    char path[MAX_PATH];
    char line[MAX_LINE];
    pid_t ppid = -1;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE *file = fopen(path, "r");
    if (file == NULL) return -1;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 5, "%d", &ppid);
            break;
        }
    }
    
    fclose(file);
    return ppid;
}

void get_process_name(pid_t pid, char *name, size_t name_size) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(name, name_size, "unknown");
        return;
    }
    
    if (fgets(name, name_size, file)) {
        name[strcspn(name, "\n")] = '\0';
    } else {
        snprintf(name, name_size, "unknown");
    }
    
    fclose(file);
}

void print_process_chain(pid_t pid) {
    if (pid <= 1) {
        char name[256];
        get_process_name(pid, name, sizeof(name));
        printf("%s(%d)", name, pid);
        return;
    }
    
    pid_t ppid = get_ppid(pid);
    if (ppid == -1) {
        printf("unknown(%d)", pid);
        return;
    }
    
    char name[256];
    get_process_name(pid, name, sizeof(name));
    
    print_process_chain(ppid);
    printf(" ← %s(%d)", name, pid);
}

int main(int argc, char *argv[]) {
    pid_t start_pid = (argc == 2) ? atoi(argv[1]) : getpid();
    
    printf("Process chain for PID %d: ", start_pid);
    print_process_chain(start_pid);
    printf("\n");
    
    return 0;
}
