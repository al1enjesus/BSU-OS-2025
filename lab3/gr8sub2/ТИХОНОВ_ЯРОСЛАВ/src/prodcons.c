#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>

typedef unsigned long long ull;

int P = 2, C = 2, B = 64;
ull N = 100000;
ull *buffer;
int buf_start = 0, buf_end = 0;
pthread_mutex_t buf_mtx;
sem_t sem_empty, sem_full;
atomic_ull produced_sum, consumed_sum;
atomic_ull total_to_produce, consumed_count;

void* producer(void* arg) {
    for (ull i = 0; i < N; ++i) {
        ull item = i + 1;
        sem_wait(&sem_empty);
        pthread_mutex_lock(&buf_mtx);
        buffer[buf_end] = item;
        buf_end = (buf_end + 1) % B;
        atomic_fetch_add(&produced_sum, item);
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_full);
    }
    return NULL;
}

void* consumer(void* arg) {
    while (atomic_load(&consumed_count) < atomic_load(&total_to_produce)) {
        sem_wait(&sem_full);
        pthread_mutex_lock(&buf_mtx);
        if (atomic_load(&consumed_count) >= atomic_load(&total_to_produce)) {
            pthread_mutex_unlock(&buf_mtx);
            sem_post(&sem_full);
            break;
        }
        ull item = buffer[buf_start];
        buf_start = (buf_start + 1) % B;
        atomic_fetch_add(&consumed_sum, item);
        atomic_fetch_add(&consumed_count, 1);
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_empty);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch(opt) {
            case 'P': P = atoi(optarg); break;
            case 'C': C = atoi(optarg); break;
            case 'N': N = strtoull(optarg, NULL, 10); break;
            case 'B': B = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_per_producer> -B <buffer_size>\n", argv[0]);
                return 2;
        }
    }

    if (P <= 0 || C <= 0 || N == 0 || B <= 0) { fprintf(stderr, "Invalid arguments\n"); return 2; }

    buffer = calloc((size_t)B, sizeof(ull));
    if (!buffer) { perror("calloc"); return 2; }

    if (pthread_mutex_init(&buf_mtx, NULL) != 0) { perror("pthread_mutex_init"); return 3; }
    if (sem_init(&sem_empty, 0, B) != 0) { perror("sem_init"); return 3; }
    if (sem_init(&sem_full, 0, 0) != 0) { perror("sem_init"); return 3; }

    atomic_init(&produced_sum, 0ULL);
    atomic_init(&consumed_sum, 0ULL);
    atomic_init(&total_to_produce, (ull)(P * N));
    atomic_init(&consumed_count, 0ULL);

    pthread_t *prods = malloc(sizeof(pthread_t) * P);
    pthread_t *cons = malloc(sizeof(pthread_t) * C);
    if (!prods || !cons) { perror("malloc"); return 4; }

    for (long i = 0; i < P; ++i)
        if (pthread_create(&prods[i], NULL, producer, (void*)i) != 0) { perror("pthread_create"); return 5; }
    for (int i = 0; i < C; ++i)
        if (pthread_create(&cons[i], NULL, consumer, NULL) != 0) { perror("pthread_create"); return 5; }

    for (int i = 0; i < P; ++i) pthread_join(prods[i], NULL);
    for (int i = 0; i < C; ++i) pthread_join(cons[i], NULL);

    ull expected_items = (ull)P * (ull)N;
    ull actual_consumed = atomic_load(&consumed_count);
    ull expected_sum = atomic_load(&produced_sum);
    ull actual_sum = atomic_load(&consumed_sum);

    printf("P=%d C=%d N=%llu B=%d\n", P, C, N, B);
    printf("expected items = %llu\n", expected_items);
    printf("actual consumed items = %llu\n", actual_consumed);
}