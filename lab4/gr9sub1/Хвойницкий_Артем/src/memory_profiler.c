#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

typedef struct {
    long vsz_kb, rss_kb, pss_kb, uss_kb;
    long minflt, majflt;
    unsigned long heap_bytes, stack_bytes, libs_bytes, anon_bytes, other_bytes;
    char comm[256];
} metrics_t;

static int read_line_kb(FILE* f, const char* key, long* out) {
    char line[512]; rewind(f);
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char* p = line; while (*p && (*p < '0' || *p>'9')) ++p; if (*p) { *out = strtol(p, NULL, 10); return 0; }
        }
    }
    return -1;
}

static long read_smaps_rollup_kb(pid_t pid, const char* key) {
    char path[128]; snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE* f = fopen(path, "r"); if (!f) return -1;
    char line[512]; long val = -1; while (fgets(line, sizeof(line), f)) if (strncmp(line, key, strlen(key)) == 0) { char* p = line; while (*p && (*p < '0' || *p>'9'))++p; if (*p) val = strtol(p, NULL, 10); }
    fclose(f); return val;
}

static void read_stat_faults(pid_t pid, long* minflt, long* majflt) {
    char path[128]; snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* f = fopen(path, "r"); if (!f) { *minflt = *majflt = -1; return; }
    char* buf = NULL; size_t n = 0; if (getline(&buf, &n, f) < 0) { *minflt = *majflt = -1; fclose(f); return; }
    fclose(f);
    char* rp = strrchr(buf, ')'); if (!rp) { *minflt = *majflt = -1; free(buf); return; }
    char* rest = rp + 2;
    int idx = 3; char* tok = strtok(rest, " "); long min = -1, maj = -1; while (tok) {
        if (idx == 10) min = strtol(tok, NULL, 10);
        if (idx == 12) maj = strtol(tok, NULL, 10);
        tok = strtok(NULL, " "); idx++;
    }
    *minflt = min; *majflt = maj; free(buf);
}

static void add_range(unsigned long* acc, unsigned long a, unsigned long b) { if (b > a) *acc += (b - a); }

static void parse_maps(pid_t pid, metrics_t* m) {
    char path[128]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r"); if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        unsigned long a = 0, b = 0, offset = 0, inode = 0;
        char perms[8] = { 0 }, dev[32] = { 0 }, mapname[512] = { 0 };
        sscanf(line, "%lx-%lx %7s %lx %31s %lu %511[^\n]", &a, &b, perms, &offset, dev, &inode, mapname);
        const char* name = mapname;
        if (strstr(name, "[heap]")) add_range(&m->heap_bytes, a, b);
        else if (strstr(name, "[stack]")) add_range(&m->stack_bytes, a, b);
        else if (strstr(name, ".so") || strstr(name, "/lib")) add_range(&m->libs_bytes, a, b);
        else if (strstr(name, "[anon]") || name[0] == '\0') add_range(&m->anon_bytes, a, b);
        else add_range(&m->other_bytes, a, b);
    }
    fclose(f);
}

static void read_comm(pid_t pid, char* out, size_t n) {
    char path[128]; snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE* f = fopen(path, "r"); if (!f) { snprintf(out, n, "pid%d", pid); return; }
    if (!fgets(out, n, f)) snprintf(out, n, "pid%d", pid);
    size_t L = strlen(out); if (L && out[L - 1] == '\n') out[L - 1] = 0;
    fclose(f);
}

static int collect(pid_t pid, metrics_t* m) {
    memset(m, 0, sizeof(*m)); m->vsz_kb = m->rss_kb = m->pss_kb = m->uss_kb = -1; m->minflt = m->majflt = -1;
    read_comm(pid, m->comm, sizeof(m->comm));

    char path[128]; snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE* f = fopen(path, "r"); if (!f) { perror("status"); return -1; }
    read_line_kb(f, "VmSize:", &m->vsz_kb);
    read_line_kb(f, "VmRSS:", &m->rss_kb);
    fclose(f);

    m->pss_kb = read_smaps_rollup_kb(pid, "Pss:");
    long priv = read_smaps_rollup_kb(pid, "Private:");
    long pc = read_smaps_rollup_kb(pid, "Private_Clean:");
    long pd = read_smaps_rollup_kb(pid, "Private_Dirty:");
    if (priv > 0) m->uss_kb = priv; else if (pc >= 0 && pd >= 0) m->uss_kb = pc + pd;

    read_stat_faults(pid, &m->minflt, &m->majflt);
    parse_maps(pid, m);
    return 0;
}

static void print_metrics(const metrics_t* m, pid_t pid) {
    printf("Process: %s (PID %d)\n", m->comm, pid);
    printf("VSZ: %ld kB\nRSS: %ld kB\nPSS: %ld kB\nUSS: %ld kB\n",
        m->vsz_kb, m->rss_kb, m->pss_kb, m->uss_kb);
    printf("Page Faults: Minor: %ld  Major: %ld\n", m->minflt, m->majflt);
    printf("\n[Memory layout by maps categories]\n");
    printf("  Heap:        %.2f MB\n", m->heap_bytes / 1048576.0);
    printf("  Stack:       %.2f MB\n", m->stack_bytes / 1048576.0);
    printf("  Libraries:   %.2f MB\n", m->libs_bytes / 1048576.0);
    printf("  Anonymous:   %.2f MB\n", m->anon_bytes / 1048576.0);
    printf("  Other:       %.2f MB\n", m->other_bytes / 1048576.0);
}

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s <PID>\n"
        "       %s --watch <PID> [--interval 1]\n"
        "       %s --compare <PID1> <PID2>\n", argv0, argv0, argv0);
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    if (strcmp(argv[1], "--watch") == 0 && argc >= 3) {
        pid_t pid = (pid_t)strtol(argv[2], NULL, 10); double interval = 1.0;
        for (int i = 3; i < argc; i++) if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) interval = atof(argv[++i]);
        long prev_min = -1, prev_maj = -1; int first = 1;
        while (1) {
            metrics_t cur;
            if (collect(pid, &cur) == 0) {
                printf("\n=== %s (PID %d) ===\n", cur.comm, pid);
                printf("VSZ: %ld kB  RSS: %ld kB  PSS: %ld kB  USS: %ld kB\n",
                    cur.vsz_kb, cur.rss_kb, cur.pss_kb, cur.uss_kb);
                long dmin = (first || prev_min < 0) ? 0 : (cur.minflt - prev_min);
                long dmaj = (first || prev_maj < 0) ? 0 : (cur.majflt - prev_maj);
                printf("Faults: minor=%ld (+%ld)  major=%ld (+%ld)\n", cur.minflt, dmin, cur.majflt, dmaj);
                prev_min = cur.minflt; prev_maj = cur.majflt; first = 0;
            }
            fflush(stdout);
            struct timespec ts; ts.tv_sec = (time_t)interval; ts.tv_nsec = (long)((interval - (time_t)interval) * 1e9);
            nanosleep(&ts, NULL);
        }
    }

    if (strcmp(argv[1], "--compare") == 0 && argc >= 4) {
        pid_t p1 = (pid_t)strtol(argv[2], NULL, 10); pid_t p2 = (pid_t)strtol(argv[3], NULL, 10);
        metrics_t a, b; collect(p1, &a); collect(p2, &b);
        printf("PID %-6d VSZ %8ld RSS %8ld PSS %8ld USS %8ld | minflt %8ld majflt %6ld\n",
            p1, a.vsz_kb, a.rss_kb, a.pss_kb, a.uss_kb, a.minflt, a.majflt);
        printf("PID %-6d VSZ %8ld RSS %8ld PSS %8ld USS %8ld | minflt %8ld majflt %6ld\n",
            p2, b.vsz_kb, b.rss_kb, b.pss_kb, b.uss_kb, b.minflt, b.majflt);
        return 0;
    }

    pid_t pid = (pid_t)strtol(argv[1], NULL, 10);
    metrics_t m; if (collect(pid, &m) == 0) { print_metrics(&m, pid); return 0; }
    return 1;
}