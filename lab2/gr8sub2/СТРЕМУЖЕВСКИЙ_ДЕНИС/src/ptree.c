#define _XOPEN_SOURCE 700
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(NO_UTF8_ARROWS)
#  define ARROW " <- "
#else
#  define ARROW " \u2190 "
#endif

static int read_proc_status(pid_t pid, char *name, size_t name_sz, pid_t *ppid_out) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int have_name = 0, have_ppid = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (!have_name && strncmp(line, "Name:", 5) == 0) {
            char tmp[128];
            if (sscanf(line, "Name:%127s", tmp) == 1) {
                if (name_sz > 0) {
                    snprintf(name, name_sz, "%s", tmp);
                }
                have_name = 1;
            }
        } else if (!have_ppid && strncmp(line, "PPid:", 5) == 0) {
            long p = 0;
            if (sscanf(line, "PPid:%ld", &p) == 1) {
                *ppid_out = (pid_t)p;
                have_ppid = 1;
            }
        }
        if (have_name && have_ppid) break;
    }
    fclose(f);
    return (have_name && have_ppid) ? 0 : -2;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    pid_t cur = getpid();
    char name[128];
    pid_t ppid = 0;

    if (read_proc_status(cur, name, sizeof(name), &ppid) != 0) {
        fprintf(stderr, "Failed to read /proc/%ld/status\n", (long)cur);
        return 1;
    }

    printf("%s(%ld)", name, (long)cur);

    const int MAX_HOPS = 1024;
    int hops = 0;

    while (ppid > 0 && cur != 1 && hops < MAX_HOPS) {
        cur = ppid;
        if (read_proc_status(cur, name, sizeof(name), &ppid) != 0) {
            printf("%s?(%ld)", ARROW, (long)cur);
            break;
        }
        printf("%s%s(%ld)", ARROW, name, (long)cur);
        hops++;
    }
    printf("\n");
    fflush(stdout);
    return 0;
}
