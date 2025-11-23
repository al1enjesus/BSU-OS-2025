#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty, private_clean, private_dirty;
} MemoryMetrics;

typedef struct { unsigned long minor_faults, major_faults; } PageFaults;

typedef struct {
    unsigned long start, end;
    char perms[5];
    char path[256];
} MemorySegment;

static void print_size_kb(unsigned long kb) {
    if (kb < 1024) printf("%lu KB", kb);
    else if (kb < 1024*1024) printf("%.1f MB", kb/1024.0);
    else printf("%.2f GB", kb/1024.0/1024.0);
}

static int read_memory_metrics(pid_t pid, MemoryMetrics *m) {
    memset(m, 0, sizeof(*m));
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r"); if (!f) { perror("open status"); return -1; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "VmSize: %lu kB", &m->vm_size);
        sscanf(line, "VmRSS: %lu kB",  &m->vm_rss);
        sscanf(line, "VmData: %lu kB", &m->vm_data);
        sscanf(line, "VmStk: %lu kB",  &m->vm_stk);
        sscanf(line, "VmExe: %lu kB",  &m->vm_exe);
        sscanf(line, "VmLib: %lu kB",  &m->vm_lib);
    }
    fclose(f);
    return 0;
}

static int read_pss_rollup(pid_t pid, MemoryMetrics *m) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "Pss: %lu kB", &m->pss);
        sscanf(line, "Shared_Clean: %lu kB",  &m->shared_clean);
        sscanf(line, "Shared_Dirty: %lu kB",  &m->shared_dirty);
        sscanf(line, "Private_Clean: %lu kB", &m->private_clean);
        sscanf(line, "Private_Dirty: %lu kB", &m->private_dirty);
    }
    fclose(f);
    return 0;
}

static int read_pss_from_smaps(pid_t pid, MemoryMetrics *m) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/smaps", pid);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[256]; unsigned long v=0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Pss: %lu kB", &v)==1) m->pss += v;
        else if (sscanf(line, "Shared_Clean: %lu kB", &v)==1) m->shared_clean += v;
        else if (sscanf(line, "Shared_Dirty: %lu kB", &v)==1) m->shared_dirty += v;
        else if (sscanf(line, "Private_Clean: %lu kB", &v)==1) m->private_clean += v;
        else if (sscanf(line, "Private_Dirty: %lu kB", &v)==1) m->private_dirty += v;
    }
    fclose(f); return 0;
}

static int read_page_faults(pid_t pid, PageFaults *pf) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r"); if (!f) { perror("open stat"); return -1; }
    char *buf = NULL; size_t n=0; ssize_t len = getline(&buf, &n, f);
    fclose(f); if (len <= 0) { free(buf); return -1; }

    char *p = strrchr(buf, ')'); if (!p) { free(buf); return -1; }
    p += 2;
    char state; long ppid,pgrp,session,tty_nr,tpgid; unsigned long flags,minflt,cminflt,majflt,cmajflt;
    int scanned = sscanf(p, "%c %ld %ld %ld %ld %ld %lu %lu %lu %lu %lu",
                         &state, &ppid,&pgrp,&session,&tty_nr,&tpgid,
                         &flags, &minflt, &cminflt, &majflt, &cmajflt);
    free(buf);
    if (scanned < 11) return -1;
    pf->minor_faults = minflt;
    pf->major_faults = majflt;
    return 0;
}

static int read_memory_map(pid_t pid, MemorySegment **segs, int *count) {
    *segs = NULL; *count = 0;
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r"); if (!f) { perror("open maps"); return -1; }

    int c=0; char line[512];
    while (fgets(line, sizeof(line), f)) c++;
    rewind(f);

    MemorySegment *arr = (MemorySegment*)calloc(c, sizeof(MemorySegment));
    if (!arr) { fclose(f); return -1; }

    int i=0;
    while (i<c && fgets(line, sizeof(line), f)) {
        MemorySegment *s = &arr[i];
        s->path[0] = '\0';
        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", &s->start, &s->end, s->perms, s->path);
        i++;
    }
    fclose(f);
    *segs = arr; *count = c; return 0;
}

static int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r"); if (!f){snprintf(name,len,"pid:%d",pid); return -1;}
    if (fgets(name, len, f)) name[strcspn(name, "\n")] = 0;
    fclose(f); return 0;
}

static void print_memory_map_summary(pid_t pid) {
    MemorySegment *s=NULL; int n=0;
    if (read_memory_map(pid, &s, &n)!=0) return;
    printf("Memory Map (%d segments):\n", n);
    printf("%-23s %-6s %-10s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("-----------------------------------------------------------------------\n");
    for (int i=0;i<n && i<20;i++) {
        unsigned long kb = (s[i].end - s[i].start)/1024UL;
        printf("%016lx-%016lx %-6s %8lu KB  %s\n",
               s[i].start, s[i].end, s[i].perms, kb, s[i].path[0]?s[i].path:"(anonymous)");
    }
    if (n>20) printf("... (%d more)\n", n-20);
    free(s);
}

static void print_process_once(pid_t pid, int show_map) {
    char name[256]; get_process_name(pid, name, sizeof(name));
    printf("Process: %s (PID %d)\n=====================================\n\n", name, pid);

    MemoryMetrics m; if (read_memory_metrics(pid, &m)==0) {
        printf("Memory Metrics:\n");
        printf("  VSZ (Virtual):      "); print_size_kb(m.vm_size); printf("\n");
        printf("  RSS (Resident):     "); print_size_kb(m.vm_rss);  printf("\n");
        if (read_pss_rollup(pid, &m)!=0) read_pss_from_smaps(pid, &m);
        if (m.pss) {
            printf("  PSS (Proportional): "); print_size_kb(m.pss); printf(" (more accurate)\n");
            unsigned long uss = m.private_clean + m.private_dirty;
            printf("  USS (Unique):       "); print_size_kb(uss); printf("\n");
            printf("\nBreakdown:\n");
            printf("  Shared clean:       "); print_size_kb(m.shared_clean); printf("\n");
            printf("  Shared dirty:       "); print_size_kb(m.shared_dirty); printf("\n");
            printf("  Private clean:      "); print_size_kb(m.private_clean); printf("\n");
            printf("  Private dirty:      "); print_size_kb(m.private_dirty); printf("\n");
        }
        printf("\nRegions:\n");
        printf("  Text:               "); print_size_kb(m.vm_exe);  printf("\n");
        printf("  Data+Heap:          "); print_size_kb(m.vm_data); printf("\n");
        printf("  Stack:              "); print_size_kb(m.vm_stk);  printf("\n");
        printf("  Libs:               "); print_size_kb(m.vm_lib);  printf("\n");
    }
    PageFaults pf; if (read_page_faults(pid, &pf)==0) {
        printf("\nPage Faults:\n  Minor: %lu\n  Major: %lu\n", pf.minor_faults, pf.major_faults);
    }
    if (show_map) { printf("\n"); print_memory_map_summary(pid); }
}

static void watch_process(pid_t pid, int interval) {
    printf("Monitoring PID %d (update every %d sec, Ctrl+C to stop)\n\n", pid, interval);
    MemoryMetrics prevm={0}; PageFaults prevf={0}; int first=1;
    while (1) {
        time_t t=time(NULL); printf("\n----------------------------------------\nTime: %s", ctime(&t));
        MemoryMetrics m={0}; PageFaults f={0};
        if (read_memory_metrics(pid, &m)!=0) { puts("Process ended."); break; }
        read_pss_rollup(pid, &m); read_page_faults(pid, &f);

        printf("VSZ: "); print_size_kb(m.vm_size);
        if (!first) { long d=(long)m.vm_size - (long)prevm.vm_size; if (d) printf(" (%+ld KB)", d); }
        printf("\nRSS: "); print_size_kb(m.vm_rss);
        if (!first) { long d=(long)m.vm_rss - (long)prevm.vm_rss; if (d) printf(" (%+ld KB)", d); }
        if (m.pss) {
            printf("\nPSS: "); print_size_kb(m.pss);
            if (!first) { long d=(long)m.pss - (long)prevm.pss; if (d) printf(" (%+ld KB)", d); }
        }
        printf("\n\nPage Faults:\n  Minor: %lu", f.minor_faults);
        if (!first) { long d=(long)f.minor_faults - (long)prevf.minor_faults; if (d>0) printf(" (+%ld)", d); }
        printf("\n  Major: %lu", f.major_faults);
        if (!first) { long d=(long)f.major_faults - (long)prevf.major_faults; if (d>0) printf(" (+%ld)", d); }
        printf("\n");

        prevm=m; prevf=f; first=0;
        sleep(interval);
    }
}

static void compare_processes(pid_t a, pid_t b) {
    MemoryMetrics m1={0}, m2={0};
    if (read_memory_metrics(a,&m1)!=0 || read_pss_rollup(a,&m1)!=0) read_pss_from_smaps(a,&m1);
    if (read_memory_metrics(b,&m2)!=0 || read_pss_rollup(b,&m2)!=0) read_pss_from_smaps(b,&m2);

    printf("%-20s %15s %15s %15s\n", "Metric", "PID A", "PID B", "Delta(B-A)");
    printf("---------------------------------------------------------------\n");
    #define ROW(title, v1, v2) do{ \
        char b1[32], b2[32], bd[32]; \
    }while(0)
    printf("%-20s %15lu %15lu %15ld\n", "VSZ(KB)", m1.vm_size, m2.vm_size, (long)m2.vm_size-(long)m1.vm_size);
    printf("%-20s %15lu %15lu %15ld\n", "RSS(KB)", m1.vm_rss,  m2.vm_rss,  (long)m2.vm_rss -(long)m1.vm_rss);
    printf("%-20s %15lu %15lu %15ld\n", "PSS(KB)", m1.pss,     m2.pss,     (long)m2.pss    -(long)m1.pss);
    unsigned long uss1 = m1.private_clean+m1.private_dirty;
    unsigned long uss2 = m2.private_clean+m2.private_dirty;
    printf("%-20s %15lu %15lu %15ld\n", "USS(KB)", uss1, uss2, (long)uss2-(long)uss1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID> [--watch [interval]] [--compare PID2] [--map]\n", argv[0]);
        return 1;
    }
    pid_t pid = (pid_t)atoi(argv[1]);
    char procdir[256]; snprintf(procdir, sizeof(procdir), "/proc/%d", pid);
    if (access(procdir, F_OK)!=0) { fprintf(stderr, "Process %d not accessible\n", pid); return 1; }

    int show_map=0, watch=0, interval=1, compare=0; pid_t pid2=0;
    for (int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"--map")) show_map=1;
        else if (!strcmp(argv[i],"--watch")) { watch=1; if (i+1<argc && argv[i+1][0]!='-'){ interval=atoi(argv[++i]); } }
        else if (!strcmp(argv[i],"--compare") && i+1<argc) { compare=1; pid2=(pid_t)atoi(argv[++i]); }
    }

    if (compare) compare_processes(pid, pid2);
    else if (watch) watch_process(pid, interval);
    else print_process_once(pid, show_map);

    return 0;
}
