#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void print_memory_metrics(const char *stage) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        perror("Failed to open /proc/self/status");
        return;
    }

    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line + 7, "%lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line + 7, "%lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line + 6, "%lu kB", &vm_stk);
        }
    }
    fclose(f);

    unsigned long pss = 0, private_clean = 0, private_dirty = 0;
    f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) {
        fprintf(stderr, "Warning: /proc/self/smaps_rollup not available\n");
    } else {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "Pss: ", 5) == 0) {
                sscanf(line + 5, "%lu kB", &pss);
            } else if (strncmp(line, "Private_Clean: ", 15) == 0) {
                sscanf(line + 15, "%lu kB", &private_clean);
            } else if (strncmp(line, "Private_Dirty: ", 14) == 0) {
                sscanf(line + 14, "%lu kB", &private_dirty);
            }
        }
        fclose(f);
    }
    unsigned long uss = private_clean + private_dirty;

    unsigned long minflt = 0, majflt = 0;
    f = fopen("/proc/self/stat", "r");
    if (f) {
        if (fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %lu", &minflt, &majflt) != 2) {
            fprintf(stderr, "Warning: Failed to read page faults from /proc/self/stat\n");
            minflt = majflt = 0;
        }
        fclose(f);
    } else {
        perror("Failed to open /proc/self/stat");
    }

    printf(ANSI_COLOR_GREEN "Metrics and Map %s:\n" ANSI_COLOR_RESET, stage);
    printf(ANSI_COLOR_YELLOW "Memory Metrics (%s):\n" ANSI_COLOR_RESET, stage);
    printf(ANSI_COLOR_YELLOW "  Minor Faults: %lu\n" ANSI_COLOR_RESET, minflt);
    printf(ANSI_COLOR_YELLOW "  Major Faults: %lu\n" ANSI_COLOR_RESET, majflt);
    printf(ANSI_COLOR_YELLOW "  VmSize (VSZ): %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, vm_size, vm_size / 1024.0);
    printf(ANSI_COLOR_YELLOW "  VmRSS:        %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, vm_rss, vm_rss / 1024.0);
    printf(ANSI_COLOR_YELLOW "  VmData:       %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, vm_data, vm_data / 1024.0);
    printf(ANSI_COLOR_YELLOW "  VmStk:        %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, vm_stk, vm_stk / 1024.0);
    printf(ANSI_COLOR_YELLOW "  PSS:          %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, pss, pss / 1024.0);
    printf(ANSI_COLOR_YELLOW "  USS:          %lu KB (%.1f MB)\n" ANSI_COLOR_RESET, uss, uss / 1024.0);
}

void print_memory_map(const char *stage) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) {
        perror("Failed to open /proc/self/maps");
        return;
    }

    printf(ANSI_COLOR_BLUE "Memory Map (%s):\n" ANSI_COLOR_RESET, stage);
    printf("Address Range        Perms    Size (KB)  Path\n");
    printf("------------------------------------------------\n");

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path[256];
        int offset, dev_major, dev_minor, inode;
        sscanf(line, "%lx-%lx %s %x %x:%x %d %s", &start, &end, perms, &offset, &dev_major, &dev_minor, &inode, path);
        unsigned long size = (end - start) / 1024;
        printf("%016lx-%016lx %-8s %8lu KB %s\n", start, end, perms, size, path);
    }
    fclose(f);

    // ASCII visualization
    printf(ANSI_COLOR_BLUE "ASCII Memory Map Visualization (%s):\n" ANSI_COLOR_RESET, stage);
    printf("[ Low addresses ]\n");
    f = fopen("/proc/self/maps", "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            unsigned long start, end;
            char perms[5], path[256];
            int offset, dev_major, dev_minor, inode;
            sscanf(line, "%lx-%lx %s %x %x:%x %d %s", &start, &end, perms, &offset, &dev_major, &dev_minor, &inode, path);
            unsigned long size = (end - start) / 1024;
            int bars = size / 100; // 1 bar = 100 KB
            printf("| %016lx-%016lx | %-8s | %8lu KB | ", start, end, perms, size);
            for (int i = 0; i < bars; i++) printf("█");
            printf(" | %s\n", path);
        }
        fclose(f);
    }
    printf("[ High addresses ]\n");
}

int main() {
    char stack_array[1024]; // 1 КБ на стеке
    void *heap_ptr = NULL;
    void *mmap_ptr = NULL;

    // До выделения памяти
    print_memory_metrics("Before Allocation");
    print_memory_map("Before Allocation");
    sleep(10); // Пауза 10 секунд для проверки /proc/[PID]/smaps_rollup
    printf("Press Enter to allocate memory...\n");
    getchar();

    // Выделение памяти
    heap_ptr = malloc(2 * 1024 * 1024); // 2 МБ
    if (heap_ptr) {
        memset(heap_ptr, 0, 2 * 1024 * 1024);
        printf("1. Stack: 1 KB at %p\n", stack_array);
        printf("2. Heap: 2 MB at %p\n", heap_ptr);
    }
    mmap_ptr = mmap(NULL, 1 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_ptr != MAP_FAILED) {
        memset(mmap_ptr, 0, 1 * 1024 * 1024);
        printf("3. Anonymous mmap: 1 MB at %p\n", mmap_ptr);
    }
    print_memory_metrics("After Allocation");
    print_memory_map("After Allocation");
    sleep(10); // Пауза 10 секунд
    printf("Press Enter to free memory and show metrics again...\n");
    getchar();

    // Освобождение памяти
    if (heap_ptr) free(heap_ptr);
    if (mmap_ptr != MAP_FAILED) munmap(mmap_ptr, 1 * 1024 * 1024);
    print_memory_metrics("After Freeing");
    print_memory_map("After Freeing");
    sleep(10); // Пауза 10 секунд

    return 0;
}
