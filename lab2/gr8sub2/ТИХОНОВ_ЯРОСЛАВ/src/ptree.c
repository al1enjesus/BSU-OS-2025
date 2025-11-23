#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 255
#define MAX_NAME 255
#define MAX_DEPTH 256  // ограничение глубины для предотвращения бесконечного цикла

// Чтение имени и PPid из /proc/<pid>/status
int read_status(int pid, char *name, int *ppid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;  // процесс мог завершиться

    char line[MAX_LINE + 1];
    *ppid = -1;
    name[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Name:", 5) == 0) {
            if (sscanf(line, "Name:\t%255s", name) != 1) {
                // непредвиденный формат строки
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

// Альтернатива: получение имени процесса через /proc/<pid>/exe
int get_exe_name(int pid, char *name, size_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    ssize_t len = readlink(path, name, size - 1);
    if (len == -1) return -1;
    name[len] = '\0';
    return 0;
}

int main(void) {
    int pid = getpid();
    char name[MAX_NAME + 1];
    int ppid;

    int depth = 0;
    while (pid > 0 && depth < MAX_DEPTH) {
        // читаем статус
        if (read_status(pid, name, &ppid) != 0) {
            fprintf(stderr, "Process %d disappeared.\n", pid);
            break;
        }

        // если имя пустое, пробуем через exe
        if (name[0] == '\0') {
            if (get_exe_name(pid, name, sizeof(name)) != 0) {
                strncpy(name, "unknown", sizeof(name));
                name[sizeof(name) - 1] = '\0';
            }
        }

        printf("%s(%d)", name, pid);
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
