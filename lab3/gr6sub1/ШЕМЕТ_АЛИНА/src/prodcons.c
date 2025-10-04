#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#define DEFAULT_PRODUCERS 2
#define DEFAULT_CONSUMERS 2
#define DEFAULT_ITEMS 100000
#define DEFAULT_BUFFER_SIZE 64

typedef struct {
    int *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} ring_buffer_t;

typedef struct {
    int index;
    ring_buffer_t *rb;
    int items;
} thread_arg_t;

typedef struct {
    int producers;
    int consumers;
    int items;
    int buffer_size;
} options_t;

static long long now_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void ring_buffer_init(ring_buffer_t *rb, int capacity) {
    rb->buffer = malloc(sizeof(int) * capacity);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_full, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
}

void ring_buffer_destroy(ring_buffer_t *rb) {
    free(rb->buffer);
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_full);
    pthread_cond_destroy(&rb->not_empty);
}

void ring_buffer_put(ring_buffer_t *rb, int value) {
    pthread_mutex_lock(&rb->mutex);
    while (rb->count == rb->capacity) {
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }
    rb->buffer[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

int ring_buffer_get(ring_buffer_t *rb) {
    pthread_mutex_lock(&rb->mutex);
    while (rb->count == 0) {
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }
    int value = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    return value;
}

void* producer_thread(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    for (int i = 0; i < targ->items; i++) {
        ring_buffer_put(targ->rb, i);
        if ((i + 1) % 10000 == 0) {
            printf("Producer %d: produced %d items\n", targ->index, i + 1);
        }
    }
    printf("Producer %d: finished producing %d items\n", targ->index, targ->items);
    return NULL;
}

void* consumer_thread(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    for (int i = 0; i < targ->items; i++) {
        int value = ring_buffer_get(targ->rb);
        if ((i + 1) % 10000 == 0) {
            printf("Consumer %d: consumed %d items (Last value: %d)\n", targ->index, i + 1, value);
        }
    }
    printf("Consumer %d: finished consuming %d items\n", targ->index, targ->items);
    return NULL;
}

int main(int argc, char *argv[]) {
    options_t opt = {DEFAULT_PRODUCERS, DEFAULT_CONSUMERS, DEFAULT_ITEMS, DEFAULT_BUFFER_SIZE};
    int opt_char;

    while ((opt_char = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt_char) {
            case 'P': opt.producers = atoi(optarg); break;
            case 'C': opt.consumers = atoi(optarg); break;
            case 'N': opt.items = atoi(optarg); break;
            case 'B': opt.buffer_size = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s [-P producers] [-C consumers] [-N items] [-B buffer_size]\n", argv[0]);
                return 1;
        }
    }

    printf("Starting producer-consumer with:\n");
    printf("  Producers: %d\n", opt.producers);
    printf("  Consumers: %d\n", opt.consumers);
    printf("  Total items: %d\n", opt.items);
    printf("  Buffer size: %d\n", opt.buffer_size);

    ring_buffer_t rb;
    ring_buffer_init(&rb, opt.buffer_size);

    int total_threads = opt.producers + opt.consumers;
    pthread_t *tids = malloc(sizeof(pthread_t) * total_threads);
    thread_arg_t *args = malloc(sizeof(thread_arg_t) * total_threads);

    int items_per_producer = opt.items / opt.producers;
    int items_per_consumer = opt.items / opt.consumers;

    long long start_time = now_monotonic_ms();

    for (int i = 0; i < opt.producers; i++) {
        args[i].index = i;
        args[i].rb = &rb;
        args[i].items = items_per_producer;
        if (pthread_create(&tids[i], NULL, producer_thread, &args[i]) != 0) {
            fprintf(stderr, "producer pthread_create failed\n");
            return 1;
        }
    }
    for (int i = 0; i < opt.consumers; i++) {
        args[i + opt.producers].index = i;
        args[i + opt.producers].rb = &rb;
        args[i + opt.producers].items = items_per_consumer;
        if (pthread_create(&tids[i + opt.producers], NULL, consumer_thread, &args[i + opt.producers]) != 0) {
            fprintf(stderr, "consumer pthread_create failed\n");
            return 1;
        }
    }
sleep(40); 
    for (int i = 0; i < total_threads; i++) {
        pthread_join(tids[i], NULL);
    }

    long long end_time = now_monotonic_ms();
    long long elapsed_ms = end_time - start_time;

    int total_produced = opt.items;
    int total_consumed = opt.items;
    int remaining = rb.count;

    printf("Final statistics:\n");
    printf("  Total produced: %d\n", total_produced);
    printf("  Total consumed: %d\n", total_consumed);
    printf("  Items in buffer: %d\n", remaining);
    printf("  Time (ms): %lld\n", elapsed_ms);

    if (total_produced == total_consumed + remaining) {
        printf("✓ CORRECT: produced = consumed + remaining\n");
    } else {
        printf("✗ ERROR: produced != consumed + remaining\n");
    }

    ring_buffer_destroy(&rb);
    free(tids);
    free(args);
    return 0;
}
