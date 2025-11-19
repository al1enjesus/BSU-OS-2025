#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>

static void shuffle(size_t* a, size_t n){
    for(size_t i=n-1;i>0;i--){
        size_t j = (size_t) (rand()%(i+1));
        size_t t=a[i]; a[i]=a[j]; a[j]=t;
    }
}

static void touch_seq(char* p, size_t bytes){
    for(size_t i=0;i<bytes;i+=4096) p[i]++;
}

static void touch_rand(char* p, size_t bytes){
    size_t pages = (bytes+4095)/4096;
    size_t* idx = malloc(pages*sizeof(size_t));
    for(size_t i=0;i<pages;i++) idx[i]=i*4096;
    shuffle(idx, pages);
    for(size_t i=0;i<pages;i++) p[idx[i]]++;
    free(idx);
}

static void print_faults(const char* tag){
    struct rusage ru; getrusage(RUSAGE_SELF,&ru);
    printf("%s: minor=%ld major=%ld\n", tag, ru.ru_minflt, ru.ru_majflt);
}

int main(int argc, char** argv){
    size_t MiB = 100; /* объём по умолчанию */
    if(argc>=2) MiB = strtoul(argv[1], NULL, 10);
    size_t bytes = MiB*(size_t)1024*1024;

    char* buf = NULL; int r = posix_memalign((void**)&buf, 4096, bytes);
    if(r!=0 || !buf){ perror("alloc"); return 1; }
    memset(buf, 0, bytes);

    print_faults("before");
    touch_seq(buf, bytes);
    print_faults("after_seq");
    touch_rand(buf, bytes);
    print_faults("after_rand");

    free(buf);
    return 0;
}