#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

typedef struct {
    int* buffer;
    int capacity;
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int producers_done;
    long long total_produced;
    long long total_consumed;
    long long sum_produced;
    long long sum_consumed;
} bounded_buffer_t;

typedef struct {
    int id;
    bounded_buffer_t* buffer;
    int items_to_produce;
} producer_data_t;

typedef struct {
    int id;
    bounded_buffer_t* buffer;
} consumer_data_t;

void bounded_buffer_init(bounded_buffer_t* buffer, int capacity) {
    buffer->buffer = malloc(capacity * sizeof(int));
    buffer->capacity = capacity;
    buffer->count = 0;
    buffer->in = 0;
    buffer->out = 0;
    buffer->producers_done = 0;
    buffer->total_produced = 0;
    buffer->total_consumed = 0;
    buffer->sum_produced = 0;
    buffer->sum_consumed = 0;
    
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
}

void bounded_buffer_destroy(bounded_buffer_t* buffer) {
    free(buffer->buffer);
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
}

void bounded_buffer_put(bounded_buffer_t* buffer, int item) {
    pthread_mutex_lock(&buffer->mutex);
    
    while (buffer->count == buffer->capacity) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }
    
    buffer->buffer[buffer->in] = item;
    buffer->in = (buffer->in + 1) % buffer->capacity;
    buffer->count++;
    buffer->total_produced++;
    buffer->sum_produced += item;
    
    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}

int bounded_buffer_get(bounded_buffer_t* buffer) {
    pthread_mutex_lock(&buffer->mutex);
    
    while (buffer->count == 0 && !buffer->producers_done) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }
    
    if (buffer->count == 0 && buffer->producers_done) {
        pthread_mutex_unlock(&buffer->mutex);
        return -1; // Signal to stop
    }
    
    int item = buffer->buffer[buffer->out];
    buffer->out = (buffer->out + 1) % buffer->capacity;
    buffer->count--;
    buffer->total_consumed++;
    buffer->sum_consumed += item;
    
    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
    
    return item;
}

void* producer_func(void* arg) {
    producer_data_t* data = (producer_data_t*)arg;
    
    for (int i = 0; i < data->items_to_produce; i++) {
        int item = rand() % 100 + 1; // Generate random number 1-100
        bounded_buffer_put(data->buffer, item);
    }
    
    printf("Producer %d finished, produced %d items\n", data->id, data->items_to_produce);
    return NULL;
}

void* consumer_func(void* arg) {
    consumer_data_t* data = (consumer_data_t*)arg;
    int items_consumed = 0;
    
    while (1) {
        int item = bounded_buffer_get(data->buffer);
        if (item == -1) {
            break; // No more items and producers are done
        }
        items_consumed++;
    }
    
    printf("Consumer %d finished, consumed %d items\n", data->id, items_consumed);
    return NULL;
}

int main(int argc, char* argv[]) {
    int num_producers = 2;
    int num_consumers = 2;
    int total_items = 100000;
    int buffer_size = 64;
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt) {
            case 'P':
                num_producers = atoi(optarg);
                break;
            case 'C':
                num_consumers = atoi(optarg);
                break;
            case 'N':
                total_items = atoi(optarg);
                break;
            case 'B':
                buffer_size = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer_size>\n", argv[0]);
                exit(1);
        }
    }
    
    printf("Configuration:\n");
    printf("  Producers: %d\n", num_producers);
    printf("  Consumers: %d\n", num_consumers);
    printf("  Total items: %d\n", total_items);
    printf("  Buffer size: %d\n", buffer_size);
    
    srand(time(NULL));
    
    bounded_buffer_t buffer;
    bounded_buffer_init(&buffer, buffer_size);
    
    pthread_t* producers = malloc(num_producers * sizeof(pthread_t));
    pthread_t* consumers = malloc(num_consumers * sizeof(pthread_t));
    
    producer_data_t* producer_data = malloc(num_producers * sizeof(producer_data_t));
    consumer_data_t* consumer_data = malloc(num_consumers * sizeof(consumer_data_t));
    
    int items_per_producer = total_items / num_producers;
    
    clock_t start_time = clock();
    
    // Create producer threads
    for (int i = 0; i < num_producers; i++) {
        producer_data[i].id = i;
        producer_data[i].buffer = &buffer;
        producer_data[i].items_to_produce = items_per_producer;
        pthread_create(&producers[i], NULL, producer_func, &producer_data[i]);
    }
    
    // Create consumer threads
    for (int i = 0; i < num_consumers; i++) {
        consumer_data[i].id = i;
        consumer_data[i].buffer = &buffer;
        pthread_create(&consumers[i], NULL, consumer_func, &consumer_data[i]);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    
    // Signal consumers that producers are done
    pthread_mutex_lock(&buffer.mutex);
    buffer.producers_done = 1;
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);
    
    // Wait for all consumers to finish
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    clock_t end_time = clock();
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Print results
    printf("\nResults:\n");
    printf("  Total produced: %lld\n", buffer.total_produced);
    printf("  Total consumed: %lld\n", buffer.total_consumed);
    printf("  Sum produced: %lld\n", buffer.sum_produced);
    printf("  Sum consumed: %lld\n", buffer.sum_consumed);
    printf("  Execution time: %.4f seconds\n", execution_time);
    printf("  Correct: %s\n", 
           (buffer.total_produced == buffer.total_consumed && 
            buffer.sum_produced == buffer.sum_consumed) ? "YES" : "NO");
    
    // Cleanup
    bounded_buffer_destroy(&buffer);
    free(producers);
    free(consumers);
    free(producer_data);
    free(consumer_data);
    
    return 0;
}
