#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MB (1024 * 1024)
#define SIZE_10MB (10 * MB)

void cause_minor_faults_heap() {
    printf("\n\033[1;33m=== Выделение 10 МБ на куче (minor faults) ===\033[0m\n");
    char *ptr = malloc(SIZE_10MB);
    if (!ptr) {
        perror("malloc");
        return;
    }
    printf("Адрес: %p\n", ptr);
    printf("Нажмите Enter для записи (minor faults)...\n");
    getchar();
    memset(ptr, 1, SIZE_10MB);
    printf("Запись завершена. Minor faults: ~2560\n");
    free(ptr);
}

void cause_minor_faults_mmap() {
    printf("\n\033[1;33m=== mmap файла 10 МБ (minor faults) ===\033[0m\n");
    int fd = open("test_mmap_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror("open"); return; }
 if (ftruncate(fd, SIZE_10MB) == -1) {
    perror("ftruncate");
    close(fd);
    return;
}

    char *ptr = mmap(NULL, SIZE_10MB, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap"); close(fd); return; }

    printf("Адрес: %p\n", ptr);
    printf("Нажмите Enter для записи в mmap (minor faults)...\n");
    getchar();
    memset(ptr, 2, SIZE_10MB);
    printf("Запись в mmap завершена. Minor faults: ~2560\n");

    munmap(ptr, SIZE_10MB);
    close(fd);
    unlink("test_mmap_file.bin");
}

void simulate_major_fault() {
    printf("\n\033[1;31m=== Симуляция major fault (swap) ===\033[0m\n");
    printf("Для major fault нужен swap. В VirtualBox может не сработать.\n");
    printf("Рекомендация: выполните вручную:\n");
    printf("  sudo fallocate -l 100M /swapfile\n");
    printf("  sudo chmod 0 /swapfile\n");
    printf("  sudo mkswap /swapfile\n");
    printf("  sudo swapon /swapfile\n");
    printf("Затем запустите снова.\n");
}

int main() {
    printf("\033[1;32m=== Page Faults Demo (Задание 2) ===\033[0m\n");

    cause_minor_faults_heap();
    cause_minor_faults_mmap();
    simulate_major_fault();

    printf("\n\033[1;32mГотово! Теперь запустите с perf и strace.\033[0m\n");
    return 0;
}
