#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_chain(pid_t pid) {
    char proc_name[256];
    char path[256];
    FILE *f;
    char line[256];
    pid_t ppid;
    int first = 1;

    while (pid != 1 && pid != 0) {
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        f = fopen(path, "r");
        if (f) {
            if (fgets(proc_name, sizeof(proc_name), f)) {
                proc_name[strcspn(proc_name, "\n")] = 0;
            }
            fclose(f);
        } else {
            strcpy(proc_name, "unknown");
        }

        if (first) {
            printf("%s(%d)", proc_name, pid);
            first = 0;
        } else {
            printf(" ← %s(%d)", proc_name, pid);
        }

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        f = fopen(path, "r");
        if (!f) break;

        ppid = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "PPid:", 5) == 0) {
                ppid = atoi(line + 5);
                break;
            }
        }
        fclose(f);

        if (ppid == 0) break;
        pid = ppid;
    }

    if (pid == 1) {
        snprintf(path, sizeof(path), "/proc/1/comm", pid);
        f = fopen(path, "r");
        if (f) {
            if (fgets(proc_name, sizeof(proc_name), f)) {
                proc_name[strcspn(proc_name, "\n")] = 0;
            }
            fclose(f);
        } else {
            strcpy(proc_name, "systemd");
        }
        printf(" ← %s(1)", proc_name);
    }

    printf("\n");
}

int main() {
    print_chain(getpid());
    return 0;
}
