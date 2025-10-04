
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>

typedef struct {
    int producer_index;
    int consumer_index;
    long long items;
    int num_producers;
    int num_consumers;
    int buffer_size;
    atomic_llong consumed_sum;
} thread_args_t;

static int* buffer;
static int buf_head = 0;
static int buf_tail = 0;
static sem_t sem_empty;
static sem_t sem_full;
static sem_t mutex;
static atomic_llong produced_count;
static atomic_llong consumed_count;

static void* producer_thread(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    long long items_per_producer = a->items / a->num_producers;
    for (long long i = 0; i < items_per_producer; i++) {
        int value = (a->producer_index + 1) * 1000000 + i;
        
        sem_wait(&sem_empty);
        sem_wait(&mutex);
        buffer[buf_tail] = value;
        buf_tail = (buf_tail + 1) % a->buffer_size;
        sem_post(&mutex);
        sem_post(&sem_full);
        atomic_fetch_add_explicit(&produced_count, 1, memory_order_relaxed);
    }
    return NULL;
}

static void* consumer_thread(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    long long items_per_consumer = a->items / a->num_consumers;
    for (long long i = 0; i < items_per_consumer; i++) {
        sem_wait(&sem_full);
        sem_wait(&mutex);
        int v = buffer[buf_head];
        buf_head = (buf_head + 1) % a->buffer_size;
        sem_post(&mutex);
        sem_post(&sem_empty);
        atomic_fetch_add_explicit(&consumed_count, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&a->consumed_sum, v, memory_order_relaxed);
        
    }
    return NULL;
}

int main(int argc, char** argv) {
    int num_producers = 0, num_consumers = 0, buffer_size = 0;
    long long items = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) num_producers = atoi(argv[++i]);
        else if (strcmp(argv[i], "-C") == 0) num_consumers = atoi(argv[++i]);
        else if (strcmp(argv[i], "-N") == 0) items = atoll(argv[++i]);
        else if (strcmp(argv[i], "-B") == 0) buffer_size = atoi(argv[++i]);
    }
    if (num_producers <= 0 || num_consumers <= 0 || items <= 0 || buffer_size <= 0) {
        fprintf(stderr, "Usage: %s -P <num_producers> -C <num_consumers> -N <items> -B <buffer_size>\n", argv[0]);
        return 1;
    }

    buffer = (int*)calloc(buffer_size, sizeof(int));
    if (!buffer) {
        fprintf(stderr, "Buffer allocation failed\n");
        return 1;
    }

    sem_init(&mutex, 0, 1);
if (sem_init(&mutex, 0, 1) != 0) {
    perror("Failed to initialize mutex semaphore");
    free(buffer);
    return 1;
}

sem_init(&sem_empty, 0, buffer_size);
if (sem_init(&sem_empty, 0, buffer_size) != 0) {
    perror("Failed to initialize sem_empty semaphore");
    sem_destroy(&mutex);
    free(buffer);
    return 1;
}

sem_init(&sem_full, 0, 0);
if (sem_init(&sem_full, 0, 0) != 0) {
    perror("Failed to initialize sem_full semaphore");
    sem_destroy(&mutex);
    sem_destroy(&sem_empty);
    free(buffer);
    return 1;
}
    atomic_store(&produced_count, 0);
    atomic_store(&consumed_count, 0);

    pthread_t* tids = (pthread_t*)calloc((size_t)(num_producers + num_consumers), sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)calloc((size_t)(num_producers + num_consumers), sizeof(thread_args_t));
    if (!tids || !args) {
        fprintf(stderr, "Allocation failed\n");
        free(buffer); sem_destroy(&mutex); sem_destroy(&sem_empty); sem_destroy(&sem_full);
        return 1;
    }

    for (int i = 0; i < num_producers; i++) {
        args[i].producer_index = i;
        args[i].items = items;
        args[i].num_producers = num_producers;
        args[i].num_consumers = num_consumers;
        args[i].buffer_size = buffer_size;
        atomic_init(&args[i].consumed_sum, 0);
        if (pthread_create(&tids[i], NULL, producer_thread, &args[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            free(buffer); free(tids); free(args); sem_destroy(&mutex); sem_destroy(&sem_empty); sem_destroy(&sem_full);
            return 1;
        }
    }

    for (int i = 0; i < num_consumers; i++) {
        args[num_producers + i].consumer_index = i;
        args[num_producers + i].items = items;
        args[num_producers + i].num_producers = num_producers;
        args[num_producers + i].num_consumers = num_consumers;
        args[num_producers + i].buffer_size = buffer_size;
        atomic_init(&args[num_producers + i].consumed_sum, 0);
        if (pthread_create(&tids[num_producers + i], NULL, consumer_thread, &args[num_producers + i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            free(buffer); free(tids); free(args); sem_destroy(&mutex); sem_destroy(&sem_empty); sem_destroy(&sem_full);
            return 1;
        }
    }

    for (int i = 0; i < num_producers + num_consumers; i++) {
        pthread_join(tids[i], NULL);
    }

    long long total_sum = 0;
    for (int i = 0; i < num_consumers; i++) {
        total_sum += atomic_load_explicit(&args[num_producers + i].consumed_sum, memory_order_relaxed);
        printf("[main] consumer %d sum=%lld\n", i, atomic_load_explicit(&args[num_producers + i].consumed_sum, memory_order_relaxed));
    }

    

    free(buffer);
    free(tids);
    free(args);
    sem_destroy(&mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    return 0;
}

