#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct {
    int* buffer;
    int size;
    int in;
    int out;
    
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
    
    long long sum_produced;
    long long sum_consumed;
    
    int num_producers;
    int num_consumers;
    int items_per_producer;
} bounded_buffer_t;

typedef struct {
    bounded_buffer_t* buffer;
    int producer_id;
    int* produced_count;
} producer_arg_t;

typedef struct {
    bounded_buffer_t* buffer;
    int consumer_id;
    int* consumed_count;
} consumer_arg_t;

void bounded_buffer_init(bounded_buffer_t* bb, int size, int num_producers, int num_consumers, int items_per_producer) {
    bb->buffer = (int*)malloc(size * sizeof(int));
    bb->size = size;
    bb->in = 0;
    bb->out = 0;
    
    sem_init(&bb->empty, 0, size);
    sem_init(&bb->full, 0, 0);
    pthread_mutex_init(&bb->mutex, NULL);
    
    bb->sum_produced = 0;
    bb->sum_consumed = 0;
    
    bb->num_producers = num_producers;
    bb->num_consumers = num_consumers;
    bb->items_per_producer = items_per_producer;
}

void bounded_buffer_destroy(bounded_buffer_t* bb) {
    free(bb->buffer);
    sem_destroy(&bb->empty);
    sem_destroy(&bb->full);
    pthread_mutex_destroy(&bb->mutex);
}

void produce_item(bounded_buffer_t* bb, int item) {
    sem_wait(&bb->empty);
    pthread_mutex_lock(&bb->mutex);
    
    bb->buffer[bb->in] = item;
    bb->in = (bb->in + 1) % bb->size;
    bb->sum_produced += item;
    
    pthread_mutex_unlock(&bb->mutex);
    sem_post(&bb->full);
}

int consume_item(bounded_buffer_t* bb) {
    sem_wait(&bb->full);
    pthread_mutex_lock(&bb->mutex);
    
    int item = bb->buffer[bb->out];
    bb->out = (bb->out + 1) % bb->size;
    bb->sum_consumed += item;
    
    pthread_mutex_unlock(&bb->mutex);
    sem_post(&bb->empty);
    
    return item;
}

void* producer(void* arg) {
    producer_arg_t* parg = (producer_arg_t*)arg;
    bounded_buffer_t* bb = parg->buffer;
    
    for (int i = 0; i < bb->items_per_producer; i++) {
        int item = parg->producer_id * 1000 + i;
        produce_item(bb, item);
        (*parg->produced_count)++;
    }
    
    printf("Producer %d finished, produced %d items\n", parg->producer_id, *parg->produced_count);
    return NULL;
}

void* consumer(void* arg) {
    consumer_arg_t* carg = (consumer_arg_t*)arg;
    bounded_buffer_t* bb = carg->buffer;
    
    int items_per_consumer = (bb->num_producers * bb->items_per_producer) / bb->num_consumers;
    
    for (int i = 0; i < items_per_consumer; i++) {
        int item = consume_item(bb);
        (void)item;
        (*carg->consumed_count)++;
    }
    
    printf("Consumer %d finished, consumed %d items\n", carg->consumer_id, *carg->consumed_count);
    return NULL;
}

int main(int argc, char* argv[]) {
    int num_producers = 2;
    int num_consumers = 2;
    int buffer_size = 64;
    int items_per_producer = 1000;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
            num_producers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
            num_consumers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) {
            buffer_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            items_per_producer = atoi(argv[++i]);
        }
    }
    
    printf("Producer-Consumer with Semaphores\n");
    printf("Producers: %d, Consumers: %d, Buffer: %d, Items per producer: %d\n",
           num_producers, num_consumers, buffer_size, items_per_producer);
    
    bounded_buffer_t bb;
    bounded_buffer_init(&bb, buffer_size, num_producers, num_consumers, items_per_producer);
    
    pthread_t producers[num_producers];
    pthread_t consumers[num_consumers];
    
    int produced_counts[num_producers];
    int consumed_counts[num_consumers];
    
    producer_arg_t pargs[num_producers];
    consumer_arg_t cargs[num_consumers];
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < num_producers; i++) produced_counts[i] = 0;
    for (int i = 0; i < num_consumers; i++) consumed_counts[i] = 0;
    
    for (int i = 0; i < num_consumers; i++) {
        cargs[i].buffer = &bb;
        cargs[i].consumer_id = i;
        cargs[i].consumed_count = &consumed_counts[i];
        pthread_create(&consumers[i], NULL, consumer, &cargs[i]);
    }
    
    for (int i = 0; i < num_producers; i++) {
        pargs[i].buffer = &bb;
        pargs[i].producer_id = i;
        pargs[i].produced_count = &produced_counts[i];
        pthread_create(&producers[i], NULL, producer, &pargs[i]);
    }
    
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    int total_produced = 0;
    int total_consumed = 0;
    
    for (int i = 0; i < num_producers; i++) total_produced += produced_counts[i];
    for (int i = 0; i < num_consumers; i++) total_consumed += consumed_counts[i];
    
    printf("\nResults:\n");
    printf("Total produced: %d, Total consumed: %d\n", total_produced, total_consumed);
    printf("Sum produced: %lld, Sum consumed: %lld\n", bb.sum_produced, bb.sum_consumed);
    printf("Time: %.6f seconds\n", time_taken);
    printf("Status: %s\n", (total_produced == total_consumed && bb.sum_produced == bb.sum_consumed) ? 
           "CORRECT" : "INCORRECT");
    
    bounded_buffer_destroy(&bb);
    return 0;
}
