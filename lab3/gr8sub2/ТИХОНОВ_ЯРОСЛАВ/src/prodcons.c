#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>

static int P = 2, C = 2, B = 64;
static unsigned long N = 100000;
static uint64_t *buffer;
static int in = 0, out = 0;
static sem_t sem_empty, sem_full;
static pthread_mutex_t buf_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_finish = PTHREAD_COND_INITIALIZER;
static atomic_ulong produced_sum, consumed_sum, consumed_count, total_to_produce;

void* producer(void* arg) {
    long idx = (long)arg;
    for (unsigned long i = 0; i < N; ++i) {
        uint64_t item = idx * N + i + 1;
        if (sem_wait(&sem_empty) != 0) { perror("sem_wait"); exit(EXIT_FAILURE); }
        if (pthread_mutex_lock(&buf_mtx) != 0) { perror("pthread_mutex_lock"); exit(EXIT_FAILURE); }
        buffer[in] = item;
        in = (in + 1) % B;
        atomic_fetch_add(&produced_sum, item);
        if (pthread_mutex_unlock(&buf_mtx) != 0) { perror("pthread_mutex_unlock"); exit(EXIT_FAILURE); }
        if (sem_post(&sem_full) != 0) { perror("sem_post"); exit(EXIT_FAILURE); }
    }
    return NULL;
}

void* consumer(void* arg) {
    (void)arg;
    while (1) {
        if (atomic_load(&consumed_count) >= atomic_load(&total_to_produce)) break;
        if (sem_wait(&sem_full) != 0) { perror("sem_wait"); exit(EXIT_FAILURE); }
        if (pthread_mutex_lock(&buf_mtx) != 0) { perror("pthread_mutex_lock"); exit(EXIT_FAILURE); }
        if (atomic_load(&consumed_count) >= atomic_load(&total_to_produce)) {
            if (pthread_mutex_unlock(&buf_mtx) != 0) { perror("pthread_mutex_unlock"); exit(EXIT_FAILURE); }
            if (sem_post(&sem_full) != 0) { perror("sem_post"); exit(EXIT_FAILURE); }
            break;
        }
        uint64_t item = buffer[out];
        out = (out + 1) % B;
        atomic_fetch_add(&consumed_sum, item);
        atomic_fetch_add(&consumed_count, 1);
        if (pthread_mutex_unlock(&buf_mtx) != 0) { perror("pthread_mutex_unlock"); exit(EXIT_FAILURE); }
        if (sem_post(&sem_empty) != 0) { perror("sem_post"); exit(EXIT_FAILURE); }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch(opt) {
            case 'P': P = atoi(optarg); break;
            case 'C': C = atoi(optarg); break;
            case 'N': N = strtoul(optarg, NULL, 10); break;
            case 'B': B = atoi(optarg); break;
            default: fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_per_producer> -B <buffer_size>\n", argv[0]); return 2;
        }
    }

    if (P <= 0 || C <= 0 || N == 0 || B <= 0) { fprintf(stderr, "Invalid arguments\n"); return 2; }

    buffer = calloc((size_t)B, sizeof(uint64_t));
    if (!buffer) { perror("calloc"); return 2; }

    if (sem_init(&sem_empty, 0, B) != 0) { perror("sem_init empty"); return 2; }
    if (sem_init(&sem_full, 0, 0) != 0) { perror("sem_init full"); return 2; }

    atomic_init(&produced_sum, 0ULL);
    atomic_init(&consumed_sum, 0ULL);
    atomic_init(&total_to_produce, (unsigned long)(P * (unsigned long long)N));
    atomic_init(&consumed_count, 0UL);

    pthread_t *prods = malloc(sizeof(pthread_t) * P);
    pthread_t *cons = malloc(sizeof(pthread_t) * C);
    if (!prods || !cons) { perror("malloc threads"); return 2; }

    for (long i = 0; i < P; ++i) {
        if (pthread_create(&prods[i], NULL, producer, (void*)i) != 0) { perror("pthread_create producer"); exit(EXIT_FAILURE); }
    }
    for (int i = 0; i < C; ++i) {
        if (pthread_create(&cons[i], NULL, consumer, NULL) != 0) { perror("pthread_create consumer"); exit(EXIT_FAILURE); }
    }

    for (int i = 0; i < P; ++i) pthread_join(prods[i], NULL);
    for (int i = 0; i < C; ++i) pthread_join(cons[i], NULL);

    unsigned long long expected_items = (unsigned long long)P * (unsigned long long)N;
    unsigned long long actual_consumed = atomic_load(&consumed_count);
    unsigned long long expected_sum = atomic_load(&produced_sum);
    unsigned long long actual_sum = atomic_load(&consumed_sum);

    printf("P=%d C=%d N=%lu B=%d\n", P, C, N, B);
    printf("expected items = %llu\n", expected_items);
    printf("actual consumed items = %llu\n", actual_consumed);
    printf("expected sum = %llu\n", expected_sum);
");
