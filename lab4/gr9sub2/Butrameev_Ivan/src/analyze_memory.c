/*
 * analyze_memory.c - Анализ собственной виртуальной памяти
 * gcc -O2 analyze_memory.c -o analyze_memory && ./analyze_memory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

typedef struct {
    unsigned long vsz, rss, pss, uss, vm_data, vm_stk;
} Metrics;

void read_status(Metrics *m) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "VmSize: %lu", &m->vsz);
        sscanf(line, "VmRSS: %lu", &m->rss);
        sscanf(line, "VmData: %lu", &m->vm_data);
        sscanf(line, "VmStk: %lu", &m->vm_stk);
    }
    fclose(f);
}

void read_smaps(Metrics *m) {
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) return;
    char line[256];
    unsigned long pc = 0, pd = 0;
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "Pss: %lu", &m->pss);
        sscanf(line, "Private_Clean: %lu", &pc);
        sscanf(line, "Private_Dirty: %lu", &pd);
    }
    m->uss = pc + pd;
    fclose(f);
}

void show_map(void) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return;
    printf("\n=== КАРТА ПАМЯТИ (/proc/self/maps) ===\n");
    printf("Адрес          Права  Размер  Назначение\n");
    printf("─────────────────────────────────────────\n");
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path[256] = "";
        int n = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                       &start, &end, perms, path);
        if (n >= 3) {
            unsigned long sz_kb = (end - start) / 1024;
            printf("%08lx-%08lx %s %7lu KB %s\n", start, end, perms, sz_kb,
                   path[0] ? path : "(anonymous)");
        }
    }
    fclose(f);
}

int main(void) {
    printf("АНАЛИЗ ПАМЯТИ (PID %d)\n", getpid());
    
    getchar();
    printf("═════════════════════════\n\n");

    Metrics m1 = {0};
    read_status(&m1);
    read_smaps(&m1);

    printf("[ДО выделения]\n");
    printf("VSZ: %lu KB (%.1f MB)\n", m1.vsz, m1.vsz/1024.0);
    printf("RSS: %lu KB (%.1f MB)\n", m1.rss, m1.rss/1024.0);
    if (m1.pss) printf("PSS: %lu KB (%.1f MB)\n", m1.pss, m1.pss/1024.0);
    if (m1.uss) printf("USS: %lu KB (%.1f MB)\n", m1.uss, m1.uss/1024.0);
    printf("Data: %lu KB, Stack: %lu KB\n\n", m1.vm_data, m1.vm_stk);

    printf("[ВЫДЕЛЯЕМ ПАМЯТЬ]\n");
    char st[1024*1024];
    memset(st, 0, sizeof(st));
    printf("✓ Stack: 1 MB at %p\n", (void*)st);

    char *hp = malloc(10*1024*1024);
    memset(hp, 0, 10*1024*1024);
    printf("✓ Heap: 10 MB at %p\n", (void*)hp);

    void *mp = mmap(NULL, 50*1024*1024, PROT_READ|PROT_WRITE,
                    MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    memset(mp, 0, 50*1024*1024);
    printf("✓ Mmap: 50 MB at %p\n\n", mp);

    Metrics m2 = {0};
    read_status(&m2);
    read_smaps(&m2);

    printf("[ПОСЛЕ выделения]\n");
    printf("VSZ: %lu KB (%.1f MB) +%.1f MB\n", m2.vsz, m2.vsz/1024.0, 
           (m2.vsz-m1.vsz)/1024.0);
    printf("RSS: %lu KB (%.1f MB) +%.1f MB\n", m2.rss, m2.rss/1024.0,
           (m2.rss-m1.rss)/1024.0);
    if (m2.pss) printf("PSS: %lu KB (%.1f MB) +%.1f MB\n", m2.pss, m2.pss/1024.0,
           (m2.pss-m1.pss)/1024.0);
    if (m2.uss) printf("USS: %lu KB (%.1f MB) +%.1f MB\n", m2.uss, m2.uss/1024.0,
           (m2.uss-m1.uss)/1024.0);
    printf("Data: %lu KB, Stack: %lu KB\n", m2.vm_data, m2.vm_stk);

    show_map();

    /*printf("\n=== ПРОВЕРКА ИЗ ДРУГОГО ТЕРМИНАЛА ===\n");
    printf("ps -o pid,vsz,rss -p %d\n", getpid());
    printf("pmap %d\n", getpid());
    printf("cat /proc/%d/status | grep ^Vm\n\n", getpid());*/
    printf("(Ctrl+C для выхода)\n");

    pause();
    free(hp);
    munmap(mp, 50*1024*1024);
    return 0;
}
