
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_ptree(pid_t pid) {
    char path[256], name[256], line[512];
    pid_t current_pid = pid;
    pid_t parent_pid;
    int first = 1;
    
    while (current_pid > 1) {
        sprintf(path, "/proc/%d/status", current_pid);
        FILE *f = fopen(path, "r");
        
        if (!f) break;
        
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Name:\t%s", name) == 1) break;
        }
        
        parent_pid = 0;
        rewind(f);
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "PPid:\t%d", &parent_pid) == 1) break;
        }
        
        fclose(f);
        
        if (!first) printf(" ← ");
        printf("%s(%d)", name, current_pid);
        first = 0;
        
        current_pid = parent_pid;
    }
    
    if (current_pid == 1) {
        printf(" ← systemd(1)");
    }
    
    printf("\n");
}

int main() {
    print_ptree(getpid());
    return 0;
}