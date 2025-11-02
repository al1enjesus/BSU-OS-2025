#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SIZE_1MB (1024 * 1024)

void print_human_readable(unsigned long bytes) {
    if (bytes < 1024)
        printf("%lu B", bytes);
    else if (bytes < 1024*1024)
        printf("%.1f KB", bytes/1024.0);
    else if (bytes < 1024*1024*1024)
        printf("%.1f MB", bytes/1024.0/1024.0);
    else
        printf("%.1f GB", bytes/1024.0/1024.0/1024.0);
}

void read_memory_metrics() {
    printf("\n=== Memory Metrics from /proc/self/status ===\n");
    
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        perror("Failed to open /proc/self/status");
        return;
    }
    
    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &vm_stk);
        }
    }
    fclose(f);
    
    printf("VSZ (Virtual):    "); print_human_readable(vm_size * 1024); printf("\n");
    printf("RSS (Resident):   "); print_human_readable(vm_rss * 1024); printf("\n");
    printf("Data/Heap:        "); print_human_readable(vm_data * 1024); printf("\n");
    printf("Stack:            "); print_human_readable(vm_stk * 1024); printf("\n");
}

void read_smaps_rollup() {
    printf("\n=== Detailed Memory from /proc/self/smaps_rollup ===\n");
    
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) {
        printf("smaps_rollup not available (older kernel)\n");
        return;
    }
    
    char line[256];
    unsigned long pss = 0, private_clean = 0, private_dirty = 0;
    unsigned long shared_clean = 0, shared_dirty = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &pss);
        } else if (strncmp(line, "Private_Clean:", 14) == 0) {
            sscanf(line, "Private_Clean: %lu kB", &private_clean);
        } else if (strncmp(line, "Private_Dirty:", 14) == 0) {
            sscanf(line, "Private_Dirty: %lu kB", &private_dirty);
        } else if (strncmp(line, "Shared_Clean:", 13) == 0) {
            sscanf(line, "Shared_Clean: %lu kB", &shared_clean);
        } else if (strncmp(line, "Shared_Dirty:", 13) == 0) {
            sscanf(line, "Shared_Dirty: %lu kB", &shared_dirty);
        }
    }
    fclose(f);
    
    printf("PSS:              "); print_human_readable(pss * 1024); printf("\n");
    unsigned long uss = (private_clean + private_dirty) * 1024;
    printf("USS (Unique):     "); print_human_readable(uss); printf("\n");
    printf("Private Clean:    "); print_human_readable(private_clean * 1024); printf("\n");
    printf("Private Dirty:    "); print_human_readable(private_dirty * 1024); printf("\n");
    printf("Shared Clean:     "); print_human_readable(shared_clean * 1024); printf("\n");
    printf("Shared Dirty:     "); print_human_readable(shared_dirty * 1024); printf("\n");
}

void print_memory_map() {
    printf("\n=== Memory Map from /proc/self/maps ===\n");
    
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) {
        perror("Failed to open /proc/self/maps");
        return;
    }
    
    printf("%-18s %-6s %10s  %s\n", "Address Range", "Perms", "Size", "Path");
    printf("----------------------------------------------------------------\n");
    
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path[256] = "";
        
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]", 
                   &start, &end, perms, path) >= 3) {
            
            unsigned long size = end - start;
            
            printf("%08lx-%08lx %-6s ", start, end, perms);
            print_human_readable(size);
            printf("  %s\n", path[0] ? path : "(anonymous)");
            
            count++;
            if (count >= 15) {
                printf("... (showing first 15 segments)\n");
                break;
            }
        }
    }
    fclose(f);
}

void demonstrate_memory_allocation() {
    printf("=== Demonstrating Different Memory Types ===\n\n");
    
    printf("Initial memory state:\n");
    read_memory_metrics();
    printf("\n");
    
    // 1. Stack allocation
    char stack_var[SIZE_1MB];  // 1 MB on stack
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack: allocated 1 MB at %p\n", (void*)stack_var);
    
    // 2. Heap allocation (malloc)
    char *heap_var = malloc(SIZE_1MB);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }
    memset(heap_var, 'H', SIZE_1MB);
    printf("2. Heap (malloc): allocated 1 MB at %p\n", (void*)heap_var);
    
    // 3. Anonymous mmap
    char *mmap_var = mmap(NULL, SIZE_1MB, PROT_READ | PROT_WRITE, 
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }
    memset(mmap_var, 'M', SIZE_1MB);
    printf("3. Anonymous mmap: allocated 1 MB at %p\n", (void*)mmap_var);
    
    // 4. File-backed mmap
    int fd = open("test_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        free(heap_var);
        munmap(mmap_var, SIZE_1MB);
        return;
    }
    
    // Set file size
    if (ftruncate(fd, SIZE_1MB) == -1) {
        perror("ftruncate failed");
        close(fd);
        free(heap_var);
        munmap(mmap_var, SIZE_1MB);
        return;
    }
    
    char *file_mmap = mmap(NULL, SIZE_1MB, PROT_READ | PROT_WRITE, 
                          MAP_SHARED, fd, 0);
    if (file_mmap == MAP_FAILED) {
        perror("file mmap failed");
        close(fd);
        free(heap_var);
        munmap(mmap_var, SIZE_1MB);
        return;
    }
    memset(file_mmap, 'F', SIZE_1MB);
    printf("4. File-backed mmap: allocated 1 MB at %p\n", (void*)file_mmap);
    
    printf("\nAfter allocation:\n");
    printf("================\n");
    
    // Show memory metrics after allocation
    read_memory_metrics();
    read_smaps_rollup();
    print_memory_map();
    
    printf("\nPress Enter to free memory and exit...");
    getchar();
    
    // Cleanup
    free(heap_var);
    munmap(mmap_var, SIZE_1MB);
    munmap(file_mmap, SIZE_1MB);
    close(fd);
    unlink("test_file.bin");
}

int main() {
    printf("Memory Analysis Demo - Variant 2\n");
    printf("================================\n");
    
    demonstrate_memory_allocation();
    
    return 0;
}
