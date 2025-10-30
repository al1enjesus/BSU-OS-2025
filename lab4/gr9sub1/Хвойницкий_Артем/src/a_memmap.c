#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static void touch(void* p, size_t sz) {
    volatile unsigned char* q = (unsigned char*)p;
    for (size_t i = 0; i < sz; i += 4096) q[i] = (unsigned char)(i);
}

static long read_kb_from_status(const char* key) {
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[512];
    long value = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char* p = line;
            while (*p && (*p < '0' || *p > '9')) ++p;
            if (*p) value = strtol(p, NULL, 10);
            break;
        }
    }
    fclose(f);
    return value;
}

static long read_kb_from_smaps_rollup(const char* key) {
    FILE* f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) return -1;
    char line[512];
    long value = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char* p = line;
            while (*p && (*p < '0' || *p > '9')) ++p;
            if (*p) value = strtol(p, NULL, 10);
        }
    }
    fclose(f);
    return value;
}

static void print_maps_head(int lines) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[1024];
    printf("\n[Memory map, first %d lines]\n", lines);
    for (int i = 0; i < lines && fgets(line, sizeof(line), f); ++i) {
        fputs(line, stdout);
    }
    fclose(f);
}

static void print_metrics(const char* phase) {
    long vsz = read_kb_from_status("VmSize:");
    long rss = read_kb_from_status("VmRSS:");
    long pss = read_kb_from_smaps_rollup("Pss:");
    long priv = read_kb_from_smaps_rollup("Private:");
    long priv_clean = read_kb_from_smaps_rollup("Private_Clean:");
    long priv_dirty = read_kb_from_smaps_rollup("Private_Dirty:");
    long uss = -1;
    if (priv > 0) uss = priv;
    else if (priv_clean >= 0 && priv_dirty >= 0) uss = priv_clean + priv_dirty;
    printf("\n== %s ==\n", phase);
    printf("VSZ: %ld kB\nRSS: %ld kB\nPSS: %ld kB\nUSS: %ld kB\n", vsz, rss, pss, uss);
}

int main(void) {
    fprintf(stderr, "PID: %d\n", getpid());
    fflush(stderr);

    char stack_var[1024];
    memset(stack_var, 0, sizeof(stack_var));
    (void)stack_var;

    print_metrics("Before allocations");

    size_t one_mb = 1024 * 1024;
    char* heap_var = (char*)malloc(one_mb);
    if (!heap_var) { perror("malloc"); return 1; }
    char* mmap_var = (char*)mmap(NULL, one_mb, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) { perror("mmap"); return 1; }

    touch(heap_var, one_mb);
    touch(mmap_var, one_mb);

    print_metrics("After malloc(1MB)+mmap(1MB) and touch");
    print_maps_head(25);

    fprintf(stderr, "\n[PAUSE] PID=%d. Press Enter AFTER you capture ps/status/smaps/maps/htop ...\n", getpid());
    fflush(stderr);
    getchar();

    munmap(mmap_var, one_mb);
    free(heap_var);
    return 0;
}
