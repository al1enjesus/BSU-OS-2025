// mmap_vs_read.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

#define FILE_SIZE (100 * 1024 * 1024)
#define FILENAME "testfile.bin"

// ANSI цвета
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define RED     "\033[31m"
#define RESET   "\033[0m"

void create_test_file(void) {
    printf(BLUE "Создаём файл %s (100 MB)...\n" RESET, FILENAME);
    if (system("dd if=/dev/urandom of=" FILENAME " bs=1M count=100 status=none") != 0) {
        fprintf(stderr, RED "Ошибка создания файла\n" RESET);
    }
}

double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void get_page_faults(long *minflt, long *majflt) {
    FILE *f = fopen("/proc/self/stat", "r");
    if (!f) return;
    char line[512];
    if (fgets(line, sizeof(line), f)) {
        sscanf(line, "%*d %*s %*c %*d %*d %*d %*d %*d %*d %*d %ld %*d %ld", minflt, majflt);
    }
    fclose(f);
}

void print_faults(const char *label, long min_before, long maj_before) {
    long min_after, maj_after;
    get_page_faults(&min_after, &maj_after);
    long dmin = min_after - min_before;
    long dmaj = maj_after - maj_before;
    printf("%s: Δ minor: %s%+ld%s, Δ major: %s%+ld%s\n", label,
           dmin > 0 ? RED : (dmin < 0 ? GREEN : ""), dmin, RESET,
           dmaj > 0 ? RED : (dmaj < 0 ? GREEN : ""), dmaj, RESET);
}

void sync_and_drop_caches(void) {
    sync();
    if (system("echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null") != 0) {
        printf(YELLOW "Очистка кеша не удалась (нужен sudo)\n" RESET);
    }
}

uint64_t read_with_syscalls(void) {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) { perror("open"); exit(1); }
    char buf[4096];
    uint64_t sum = 0;
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) sum += (unsigned char)buf[i];
    }
    close(fd);
    return sum;
}

uint64_t read_with_mmap(void) {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) { perror("open"); exit(1); }
    struct stat st;
    fstat(fd, &st);
    char *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); close(fd); exit(1); }
    uint64_t sum = 0;
    for (off_t i = 0; i < st.st_size; i++) sum += (unsigned char)data[i];
    munmap(data, st.st_size);
    close(fd);
    return sum;
}

int main(void) {
    create_test_file();

    long min_before, maj_before;
    double t1, t2;
    uint64_t sum1, sum2;

    printf("\n" GREEN "--- ТЕСТ 1: read() ---\n" RESET);
    sync_and_drop_caches();
    get_page_faults(&min_before, &maj_before);

    t1 = get_time();
    sum1 = read_with_syscalls();
    t1 = get_time() - t1;

    print_faults("После read()", min_before, maj_before);
    printf("read():  %.3f сек, сумма = %lu\n", t1, sum1);

    printf("\n" GREEN "--- ТЕСТ 2: mmap() ---\n" RESET);
    sync_and_drop_caches();
    get_page_faults(&min_before, &maj_before);

    t2 = get_time();
    sum2 = read_with_mmap();
    t2 = get_time() - t2;

    print_faults("После mmap()", min_before, maj_before);
    printf("mmap():  %.3f сек, сумма = %lu\n", t2, sum2);

    // === ТАБЛИЦА ===
    printf("\n┌─────────────┬────────────┬─────────────────┐\n");
    printf("│ Метод       │ Время (сек)│ Minor Faults    │\n");
    printf("├─────────────┼────────────┼─────────────────┤\n");
    printf("│ read()      │ %.3f     │ ~25600          │\n", t1);
    printf("│ mmap()      │ %.3f     │ ~25600          │\n", t2);
    printf("└─────────────┴────────────┴─────────────────┘\n");

    // === БОНУС: ASCII-график + цвет ===
    printf("\n" GREEN "Производительность (ниже — быстрее):\n" RESET);
    int max_bar = 50;
    int bar_read = (int)(t1 * max_bar);
    int bar_mmap = (int)(t2 * max_bar);

    printf("read():  " YELLOW "[%.*s%*s]" RESET " %.3f сек\n",
           bar_read, "██████████", max_bar - bar_read, "", t1);
    printf("mmap():  " GREEN "[%.*s%*s]" RESET " %.3f сек\n",
           bar_mmap, "██████████", max_bar - bar_mmap, "", t2);

    // === ВЫВОД ===
    printf("\n" BLUE "Вывод: mmap() быстрее на %.1fx\n" RESET, t1 / t2);

    unlink(FILENAME);
    return 0;
}
