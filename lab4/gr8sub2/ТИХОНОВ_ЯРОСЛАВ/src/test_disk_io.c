#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#define FILE_NAME "disk_test.bin"
#define FILE_SIZE (500 * 1024 * 1024) // 500 MB
#define BUF_SIZE  (64 * 1024)         // 64 KB буфер

double now_sec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

void write_file() {
    printf("\n=== Запись файла %s (%d MB) ===\n", FILE_NAME, FILE_SIZE / (1024 * 1024));
    int fd = open(FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open for write");
        exit(1);
    }

    char *buf = malloc(BUF_SIZE);
    for (int i = 0; i < BUF_SIZE; i++)
        buf[i] = (char)(rand() % 256);

    double start = now_sec();
    size_t written = 0;
    while (written < FILE_SIZE) {
        ssize_t n = write(fd, buf, BUF_SIZE);
        if (n < 0) {
            perror("write");
            close(fd);
            free(buf);
            exit(1);
        }
        written += n;
    }
    fsync(fd);
    close(fd);
    double end = now_sec();
    printf("Время записи: %.2f секунд, скорость: %.2f MB/s\n",
           end - start, (FILE_SIZE / (1024.0 * 1024.0)) / (end - start));
    free(buf);
}

void read_file() {
    printf("\n=== Чтение файла %s ===\n", FILE_NAME);
    int fd = open(FILE_NAME, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        exit(1);
    }

    char *buf = malloc(BUF_SIZE);
    double start = now_sec();
    size_t read_total = 0;
    ssize_t n;
    while ((n = read(fd, buf, BUF_SIZE)) > 0)
        read_total += n;
    double end = now_sec();
    close(fd);
    printf("Время чтения: %.2f секунд, скорость: %.2f MB/s\n",
           end - start, (read_total / (1024.0 * 1024.0)) / (end - start));
    free(buf);
}

int main() {
    printf("PID: %d\n", getpid());
    printf("Программа активно пишет и читает с диска.\n");
    printf("Открой другой терминал и наблюдай:\n");
    printf("  iostat -x 1 10\n");
    printf("  pidstat -d 1 10 -p %d\n", getpid());
    printf("  cat /proc/%d/io\n", getpid());
    printf("  sudo iotop -b -n 10\n");
    printf("\nНажми Enter, чтобы начать запись...\n");
    getchar();

    write_file();

    printf("\nНажми Enter, чтобы начать чтение...\n");
    getchar();

    read_file();

    printf("\nГотово.\n");
    return 0;
}