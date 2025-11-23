#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty, private_clean, private_dirty;
} MemoryMetrics;

typedef struct {
    unsigned long minor_faults, major_faults;
} PageFaults;

typedef struct {
    unsigned long start, end;
    char perms[5];
    char path[256];
} MemorySegment;

#define C_RESET "\x1b[0m"
#define C_DIM   "\x1b[90m"
#define C_HEAP  "\x1b[32m"
#define C_STACK "\x1b[34m"
#define C_LIB   "\x1b[33m"
#define C_EXE   "\x1b[35m"
#define C_VSYS  "\x1b[36m"

static int g_enable_color = 0;

static const char* color_for_path(const MemorySegment *s) {
    const char *p = s->path;
    if (strstr(p, "[heap]"))  return C_HEAP;
    if (strstr(p, "[stack"))  return C_STACK;
    if (strstr(p, "[vdso]") || strstr(p, "[vvar]")) return C_VSYS;
    if (strstr(p, ".so"))     return C_LIB;
    if (s->perms[2] == 'x')   return C_EXE;
    if (p[0] == 0 || p[0] == '[') return C_DIM;
    return C_RESET;
}

static int is_number_str(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

static pid_t find_pid_by_name(const char *needle) {
    DIR *d = opendir("/proc");
    if (!d) return -1;
    pid_t best = -1;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (!is_number_str(de->d_name)) continue;
        pid_t pid = (pid_t) atoi(de->d_name);
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        char name[256];
        if (fgets(name, sizeof(name), f)) {
            name[strcspn(name, "\n")] = 0;
            if (strstr(name, needle)) {
                best = pid;
                fclose(f);
                break;
            }
        }
        fclose(f);
    }
    closedir(d);
    return best;
}

static int read_memory_metrics(pid_t pid, MemoryMetrics *m) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open status");
        return -1;
    }
    memset(m, 0, sizeof(*m));
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &m->vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &m->vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &m->vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &m->vm_stk) == 1) continue;
        if (sscanf(line, "VmExe: %lu kB", &m->vm_exe) == 1) continue;
        if (sscanf(line, "VmLib: %lu kB", &m->vm_lib) == 1) continue;
    }
    fclose(f);
    return 0;
}

static int read_pss_from_smaps(pid_t pid, MemoryMetrics *m) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[512];
    unsigned long pss=0, sc=0, sd=0, pc=0, pd=0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long v;
        if (sscanf(line, "Pss: %lu kB", &v) == 1) { pss += v; continue; }
        if (sscanf(line, "Shared_Clean: %lu kB", &v) == 1) { sc += v; continue; }
        if (sscanf(line, "Shared_Dirty: %lu kB", &v) == 1) { sd += v; continue; }
        if (sscanf(line, "Private_Clean: %lu kB", &v) == 1) { pc += v; continue; }
        if (sscanf(line, "Private_Dirty: %lu kB", &v) == 1) { pd += v; continue; }
    }
    fclose(f);
    m->pss = pss;
    m->shared_clean = sc;
    m->shared_dirty = sd;
    m->private_clean = pc;
    m->private_dirty = pd;
    return 0;
}

static int read_pss(pid_t pid, MemoryMetrics *m) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Pss: %lu kB", &m->pss) == 1) continue;
            if (sscanf(line, "Shared_Clean: %lu kB", &m->shared_clean) == 1) continue;
            if (sscanf(line, "Shared_Dirty: %lu kB", &m->shared_dirty) == 1) continue;
            if (sscanf(line, "Private_Clean: %lu kB", &m->private_clean) == 1) continue;
            if (sscanf(line, "Private_Dirty: %lu kB", &m->private_dirty) == 1) continue;
        }
        fclose(f);
        return 0;
    }
    return read_pss_from_smaps(pid, m);
}

static int read_page_faults(pid_t pid, PageFaults *pf) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open stat");
        return -1;
    }
    char buf[8192];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    char *rp = strrchr(buf, ')');
    if (!rp) return -1;
    char *rest = rp + 2;
    int idx = 0;
    unsigned long minflt = 0, majflt = 0;
    for (char *tok = strtok(rest, " "); tok; tok = strtok(NULL, " "), idx++) {
        if (idx == 7) minflt = strtoul(tok,NULL, 10);
        if (idx == 9) majflt = strtoul(tok,NULL, 10);
    }
    pf->minor_faults = minflt;
    pf->major_faults = majflt;
    return 0;
}

static int read_memory_map(pid_t pid, MemorySegment **segments, int *count) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("open maps");
        return -1;
    }
    int n = 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) n++;
    if (n == 0) {
        fclose(f);
        *segments = NULL;
        *count = 0;
        return 0;
    }
    rewind(f);
    *segments = (MemorySegment *) calloc(n, sizeof(MemorySegment));
    if (!*segments) {
        fclose(f);
        return -1;
    }
    int i = 0;
    while (i < n && fgets(line, sizeof(line), f)) {
        MemorySegment *s = &(*segments)[i];
        s->path[0] = '\0';
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", &s->start, &s->end, s->perms, s->path) >= 3) {
            i++;
        }
    }
    fclose(f);
    *count = i;
    return 0;
}

static void print_size(unsigned long kb) {
    if (kb < 1024) printf("%4lu KB", kb);
    else if (kb < 1024UL * 1024UL) printf("%6.1f MB", kb / 1024.0);
    else printf("%6.2f GB", kb / (1024.0 * 1024.0));
}

static int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(name, len, "unknown");
        return -1;
    }
    if (fgets(name, len, f)) name[strcspn(name, "\n")] = 0;
    fclose(f);
    return 0;
}

typedef struct {
    unsigned long kb;
} Acc;

static int ends_with(const char *s, const char *suf) {
    size_t n = strlen(s), m = strlen(suf);
    return n >= m && strcmp(s + n - m, suf) == 0;
}

static void summarize_map(pid_t pid) {
    MemorySegment *segs = NULL;
    int cnt = 0;
    if (read_memory_map(pid, &segs, &cnt) != 0) return;

    unsigned long kb_heap = 0, kb_stack = 0, kb_vdso = 0, kb_vvar = 0, kb_anon = 0, kb_libs = 0, kb_exe = 0, kb_other =
            0;

    for (int i = 0; i < cnt; i++) {
        unsigned long sz_kb = (segs[i].end - segs[i].start) / 1024;
        const char *p = segs[i].path;
        if (strstr(p, "[heap]")) kb_heap += sz_kb;
        else if (strstr(p, "[stack")) kb_stack += sz_kb;
        else if (strstr(p, "[vdso]")) kb_vdso += sz_kb;
        else if (strstr(p, "[vvar]")) kb_vvar += sz_kb;
        else if (p[0] == 0 || p[0] == '[') kb_anon += sz_kb;
        else if (strstr(p, ".so")) kb_libs += sz_kb;
        else if (segs[i].perms[2] == 'x') kb_exe += sz_kb;
        else kb_other += sz_kb;
    }

    printf("Memory Map Summary:\n");
    printf("  Heap:      ");
    print_size(kb_heap);
    printf("\n");
    printf("  Stack:     ");
    print_size(kb_stack);
    printf("\n");
    printf("  Libs:      ");
    print_size(kb_libs);
    printf("\n");
    printf("  Executable:");
    print_size(kb_exe);
    printf("\n");
    printf("  [vdso]:    ");
    print_size(kb_vdso);
    printf("\n");
    printf("  [vvar]:    ");
    print_size(kb_vvar);
    printf("\n");
    printf("  Anonymous: ");
    print_size(kb_anon);
    printf("\n");
    printf("  Other:     ");
    print_size(kb_other);
    printf("\n");

    if (cnt > 0) {
        int max_show = cnt < 10 ? cnt : 10;
        int *idx = (int *) malloc(cnt * sizeof(int));
        for (int i = 0; i < cnt; i++) idx[i] = i;
        for (int k = 0; k < max_show; k++) {
            int best = k;
            for (int j = k + 1; j < cnt; j++) {
                unsigned long sz_best = (segs[idx[best]].end - segs[idx[best]].start);
                unsigned long sz_j = (segs[idx[j]].end - segs[idx[j]].start);
                if (sz_j > sz_best) best = j;
            }
            int tmp = idx[k];
            idx[k] = idx[best];
            idx[best] = tmp;
        }
        printf("\nTop-%d largest segments:\n", max_show);
        printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
        printf("----------------------------------------------------------------\n");
        for (int k = 0; k < max_show; k++) {
            MemorySegment *s = &segs[idx[k]];
            unsigned long size_kb = (s->end - s->start) / 1024;
            printf("%08lx-%08lx %-6s ", s->start, s->end, s->perms);
            print_size(size_kb);
            if (g_enable_color) {
                const char *col = color_for_path(s);
                printf("  %s%s%s\n", col, s->path[0] ? s->path : "(anonymous)", C_RESET);
            } else {
                printf("  %s\n", s->path[0] ? s->path : "(anonymous)");
            }
        }
        free(idx);
    }

    free(segs);
}

static void print_memory_map_header(void) {
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");
}

static void print_memory_map_sample(pid_t pid, int limit) {
    MemorySegment *segs = NULL;
    int cnt = 0;
    if (read_memory_map(pid, &segs, &cnt) != 0) return;
    printf("Memory Map (%d segments):\n", cnt);
    print_memory_map_header();
    int show = cnt < limit ? cnt : limit;
    for (int i = 0; i < show; i++) {
        MemorySegment *s = &segs[i];
        unsigned long size_kb = (s->end - s->start) / 1024;
        printf("%08lx-%08lx %-6s ", s->start, s->end, s->perms);
        print_size(size_kb);
        if (g_enable_color) {
            const char *col = color_for_path(s);
            printf("  %s%s%s\n", col, s->path[0] ? s->path : "(anonymous)", C_RESET);
        } else {
            printf("  %s\n", s->path[0] ? s->path : "(anonymous)");
        }
    }
    if (cnt > show) printf("... (%d more segments)\n", cnt - show);
    free(segs);
}

static void print_process_info(pid_t pid) {
    char proc_name[256];
    get_process_name(pid, proc_name, sizeof(proc_name));
    printf("Process: %s (PID %d)\n", proc_name, pid);
    printf("=====================================\n\n");

    MemoryMetrics m;
    if (read_memory_metrics(pid, &m) == 0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):     ");
        print_size(m.vm_size);
        printf("\n");
        printf("  RSS (Resident):    ");
        print_size(m.vm_rss);
        printf("\n");
        if (read_pss(pid, &m) == 0 && m.pss) {
            printf("  PSS (Proportional):");
            print_size(m.pss);
            printf(" (more accurate)\n");
            unsigned long uss = m.private_clean + m.private_dirty;
            printf("  USS (Unique):      ");
            print_size(uss);
            printf("\n");
            printf("\nMemory Breakdown:\n");
            printf("  Shared (clean):    ");
            print_size(m.shared_clean);
            printf("\n");
            printf("  Shared (dirty):    ");
            print_size(m.shared_dirty);
            printf("\n");
            printf("  Private (clean):   ");
            print_size(m.private_clean);
            printf("\n");
            printf("  Private (dirty):   ");
            print_size(m.private_dirty);
            printf("\n");
        }
        printf("\nRegions:\n");
        printf("  Text (code):       ");
        print_size(m.vm_exe);
        printf("\n");
        printf("  Data + Heap:       ");
        print_size(m.vm_data);
        printf("\n");
        printf("  Stack:             ");
        print_size(m.vm_stk);
        printf("\n");
        printf("  Libraries:         ");
        print_size(m.vm_lib);
        printf("\n");
    }

    printf("\n");
    PageFaults pf;
    if (read_page_faults(pid, &pf) == 0) {
        printf("Page Faults:\n");
        printf("  Minor: %lu\n", pf.minor_faults);
        printf("  Major: %lu\n", pf.major_faults);
    }
}

static void draw_rss_graph(unsigned long *vals, int n) {
    if (n <= 0) return;
    unsigned long mx = 0;
    for (int i = 0; i < n; i++) if (vals[i] > mx) mx = vals[i];
    if (mx == 0) mx = 1;
    printf("\nRSS graph (last %d):\n", n);
    for (int i = 0; i < n; i++) {
        int bar = (int) ((vals[i] * (size_t) 60) / mx);
        if (bar < 0) bar = 0;
        if (bar > 60) bar = 60;
        printf("%6lu KB | ", vals[i]);
        for (int j = 0; j < bar; j++) putchar('#');
        putchar('\n');
    }
}

static FILE *open_csv(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "a");
    if (!f) {
        perror("open csv");
        return NULL;
    }
    fseek(f, 0,SEEK_END);
    if (ftell(f) == 0) fprintf(f, "timestamp,pid,VSZ_kB,RSS_kB,PSS_kB,USS_kB,minor,major\n");
    return f;
}

static void watch_process(pid_t pid, int interval, const char *csv_path) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);

    const int HIST = 20;
    unsigned long hist[HIST];
    int hsz = 0;
    PageFaults prev_pf = {0, 0};
    MemoryMetrics prev_m = {0};
    int first = 1;
    FILE *csv = open_csv(csv_path);

    while (1) {
        printf("\n========================================\n");
        time_t now = time(NULL);
        printf("Time: %s", ctime(&now));

        MemoryMetrics m;
        PageFaults pf;
        if (read_memory_metrics(pid, &m) != 0) {
            printf("Process ended / inaccessible\n");
            break;
        }
        read_pss(pid, &m);
        read_page_faults(pid, &pf);

        char name[256];
        get_process_name(pid, name, sizeof(name));
        printf("Process: %s (PID %d)\n\n", name, pid);

        printf("VSZ:  ");
        print_size(m.vm_size);
        if (!first) {
            long d = (long) m.vm_size - (long) prev_m.vm_size;
            if (d) printf("  (%+ld KB)", d);
        }
        printf("\n");

        printf("RSS:  ");
        print_size(m.vm_rss);
        if (!first) {
            long d = (long) m.vm_rss - (long) prev_m.vm_rss;
            if (d) printf("  (%+ld KB)", d);
        }
        printf("\n");

        if (m.pss) {
            printf("PSS:  ");
            print_size(m.pss);
            if (!first) {
                long d = (long) m.pss - (long) prev_m.pss;
                if (d) printf("  (%+ld KB)", d);
            }
            printf("\n");
        }

        printf("\nPage Faults:\n");
        printf("  Minor: %lu", pf.minor_faults);
        if (!first) {
            long d = (long) pf.minor_faults - (long) prev_pf.minor_faults;
            if (d > 0) printf("  (+%ld)", d);
        }
        printf("\n");
        printf("  Major: %lu", pf.major_faults);
        if (!first) {
            long d = (long) pf.major_faults - (long) prev_pf.major_faults;
            if (d > 0) printf("  (+%ld)", d);
        }
        printf("\n");

        if (hsz < HIST) hist[hsz++] = m.vm_rss;
        else {
            for (int i = 1; i < HIST; i++) hist[i - 1] = hist[i];
            hist[HIST - 1] = m.vm_rss;
        }
        draw_rss_graph(hist, hsz);

        if (csv) {
            unsigned long uss = m.private_clean + m.private_dirty;
            fprintf(csv, "%ld,%d,%lu,%lu,%lu,%lu,%lu,%lu\n",
                    (long) now, (int) pid, m.vm_size, m.vm_rss, m.pss, uss,
                    (unsigned long) pf.minor_faults, (unsigned long) pf.major_faults);
            fflush(csv);
        }

        prev_m = m;
        prev_pf = pf;
        first = 0;
        sleep(interval);
    }
    if (csv) fclose(csv);
}

static void compare_processes(pid_t p1, pid_t p2) {
    printf("Comparing processes: %d vs %d\n", p1, p2);
    printf("=====================================\n\n");

    MemoryMetrics a, b;
    if (read_memory_metrics(p1, &a) != 0 || read_memory_metrics(p2, &b) != 0) {
        printf("Failed to read metrics\n"); return;
    }
    read_pss(p1, &a);
    read_pss(p2, &b);

    unsigned long a_uss = a.private_clean + a.private_dirty;
    unsigned long b_uss = b.private_clean + b.private_dirty;

    struct Row { const char* name; unsigned long x; unsigned long y; } rows[] = {
        {"VSZ (KB)", a.vm_size, b.vm_size},
        {"RSS (KB)", a.vm_rss,  b.vm_rss },
        {"PSS (KB)", a.pss,     b.pss    },
        {"USS (KB)", a_uss,     b_uss    },
        {"Heap (KB)",a.vm_data, b.vm_data},
        {"Stack (KB)",a.vm_stk, b.vm_stk },
        {"Libs (KB)",a.vm_lib,  b.vm_lib },
    };

    printf("%-18s %15s %15s %15s\n", "Metric", "PID1", "PID2", "Δ (PID1-PID2)");
    printf("---------------------------------------------------------------\n");
    for (size_t i=0;i<sizeof(rows)/sizeof(rows[0]);++i) {
        long d = (long)rows[i].x - (long)rows[i].y;
        double pct = rows[i].y ? (100.0 * (double)d / (double)rows[i].y) : 0.0;
        printf("%-18s %15lu %15lu %+15ld (%.1f%%)\n",
               rows[i].name, rows[i].x, rows[i].y, d, pct);
    }
}

typedef struct {
    char path[512];
    int share_count;
    unsigned long self_kb;
} LibShare;

static int is_pid_dir(const char *name) {
    if (!name || !*name) return 0;
    for (const char *p = name; *p; ++p) if (*p < '0' || *p > '9') return 0;
    return 1;
}

static void analyze_shared_libs(pid_t pid, int top_n) {
    MemorySegment *segs = NULL; int cnt = 0;
    if (read_memory_map(pid, &segs, &cnt) != 0) return;
    LibShare *libs = NULL; int nlibs = 0, cap = 0;

    for (int i = 0; i < cnt; i++) {
        if (!strstr(segs[i].path, ".so")) continue;
        if (!segs[i].path[0]) continue;
        int idx = -1;
        for (int j = 0; j < nlibs; j++) {
            if (strcmp(libs[j].path, segs[i].path) == 0) { idx = j; break; }
        }
        if (idx == -1) {
            if (nlibs == cap) {
                cap = cap ? cap*2 : 16;
                libs = (LibShare*)realloc(libs, cap * sizeof(LibShare));
            }
            memset(&libs[nlibs], 0, sizeof(LibShare));
            strncpy(libs[nlibs].path, segs[i].path, sizeof(libs[nlibs].path)-1);
            idx = nlibs++;
        }
        libs[idx].self_kb += (segs[i].end - segs[i].start) / 1024;
    }

    DIR *d = opendir("/proc");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (!is_pid_dir(de->d_name)) continue;
            pid_t q = (pid_t)atoi(de->d_name);
            char mpath[256];
            snprintf(mpath, sizeof(mpath), "/proc/%d/maps", q);
            FILE *mf = fopen(mpath, "r");
            if (!mf) continue;
            char line[1024];
            int *hit = (int*)calloc(nlibs, sizeof(int));
            while (fgets(line, sizeof(line), mf)) {
                char pathbuf[512]=""; unsigned long a,b; char perms[8];
                if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %511[^\n]", &a,&b,perms,pathbuf) >= 3) {
                    if (pathbuf[0] && strstr(pathbuf, ".so")) {
                        for (int j = 0; j < nlibs; j++) {
                            if (!hit[j] && strcmp(pathbuf, libs[j].path) == 0) {
                                hit[j] = 1;
                            }
                        }
                    }
                }
            }
            for (int j = 0; j < nlibs; j++) if (hit[j]) libs[j].share_count++;
            free(hit);
            fclose(mf);
        }
        closedir(d);
    }

    for (int i = 0; i < nlibs; i++) {
        int best = i;
        for (int j = i+1; j < nlibs; j++) {
            if (libs[j].share_count > libs[best].share_count ||
               (libs[j].share_count == libs[best].share_count && libs[j].self_kb > libs[best].self_kb)) {
                best = j;
            }
        }
        if (best != i) { LibShare tmp = libs[i]; libs[i] = libs[best]; libs[best] = tmp; }
    }

    if (nlibs == 0) {
        printf("Shared Libraries: none\n");
    } else {
        int show = (top_n>0 && top_n<nlibs) ? top_n : nlibs;
        printf("Shared Libraries (top %d):\n", show);
        printf("%-6s %-10s %-10s %s\n", "Rank", "SharedBy", "SelfSize", "Path");
        for (int i = 0; i < show; i++) {
            printf("%-6d %-10d ", i+1, libs[i].share_count);
            print_size(libs[i].self_kb);
            printf("  ");
            if (g_enable_color) printf("%s%s%s\n", C_LIB, libs[i].path, C_RESET);
            else                printf("%s\n", libs[i].path);
        }
        if (show < nlibs) printf("... (%d more)\n", nlibs - show);
    }

    free(libs);
    free(segs);
}

static void interactive_mode(pid_t pid) {
    printf("Interactive mode for PID %d\n", pid);
    printf("Commands:\n");
    printf("  s  - summary (metrics + faults)\n");
    printf("  m  - map summary + sample\n");
    printf("  l  - shared libs analysis (top 15)\n");
    printf("  wN - watch every N sec (e.g., w2)\n");
    printf("  q  - quit\n");
    char cmd[64];
    while (1) {
        printf("\n> ");
        fflush(stdout);
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        if (cmd[0]=='q') break;
        if (cmd[0]=='s') {
            print_process_info(pid);
        } else if (cmd[0]=='m') {
            summarize_map(pid);
            printf("\n");
            print_memory_map_sample(pid, 20);
        } else if (cmd[0]=='l') {
            analyze_shared_libs(pid, 15);
        } else if (cmd[0]=='w') {
            int sec = atoi(cmd+1); if (sec<=0) sec=1;
            watch_process(pid, sec, NULL);
        } else {
            printf("Unknown. Try: s | m | l | wN | q\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        printf("Usage: %s <PID|NAME> [options]\n", argv[0]);
        printf("Options:\n"
               "  --watch [sec]       Monitor periodically (default: 1s)\n"
               "  --compare <PID2>    Compare with PID2 (shows diffs and %)\n"
               "  --map               Show memory map summary + sample\n"
               "  --libs              Analyze shared libraries (.so) usage\n"
               "  --interactive       Simple interactive TUI (s/m/l/wN/q)\n"
               "  --csv <file.csv>    Append metrics during --watch\n"
               "  --no-color          Disable ANSI colors in map output\n");
        return 0;
    }

    int watch_mode = 0, watch_interval = 1;
    int compare_mode = 0, show_map = 0, want_libs = 0, interactive = 0, no_color = 0;
    pid_t pid2 = 0;
    const char *csv_path = NULL;

    /* resolve target PID from first arg (number or process name substring) */
    pid_t pid = -1;
    if (is_number_str(argv[1])) pid = (pid_t)atoi(argv[1]);
    else                        pid = find_pid_by_name(argv[1]);

    if (pid <= 0) {
        fprintf(stderr, "Error: can't resolve PID for '%s'\n", argv[1]);
        return 1;
    }

    /* parse options */
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--watch")) {
            watch_mode = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') watch_interval = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--compare") && i + 1 < argc) {
            compare_mode = 1;
            pid2 = (pid_t)atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--map")) {
            show_map = 1;
        } else if (!strcmp(argv[i], "--libs")) {
            want_libs = 1;
        } else if (!strcmp(argv[i], "--interactive")) {
            interactive = 1;
        } else if (!strcmp(argv[i], "--csv") && i + 1 < argc) {
            csv_path = argv[++i];
        } else if (!strcmp(argv[i], "--no-color")) {
            no_color = 1;
        }
    }

    g_enable_color = isatty(1) && !no_color;

    char procdir[256];
    snprintf(procdir, sizeof(procdir), "/proc/%d", pid);
    if (access(procdir, F_OK) != 0) {
        fprintf(stderr, "Error: PID %d not accessible\n", pid);
        return 1;
    }

    if (compare_mode) {
        compare_processes(pid, pid2);
    } else if (watch_mode) {
        watch_process(pid, watch_interval, csv_path);
    } else if (interactive) {
        interactive_mode(pid);
    } else {
        print_process_info(pid);
        if (show_map) {
            printf("\n");
            summarize_map(pid);
            printf("\n");
            print_memory_map_sample(pid, 20);
        }
        if (want_libs) {
            printf("\n");
            analyze_shared_libs(pid, 15);
        }
    }
    return 0;
}
