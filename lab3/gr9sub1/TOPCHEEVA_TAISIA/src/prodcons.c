#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>

typedef struct {
    int id;
    long long items;
    int num_producers;
    int num_consumers;
    int buffer_size;
    atomic_llong local_consumed_sum;
} thread_args_t;

static long long* buffer;
static int* buffer_producer_id;
static int buffer_head = 0;
static int buffer_tail = 0;
static sem_t sem_empty;
static sem_t sem_full;
static sem_t mutex;
static atomic_llong produced_count;
static atomic_llong consumed_count;
static atomic_llong global_consumed_sum;
static atomic_llong* producer_counts;

static void* producer_thread(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    long long items_per_producer = a->items / a->num_producers;
    long long remainder = a->items % a->num_producers;
    
    if (a->id < remainder) {
        items_per_producer++;
    }
    
    for (long long i = 0; i < items_per_producer; i++) {
        
        long long value = (long long)(a->id) * 1000000LL + i + 1;
        
        sem_wait(&sem_empty);
        sem_wait(&mutex);
        
        buffer[buffer_tail] = value;
        buffer_producer_id[buffer_tail] = a->id;
        buffer_tail = (buffer_tail + 1) % a->buffer_size;
        
        sem_post(&mutex);
        sem_post(&sem_full);
        
        atomic_fetch_add_explicit(&produced_count, 1, memory_order_relaxed);
    }
    
    printf("[producer %d] finished, produced %lld items\n", a->id, items_per_producer);
    return NULL;
}

static void* consumer_thread(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    long long items_per_consumer = a->items / a->num_consumers;
    long long remainder = a->items % a->num_consumers;
    long long local_sum = 0;
  
    if (a->id < remainder) {
        items_per_consumer++;
    }
    
    for (long long i = 0; i < items_per_consumer; i++) {
        sem_wait(&sem_full);
        sem_wait(&mutex);
        
        long long value = buffer[buffer_head];
        int producer_id = buffer_producer_id[buffer_head];
        buffer_head = (buffer_head + 1) % a->buffer_size;
        
        sem_post(&mutex);
        sem_post(&sem_empty);
        
        atomic_fetch_add_explicit(&consumed_count, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&producer_counts[producer_id], 1, memory_order_relaxed);
        
        local_sum += value;
        atomic_fetch_add_explicit(&global_consumed_sum, value, memory_order_relaxed);
    }
    
    atomic_store_explicit(&a->local_consumed_sum, local_sum, memory_order_relaxed);
    printf("[consumer %d] finished, consumed %lld items, local sum=%lld\n", 
           a->id, items_per_consumer, local_sum);
    return NULL;
}

long long calculate_expected_sum(long long items, int num_producers) {
    long long sum = 0;
    long long items_per_producer = items / num_producers;
    long long remainder = items % num_producers;
    
    for (int producer_id = 0; producer_id < num_producers; producer_id++) {
        long long producer_items = items_per_producer;
        if (producer_id < remainder) {
            producer_items++;
        }
       
        long long first_value = (long long)producer_id * 1000000LL + 1;
        long long last_value = (long long)producer_id * 1000000LL + producer_items;
        long long producer_sum = (first_value + last_value) * producer_items / 2;
        
        sum += producer_sum;
    }
    
    return sum;
}

int main(int argc, char** argv) {
    int num_producers = 0, num_consumers = 0, buffer_size = 0;
    long long items = 0;
    
  
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
            num_producers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
            num_consumers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            items = atoll(argv[++i]);
        } else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) {
            buffer_size = atoi(argv[++i]);
        }
    }
    
   
    if (num_producers <= 0 || num_consumers <= 0 || items <= 0 || buffer_size <= 0) {
        fprintf(stderr, "Usage: %s -P <num_producers> -C <num_consumers> -N <items> -B <buffer_size>\n", argv[0]);
        fprintf(stderr, "All values must be positive integers\n");
        return 1;
    }
    
 
    buffer = (long long*)calloc(buffer_size, sizeof(long long));
    buffer_producer_id = (int*)calloc(buffer_size, sizeof(int));
    producer_counts = (atomic_llong*)calloc(num_producers, sizeof(atomic_llong));
    
    if (!buffer || !buffer_producer_id || !producer_counts) {
        fprintf(stderr, "Memory allocation failed\n");
        free(buffer);
        free(buffer_producer_id);
        free(producer_counts);
        return 1;
    }
    
 
    if (sem_init(&mutex, 0, 1) != 0 ||
        sem_init(&sem_empty, 0, buffer_size) != 0 ||
        sem_init(&sem_full, 0, 0) != 0) {
        fprintf(stderr, "Semaphore initialization failed\n");
        free(buffer);
        free(buffer_producer_id);
        free(producer_counts);
        return 1;
    }
  
    atomic_init(&produced_count, 0);
    atomic_init(&consumed_count, 0);
    atomic_init(&global_consumed_sum, 0);
    for (int i = 0; i < num_producers; i++) {
        atomic_init(&producer_counts[i], 0);
    }
   
    pthread_t* threads = (pthread_t*)malloc((num_producers + num_consumers) * sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)malloc((num_producers + num_consumers) * sizeof(thread_args_t));
    
    if (!threads || !args) {
        fprintf(stderr, "Thread allocation failed\n");
        free(buffer);
        free(buffer_producer_id);
        free(producer_counts);
        free(threads);
        free(args);
        sem_destroy(&mutex);
        sem_destroy(&sem_empty);
        sem_destroy(&sem_full);
        return 1;
    }
    
    printf("Starting producers-consumers simulation:\n");
    printf("  Producers: %d, Consumers: %d, Items: %lld, Buffer: %d\n", 
           num_producers, num_consumers, items, buffer_size);
   
    for (int i = 0; i < num_producers; i++) {
        args[i].id = i;
        args[i].items = items;
        args[i].num_producers = num_producers;
        args[i].num_consumers = num_consumers;
        args[i].buffer_size = buffer_size;
        atomic_init(&args[i].local_consumed_sum, 0);
        
        if (pthread_create(&threads[i], NULL, producer_thread, &args[i]) != 0) {
            fprintf(stderr, "Failed to create producer thread %d\n", i);
        }
    }

    for (int i = 0; i < num_consumers; i++) {
        int consumer_id = i;
        args[num_producers + i].id = consumer_id;
        args[num_producers + i].items = items;
        args[num_producers + i].num_producers = num_producers;
        args[num_producers + i].num_consumers = num_consumers;
        args[num_producers + i].buffer_size = buffer_size;
        atomic_init(&args[num_producers + i].local_consumed_sum, 0);
        
        if (pthread_create(&threads[num_producers + i], NULL, consumer_thread, &args[num_producers + i]) != 0) {
            fprintf(stderr, "Failed to create consumer thread %d\n", i);
        }
    }
  
    for (int i = 0; i < num_producers + num_consumers; i++) {
        pthread_join(threads[i], NULL);
    }
   
    long long expected_sum = calculate_expected_sum(items, num_producers);
 
    printf("\n=== FINAL RESULTS ===\n");
    
    long long total_local_sum = 0;
    for (int i = 0; i < num_consumers; i++) {
        long long consumer_sum = atomic_load_explicit(&args[num_producers + i].local_consumed_sum, memory_order_relaxed);
        total_local_sum += consumer_sum;
        printf("[main] consumer %d local sum = %lld\n", i, consumer_sum);
    }
    
    printf("\nProducer statistics:\n");
    long long total_produced = 0;
    for (int i = 0; i < num_producers; i++) {
        long long count = atomic_load(&producer_counts[i]);
        total_produced += count;
        printf("  producer %d: %lld items consumed\n", i, count);
    }
    
    long long actual_produced = atomic_load(&produced_count);
    long long actual_consumed = atomic_load(&consumed_count);
    long long global_sum = atomic_load(&global_consumed_sum);
    
    printf("\nSummary:\n");
    printf("  Expected items: %lld\n", items);
    printf("  Produced: %lld items\n", actual_produced);
    printf("  Consumed: %lld items\n", actual_consumed);
    printf("  Expected sum: %lld\n", expected_sum);
    printf("  Global sum: %lld\n", global_sum);
    printf("  Sum of local sums: %lld\n", total_local_sum);
    
 
    int correct = 1;
    if (actual_produced != items) {
        printf("✗ ERROR: Produced count mismatch! Expected %lld, got %lld\n", items, actual_produced);
        correct = 0;
    }
    if (actual_consumed != items) {
        printf("✗ ERROR: Consumed count mismatch! Expected %lld, got %lld\n", items, actual_consumed);
        correct = 0;
    }
    if (global_sum != total_local_sum) {
        printf("✗ ERROR: Sum mismatch! Global %lld != Local sum %lld\n", global_sum, total_local_sum);
        correct = 0;
    }
    if (global_sum != expected_sum) {
        printf("✗ ERROR: Result sum mismatch! Expected %lld, got %lld\n", expected_sum, global_sum);
        correct = 0;
    }
    
    if (correct) {
        printf("✓ All checks passed - program executed correctly\n");
    }
  
    free(buffer);
    free(buffer_producer_id);
    free(producer_counts);
    free(threads);
    free(args);
    sem_destroy(&mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    
    return correct ? 0 : 1;
}
