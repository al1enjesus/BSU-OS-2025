#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>
#include <limits.h>
#include <errno.h>

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t sem_empty;
    sem_t sem_full;
} ring_buffer_t;

typedef struct {
    ring_buffer_t* rb;
    long long items_to_produce;
    int producer_index;
} producer_args_t;

typedef struct {
    ring_buffer_t* rb;
    long long consumed_count;
    long long consumed_sum;
    int consumer_index;
} consumer_args_t;

static void rb_init(ring_buffer_t* rb, int capacity) {
    rb->data = (int*)malloc(sizeof(int) * (size_t)capacity);
    if (!rb->data) {
        fprintf(stderr, "Allocation failed\n");
        exit(1);
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    if (sem_init(&rb->sem_empty, 0, (unsigned int)capacity) != 0) {
        perror("sem_init empty");
        exit(1);
    }
    if (sem_init(&rb->sem_full, 0, 0) != 0) {
        perror("sem_init full");
        exit(1);
    }
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&rb->sem_empty);
    sem_destroy(&rb->sem_full);
}

static void rb_push(ring_buffer_t* rb, int value) {
    while (sem_wait(&rb->sem_empty) == -1 && errno == EINTR) {}
    pthread_mutex_lock(&rb->mutex);
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->sem_full);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    while (sem_wait(&rb->sem_full) == -1 && errno == EINTR) {}
    pthread_mutex_lock(&rb->mutex);
    int v = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->sem_empty);
    *value = v;
    return 1;
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (long long i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + (int)(i % 1000000);
        rb_push(a->rb, value);
    }
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    for (;;) {
        int v = 0;
        rb_pop(a->rb, &v);
        if (v == INT_MIN) break;
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

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Allocation failed\n");
        free(pt); free(ct); free(pargs); free(cargs);
        rb_destroy(&rb);
        return 1;
    }

    long long base = N / P;
    long long rem = N % P;

    for (int i = 0; i < P; i++) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = base + (i < rem ? 1 : 0);
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) {
            fprintf(stderr, "pthread_create producer failed\n");
            free(pt); free(ct); free(pargs); free(cargs);
            rb_destroy(&rb);
            return 1;
        }
    }

    for (int i = 0; i < C; i++) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&ct[i], NULL, consumer_thread, &cargs[i]) != 0) {
            fprintf(stderr, "pthread_create consumer failed\n");
            free(pt); free(ct); free(pargs); free(cargs);
            rb_destroy(&rb);
            return 1;
        }
    }

    long long produced_total = 0;
    for (int i = 0; i < P; i++) {
        pthread_join(pt[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }

    for (int i = 0; i < C; i++) {
        rb_push(&rb, INT_MIN);
    }

    for (int i = 0; i < C; i++) {
        pthread_join(ct[i], NULL);
    }

    long long consumed_total = 0;
    long long consumed_sum = 0;
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    printf("[prodcons] P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld\n",
           P, C, N, B, produced_total, consumed_total, consumed_sum);

    free(pt); free(ct); free(pargs); free(cargs);
    rb_destroy(&rb);
    return 0;
}
