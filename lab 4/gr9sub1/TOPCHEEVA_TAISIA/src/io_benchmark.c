#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

void test_method(const char* method_name, const char* filename, int buffer_size, int use_stdio) {
    int total_size = 5 * 1024 * 1024;
    
    clock_t start = clock();
    
    if (use_stdio) {
        FILE* f = fopen(filename, "wb");
        if (!f) return;
        
        char* buffer = malloc(buffer_size);
        memset(buffer, 'A', buffer_size);
        
        for (int written = 0; written < total_size; written += buffer_size) {
            fwrite(buffer, 1, buffer_size, f);
        }
        fflush(f);
        fclose(f);
        free(buffer);
    } else {
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) return;
        
        char* buffer = malloc(buffer_size);
        memset(buffer, 'A', buffer_size);
        
        for (int written = 0; written < total_size; written += buffer_size) {
            write(fd, buffer, buffer_size);
        }
        fsync(fd);
        close(fd);
        free(buffer);
    }
    
    clock_t end = clock();
    double time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (time_used > 0) {
        printf("%s (буфер %6d байт): %.2f сек, %.2f MB/с\n", 
               method_name, buffer_size, time_used, 5.0 / time_used);
    }
}

int main() {
    printf("=== I/O Benchmark ===\n");
    printf("Тестовый файл: 5 MB\n\n");
    
    int buffer_sizes[] = {512, 4096, 16384, 65536};
    int num_sizes = 4;
    
    for (int i = 0; i < num_sizes; i++) {
        char fname1[100], fname2[100];
        sprintf(fname1, "test_fwrite_%d.bin", buffer_sizes[i]);
        sprintf(fname2, "test_write_%d.bin", buffer_sizes[i]);
        
        test_method("fwrite", fname1, buffer_sizes[i], 1);
        test_method("write ", fname2, buffer_sizes[i], 0);
        printf("---\n");
        
        remove(fname1);
        remove(fname2);
    }
    
    return 0;
}
