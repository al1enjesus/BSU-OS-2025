#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

void print_separator() {
    printf("\n==================================================\n");
}

void print_memory_info() {
    printf("\n=== Memory Information ===\n");
    
    FILE *status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmSize:", 7) == 0 || 
                strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmData:", 7) == 0 ||
                strncmp(line, "VmStk:", 6) == 0 ||
                strncmp(line, "VmExe:", 6) == 0 ||
                strncmp(line, "VmLib:", 6) == 0) {
                printf("%s", line);
            }
        }
        fclose(status);
    }
    
    FILE *smaps = fopen("/proc/self/smaps_rollup", "r");
    if (smaps) {
        char line[256];
        printf("\n=== Detailed Memory Breakdown ===\n");
        while (fgets(line, sizeof(line), smaps)) {
            if (strncmp(line, "Pss:", 4) == 0 ||
                strncmp(line, "Private", 7) == 0 ||
                strncmp(line, "Shared", 6) == 0) {
                printf("%s", line);
            }
        }
        fclose(smaps);
    }
}

void print_memory_map() {
    printf("\n=== Memory Map ===\n");
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), maps) && count < 20) {
            printf("%s", line);
            count++;
        }
        fclose(maps);
        if (count == 20) {
            printf("... (showing first 20 segments only)\n");
        }
    }
}

int main() {
    printf("Initial memory state:\n");
    print_memory_info();
    
    print_separator();
    printf("Allocating memory using different methods...\n");
    
    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));
    printf("✓ Allocated 1 KB on stack\n");
    
    size_t heap_size = 1024 * 1024; 
    char *heap_var = malloc(heap_size);
    if (heap_var) {
        for (size_t i = 0; i < heap_size; i += 4096) {
            heap_var[i] = 'H';
        }
        printf("✓ Allocated 1 MB on heap (malloc)\n");
    }
    
    size_t mmap_size = 1024 * 1024; 
    char *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var != MAP_FAILED) {
        for (size_t i = 0; i < mmap_size; i += 4096) {
            mmap_var[i] = 'M';
        }
        printf("✓ Allocated 1 MB via mmap (anonymous)\n");
    }
    
    print_separator();
    printf("Memory state after allocations:\n");
    print_memory_info();
    print_memory_map();
    
    if (heap_var) free(heap_var);
    if (mmap_var != MAP_FAILED) munmap(mmap_var, mmap_size);
    
    return 0;
}
