#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

static void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen status"); return; }

    char line[256];
    unsigned long vm_size=0, vm_rss=0, vm_data=0, vm_stk=0, vm_exe=0, vm_lib=0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB",  &vm_rss)  == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB",  &vm_stk)  == 1) continue;
        if (sscanf(line, "VmExe: %lu kB",  &vm_exe)  == 1) continue;
        if (sscanf(line, "VmLib: %lu kB",  &vm_lib)  == 1) continue;
    }
    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss,  vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk,  vm_stk/1024.0);
    printf("  Text(Code):     %lu KB (%.1f MB)\n", vm_exe,  vm_exe/1024.0);
    printf("  Libs:           %lu KB (%.1f MB)\n", vm_lib,  vm_lib/1024.0);
}

static void print_memory_map(pid_t pid) {
    char path[256]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen maps"); return; }

    printf("\nMemory Map:\n");
    printf("%-23s %-6s %-10s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("-----------------------------------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start=0, end=0; char perms[5]=""; char name[256]="";
        // path optional
        int n = sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", &start, &end, perms, name);
        unsigned long kb = (end - start) / 1024UL;
        if (n == 3) name[0] = '\0';
        printf("%016lx-%016lx %-6s %8lu KB  %s\n",
               start, end, perms, kb, name[0] ? name : "(anonymous)");
    }
    fclose(f);
}

static void touch_pages(char *p, size_t size) {
    const size_t step = 4096;
    for (size_t i=0; i<size; i+=step) p[i] = (char)(i & 0xff);
}

static void demonstrate_memory_types(void) {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable: 1 KB at %p\n", (void*)stack_var);

    size_t heap_size = 10 * 1024 * 1024;
    char *heap_var = (char*)malloc(heap_size);
    if (!heap_var) { perror("malloc"); return; }
    touch_pages(heap_var, heap_size); // реальный аллок
    printf("2. Heap allocated & touched: 10 MB at %p\n", (void*)heap_var);

    size_t mmap_size = 50 * 1024 * 1024;
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE,
                          MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) { perror("mmap"); free(heap_var); return; }
    touch_pages((char*)mmap_var, mmap_size);
    printf("3. Anonymous mmap & touched: 50 MB at %p\n", mmap_var);

    printf("\nCheck /proc/%d/maps for regions. Press Enter to show metrics & map...\n", getpid());
    getchar();

    print_memory_metrics(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    munmap(mmap_var, mmap_size);
    free(heap_var);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = (pid_t)atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        printf("Memory Info Demo\n================\n");
        demonstrate_memory_types();
    }
    return 0;
}
