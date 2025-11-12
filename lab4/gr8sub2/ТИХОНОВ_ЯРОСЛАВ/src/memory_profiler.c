/*
 memory_profiler.c
 Простой memory profiler для Linux (C).
 Сбор: gcc -O2 memory_profiler.c -o memory_profiler
 Использование:
   ./memory_profiler PID
   ./memory_profiler PID --csv out.csv
   ./memory_profiler --compare PID1 PID2
   ./memory_profiler PID --interval 2 --history 60
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINE 4096
#define HISTORY_DEFAULT 40

static volatile int running = 1;
static void onint(int sig){ (void)sig; running = 0; }

typedef struct {
    unsigned long vmsize_kb; // VSZ
    unsigned long vmrss_kb;  // RSS from status
    unsigned long rss_kb;    // Rss from smaps_rollup (if available)
    unsigned long pss_kb;    // PSS from smaps_rollup
    unsigned long private_kb; // Private_Clean + Private_Dirty => USS approx
    unsigned long shared_kb;  // Shared_Clean + Shared_Dirty
    unsigned long minflt;    // minor page faults (from /proc/[pid]/stat)
    unsigned long majflt;    // major page faults
} mem_snapshot_t;

static int read_status_vmsize_rss(pid_t pid, unsigned long *vmsize_kb, unsigned long *vmrss_kb) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char *line = NULL;
    size_t ln = 0;
    while (getline(&line, &ln, f) > 0) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line+7, "%lu", vmsize_kb);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line+6, "%lu", vmrss_kb);
        }
    }
    free(line);
    fclose(f);
    return 0;
}

static int read_smaps_rollup(pid_t pid, mem_snapshot_t *out) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char *line = NULL;
    size_t ln = 0;
    unsigned long pss = 0, rss = 0, priv_clean = 0, priv_dirty = 0, shd_clean=0, shd_dirty=0;
    while (getline(&line, &ln, f) > 0) {
        if (sscanf(line, "Pss: %lu kB", &pss) == 1) continue;
        if (sscanf(line, "Rss: %lu kB", &rss) == 1) continue;
        if (sscanf(line, "Private_Clean: %lu kB", &priv_clean) == 1) continue;
        if (sscanf(line, "Private_Dirty: %lu kB", &priv_dirty) == 1) continue;
        if (sscanf(line, "Shared_Clean: %lu kB", &shd_clean) == 1) continue;
        if (sscanf(line, "Shared_Dirty: %lu kB", &shd_dirty) == 1) continue;
    }
    free(line);
    fclose(f);
    out->pss_kb = pss;
    out->rss_kb = rss;
    out->private_kb = priv_clean + priv_dirty;
    out->shared_kb = shd_clean + shd_dirty;
    return 0;
}

// parse /proc/[pid]/stat to get minflt and majflt; handle comm in parentheses
static int read_stat_faults(pid_t pid, unsigned long *minflt, unsigned long *majflt) {
    char path[64], buf[4096];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return -1; }
    fclose(f);

    // Find position after closing parenthesis of comm
    char *p = strrchr(buf, ')');
    if (!p) return -1;
    p++; // now at space after )
    // Now tokenise from here. According to proc man:
    // fields: 1 pid, 2 comm, 3 state, 4 ppid, 5 pgrp, 6 session, 7 tty_nr, 8 tpgid, 9 flags,
    // 10 minflt, 11 cminflt, 12 majflt, 13 cmajflt ...
    // But because we started after field 2, field index offset = 3 for token 1 here.
    // We'll extract tokens until we reach fields 10 and 12 overall => token indices (after p):
    // token1 -> field3, so we need token8 -> field10, token10 -> field12
    char *save = NULL;
    int tok = 3;
    char *tokstr = strtok_r(p, " \t\n", &save);
    unsigned long local_min=0, local_maj=0;
    while (tokstr) {
        if (tok == 10) { local_min = strtoul(tokstr, NULL, 10); }
        if (tok == 12) { local_maj = strtoul(tokstr, NULL, 10); break; }
        tok++;
        tokstr = strtok_r(NULL, " \t\n", &save);
    }
    *minflt = local_min;
    *majflt = local_maj;
    return 0;
}

typedef struct map_segment {
    unsigned long start;
    unsigned long end;
    char perms[8];
    char pathname[1024];
    struct map_segment *next;
} map_segment_t;

static map_segment_t *read_maps(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    char *line = NULL;
    size_t ln = 0;
    map_segment_t *head = NULL, *tail = NULL;
    while (getline(&line, &ln, f) > 0) {
        unsigned long start=0,end=0; char perms[8]=""; char rest[MAX_LINE];
        // line format: addr perms offset dev inode pathname
        // We'll parse start-end and perms, then the rest (we'll extract pathname if any)
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %[^\n]", &start, &end, perms, rest) >= 3) {
            map_segment_t *m = calloc(1, sizeof(map_segment_t));
            m->start = start; m->end = end; strncpy(m->perms, perms, sizeof(m->perms)-1);
            if (strlen(rest) > 0) {
                // trim leading spaces
                char *s = rest;
                while (*s && isspace((unsigned char)*s)) s++;
                strncpy(m->pathname, s, sizeof(m->pathname)-1);
            } else m->pathname[0]=0;
            m->next = NULL;
            if (!head) head = tail = m; else { tail->next = m; tail = m; }
        } else {
            // Try simpler parse without pathname
            if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) >= 3) {
                map_segment_t *m = calloc(1, sizeof(map_segment_t));
                m->start = start; m->end = end; strncpy(m->perms, perms, sizeof(m->perms)-1);
                m->pathname[0]=0; m->next=NULL;
                if (!head) head = tail = m; else { tail->next = m; tail = m; }
            }
        }
    }
    free(line);
    fclose(f);
    return head;
}

static void free_maps(map_segment_t *m) {
    while (m) { map_segment_t *n = m->next; free(m); m = n; }
}

static void print_grouped_maps(map_segment_t *head) {
    unsigned long heap_kb = 0, stack_kb = 0, libs_kb = 0, anon_kb = 0, exe_kb = 0, other_kb = 0;
    map_segment_t *m = head;
    printf("\n--- Maps (grouped) ---\n");
    while (m) {
        unsigned long sz_kb = (m->end - m->start) / 1024;
        if (strstr(m->pathname, "[heap]")) heap_kb += sz_kb;
        else if (strstr(m->pathname, "[stack]")) stack_kb += sz_kb;
        else if (m->pathname[0] == 0) anon_kb += sz_kb;
        else if (strstr(m->pathname, ".so") || strstr(m->pathname, "/lib") || strstr(m->pathname, "/usr/lib")) libs_kb += sz_kb;
        else if (strstr(m->pathname, "/")) { // file-backed (exe or data)
            if (strstr(m->pathname, " (deleted)")) other_kb += sz_kb;
            else exe_kb += sz_kb;
        } else other_kb += sz_kb;
        m = m->next;
    }
    printf("heap:      %10lu kB\n", heap_kb);
    printf("stack:     %10lu kB\n", stack_kb);
    printf("libs:      %10lu kB\n", libs_kb);
    printf("exe/file:  %10lu kB\n", exe_kb);
    printf("anon:      %10lu kB\n", anon_kb);
    printf("other:     %10lu kB\n", other_kb);
}

// Print full maps (optionally trimmed)
static void print_maps_long(map_segment_t *head, int limit) {
    printf("\n--- /proc/<pid>/maps (first %d entries) ---\n", limit);
    int count = 0;
    map_segment_t *m = head;
    while (m && (limit <= 0 || count < limit)) {
        printf("%08lx-%08lx %s %s\n", m->start, m->end, m->perms, m->pathname[0] ? m->pathname : "");
        count++; m = m->next;
    }
}

static void print_snapshot(mem_snapshot_t *s, mem_snapshot_t *prev) {
    long delta_vsz = prev ? (long)s->vmsize_kb - (long)prev->vmsize_kb : 0;
    long delta_vmrss = prev ? (long)s->vmrss_kb - (long)prev->vmrss_kb : 0;
    long delta_rss = prev ? (long)s->rss_kb - (long)prev->rss_kb : 0;
    long delta_pss = prev ? (long)s->pss_kb - (long)prev->pss_kb : 0;
    long delta_uss = prev ? (long)s->private_kb - (long)prev->private_kb : 0;
    long delta_minflt = prev ? (long)s->minflt - (long)prev->minflt : 0;
    long delta_majflt = prev ? (long)s->majflt - (long)prev->majflt : 0;

    printf("VSZ: %8lu kB (Δ %+ld)  VmRSS: %8lu kB (Δ %+ld)\n",
           s->vmsize_kb, delta_vsz, s->vmrss_kb, delta_vmrss);
    printf("RSS(smaps): %6lu kB (Δ %+ld)  PSS: %6lu kB (Δ %+ld)  USS: %6lu kB (Δ %+ld)\n",
           s->rss_kb, delta_rss, s->pss_kb, delta_pss, s->private_kb, delta_uss);
    printf("Shared: %6lu kB  pagefaults: minor %lu (Δ %+ld) major %lu (Δ %+ld)\n",
           s->shared_kb, s->minflt, delta_minflt, s->majflt, delta_majflt);
}

// Simple ASCII graph for RSS history
static void ascii_graph(unsigned long *hist, int n, int width) {
    unsigned long maxv = 1;
    for (int i=0;i<n;i++) if (hist[i] > maxv) maxv = hist[i];
    printf("\nRSS trend (last %d):\n", n);
    for (int i=0;i<n;i++) {
        int bar = (int)((double)hist[i] / maxv * width + 0.5);
        printf("%6lukB |", hist[i]);
        for (int j=0;j<bar;j++) putchar('#');
        putchar('\n');
    }
}

// CSV header
static void csv_write_header(FILE *csv) {
    fprintf(csv, "timestamp_unix,vm_size_kb,vm_rss_kb,rss_smaps_kb,pss_kb,uss_kb,minflt,majflt\n");
}

static void csv_write_row(FILE *csv, mem_snapshot_t *s) {
    time_t t = time(NULL);
    fprintf(csv, "%ld,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
            (long)t, s->vmsize_kb, s->vmrss_kb, s->rss_kb, s->pss_kb, s->private_kb, s->minflt, s->majflt);
    fflush(csv);
}

// Compare mode: list mapped file paths for both PIDs and show intersection/unique
static int compare_maps(pid_t a, pid_t b) {
    map_segment_t *ma = read_maps(a);
    map_segment_t *mb = read_maps(b);
    if (!ma || !mb) {
        fprintf(stderr, "Cannot read maps for one of the PIDs (permission?).\n");
        free_maps(ma); free_maps(mb);
        return -1;
    }
    // Build sets of pathnames (only non-empty, unique)
    // For simplicity use dynamic arrays with strdup
    char **A = NULL, **B = NULL; size_t an=0,bn=0;
    map_segment_t *m;
    for (m=ma;m;m=m->next) {
        if (m->pathname[0]==0) continue;
        // trim newline
        char *p = m->pathname;
        while (*p && (*p==' '||*p=='\t')) p++;
        if (*p==0) continue;
        // avoid duplicates
        int found=0;
        for (size_t i=0;i<an;i++) if (strcmp(A[i], p)==0) { found=1; break; }
        if (!found) { A = realloc(A, (an+1)*sizeof(char*)); A[an++] = strdup(p); }
    }
    for (m=mb;m;m=m->next) {
        if (m->pathname[0]==0) continue;
        char *p = m->pathname;
        while (*p && (*p==' '||*p=='\t')) p++;
        if (*p==0) continue;
        int found=0;
        for (size_t i=0;i<bn;i++) if (strcmp(B[i], p)==0) { found=1; break; }
        if (!found) { B = realloc(B, (bn+1)*sizeof(char*)); B[bn++] = strdup(p); }
    }

    // Intersection
    printf("\n--- Compare PID %d vs PID %d ---\n", a, b);
    size_t common=0;
    for (size_t i=0;i<an;i++) {
        for (size_t j=0;j<bn;j++) if (strcmp(A[i], B[j])==0) { common++; break; }
    }
    printf("mapped entries: PID %d => %zu, PID %d => %zu, common => %zu\n", a, an, b, bn, common);

    // Print few examples
    printf("\nCommon libraries (sample up to 20):\n");
    size_t shown=0;
    for (size_t i=0;i<an && shown<20;i++) {
        for (size_t j=0;j<bn;j++) if (strcmp(A[i], B[j])==0) { printf("  %s\n", A[i]); shown++; break; }
    }
    printf("\nUnique to %d (sample up to 10):\n", a);
    shown=0;
    for (size_t i=0;i<an && shown<10;i++) {
        int found=0;
        for (size_t j=0;j<bn;j++) if (strcmp(A[i], B[j])==0) { found=1; break; }
        if (!found) { printf("  %s\n", A[i]); shown++; }
    }
    printf("\nUnique to %d (sample up to 10):\n", b);
    shown=0;
    for (size_t i=0;i<bn && shown<10;i++) {
        int found=0;
        for (size_t j=0;j<an;j++) if (strcmp(B[i], A[j])==0) { found=1; break; }
        if (!found) { printf("  %s\n", B[i]); shown++; }
    }

    for (size_t i=0;i<an;i++) free(A[i]);
    for (size_t i=0;i<bn;i++) free(B[i]);
    free(A); free(B);
    free_maps(ma); free_maps(mb);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PID [--csv out.csv] [--interval N] [--history N] [--maps N]\n", argv[0]);
        fprintf(stderr, "       %s --compare PID1 PID2\n", argv[0]);
        return 1;
    }
    signal(SIGINT, onint);
    // parse args
    pid_t pid = 0;
    char *csv_path = NULL;
    int interval = 1;
    int history = HISTORY_DEFAULT;
    int maps_limit = 20;
    int compare_mode = 0;
    pid_t pid2 = 0;

    int i = 1;
    if (strcmp(argv[1], "--compare") == 0) {
        if (argc < 4) { fprintf(stderr, "compare requires two PIDs\n"); return 1; }
        compare_mode = 1;
        pid = (pid_t)atoi(argv[2]);
        pid2 = (pid_t)atoi(argv[3]);
        return compare_maps(pid, pid2);
    }

    pid = (pid_t)atoi(argv[1]);
    for (i=2;i<argc;i++) {
        if (strcmp(argv[i], "--csv")==0 && i+1<argc) { csv_path = argv[++i]; }
        else if (strcmp(argv[i], "--interval")==0 && i+1<argc) { interval = atoi(argv[++i]); if (interval<1) interval=1; }
        else if (strcmp(argv[i], "--history")==0 && i+1<argc) { history = atoi(argv[++i]); if (history<4) history=4; }
        else if (strcmp(argv[i], "--maps")==0 && i+1<argc) { maps_limit = atoi(argv[++i]); }
        else { fprintf(stderr, "Unknown arg: %s\n", argv[i]); }
    }

    // initial read: maps and grouped
    map_segment_t *maps = read_maps(pid);
    if (!maps) {
        fprintf(stderr, "Cannot open /proc/%d/maps - maybe permission denied or process gone.\n", pid);
        return 2;
    }
    print_grouped_maps(maps);
    print_maps_long(maps, maps_limit);

    // CSV file if requested
    FILE *csv = NULL;
    if (csv_path) {
        csv = fopen(csv_path, "w");
        if (!csv) { perror("fopen csv"); }
        else csv_write_header(csv);
    }

    // history buffer for RSS (smaps or vmrss)
    unsigned long *hist = calloc(history, sizeof(unsigned long));
    int hist_pos = 0;

    mem_snapshot_t cur = {0}, prev = {0};
    // initial populate prev to zeros, then loop
    while (running) {
        // read status (VmSize and VmRSS)
        read_status_vmsize_rss(pid, &cur.vmsize_kb, &cur.vmrss_kb);
        // read smaps_rollup (if accessible)
        if (read_smaps_rollup(pid, &cur) != 0) {
            // fallback: use vmrss as rss_kb
            cur.rss_kb = cur.vmrss_kb;
            cur.pss_kb = 0;
            cur.private_kb = 0;
            cur.shared_kb = 0;
        }
        // read page faults
        read_stat_faults(pid, &cur.minflt, &cur.majflt);

        // print timestamp
        time_t now = time(NULL);
        struct tm tm; localtime_r(&now, &tm);
        char tbuf[64]; strftime(tbuf, sizeof(tbuf), "%F %T", &tm);
        printf("\n[%s] PID %d\n", tbuf, pid);

        print_snapshot(&cur, (prev.vmsize_kb||prev.vmrss_kb) ? &prev : NULL);

        // push history (use cur.rss_kb if available else vmrss)
        unsigned long rss_val = cur.rss_kb ? cur.rss_kb : cur.vmrss_kb;
        hist[hist_pos % history] = rss_val;
        hist_pos++;
        int used = hist_pos < history ? hist_pos : history;
        // build compact history array for graph
        unsigned long *hview = malloc(used * sizeof(unsigned long));
        for (int k=0;k<used;k++) {
            int idx = (hist_pos - used + k) % history;
            if (idx<0) idx += history;
            hview[k] = hist[idx];
        }
        ascii_graph(hview, used, 50);
        free(hview);

        if (csv) csv_write_row(csv, &cur);

        // copy current to prev
        prev = cur;

        // sleep interval seconds with interruption check
        for (int s=0;s<interval && running;s++) sleep(1);
    }

    printf("\nExiting. Bye.\n");
    free(hist);
    free_maps(maps);
    if (csv) fclose(csv);
    return 0;
}
