#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}

static off_t filesize(int fd){
    struct stat st; if(fstat(fd,&st)==-1) return -1; return st.st_size;
}

static int bench_readsys(const char* path){
    int fd = open(path, O_RDONLY);
    if(fd<0){ perror("open"); return 1; }
    off_t sz = filesize(fd); if(sz<0){ perror("fstat"); close(fd); return 1; }
    const size_t BUFSZ = 1<<20; // 1 MiB
    char* buf = (char*)malloc(BUFSZ); if(!buf){ perror("malloc"); close(fd); return 1; }

    double t0 = now_sec();
    size_t total=0; for(;;){
        ssize_t r = read(fd, buf, BUFSZ);
        if(r<0){ perror("read"); free(buf); close(fd); return 1; }
        if(r==0) break;
        total += (size_t)r;
    }
    double t1 = now_sec();
    double dt = t1-t0;
    double mb = sz/ (1024.0*1024.0);
    printf("readsys: size=%.2f MiB time=%.3f s throughput=%.2f MiB/s\n", mb, dt, mb/dt);

    free(buf); close(fd); return 0;
}

static int bench_mmap(const char* path){
    int fd = open(path, O_RDONLY);
    if(fd<0){ perror("open"); return 1; }
    off_t sz = filesize(fd); if(sz<=0){ perror("fstat"); close(fd); return 1; }

    double t0 = now_sec();
    void* p = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if(p==MAP_FAILED){ perror("mmap"); close(fd); return 1; }

    volatile unsigned long long sum=0;
    const unsigned char* c=(const unsigned char*)p;
    for(off_t i=0;i<sz;i+=4096) sum += c[i]; // "прочёсываем" каждые 4KiB
    if(sz>0) sum += c[sz-1];

    double t1 = now_sec();
    double dt = t1-t0;
    double mb = sz/ (1024.0*1024.0);
    printf("mmap: size=%.2f MiB time=%.3f s throughput~=%.2f MiB/s (approx)\n", mb, dt, mb/dt);

    munmap((void*)p, sz); close(fd); (void)sum; return 0;
}

int main(int argc, char** argv){
    if(argc!=3){
        fprintf(stderr, "Usage:\n  %s readsys <file>\n  %s mmap <file>\n", argv[0], argv[0]);
        return 2;
    }
    const char* mode = argv[1];
    const char* path = argv[2];
    if(strcmp(mode,"readsys")==0) return bench_readsys(path);
    if(strcmp(mode,"mmap")==0)    return bench_mmap(path);
    fprintf(stderr, "unknown mode: %s\n", mode); return 2;
}