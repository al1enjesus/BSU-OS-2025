#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

static void print_size_human(unsigned long bytes) {
    double b = (double)bytes;
    const char *units[] = {"B","KB","MB","GB","TB"};
    int i=0;
    while (b >= 1024.0 && i < 4) { b/=1024.0; i++; }
    printf("%.2f %s", b, units[i]);
}

static int read_status_kb(const char* key, unsigned long *kb_out) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[512];
    int ok = -1;
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, klen)==0) {
            unsigned long v=0;
            if (sscanf(line + klen, " %lu kB", &v)==1) {
                *kb_out = v;
                ok = 0;
                break;
            }
        }
    }
    fclose(f);
    return ok;
}

static int read_smaps_rollup(unsigned long *pss_kb, unsigned long *priv_kb) {
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) return -1;
    char line[512];
    int seen_pss=0, seen_priv=0;
    while (fgets(line, sizeof(line), f)) {
        if (!seen_pss && sscanf(line, "Pss: %lu kB", pss_kb)==1) seen_pss=1;
        if (!seen_priv && sscanf(line, "Private_Clean: %lu kB", priv_kb)==1) {
            // priv_kb will be overwritten to contain Private_Clean + Private_Dirty
            unsigned long pc = *priv_kb, pd=0;
            // peek next lines via another pass; simpler: store and keep scanning
            while (fgets(line, sizeof(line), f)) {
                unsigned long v=0;
                if (sscanf(line, "Private_Dirty: %lu kB", &v)==1) {
                    pd = v;
                    break;
                }
            }
            *priv_kb = pc + pd;
            seen_priv=1;
            break; // we can break if Pss already seen; else continue
        }
    }
    fclose(f);
    return (seen_pss?0:-1) + (seen_priv?0:-1) == -2 ? -1 : 0;
}

static void show_maps_sample(int max_lines) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("maps"); return; }
    printf("---- /proc/self/maps (first %d lines) ----\n", max_lines);
    char line[512]; int n=0;
    while (n<max_lines && fgets(line, sizeof(line), f)) {
        fputs(line, stdout);
        n++;
    }
    if (!feof(f)) printf("... (truncated)\n");
    fclose(f);
}

static void print_metrics(const char* tag) {
    unsigned long vms_kb=0, rss_kb=0;
    unsigned long pss_kb=0, uss_kb=0;

    read_status_kb("VmSize:", &vms_kb);
    read_status_kb("VmRSS:", &rss_kb);
    read_smaps_rollup(&pss_kb, &uss_kb);

    printf("\n[%s] Metrics:\n", tag);
    printf("  VSZ: "); print_size_human(vms_kb*1024UL); printf("\n");
    printf("  RSS: "); print_size_human(rss_kb*1024UL); printf("\n");
    if (pss_kb) { printf("  PSS: "); print_size_human(pss_kb*1024UL); printf("\n"); }
    if (uss_kb) { printf("  USS: "); print_size_human(uss_kb*1024UL); printf("\n"); }
}

int main(void) {
    printf("=== VM Analyzer (before allocation) ===\n");
    print_metrics("BEFORE");
    show_maps_sample(20);

    // Stack allocation
    static char stack_buf[1024]; // tiny stack buffer to be visible
    memset(stack_buf, 0xAB, sizeof(stack_buf));

    // Heap allocation 1 MB (touch each page to commit)
    size_t heap_sz = 1024*1024;
    char *heap = malloc(heap_sz);
    if (!heap) { perror("malloc"); return 1; }
    for (size_t i=0; i<heap_sz; i+=4096) heap[i]=1;

    // Anonymous mmap 1 MB (touch to fault pages in)
    size_t mmap_sz = 1024*1024;
    char *mm = mmap(NULL, mmap_sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mm==MAP_FAILED) { perror("mmap"); free(heap); return 1; }
    for (size_t i=0; i<mmap_sz; i+=4096) mm[i]=2;

    printf("\n=== VM Analyzer (after allocation) ===\n");
    print_metrics("AFTER");
    show_maps_sample(20);

    // Keep process alive briefly for external ps/strace if needed
    printf("\nSleeping 2s so you can check ps/Proc...\n");
    fflush(stdout);
    usleep(2000*1000);

    // cleanup
    munmap(mm, mmap_sz);
    free(heap);
    return 0;
}