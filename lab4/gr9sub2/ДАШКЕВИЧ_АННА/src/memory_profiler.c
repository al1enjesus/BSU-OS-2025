// src/memory_profiler.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long vsz_kb;
    unsigned long rss_kb;
    unsigned long pss_kb;
    unsigned long uss_kb;
    unsigned long minor_flts;
    unsigned long major_flts;
    unsigned long shared_kb;  // shared = RSS - USS (approx), refine via smaps if needed
    unsigned long priv_kb;    // USS
} memstats_t;

static void print_human(unsigned long bytes) {
    double b = (double)bytes;
    const char *u[]={"B","KB","MB","GB","TB"};
    int i=0; while (b>=1024.0 && i<4){ b/=1024.0; i++; }
    printf("%.2f %s", b, u[i]);
}

static int read_status_kb_pid(pid_t pid, const char* key, unsigned long* out_kb) {
    char path[128]; snprintf(path,sizeof(path),"/proc/%d/status",pid);
    FILE *f = fopen(path,"r"); if(!f) return -1;
    char line[512]; size_t klen=strlen(key); int ok=-1;
    while (fgets(line,sizeof(line),f)) {
        if (strncmp(line,key,klen)==0) {
            unsigned long v=0;
            if (sscanf(line + klen, " %lu kB", &v)==1) { *out_kb=v; ok=0; break; }
        }
    }
    fclose(f);
    return ok;
}

static int read_smaps_rollup_pid(pid_t pid, unsigned long* pss_kb, unsigned long* uss_kb) {
    char path[128]; snprintf(path,sizeof(path),"/proc/%d/smaps_rollup",pid);
    FILE *f = fopen(path,"r"); if(!f) return -1;
    char line[512]; int seen_pss=0, seen_pclean=0, seen_pdirty=0;
    unsigned long pc=0,pd=0;
    while (fgets(line,sizeof(line),f)) {
        if (!seen_pss) {
            unsigned long v=0;
            if (sscanf(line,"Pss: %lu kB",&v)==1){ *pss_kb=v; seen_pss=1; }
        }
        if (!seen_pclean) {
            unsigned long v=0;
            if (sscanf(line,"Private_Clean: %lu kB",&v)==1){ pc=v; seen_pclean=1; }
        }
        if (!seen_pdirty) {
            unsigned long v=0;
            if (sscanf(line,"Private_Dirty: %lu kB",&v)==1){ pd=v; seen_pdirty=1; }
        }
    }
    fclose(f);
    if (seen_pclean || seen_pdirty) *uss_kb = pc + pd;
    return seen_pss?0:-1;
}

static int read_stat_faults_pid(pid_t pid, unsigned long* minflt, unsigned long* majflt, char* comm, size_t commsz) {
    char path[128]; snprintf(path,sizeof(path),"/proc/%d/stat",pid);
    FILE *f = fopen(path,"r"); if(!f) return -1;

    unsigned long minf=0, majf=0;
    char line[4096];
    if(!fgets(line,sizeof(line),f)){ fclose(f); return -1; }
    fclose(f);

    // Extract comm between parentheses
    char *lpar = strchr(line,'(');
    char *rpar = strrchr(line,')');
    if(!lpar||!rpar||rpar<lpar) return -1;
    char commbuf[256];
    size_t clen = (size_t)(rpar-lpar-1);
    if (clen >= sizeof(commbuf)) clen = sizeof(commbuf)-1;
    memcpy(commbuf, lpar+1, clen); commbuf[clen]=0;
    if (commsz>0 && comm) { strncpy(comm, commbuf, commsz-1); comm[commsz-1]=0; }

    // Fields positions (per proc(5)):
    // 1 pid, 2 comm, 3 state, 4 ppid, 5 pgrp, 6 session, 7 tty_nr, 8 tpgid,
    // 9 flags, 10 minflt, 11 cminflt, 12 majflt, 13 cmajflt, ...
    const char *rest = rpar+2; // skip ") "
    char st;
    unsigned long ppid,pgrp,session,tty_nr,tpgid,flags,cminflt;
    int n = sscanf(rest, "%c %lu %lu %lu %lu %lu %lu %lu %lu",
                   &st, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minf, &cminflt);
    if (n < 9) return -1;

    unsigned long cmaj;
    n = sscanf(rest, "%c %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
               &st, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
               &minf, &cminflt, &majf, &cmaj);
    if (n < 11) return -1;

    *minflt = minf;
    *majflt = majf;
    return 0;
}

static void group_maps(pid_t pid) {
    char path[128]; snprintf(path,sizeof(path),"/proc/%d/maps",pid);
    FILE *f = fopen(path,"r"); if(!f){ perror("maps"); return; }
    unsigned long heap_kb=0, stack_kb=0, libs_kb=0, anon_kb=0, file_kb=0;
    char line[1024];
    while (fgets(line,sizeof(line),f)) {
        unsigned long start=0,end=0;
        char perms[8]={0}, dev[16]={0}, mapname[512]={0};
        unsigned long offset=0; unsigned int inode=0;
        // example: 00400000-0040b000 r-xp 00000000 fc:01 12345 /usr/bin/cat
        int m = sscanf(line, "%lx-%lx %7s %lx %15s %u %511[^\n]",
                       &start, &end, perms, &offset, dev, &inode, mapname);
        unsigned long sz_kb = (end>start)? (end-start)/1024UL : 0;
        if (m >= 6) {
            if (m==6) strcpy(mapname, "");
            if (strstr(mapname,"[heap]")) heap_kb += sz_kb;
            else if (strstr(mapname,"[stack]")) stack_kb += sz_kb;
            else if (strstr(mapname,".so") || strstr(mapname,"/lib")) libs_kb += sz_kb;
            else if (mapname[0]==0 || mapname[0]=='[') anon_kb += sz_kb;
            else file_kb += sz_kb;
        }
    }
    fclose(f);
    printf("Memory map grouping (by virtual size):\n");
    printf("  Heap: "); print_human(heap_kb*1024UL); printf("\n");
    printf("  Stack: "); print_human(stack_kb*1024UL); printf("\n");
    printf("  Shared libraries: "); print_human(libs_kb*1024UL); printf("\n");
    printf("  Anonymous: "); print_human(anon_kb*1024UL); printf("\n");
    printf("  File-backed: "); print_human(file_kb*1024UL); printf("\n");
}

static int collect_stats(pid_t pid, memstats_t* ms, char* comm, size_t commsz) {
    memset(ms,0,sizeof(*ms));
    if (read_status_kb_pid(pid,"VmSize:", &ms->vsz_kb)<0) return -1;
    read_status_kb_pid(pid,"VmRSS:", &ms->rss_kb);
    read_smaps_rollup_pid(pid,&ms->pss_kb,&ms->uss_kb);
    unsigned long minf=0, majf=0;
    read_stat_faults_pid(pid,&minf,&majf,comm,commsz);
    ms->minor_flts = minf;
    ms->major_flts = majf;
    if (ms->uss_kb && ms->rss_kb && ms->rss_kb >= ms->uss_kb) {
        ms->priv_kb = ms->uss_kb;
        ms->shared_kb = ms->rss_kb - ms->uss_kb; // приближение
    }
    return 0;
}

static void print_stats(pid_t pid, const memstats_t* ms, const memstats_t* prev, const char* comm) {
    printf("Process: %s (PID %d)\n", comm?comm:"?", pid);
    printf("VSZ: "); print_human(ms->vsz_kb*1024UL); printf("\n");

    printf("RSS: ");
    print_human(ms->rss_kb*1024UL);
    if (prev) {
        printf("  (Δ ");
        print_human((ms->rss_kb - prev->rss_kb)*1024UL);
        printf(")\n");
    } else {
        printf("\n");
    }

    if (ms->pss_kb) { printf("PSS: "); print_human(ms->pss_kb*1024UL); printf("\n"); }
    if (ms->uss_kb) { printf("USS: "); print_human(ms->uss_kb*1024UL); printf("\n"); }
    printf("Shared: "); print_human(ms->shared_kb*1024UL); printf("\n");
    printf("Private: "); print_human(ms->priv_kb*1024UL); printf("\n");
    printf("Page Faults: Minor %lu", ms->minor_flts);
    if (prev) printf(" (Δ %ld)", (long)(ms->minor_flts - prev->minor_flts));
    printf(", Major %lu", ms->major_flts);
    if (prev) printf(" (Δ %ld)", (long)(ms->major_flts - prev->major_flts));
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n  %s PID\n  %s --watch PID\n  %s --compare PID1 PID2\n", argv[0], argv[0], argv[0]);
        return 1;
    }
    int watch = 0, compare = 0;
    pid_t pid1=0, pid2=0;
    if (strcmp(argv[1],"--watch")==0 && argc>=3) { watch=1; pid1 = (pid_t)atoi(argv[2]); }
    else if (strcmp(argv[1],"--compare")==0 && argc>=4) { compare=1; pid1=(pid_t)atoi(argv[2]); pid2=(pid_t)atoi(argv[3]); }
    else { pid1 = (pid_t)atoi(argv[1]); }

    if (compare) {
        memstats_t a,b; char ca[256]={0}, cb[256]={0};
        if (collect_stats(pid1,&a,ca,sizeof(ca))<0 || collect_stats(pid2,&b,cb,sizeof(cb))<0) {
            fprintf(stderr,"Failed to read stats\n"); return 2;
        }
        printf("=== Compare %d (%s) vs %d (%s) ===\n", pid1, ca, pid2, cb);
        printf("PSS: "); print_human(a.pss_kb*1024UL); printf(" vs "); print_human(b.pss_kb*1024UL); printf("\n");
        printf("USS: "); print_human(a.uss_kb*1024UL); printf(" vs "); print_human(b.uss_kb*1024UL); printf("\n");
        printf("RSS: "); print_human(a.rss_kb*1024UL); printf(" vs "); print_human(b.rss_kb*1024UL); printf("\n");
        return 0;
    }

    if (!watch) {
        memstats_t ms; char comm[256]={0};
        if (collect_stats(pid1,&ms,comm,sizeof(comm))<0) { fprintf(stderr,"Failed to read\n"); return 2; }
        print_stats(pid1,&ms,NULL,comm);
        group_maps(pid1);
        return 0;
    } else {
        memstats_t prev={0}, cur={0}; char comm[256]={0};
        int first=1;
        while (1) {
            if (collect_stats(pid1,&cur,comm,sizeof(comm))<0) { fprintf(stderr,"Process ended or /proc not accessible\n"); return 3; }
            (void)system("clear"); // подавляем предупреждение и удерживаем поведение
            if (first) print_stats(pid1,&cur,NULL,comm);
            else print_stats(pid1,&cur,&prev,comm);
            group_maps(pid1);
            prev = cur;
            fflush(stdout);
            sleep(1);
            first=0;
        }
    }
}