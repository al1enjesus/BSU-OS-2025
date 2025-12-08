#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Добавлено для pid_t и getpid()
#include <sys/types.h> // Добавлено для pid_t

#define MAX_LINE 256

void print_process_chain(pid_t pid) {
    char path[50];
    char line[MAX_LINE];
    char name[100];
    pid_t ppid = 0;

    while (pid > 1) {
        // Формируем путь к файлу status
        snprintf(path, sizeof(path), "/proc/%d/status", pid);

        FILE *f = fopen(path, "r");
        if (!f) {
            perror("fopen");
            break;
        }

        // Ищем строки Name и PPid
        name[0] = '\0';
        ppid = 0;
        
        while (fgets(line, MAX_LINE, f)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line, "Name:\t%s", name);
            } else if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line, "PPid:\t%d", &ppid);
            }
        }
        fclose(f);

        if (name[0] != '\0') {
            printf("%s(%d)", name, pid);
            if (ppid > 1) {
                printf(" ~ ");
            }
        }
        pid = ppid;
    }
    printf(" systemd(1)\n");
}

int main() {
    pid_t my_pid = getpid();
    print_process_chain(my_pid);
    return 0;
}
