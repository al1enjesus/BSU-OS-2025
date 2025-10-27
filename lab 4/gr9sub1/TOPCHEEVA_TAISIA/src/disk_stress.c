#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    printf("Создаем нагрузку на диск...\n");
    printf("PID: %d\n", getpid());
    
    int file_count = 5;
    int file_size = 2 * 1024 * 1024; // 2 MB каждый
    
    for (int i = 0; i < file_count; i++) {
        char filename[100];
        sprintf(filename, "stress_file_%d.bin", i);
        
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) continue;
        
        char *buffer = malloc(4096);
        memset(buffer, i % 256, 4096);
        
        for (int written = 0; written < file_size; written += 4096) {
            write(fd, buffer, 4096);
        }
        
        close(fd);
        free(buffer);
        printf("Создан файл: %s\n", filename);
    }
    
    printf("Нагрузка создана. Файлы: stress_file_*.bin\n");
    return 0;
}
