#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>

typedef struct {
    long long counter;
    pthread_mutex_t mutex;
    atomic_long atomic_counter;
    int num_threads;
    long long increments_per_thread;
    char mode[20];
} shared_data_t;

typedef struct {
    int thread_id;
    shared_data_t* shared_data;
} thread_data_t;

void* thread_func_unsync(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    shared_data_t* shared = data->shared_data;
    
    for (long long i = 0; i < shared->increments_per_thread; i++) {
        shared->counter++;
    }
    
    return NULL;
}

void* thread_func_mutex(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    shared_data_t* shared = data->shared_data;
    
    for (long long i = 0; i < shared->increments_per_thread; i++) {
        pthread_mutex_lock(&shared->mutex);
        shared->counter++;
        pthread_mutex_unlock(&shared->mutex);
    }
    
    return NULL;
}

void* thread_func_atomic(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    shared_data_t* shared = data->shared_data;
    
    for (long long i = 0; i < shared->increments_per_thread; i++) {
        atomic_fetch_add_explicit(&shared->atomic_counter, 1, memory_order_relaxed);
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
    
    shared_data_t shared_data;
    shared_data.counter = 0;
    shared_data.num_threads = num_threads;
    shared_data.increments_per_thread = increments_per_thread;
    strcpy(shared_data.mode, mode);
    atomic_init(&shared_data.atomic_counter, 0);
    
    pthread_mutex_init(&shared_data.mutex, NULL);
    
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(num_threads * sizeof(thread_data_t));
    
    clock_t start_time = clock();
    
    // Create threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].shared_data = &shared_data;
        
        if (strcmp(mode, "unsync") == 0) {
            pthread_create(&threads[i], NULL, thread_func_unsync, &thread_data[i]);
        } else if (strcmp(mode, "mutex") == 0) {
            pthread_create(&threads[i], NULL, thread_func_mutex, &thread_data[i]);
        } else if (strcmp(mode, "atomic") == 0) {
            pthread_create(&threads[i], NULL, thread_func_atomic, &thread_data[i]);
        } else {
            printf("Unknown mode: %s\n", mode);
            free(threads);
            free(thread_data);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end_time = clock();
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Print results
    long long expected = num_threads * increments_per_thread;
    long long actual = (strcmp(mode, "atomic") == 0) ? atomic_load(&shared_data.atomic_counter) : shared_data.counter;
    
    printf("Mode: %s\n", mode);
    printf("Threads: %d\n", num_threads);
    printf("Increments per thread: %lld\n", increments_per_thread);
    printf("Expected counter: %lld\n", expected);
    printf("Actual counter: %lld\n", actual);
    printf("Execution time: %.4f seconds\n", execution_time);
    printf("Correct: %s\n", (expected == actual) ? "YES" : "NO");
    
    // Cleanup
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&shared_data.mutex);
    
    return 0;
}
