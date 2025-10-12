#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct {
    int *buffer;
    int capacity;
    int write_pos;
    int read_pos;

    sem_t sem_empty;
    sem_t sem_full;
    pthread_mutex_t lock;

    long long sum_prod;
    long long sum_cons;

    int producers;
    int consumers;
    int items_per_producer;
} ring_buffer_t;

typedef struct {
    ring_buffer_t *buf;
    int id;
    int *count;
} producer_arg_t;

typedef struct {
    ring_buffer_t *buf;
    int id;
    int *count;
} consumer_arg_t;

void ring_buffer_init(ring_buffer_t *rb, int cap, int prod, int cons, int per_prod) {
    rb->buffer = malloc(cap * sizeof(int));
    rb->capacity = cap;
    rb->write_pos = 0;
    rb->read_pos = 0;

    sem_init(&rb->sem_empty, 0, cap);
    sem_init(&rb->sem_full, 0, 0);
    pthread_mutex_init(&rb->lock, NULL);

    rb->sum_prod = 0;
    rb->sum_cons = 0;

    rb->producers = prod;
    rb->consumers = cons;
    rb->items_per_producer = per_prod;
}

void ring_buffer_destroy(ring_buffer_t *rb) {
    free(rb->buffer);
    sem_destroy(&rb->sem_empty);
    sem_destroy(&rb->sem_full);
    pthread_mutex_destroy(&rb->lock);
}

void buffer_push(ring_buffer_t *rb, int value) {
    sem_wait(&rb->sem_empty);
    pthread_mutex_lock(&rb->lock);

    rb->buffer[rb->write_pos] = value;
    rb->write_pos = (rb->write_pos + 1) % rb->capacity;
    rb->sum_prod += value;

    pthread_mutex_unlock(&rb->lock);
    sem_post(&rb->sem_full);
}

int buffer_pop(ring_buffer_t *rb) {
    sem_wait(&rb->sem_full);
    pthread_mutex_lock(&rb->lock);

    int value = rb->buffer[rb->read_pos];
    rb->read_pos = (rb->read_pos + 1) % rb->capacity;
    rb->sum_cons += value;

    pthread_mutex_unlock(&rb->lock);
    sem_post(&rb->sem_empty);

    return value;
}

void* producer_thread(void *arg) {
    producer_arg_t *p = arg;
    ring_buffer_t *rb = p->buf;

    for (int i = 0; i < rb->items_per_producer; i++) {
        int item = p->id * 1000 + i;
        buffer_push(rb, item);
        (*p->count)++;
    }

    printf("Producer %d finished, produced %d items\n", p->id, *p->count);
    return NULL;
}

void* consumer_thread(void *arg) {
    consumer_arg_t *c = arg;
    ring_buffer_t *rb = c->buf;

    int total = (rb->producers * rb->items_per_producer) / rb->consumers;

    for (int i = 0; i < total; i++) {
        buffer_pop(rb);
        (*c->count)++;
    }

    printf("Consumer %d finished, consumed %d items\n", c->id, *c->count);
    return NULL;
}

int main(int argc, char *argv[]) {
    int producers = 2, consumers = 2, buffer_size = 64, items = 1000;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 && i + 1 < argc)
            producers = atoi(argv[++i]);
        else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc)
            consumers = atoi(argv[++i]);
        else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc)
            buffer_size = atoi(argv[++i]);
        else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc)
            items = atoi(argv[++i]);
    }

    printf("Producer–Consumer with Semaphores\n");
    printf("Producers: %d, Consumers: %d, Buffer: %d, Items: %d\n",
           producers, consumers, buffer_size, items);

    ring_buffer_t rb;
    ring_buffer_init(&rb, buffer_size, producers, consumers, items);

    pthread_t prod_threads[producers];
    pthread_t cons_threads[consumers];
    int prod_counts[producers], cons_counts[consumers];
    producer_arg_t pargs[producers];
    consumer_arg_t cargs[consumers];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    memset(prod_counts, 0, sizeof(prod_counts));
    memset(cons_counts, 0, sizeof(cons_counts));

    for (int i = 0; i < consumers; i++) {
        cargs[i].buf = &rb;
        cargs[i].id = i;
        cargs[i].count = &cons_counts[i];
        pthread_create(&cons_threads[i], NULL, consumer_thread, &cargs[i]);
    }

    for (int i = 0; i < producers; i++) {
        pargs[i].buf = &rb;
        pargs[i].id = i;
        pargs[i].count = &prod_counts[i];
        pthread_create(&prod_threads[i], NULL, producer_thread, &pargs[i]);
    }

    for (int i = 0; i < producers; i++) pthread_join(prod_threads[i], NULL);
    for (int i = 0; i < consumers; i++) pthread_join(cons_threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double duration = (end.tv_sec - start.tv_sec) +
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    int total_prod = 0, total_cons = 0;
    for (int i = 0; i < producers; i++) total_prod += prod_counts[i];
    for (int i = 0; i < consumers; i++) total_cons += cons_counts[i];

    printf("\nResults:\n");
    printf("Total produced: %d, Total consumed: %d\n", total_prod, total_cons);
    printf("Sum produced: %lld, Sum consumed: %lld\n", rb.sum_prod, rb.sum_cons);
    printf("Time: %.6f seconds\n", duration);
    printf("Status: %s\n",
           (total_prod == total_cons && rb.sum_prod == rb.sum_cons)
               ? "CORRECT"
               : "INCORRECT");

    ring_buffer_destroy(&rb);
    return 0;
}
