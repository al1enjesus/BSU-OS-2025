#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void print_chain(pid_t pid) {
    char path[100], line[256];
    FILE *f;
    pid_t ppid;
    
    while (pid > 1) {
        // Читаем PPID из /proc/pid/status
        sprintf(path, "/proc/%d/status", pid);
        f = fopen(path, "r");
        if (!f) break;
        
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "PPid:\t%d", &ppid) == 1) {
                printf("%d", pid);
                if (ppid > 1) printf(" ← ");
                pid = ppid;
                break;
            }
        }
        fclose(f);
    }
    printf(" ← 1\n");
}

int main() {
    printf("Process chain: ");
    print_chain(getpid());
    return 0;
}