#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

static unsigned long read_mem_available_kb(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    unsigned long kb = 0;
    char key[64];
    unsigned long val;
    char unit[16];
    while (fscanf(f, "%63s %lu %15s", key, &val, unit) == 3) {
        if (strcmp(key, "MemAvailable:") == 0) {
            kb = val;
            break;
        }
    }
    fclose(f);
    return kb;
}

static size_t page_align_down(size_t x) {
    size_t pg = (size_t) sysconf(_SC_PAGESIZE);
    return x - (x % pg);
}

static void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %lu kB", &vm_size) == 1) continue;
        if (sscanf(line, "VmRSS: %lu kB", &vm_rss) == 1) continue;
        if (sscanf(line, "VmData: %lu kB", &vm_data) == 1) continue;
        if (sscanf(line, "VmStk: %lu kB", &vm_stk) == 1) continue;
    }
    fclose(f);

    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size / 1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss / 1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data / 1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk / 1024.0);
}

static void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    printf("\nMemory Map:\n");
    printf("%-18s %-6s %-8s %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start = 0, end = 0;
        char perms[5] = "----";
        char path_str[256] = "";
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", &start, &end, perms, path_str) < 3) {
            continue;
        }
        unsigned long size_kb = (end - start) / 1024;
        printf("%08lx-%08lx %-6s %6luKB  %s\n",
               start, end, perms, size_kb, path_str[0] ? path_str : "(anonymous)");
    }
    fclose(f);
}

static void demonstrate_memory_types(void) {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void *) stack_var);

    size_t heap_size = 10 * 1024 * 1024;
    size_t mmap_size = 50 * 1024 * 1024;

    unsigned long avail_kb = read_mem_available_kb();
    size_t need_kb = (heap_size + mmap_size) / 1024;
    if (avail_kb && need_kb > (avail_kb * 8) / 10) {
        long extra_kb = (long) need_kb - (long) ((avail_kb * 8) / 10);
        if (extra_kb > 0) {
            size_t cut = (size_t) extra_kb * 1024;
            if (cut >= mmap_size) {
                mmap_size = 0;
            } else {
                mmap_size = page_align_down(mmap_size - cut);
                if (mmap_size < 4 * 1024 * 1024) mmap_size = 0;
            }
        }
    }

    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }

    size_t pg = (size_t) sysconf(_SC_PAGESIZE);
    for (size_t i = 0; i < heap_size; i += pg) heap_var[i] = 1;
    printf("2. Heap allocated & touched: %.0f MB at %p\n", heap_size / (1024.0 * 1024.0), (void *) heap_var);

    void *mmap_var = NULL;
    if (mmap_size > 0) {
        mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mmap_var == MAP_FAILED) {
            perror("mmap failed");
            mmap_var = NULL;
        } else {
            for (size_t i = 0; i < mmap_size; i += pg) ((char *) mmap_var)[i] = 2;
            printf("3. Anonymous mmap (touched): %.0f MB at %p\n", mmap_size / (1024.0 * 1024.0), mmap_var);
        }
    }

    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    if (isatty(STDIN_FILENO)) {
        printf("Press Enter to see memory info and map...\n");
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    } else {
        printf("(Non-interactive stdin) continuing in 1s...\n");
        sleep(1);
    }

    print_memory_metrics(getpid());
    print_memory_map(getpid());

    if (isatty(STDIN_FILENO)) {
        printf("\nPress Enter to free memory and exit...\n");
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    } else {
        printf("(Non-interactive stdin) exiting in 1s...\n");
        sleep(1);
    }

    free(heap_var);
    if (mmap_var && mmap_var != MAP_FAILED && mmap_size > 0) munmap(mmap_var, mmap_size);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        printf("Memory Info Demo\n");
        printf("================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }
    return 0;
}
