#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_chain(pid_t pid) {
    if (pid == 0) return;

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;

    char *line = NULL;
    size_t len = 0;
    pid_t ppid = 0;
    char comm[256] = "unknown";

    while (getline(&line, &len, f) != -1) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line + 5, "%255s", comm);
        }
        if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 5, "%d", &ppid);
        }
    }
    free(line);
    fclose(f);

    if (ppid != 0) {
        print_chain(ppid);
        printf(" ← ");
    }
    printf("%s(%d)", comm, pid);
}

int main(void) {
    print_chain(getpid());
    putchar('\n');
    return 0;
}
