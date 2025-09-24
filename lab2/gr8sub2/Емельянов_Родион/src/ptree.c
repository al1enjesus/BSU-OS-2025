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
    FILE *fp;
    
    while (pid > 1) {
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        fp = fopen(path, "r");
        if (!fp) break;
        
        ppid = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line, "Name:\t%s", name);
            } else if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line, "PPid:\t%d", &ppid);
            }
        }
        fclose(fp);
        
        printf("%s(%d)", name, pid);
        if (ppid > 0) {
            printf(" ← ");
        }
        pid = ppid;
    }
    printf("systemd(1)\n");
}

int main(int argc, char *argv[]) {
    pid_t pid;
    
    if (argc == 2) {
        pid = atoi(argv[1]);
    } else {
        pid = getpid();
    }
    
    printf("Process tree for PID %d: ", pid);
    print_process_tree(pid);
    
    return 0;
}