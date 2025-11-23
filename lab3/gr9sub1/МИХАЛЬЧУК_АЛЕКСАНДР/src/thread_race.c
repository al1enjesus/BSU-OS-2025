#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 8

typedef struct {
    long long counter;
    pthread_mutex_t mutex;
    atomic_long atomic_counter;
    int num_threads;
    long long increments_per_thread;
    char mode[20];
} counter_t;

void* worker(void* arg) {
    counter_t* counter = (counter_t*)arg;
    
    if (strcmp(counter->mode, "unsync") == 0) {
        for (long long i = 0; i < counter->increments_per_thread; i++) {
            counter->counter++;
        }
    } else if (strcmp(counter->mode, "mutex") == 0) {
        for (long long i = 0; i < counter->increments_per_thread; i++) {
            pthread_mutex_lock(&counter->mutex);
            counter->counter++;
            pthread_mutex_unlock(&counter->mutex);
        }
    } else if (strcmp(counter->mode, "atomic") == 0) {
        for (long long i = 0; i < counter->increments_per_thread; i++) {
            atomic_fetch_add_explicit(&counter->atomic_counter, 1, memory_order_relaxed);
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <num_threads> <increments_per_thread> <unsync|mutex|atomic>\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    long long increments_per_thread = atoll(argv[2]);
    char* mode = argv[3];
    
    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Number of threads must be between 1 and %d\n", MAX_THREADS);
        return 1;
    }
    
    counter_t counter;
    counter.num_threads = num_threads;
    counter.increments_per_thread = increments_per_thread;
    strcpy(counter.mode, mode);
    counter.counter = 0;
    atomic_init(&counter.atomic_counter, 0);
    
    if (strcmp(mode, "mutex") == 0) {
        pthread_mutex_init(&counter.mutex, NULL);
    }
    
    pthread_t threads[MAX_THREADS];
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, &counter) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (strcmp(mode, "mutex") == 0) {
        pthread_mutex_destroy(&counter.mutex);
    }
    
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    long long expected = num_threads * increments_per_thread;
    long long actual;
    
    if (strcmp(mode, "atomic") == 0) {
        actual = atomic_load(&counter.atomic_counter);
    } else {
        actual = counter.counter;
    }
    
    printf("Mode: %s\n", mode);
    printf("Threads: %d, Increments per thread: %lld\n", num_threads, increments_per_thread);
    printf("Expected: %lld, Actual: %lld\n", expected, actual);
    printf("Time: %.6f seconds\n", time_taken);
    printf("Status: %s\n", (expected == actual) ? "CORRECT" : "INCORRECT");
    
    return 0;
}
