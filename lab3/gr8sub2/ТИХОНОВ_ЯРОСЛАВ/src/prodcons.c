#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>   // для usleep
#include <errno.h>

static int P = 2, C = 2, B = 64;
static unsigned long N = 100000UL;

static uint64_t *buffer = NULL;
static int head = 0, tail = 0;
static pthread_mutex_t buf_mtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem_empty, sem_full;

static atomic_ullong produced_sum;
static atomic_ullong consumed_sum;
static atomic_ulong total_to_produce;
static atomic_ulong consumed_count;

void *producer(void *arg) {
    long id = (long)arg;
    for (unsigned long i = 0; i < N; ++i) {
        uint64_t val = (uint64_t)(id * N + i + 1);
        while (sem_wait(&sem_empty) == -1 && errno == EINTR);
        pthread_mutex_lock(&buf_mtx);
        buffer[tail] = val;
        tail = (tail + 1) % B;
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_full);
        atomic_fetch_add(&produced_sum, val);
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (;;) {
        if (atomic_load(&consumed_count) >= atomic_load(&total_to_produce)) break;
        if (sem_wait(&sem_full) == -1) { if (errno == EINTR) continue; else break; }
        pthread_mutex_lock(&buf_mtx);
        uint64_t val = buffer[head];
        head = (head + 1) % B;
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_empty);
        atomic_fetch_add(&consumed_sum, val);
        atomic_fetch_add(&consumed_count, 1UL);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt) {
            case 'P': P = atoi(optarg); break;
            case 'C': C = atoi(optarg); break;
            case 'N': N = strtoul(optarg, NULL, 10); break;
            case 'B': B = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer>\n", argv[0]);
                return 2;
        }
    }

    if (P <= 0 || C <= 0 || N == 0 || B <= 0) {  // <-- исправлено
        fprintf(stderr, "Invalid arguments\n");
        return 2;
    }

    buffer = calloc(B, sizeof(uint64_t));
    if (!buffer) { perror("calloc"); return 2; }

    sem_init(&sem_empty, 0, B);
    sem_init(&sem_full, 0, 0);
    pthread_mutex_init(&buf_mtx, NULL);

    atomic_init(&produced_sum, 0ULL);
    atomic_init(&consumed_sum, 0ULL);
    atomic_init(&total_to_produce, P * N);
    atomic_init(&consumed_count, 0UL);

    pthread_t *prods = malloc(sizeof(pthread_t) * P);
    pthread_t *cons = malloc(sizeof(pthread_t) * C);

    for (long i = 0; i < P; ++i) pthread_create(&prods[i], NULL, producer, (void*)i);
    for (int i = 0; i < C; ++i) pthread_create(&cons[i], NULL, consumer, NULL);

    for (int i = 0; i < P; ++i) pthread_join(prods[i], NULL);

    while (atomic_load(&consumed_count) < atomic_load(&total_to_produce)) usleep(1000);

    for (int i = 0; i < C; ++i) sem_post(&sem_full);
    for (int i = 0; i < C; ++i) pthread_join(cons[i], NULL);

    unsigned long long expected_items = (unsigned long long)P * N;
    unsigned long long actual_consumed = atomic_load(&consumed_count);
    unsigned long long expected_sum = atomic_load(&produced_sum);
    unsigned long long actual_sum = atomic_load(&consumed_sum);

    printf("P=%d C=%d N=%lu B=%d\n", P, C, N, B);
    printf("expected items = %llu\n", expected_items);
    printf("actual consumed items = %llu\n", actual_consumed);
    printf("expected sum = %llu\n", expected_sum);
    printf("actual sum = %llu\n", actual_sum);

    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    pthread_mutex_destroy(&buf_mtx);
    free(buffer);
    free(prods);
    free(cons);


    printf("Press Enter to exit and release threads...\n");
    getchar();

    return 0;
}
