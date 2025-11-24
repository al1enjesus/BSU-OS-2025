#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

void print_file(const char *path) {
    printf("\n=== %s ===\n", path);
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen"); return; }
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        printf("%s", buf);
    }
    fclose(f);
}

int main() {
    printf("PID = %d\n", getpid());

    // STACK
    char stack_var[1024];
    stack_var[0] = 1;

    // HEAP
    char *heap_var = malloc(1024 * 1024);
    heap_var[0] = 2;

    // MMAP
    char *mmap_var = mmap(NULL, 1024 * 1024,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);
    mmap_var[0] = 3;

    printf("Memory allocated. Press ENTER to continue...\n");
    getchar();

    print_file("/proc/self/maps");
    print_file("/proc/self/status");
    print_file("/proc/self/smaps_rollup");

free(heap_var);
munmap(mmap_var,mmap_size);
    return 0;
}
