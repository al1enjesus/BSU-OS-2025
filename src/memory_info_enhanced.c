#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>

static volatile int keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
}

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char line[256];
    unsigned long vm_size = 0, vm_rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) sscanf(line, "VmSize: %lu kB", &vm_size);
        if (strncmp(line, "VmRSS:", 6) == 0) sscanf(line, "VmRSS: %lu kB", &vm_rss);
    }
    fclose(f);
    
    printf("PID %d: VSZ=%luKB RSS=%luKB\n", pid, vm_size, vm_rss);
}

void demonstrate_memory_types() {
    printf("=== Memory Allocation Demo ===\n");
    
    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    
    size_t heap_size = 2 * 1024 * 1024;
    char *heap_var = malloc(heap_size);
    memset(heap_var, 'H', heap_size);
    
    size_t mmap_size = 5 * 1024 * 1024;
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    memset(mmap_var, 'M', mmap_size);
    
    printf("Memory allocated. PID=%d\n", getpid());
    printf("Waiting 20 seconds for analysis...\n");
    
    signal(SIGINT, handle_signal);
    for(int i = 0; i < 20 && keep_running; i++) {
        printf("\rWaiting... %d/20 seconds", i);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");
    
    free(heap_var);
    munmap(mmap_var, mmap_size);
}

int main() {
    demonstrate_memory_types();
    return 0;
}
