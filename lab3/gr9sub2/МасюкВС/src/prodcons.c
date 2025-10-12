#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct {
    int *data;
    int capacity;
    int head;
    int tail;

    sem_t free_slots;
    sem_t filled_slots;
    pthread_mutex_t lock;

    long long produced_sum;
    long long consumed_sum;

    int producers;
    int consumers;
    int items_per_producer;
} buffer_t;

typedef struct {
    buffer_t *buf;
    int id;
    int *produced;
} producer_data_t;

typedef struct {
    buffer_t *buf;
    int id;
    int *consumed;
} consumer_data_t;

void buffer_init(buffer_t *b, int capacity, int prod, int cons, int n_items) {
    b->data = malloc(sizeof(int) * capacity);
    b->capacity = capacity;
    b->head = 0;
    b->tail = 0;

    sem_init(&b->free_slots, 0, capacity);
    sem_init(&b->filled_slots, 0, 0);
    pthread_mutex_init(&b->lock, NULL);

    b->produced_sum = 0;
    b->consumed_sum = 0;

    b->producers = prod;
    b->consumers = cons;
    b->items_per_producer = n_items;
}

void buffer_cleanup(buffer_t *b) {
    free(b->data);
    sem_destroy(&b->free_slots);
    sem_destroy(&b->filled_slots);
    pthread_mutex_destroy(&b->lock);
}

void buffer_put(buffer_t *b, int value) {
    sem_wait(&b->free_slots);
    pthread_mutex_lock(&b->lock);

    b->data[b->head] = value;
    b->head = (b->head + 1) % b->capacity;
    b->produced_sum += value;

    pthread_mutex_unlock(&b->lock);
    sem_post(&b->filled_slots);
}

int buffer_get(buffer_t *b) {
    sem_wait(&b->filled_slots);
    pthread_mutex_lock(&b->lock);

    int val = b->data[b->tail];
    b->tail = (b->tail + 1) % b->capacity;
    b->consumed_sum += val;

    pthread_mutex_unlock(&b->lock);
    sem_post(&b->free_slots);
    return val;
}

void *producer_func(void *arg) {
    producer_data_t *p = arg;
    buffer_t *b = p->buf;

    for (int i = 0; i < b->items_per_producer; i++) {
        int value = p->id * 1000 + i;
        buffer_put(b, value);
        (*p->produced)++;
    }

    printf("Producer %d finished, produced %d items\n", p->id, *p->produced);
    return NULL;
}

void *consumer_func(void *arg) {
    consumer_data_t *c = arg;
    buffer_t *b = c->buf;

    int items_to_consume = (b->producers * b->items_per_producer) / b->consumers;
    for (int i = 0; i < items_to_consume; i++) {
        (void)buffer_get(b);
        (*c->consumed)++;
    }

    printf("Consumer %d finished, consumed %d items\n", c->id, *c->consumed);
    return NULL;
}

int main(int argc, char *argv[]) {
    int prod_count = 2;
    int cons_count = 2;
    int buf_size = 64;
    int items_per_prod = 1000;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) prod_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) cons_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) buf_size = atoi(argv[++i]);
        else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) items_per_prod = atoi(argv[++i]);
    }

    printf("Producer-Consumer (Semaphores)\n");
    printf("Producers: %d, Consumers: %d, Buffer size: %d, Items per producer: %d\n",
           prod_count, cons_count, buf_size, items_per_prod);

    buffer_t buf;
    buffer_init(&buf, buf_size, prod_count, cons_count, items_per_prod);

    pthread_t producers[prod_count];
    pthread_t consumers[cons_count];

    int produced[prod_count];
    int consumed[cons_count];

    producer_data_t prod_args[prod_count];
    consumer_data_t cons_args[cons_count];

    for (int i = 0; i < prod_count; i++) produced[i] = 0;
    for (int i = 0; i < cons_count; i++) consumed[i] = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < cons_count; i++) {
        cons_args[i].buf = &buf;
        cons_args[i].id = i;
        cons_args[i].consumed = &consumed[i];
        pthread_create(&consumers[i], NULL, consumer_func, &cons_args[i]);
    }

    for (int i = 0; i < prod_count; i++) {
        prod_args[i].buf = &buf;
        prod_args[i].id = i;
        prod_args[i].produced = &produced[i];
        pthread_create(&producers[i], NULL, producer_func, &prod_args[i]);
    }

    for (int i = 0; i < prod_count; i++) pthread_join(producers[i], NULL);
    for (int i = 0; i < cons_count; i++) pthread_join(consumers[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    int total_prod = 0, total_cons = 0;
    for (int i = 0; i < prod_count; i++) total_prod += produced[i];
    for (int i = 0; i < cons_count; i++) total_cons += consumed[i];

    printf("\nResults:\n");
    printf("Produced: %d, Consumed: %d\n", total_prod, total_cons);
    printf("Sum produced: %lld, Sum consumed: %lld\n", buf.produced_sum, buf.consumed_sum);
    printf("Time: %.6f sec\n", elapsed);
    printf("Status: %s\n", (total_prod == total_cons && buf.produced_sum == buf.consumed_sum)
                           ? "CORRECT" : "INCORRECT");

    buffer_cleanup(&buf);
    return 0;
}
