#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    pid_t pid = argc == 1 ? getpid() : atoi(argv[1]);
    
    while (pid > 1) {
        char path[64], line[256], name[64];
        FILE *f;
        
        sprintf(path, "/proc/%d/status", pid);
        f = fopen(path, "r");
        if (!f) break;
        
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Name:\t%s", name) == 1) {
                printf("%s(%d) ← ", name, pid);
                break;
            }
        }
        fclose(f);
        
        sprintf(path, "/proc/%d/stat", pid);
        f = fopen(path, "r");
        if (!f) break;
        
        fscanf(f, "%*d %*s %*c %d", &pid);
        fclose(f);
    }
    
    printf("init(1)\n");
    return 0;
}
