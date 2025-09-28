#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 255
#define MAX_NAME 255
#define MAX_DEPTH 1024  // предотвращение бесконечной петли

// функция для чтения имени и PPid из /proc/<pid>/status
int read_status(int pid, char *name, int *ppid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[MAX_LINE + 1];  // +1 для нуль-терминатора
    *ppid = -1;
    name[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Name:", 5) == 0) {
            if (sscanf(line, "Name:\t%255s", name) != 1) {
                // если чтение не удалось
                name[0] = '\0';
            }
        } else if (strncmp(line, "PPid:", 5) == 0) {
            if (sscanf(line, "PPid:\t%d", ppid) != 1) {
                *ppid = -1;
            }
        }
    }

    fclose(f);
    return 0;
}

int main(void) {
    int pid = getpid();
    char name[MAX_NAME + 1];  // буфер для имени
    int ppid;

    int depth = 0;  // ограничение итераций
    while (pid > 0 && depth < MAX_DEPTH) {
        if (read_status(pid, name, &ppid) != 0) break;

        printf("%s(%d)", name[0] ? name : "unknown", pid);
        if (pid == 1 || ppid <= 0) break;

        printf(" ← ");
        pid = ppid;
        depth++;
    }

    if (depth == MAX_DEPTH) {
        fprintf(stderr, "\nWarning: maximum process depth reached, stopping loop.\n");
    }

    printf("\n");
    return 0;
}
