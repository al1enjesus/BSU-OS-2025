#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>
#include <stdatomic.h>

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
} ring_buffer_t;

typedef struct {
    ring_buffer_t* rb;
    long items_to_produce;
    int producer_index;
} producer_args_t;

typedef struct {
    ring_buffer_t* rb;
    long long consumed_sum;
    long long consumed_count;
    int consumer_index;
} consumer_args_t;

static atomic_long g_assigned = 0;
static long g_total_items = 100000;

static void rb_init(ring_buffer_t* rb, int capacity) {
    rb->data = (int*)malloc(sizeof(int) * capacity);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    sem_init(&rb->empty_slots, 0, capacity);
    sem_init(&rb->full_slots, 0, 0);
}
static void rb_destroy(ring_buffer_t* rb) {
    sem_destroy(&rb->empty_slots);
    sem_destroy(&rb->full_slots);
    pthread_mutex_destroy(&rb->mutex);
    free(rb->data);
}
static void rb_push(ring_buffer_t* rb, int value) {
    sem_wait(&rb->empty_slots);
    pthread_mutex_lock(&rb->mutex);
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->full_slots);
}
static int rb_pop(ring_buffer_t* rb, int* value) {
    sem_wait(&rb->full_slots);
    pthread_mutex_lock(&rb->mutex);
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->empty_slots);
    return 1;
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    (void)a;
    while (1) {
        long idx = atomic_fetch_add_explicit(&g_assigned, 1, memory_order_relaxed);
        if (idx >= g_total_items) break;
        int value = (int)(idx + 1);
        rb_push(a->rb, value);
    }
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int v;
    while (1) {
        if (!rb_pop(a->rb, &v)) break;
        if (v == -1) break;
        a->consumed_sum += v;
        a->consumed_count += 1;
    }
    return NULL;
}

static void usage(const char* prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_total> -B <buffer_size>\n", prog);
}

int main(int argc, char** argv) {
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
    if (P <= 0 || C <= 0 || B <= 0 || N < 0) {
        usage(argv[0]);
        return 1;
    }

    ring_buffer_t rb;
    rb_init(&rb, B);
    g_total_items = (long)N;
    atomic_store(&g_assigned, 0);

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    for (int i = 0; i < C; i++) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&ct[i], NULL, consumer_thread, &cargs[i]) != 0) {
            fprintf(stderr, "pthread_create consumer failed\n");
            return 1;
        }
    }

    for (int i = 0; i < P; i++) {
        pargs[i].rb = &rb;
        pargs[i].items_to_produce = 0;
        pargs[i].producer_index = i;
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) {
            fprintf(stderr, "pthread_create producer failed\n");
            return 1;
        }
    }

    for (int i = 0; i < P; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < C; i++) rb_push(&rb, -1);

    for (int i = 0; i < C; i++) pthread_join(ct[i], NULL);

    long long consumed_total = 0;
    long long consumed_sum = 0;
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    long long expected_sum = (long long)N * (N + 1) / 2;
    printf("[prodcons_sem] P=%d C=%d N=%lld B=%d consumed=%lld sum=%lld expected_sum=%lld %s\n",
        P, C, N, B, consumed_total, consumed_sum, expected_sum,
        (consumed_sum == expected_sum ? "OK" : "MISMATCH"));

    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
    return (consumed_sum == expected_sum) ? 0 : 1;
}