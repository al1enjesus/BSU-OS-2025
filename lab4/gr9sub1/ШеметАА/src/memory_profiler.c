#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

typedef struct {
    unsigned long vm_size, vm_rss, vm_data, vm_stk, vm_exe, vm_lib;
    unsigned long pss, shared_clean, shared_dirty, private_clean, private_dirty;
} MemoryMetrics;

typedef struct { unsigned long minor, major; } PageFaults;

#define MAX_HISTORY 60
typedef struct {
    unsigned long rss;
    time_t time;
} HistoryPoint;

HistoryPoint history[MAX_HISTORY];
int history_count = 0;

// === Чтение page faults — БЕЗ fscanf ===
int read_page_faults(pid_t pid, PageFaults *f) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[1024];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    char *token = strtok(line, " ");
    for (int i = 1; token && i < 12; i++) {
        if (i == 9)  f->minor = strtoul(token, NULL, 10);
        if (i == 11) f->major = strtoul(token, NULL, 10);
        token = strtok(NULL, " ");
    }
    return 0;
}

void print_size(unsigned long kb) {
    if (kb < 1024) printf("%4lu KB", kb);
    else if (kb < 1024 * 1024) printf("%6.1f MB", kb / 1024.0);
    else printf("%6.2f GB", kb / (1024.0 * 1024.0));
}

int read_memory_metrics(pid_t pid, MemoryMetrics *m) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[256]; memset(m, 0, sizeof(*m));
    while (fgets(line, sizeof(line), f)) {
        unsigned long val;
        if (sscanf(line, "VmSize: %lu kB", &val) == 1) m->vm_size = val;
        if (sscanf(line, "VmRSS: %lu kB", &val) == 1) m->vm_rss = val;
        if (sscanf(line, "VmData: %lu kB", &val) == 1) m->vm_data = val;
        if (sscanf(line, "VmStk: %lu kB", &val) == 1) m->vm_stk = val;
        if (sscanf(line, "VmExe: %lu kB", &val) == 1) m->vm_exe = val;
        if (sscanf(line, "VmLib: %lu kB", &val) == 1) m->vm_lib = val;
    }
    fclose(f); return 0;
}

int read_pss(pid_t pid, MemoryMetrics *m) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *f = fopen(path, "r"); if (!f) return -1;
    char line[256]; unsigned long val;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Pss: %lu kB", &val) == 1) m->pss = val;
        if (sscanf(line, "Shared_Clean: %lu kB", &val) == 1) m->shared_clean = val;
        if (sscanf(line, "Shared_Dirty: %lu kB", &val) == 1) m->shared_dirty = val;
        if (sscanf(line, "Private_Clean: %lu kB", &val) == 1) m->private_clean = val;
        if (sscanf(line, "Private_Dirty: %lu kB", &val) == 1) m->private_dirty = val;
    }
    fclose(f); return 0;
}

int get_process_name(pid_t pid, char *name, size_t len) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) { snprintf(name, len, "unknown"); return -1; }
    if (!fgets(name, len, f)) { snprintf(name, len, "unknown"); fclose(f); return -1; }
    name[strcspn(name, "\n")] = '\0'; fclose(f); return 0;
}

void print_ascii_graph() {
    if (history_count < 2) return;
    printf(COLOR_CYAN "\nRSS History (last %d sec):\n" COLOR_RESET, history_count);
    unsigned long max_rss = 0;
    for (int i = 0; i < history_count; i++) if (history[i].rss > max_rss) max_rss = history[i].rss;
    int width = 50;
    for (int i = 0; i < history_count; i++) {
        int bars = max_rss > 0 ? (int)((history[i].rss * width) / max_rss) : 0;
        printf("%3d: ", i);
        for (int j = 0; j < bars; j++) printf("█");
        for (int j = bars; j < width; j++) printf(" ");
        printf(" %6.1f MB\n", history[i].rss / 1024.0);
    }
}

#define CMP(field) do { \
    long d = (long)m2.field - (long)m1.field; \
    printf("%-15s ", #field); print_size(m1.field); printf(" to "); print_size(m2.field); \
    if (d != 0) { printf(" %+ld KB", d); } \
    printf("\n"); \
} while(0)

void compare_processes(pid_t p1, pid_t p2) {
    MemoryMetrics m1, m2;
    if (read_memory_metrics(p1, &m1) || read_memory_metrics(p2, &m2)) {
        printf("Cannot read processes.\n");
        return;
    }
    read_pss(p1, &m1); read_pss(p2, &m2);
    char n1[256], n2[256];
    get_process_name(p1, n1, sizeof(n1));
    get_process_name(p2, n2, sizeof(n2));
    printf(COLOR_CYAN "Comparing: %s (%d) vs %s (%d)\n\n" COLOR_RESET, n1, p1, n2, p2);
    printf("%-15s %12s %12s %12s\n", "Metric", n1, n2, "Diff");
    printf("----------------------------------------------------------\n");
    CMP(vm_size);
    CMP(vm_rss);
    if (m1.pss && m2.pss) CMP(pss);
}

void add_to_history(unsigned long rss) {
    if (history_count < MAX_HISTORY) {
        history[history_count].rss = rss;
        history[history_count].time = time(NULL);
        history_count++;
    } else {
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(HistoryPoint));
        history[MAX_HISTORY - 1].rss = rss;
        history[MAX_HISTORY - 1].time = time(NULL);
    }
}

void watch_process(pid_t pid, int interval) {
    PageFaults prev_f = {0,0};
    MemoryMetrics prev_m = {0};
    int first = 1;
    printf(COLOR_CYAN "Monitoring PID %d every %ds (Ctrl+C to stop)\n" COLOR_RESET, pid, interval);
    while (1) {
        MemoryMetrics m; PageFaults f;
        if (read_memory_metrics(pid, &m)) { printf("Process terminated.\n"); break; }
        read_pss(pid, &m); read_page_faults(pid, &f); add_to_history(m.vm_rss);
        printf("\033[2J\033[H");
        time_t now = time(NULL); printf(COLOR_YELLOW "%s" COLOR_RESET, ctime(&now));
        char name[256]; get_process_name(pid, name, sizeof(name));
        printf(COLOR_GREEN "Process: %s (PID %d)\n\n" COLOR_RESET, name, pid);
        printf("VSZ:  "); print_size(m.vm_size);
        if (!first && m.vm_size != prev_m.vm_size) printf(" (%+ld KB)", (long)m.vm_size - (long)prev_m.vm_size);
        printf("\n");
        printf("RSS:  "); print_size(m.vm_rss);
        if (!first && m.vm_rss != prev_m.vm_rss) printf(" (%+ld KB)", (long)m.vm_rss - (long)prev_m.vm_rss);
        printf("\n");
        if (m.pss) { printf("PSS:  "); print_size(m.pss); printf("\n"); }
        unsigned long uss = m.private_clean + m.private_dirty;
        if (uss) { printf("USS:  "); print_size(uss); printf("\n"); }
        printf("\nPage Faults:\n");
        printf("  Minor: %lu", f.minor);
        if (!first && f.minor > prev_f.minor) printf(" (+%lu)", f.minor - prev_f.minor);
        printf("\n");
        printf("  Major: %lu", f.major);
        if (!first && f.major > prev_f.major) printf(" (+%lu)", f.major - prev_f.major);
        printf("\n");
        print_ascii_graph();
        prev_m = m; prev_f = f; first = 0;
        sleep(interval);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Usage: %s <PID> [--watch [N]]\n", argv[0]); return 1; }
    pid_t pid = atoi(argv[1]);
    int watch = 0, interval = 1;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--watch")) {
            watch = 1;
            if (i + 1 < argc && isdigit(argv[i + 1][0])) interval = atoi(argv[++i]);
        }
    }
    if (watch) watch_process(pid, interval);
    else {
        MemoryMetrics m; PageFaults f;
        read_memory_metrics(pid, &m); read_pss(pid, &m); read_page_faults(pid, &f);
        char name[256]; get_process_name(pid, name, sizeof(name));
        printf(COLOR_GREEN "Process: %s (PID %d)\n" COLOR_RESET, name, pid);
        printf("VSZ: "); print_size(m.vm_size); printf("\n");
        printf("RSS: "); print_size(m.vm_rss); printf("\n");
        if (m.pss) { printf("PSS: "); print_size(m.pss); printf("\n"); }
        printf("Minor faults: %lu, Major: %lu\n", f.minor, f.major);
    }
    return 0;
}
