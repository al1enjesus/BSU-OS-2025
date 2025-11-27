// prodcons.c  -- Producer/Consumer (Variant 2) using POSIX semaphores
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>

#define SENTINEL -1
#define PRODUCER_BASE 1000000LL

typedef struct {
    int *data;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t slots_free;
    sem_t slots_filled;
} ring_buffer_t;

typedef struct {
    ring_buffer_t* rb;
    int items_to_produce;
    int producer_index;
} producer_args_t;

typedef struct {
    ring_buffer_t* rb;
    long long consumed_sum;
    long long consumed_count;
    int consumer_index;
} consumer_args_t;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void rb_init(ring_buffer_t* rb, int capacity) {
    rb->data = malloc(sizeof(int) * capacity);
    if (!rb->data) die("malloc");
    rb->capacity = capacity;
    rb->head = rb->tail = 0;
    if (pthread_mutex_init(&rb->mutex, NULL) != 0) die("pthread_mutex_init");
    if (sem_init(&rb->slots_free, 0, capacity) != 0) die("sem_init slots_free");
    if (sem_init(&rb->slots_filled, 0, 0) != 0) die("sem_init slots_filled");
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&rb->slots_free);
    sem_destroy(&rb->slots_filled);
}

static void sem_wait_retry(sem_t *s) {
    while (sem_wait(s) != 0) {
        if (errno == EINTR) continue;
        die("sem_wait");
    }
}

static void rb_push(ring_buffer_t* rb, int value) {
    sem_wait_retry(&rb->slots_free);
    if (pthread_mutex_lock(&rb->mutex) != 0) die("pthread_mutex_lock");
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    if (pthread_mutex_unlock(&rb->mutex) != 0) die("pthread_mutex_unlock");
    if (sem_post(&rb->slots_filled) != 0) die("sem_post slots_filled");
}

static int rb_pop(ring_buffer_t* rb, int *value) {
    sem_wait_retry(&rb->slots_filled);
    if (pthread_mutex_lock(&rb->mutex) != 0) die("pthread_mutex_lock");
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    if (pthread_mutex_unlock(&rb->mutex) != 0) die("pthread_mutex_unlock");
    if (sem_post(&rb->slots_free) != 0) die("sem_post slots_free");
    return 1;
}

static void *producer_thread(void *arg) {
    producer_args_t *a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; ++i) {
        int value = (int)((a->producer_index + 1) * PRODUCER_BASE + i);
        rb_push(a->rb, value);
    }
    return NULL;
}

static void *consumer_thread(void *arg) {
    consumer_args_t *a = (consumer_args_t*)arg;
    int v;
    while (1) {
        rb_pop(a->rb, &v);
        if (v == SENTINEL) break; // sentinel — завершение
        a->consumed_sum += v;
        a->consumed_count += 1;
    }
    return NULL;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_total> -B <buffer_size>\n", prog);
}

static double timespec_diff(const struct timespec *a, const struct timespec *b) {
    return (double)(b->tv_sec - a->tv_sec) + (b->tv_nsec - a->tv_nsec) / 1e9;
}

int main(int argc, char **argv) {
    int P = 2, C = 2, B = 64;
    long long N = 100000;

    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt) {
            case 'P': P = atoi(optarg); break;
            case 'C': C = atoi(optarg); break;
            case 'N': N = atoll(optarg); break;
            case 'B': B = atoi(optarg); break;
            default: usage(argv[0]); return 1;
        }
    }

    if (P <= 0 || C <= 0 || B <= 0 || N < 0) { usage(argv[0]); return 1; }

    ring_buffer_t rb;
    rb_init(&rb, B);

    pthread_t *prod = calloc((size_t)P, sizeof(pthread_t));
    pthread_t *cons = calloc((size_t)C, sizeof(pthread_t));
    producer_args_t *pargs = calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t *cargs = calloc((size_t)C, sizeof(consumer_args_t));
    if (!prod || !cons || !pargs || !cargs) die("calloc");

    // стартуем потребителей (они будут ждать filled_slots)
    for (int i = 0; i < C; ++i) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&cons[i], NULL, consumer_thread, &cargs[i]) != 0) die("pthread_create consumer");
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    // стартуем производителей
    int per = (int)(N / P);
    int rem = (int)(N % P);
    for (int i = 0; i < P; ++i) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = per + (i < rem ? 1 : 0);
        if (pthread_create(&prod[i], NULL, producer_thread, &pargs[i]) != 0) die("pthread_create producer");
    }

    long long produced_total = 0;
    for (int i = 0; i < P; ++i) {
        pthread_join(prod[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }

    // после того как все производители закончили, кладём C *sentinel*-ов
    for (int i = 0; i < C; ++i) rb_push(&rb, SENTINEL);

    for (int i = 0; i < C; ++i) pthread_join(cons[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = timespec_diff(&t0, &t1);

    long long consumed_total = 0;
    long long consumed_sum = 0;
    for (int i = 0; i < C; ++i) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    // вычислим ожидаемую сумму (опционально) для проверки
    long long expected_sum = 0;
    for (int p = 0; p < P; ++p) {
        long long items = pargs[p].items_to_produce;
        long long base = (long long)(p + 1) * PRODUCER_BASE;
        expected_sum += base * items + (items * (items - 1)) / 2;
    }

    printf("[prodcons: sem] P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld expected_sum=%lld time=%.6fs\n",
           P, C, N, B, produced_total, consumed_total, consumed_sum, expected_sum, elapsed);

    free(prod); free(cons); free(pargs); free(cargs);
    rb_destroy(&rb);
    return 0;
}

