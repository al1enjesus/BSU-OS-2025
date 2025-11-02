 #define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>

#define BUFFER_SIZE (64 * 1024)
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

typedef struct {
    long minor_faults;
    long major_faults;
} PageFaults;

typedef struct {
    double time_seconds;
    long minor_faults;
    long major_faults;
    unsigned long long checksum;
} BenchmarkResult;

PageFaults get_page_faults(void) {
    PageFaults stats = {0, 0};
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    stats.minor_faults = usage.ru_minflt;
    stats.major_faults = usage.ru_majflt;
    return stats;
}

double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void print_bar(const char* label, double value, double max_value, const char* unit) {
    int bar_width = 40;
    int filled = (int)((value / max_value) * bar_width);
    if (filled > bar_width) filled = bar_width;
    
    printf("%-20s [", label);
    for (int i = 0; i < filled; i++) printf("█");
    for (int i = filled; i < bar_width; i++) printf("░");
    printf("] %.3f %s\n", value, unit);
}

BenchmarkResult read_with_syscalls(const char *filename) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Метод 1: системный вызов read() ===" COLOR_RESET "\n");
    BenchmarkResult result = {0, 0, 0, 0};
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open не удался");
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat не удался");
        close(fd);
        return result;
    }

    printf("Размер файла: %ld байт (%.2f МБ)\n", sb.st_size, 
           sb.st_size / (1024.0 * 1024.0));

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc не удался");
        close(fd);
        return result;
    }

    PageFaults start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            sum += (unsigned char)buffer[i];
        }
    }

    if (bytes_read == -1) {
        perror("read не удался");
    }

    double end_time = get_time();
    PageFaults end_faults = get_page_faults();

    result.time_seconds = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf(COLOR_GREEN "Затрачено времени: %.3f секунд\n" COLOR_RESET, result.time_seconds);
    printf("Минорные ошибки страниц: %ld\n", result.minor_faults);
    printf("Мажорные ошибки страниц: %ld\n", result.major_faults);
    printf("Контрольная сумма: %llu\n", result.checksum);
    printf(COLOR_YELLOW "Пропускная способность: %.2f МБ/с\n" COLOR_RESET, 
           (sb.st_size / (1024.0 * 1024.0)) / result.time_seconds);

    free(buffer);
    close(fd);
    return result;
}

BenchmarkResult read_with_mmap(const char *filename) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Метод 2: mmap() ===" COLOR_RESET "\n");
    BenchmarkResult result = {0, 0, 0, 0};

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open не удался");
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat не удался");
        close(fd);
        return result;
    }

    printf("Размер файла: %ld байт (%.2f МБ)\n", sb.st_size,
           sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap не удался");
        close(fd);
        return result;
    }

    PageFaults start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;
    
    for (off_t i = 0; i < sb.st_size; i++) {
        sum += bytes[i];
    }

    double end_time = get_time();
    PageFaults end_faults = get_page_faults();

    result.time_seconds = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf(COLOR_GREEN "Затрачено времени: %.3f секунд\n" COLOR_RESET, result.time_seconds);
    printf("Минорные ошибки страниц: %ld\n", result.minor_faults);
    printf("Мажорные ошибки страниц: %ld\n", result.major_faults);
    printf("Контрольная сумма: %llu\n", result.checksum);
    printf(COLOR_YELLOW "Пропускная способность: %.2f МБ/с\n" COLOR_RESET,
           (sb.st_size / (1024.0 * 1024.0)) / result.time_seconds);

    munmap(data, sb.st_size);
    close(fd);
    return result;
}

BenchmarkResult read_with_mmap_sequential(const char *filename) {
    printf("\n" COLOR_BOLD COLOR_CYAN "=== Метод 3: mmap() + madvise(SEQUENTIAL) ===" COLOR_RESET "\n");
    BenchmarkResult result = {0, 0, 0, 0};

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open не удался");
        return result;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat не удался");
        close(fd);
        return result;
    }

    printf("Размер файла: %ld байт (%.2f МБ)\n", sb.st_size,
           sb.st_size / (1024.0 * 1024.0));

    void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap не удался");
        close(fd);
        return result;
    }

    madvise(data, sb.st_size,POSIX_MADV_SEQUENTIAL);

    PageFaults start_faults = get_page_faults();
    double start_time = get_time();

    unsigned long long sum = 0;
    unsigned char *bytes = (unsigned char *)data;
    
    for (off_t i = 0; i < sb.st_size; i++) {
        sum += bytes[i];
    }

    double end_time = get_time();
    PageFaults end_faults = get_page_faults();

    result.time_seconds = end_time - start_time;
    result.minor_faults = end_faults.minor_faults - start_faults.minor_faults;
    result.major_faults = end_faults.major_faults - start_faults.major_faults;
    result.checksum = sum;

    printf(COLOR_GREEN "Затрачено времени: %.3f секунд\n" COLOR_RESET, result.time_seconds);
    printf("Минорные ошибки страниц: %ld\n", result.minor_faults);
    printf("Мажорные ошибки страниц: %ld\n", result.major_faults);
    printf("Контрольная сумма: %llu\n", result.checksum);
    printf(COLOR_YELLOW "Пропускная способность: %.2f МБ/с\n" COLOR_RESET,
           (sb.st_size / (1024.0 * 1024.0)) / result.time_seconds);

    munmap(data, sb.st_size);
    close(fd);
    return result;
}

void create_test_file(const char *filename, size_t size_mb) {
    printf("Создание тестового файла '%s' (%zu МБ)...\n", filename, size_mb);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open не удался");
        return;
    }

    size_t total_bytes = size_mb * 1024 * 1024;
    char buffer[4096];

    for (int i = 0; i < 4096; i++) {
        buffer[i] = (char)(i % 256);
    }

    for (size_t written = 0; written < total_bytes; written += sizeof(buffer)) {
        size_t to_write = (total_bytes - written < sizeof(buffer)) ?
                          (total_bytes - written) : sizeof(buffer);
        if (write(fd, buffer, to_write) == -1) {
            perror("write не удался");
            break;
        }
    }

    close(fd);
    printf(COLOR_GREEN "✓ Тестовый файл успешно создан.\n" COLOR_RESET);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(COLOR_BOLD "Использование: %s <имя_файла> [--create РАЗМЕР_МБ]\n" COLOR_RESET, argv[0]);
        printf("Примеры:\n");
        printf("  %s testfile.bin --create 100\n", argv[0]);
        printf("  %s /путь/к/файлу.bin\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int create_file = 0;
    size_t file_size_mb = 100;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--create") == 0 && i + 1 < argc) {
            create_file = 1;
            file_size_mb = atoi(argv[i + 1]);
            i++;
        }
    }

    if (create_file) {
        create_test_file(filename, file_size_mb);
        printf("\n");
    }

    if (access(filename, F_OK) != 0) {
        fprintf(stderr, COLOR_RED "Ошибка: Файл '%s' не существует.\n" COLOR_RESET, filename);
        return 1;
    }

    printf(COLOR_BOLD COLOR_BLUE "Сравнение методов I/O для файла: %s\n" COLOR_RESET, filename);
    printf("==========================================\n");

    int num_runs = 3;
    BenchmarkResult reads[3], mmaps[3], mmaps_seq[3];

    for (int run = 0; run < num_runs; run++) {
        printf("\n" COLOR_BOLD "--- Запуск %d/%d ---" COLOR_RESET "\n", run + 1, num_runs);
        
        if (run > 0) {
            printf(COLOR_YELLOW "(Примечание: Запускайте с очищенным кэшем для лучших результатов)\n" COLOR_RESET);
        }

        reads[run] = read_with_syscalls(filename);
        sleep(1);
        
        mmaps[run] = read_with_mmap(filename);
        sleep(1);
        
        mmaps_seq[run] = read_with_mmap_sequential(filename);
        sleep(1);
    }

    printf("\n\n" COLOR_BOLD COLOR_BLUE "=== ИТОГИ ===" COLOR_RESET "\n");
    printf("%-25s %-12s %-20s %-20s\n", 
           "Метод", "Время (средн)", "Минорные ошибки", "Мажорные ошибки");
    printf("-------------------------------------------------------------------------------\n");

    double read_time_avg = 0, mmap_time_avg = 0, mmap_seq_time_avg = 0;
    long read_minflt_avg = 0, mmap_minflt_avg = 0, mmap_seq_minflt_avg = 0;

    for (int i = 0; i < num_runs; i++) {
        read_time_avg += reads[i].time_seconds;
        mmap_time_avg += mmaps[i].time_seconds;
        mmap_seq_time_avg += mmaps_seq[i].time_seconds;
        read_minflt_avg += reads[i].minor_faults;
        mmap_minflt_avg += mmaps[i].minor_faults;
        mmap_seq_minflt_avg += mmaps_seq[i].minor_faults;
    }

    read_time_avg /= num_runs;
    mmap_time_avg /= num_runs;
    mmap_seq_time_avg /= num_runs;
    read_minflt_avg /= num_runs;
    mmap_minflt_avg /= num_runs;
    mmap_seq_minflt_avg /= num_runs;

    printf("%-25s " COLOR_GREEN "%.3f сек" COLOR_RESET "    %-20ld %-20ld\n",
           "read()", read_time_avg, read_minflt_avg, 0L);
    printf("%-25s " COLOR_GREEN "%.3f сек" COLOR_RESET "    %-20ld %-20ld\n",
           "mmap()", mmap_time_avg, mmap_minflt_avg, 0L);
    printf("%-25s " COLOR_GREEN "%.3f сек" COLOR_RESET "    %-20ld %-20ld\n",
           "mmap() + SEQUENTIAL", mmap_seq_time_avg, mmap_seq_minflt_avg, 0L);

    printf("\n" COLOR_BOLD "=== ASCII График производительности ===" COLOR_RESET "\n\n");
    double max_time = read_time_avg;
    if (mmap_time_avg > max_time) max_time = mmap_time_avg;
    if (mmap_seq_time_avg > max_time) max_time = mmap_seq_time_avg;
    
    print_bar("read()", read_time_avg, max_time, "сек");
    print_bar("mmap()", mmap_time_avg, max_time, "сек");
    print_bar("mmap()+SEQUENTIAL", mmap_seq_time_avg, max_time, "сек");

    double speedup_mmap = read_time_avg / mmap_time_avg;
    double speedup_seq = read_time_avg / mmap_seq_time_avg;
    
    printf("\n" COLOR_BOLD "=== Ускорение ===" COLOR_RESET "\n");
    printf("mmap() быстрее read() в " COLOR_GREEN "%.2fx" COLOR_RESET " раз\n", speedup_mmap);
    printf("mmap()+SEQUENTIAL быстрее read() в " COLOR_GREEN "%.2fx" COLOR_RESET " раз\n", speedup_seq);



    printf("\n" COLOR_BOLD "=== Анализ ===" COLOR_RESET "\n");
    printf(COLOR_CYAN "mmap() может быть быстрее когда:\n" COLOR_RESET);
    printf("  • Чтение больших файлов (>10 МБ)\n");
    printf("  • Кэш страниц заполнен\n");
    printf("  • Ядро выполняет предзагрузку с MADV_SEQUENTIAL\n\n");
    printf(COLOR_CYAN "read() может быть быстрее когда:\n" COLOR_RESET);
    printf("  • Файл небольшой (<1 МБ)\n");
    printf("  • Последовательное чтение\n");
    printf("  • Размер буфера оптимален\n\n");
    printf(COLOR_YELLOW "Примечание:" COLOR_RESET " Результаты зависят от кэша, типа диска (HDD/SSD) и нагрузки\n");

    return 0;
}
