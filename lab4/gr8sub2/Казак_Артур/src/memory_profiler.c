#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Чтение строки по ключу "Key:" из файла /proc/<pid>/status */
static long read_kb_from_status(pid_t pid, const char* key){
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE* f = fopen(path, "r"); if(!f) return -1;
    char *line=NULL; size_t n=0; long val=-1;
    while(getline(&line,&n,f)!=-1){
        if(strncmp(line, key, strlen(key))==0){
            /* формат: Key:\t  12345 kB */
            char *p = strrchr(line, '\t'); if(!p) p = line + strlen(key);
            long kb=0; if(sscanf(line+strlen(key), " %ld kB", &kb)==1) val=kb;
            break;
        }
    }
    free(line); fclose(f); return val;
}

/* Чтение PSS и Private_* (USS) из smaps_rollup */
static long read_kb_from_smaps_rollup(pid_t pid, const char* key){
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE* f = fopen(path, "r"); if(!f) return -1;
    char *line=NULL; size_t n=0; long val=-1;
    while(getline(&line,&n,f)!=-1){
        if(strncmp(line, key, strlen(key))==0){
            long kb=0; if(sscanf(line+strlen(key), " %ld kB", &kb)==1) { val=kb; break; }
        }
    }
    free(line); fclose(f); return val;
}

static int read_faults_from_stat(pid_t pid, unsigned long* minor, unsigned long* major){
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* f = fopen(path, "r"); if(!f) return -1;
    /* поля: 1-pid 2-comm 3-state ... 10-minflt 12-majflt */
    int field=1; unsigned long v; char comm[512], state;
    /* отдельно читаем первые три поля с учетом (comm) в скобках */
    if (fscanf(f, "%*d %511s %c", comm, &state)!=2){ fclose(f); return -1; }
    /* далее пройти до 10 и 12 полей */
    /* уже считаны 3 поля, нужно добраться до 10-го (minflt) и 12-го (majflt) */
    unsigned long dummy;
    for (field=4; field<=9; ++field) if(fscanf(f, "%lu", &dummy)!=1){ fclose(f); return -1; }
    if(fscanf(f, "%lu", minor)!=1){ fclose(f); return -1; }   /* 10 */
    if(fscanf(f, "%lu", &dummy)!=1){ fclose(f); return -1; }  /* 11 */
    if(fscanf(f, "%lu", major)!=1){ fclose(f); return -1; }   /* 12 */
    fclose(f); return 0;
}

int main(int argc, char** argv){
    pid_t pid;
    if(argc>=2) pid = (pid_t)atoi(argv[1]);
    else pid = getpid();

    long vmsize = read_kb_from_status(pid, "VmSize:");
    long vmrss  = read_kb_from_status(pid, "VmRSS:");
    long pss    = read_kb_from_smaps_rollup(pid, "Pss:");
    long priv_c = read_kb_from_smaps_rollup(pid, "Private_Clean:");
    long priv_d = read_kb_from_smaps_rollup(pid, "Private_Dirty:");
    long uss    = -1;
    if (priv_c>=0 && priv_d>=0) uss = priv_c + priv_d;

    unsigned long minflt=0, majflt=0;
    int fr = read_faults_from_stat(pid, &minflt, &majflt);

    printf("PID: %d\n", pid);
    printf("VM Size (VSZ): %ld kB\n", vmsize);
    printf("Resident (RSS): %ld kB\n", vmrss);
    printf("Proportional Set Size (PSS): %ld kB\n", pss);
    printf("Unique Set Size (USS): %ld kB\n", uss);
    if(fr==0) printf("Page Faults: minor=%lu major=%lu\n", minflt, majflt);
    else puts("Page Faults: <error reading /proc/<pid>/stat>");

    /* карта памяти: покажем первые 40 строк */
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* m = fopen(path, "r");
    if(m){
        puts("\n--- /proc/<pid>/maps (first 40 lines) ---");
        char *line=NULL; size_t n=0; int count=0;
        while(count<40 && getline(&line,&n,m)!=-1){ fputs(line, stdout); count++; }
        free(line); fclose(m);
    } else {
        perror("maps");
    }
    return 0;
}