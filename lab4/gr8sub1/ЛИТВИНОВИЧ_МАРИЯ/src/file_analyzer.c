#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

long read_syscalls(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    char buf[4096];
    long sum = 0;
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) sum += buf[i];
    }

    close(fd);
    return sum;
}

long read_mmap(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;

    char *ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }

    long sum = 0;
    for (size_t i = 0; i < size; i++)
        sum += ptr[i];

    munmap(ptr, size);
    close(fd);
    return sum;
}

int main() {
    const char *file = "testfile.bin";

    clock_t t1 = clock();
    long s1 = read_syscalls(file);
    clock_t t2 = clock();

    clock_t t3 = clock();
    long s2 = read_mmap(file);
    clock_t t4 = clock();

    printf("syscalls sum = %ld time = %.3f sec\n", s1, (double)(t2 - t1)/CLOCKS_PER_SEC);
    printf("mmap      sum = %ld time = %.3f sec\n", s2, (double)(t4 - t3)/CLOCKS_PER_SEC);

printf("PID = %d\n", getpid());
printf("Press ENTER to exit...\n");
getchar();

    return 0;
}
