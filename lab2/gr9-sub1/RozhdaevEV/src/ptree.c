#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256

void print_process_tree(pid_t pid) {
    char path[64];
    char line[MAX_LINE];
    char name[64];
    pid_t ppid;
    FILE *file;
    
    while (pid > 1) {
 
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        file = fopen(path, "r");
        if (file == NULL) {
            printf(" ← [process %d not found]", pid);
            break;
        }
        
        ppid = -1;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line, "Name:\t%s", name);
            } else if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line, "PPid:\t%d", &ppid);
            }
        }
        fclose(file);
        
        if (ppid == -1) {
            printf("%s(%d)", name, pid);
            break;
        } else {
            printf("%s(%d) ← ", name, pid);
        }
        
        pid = ppid;
    }
    printf("init/systemd(1)\n");
}

int main() {
    pid_t current_pid = getpid();
    printf("Process tree for PID %d:\n", current_pid);
    print_process_tree(current_pid);
    return 0;
}
