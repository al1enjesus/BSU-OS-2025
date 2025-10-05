#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

static long long counter = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void* unsync_thread(void* arg) {
    long long iterations = *(long long*)arg;
    for (long long i = 0; i < iterations; i++) {
        counter++; 
    }
    return NULL;
}

static void* mutex_thread(void* arg) {
    long long iterations = *(long long*)arg;
    for (long long i = 0; i < iterations; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

static void* atomic_thread(void* arg) {
    long long iterations = *(long long*)arg;
    for (long long i = 0; i < iterations; i++) {
        atomic_fetch_add_explicit((atomic_llong*)&counter, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <threads> <iterations> <unsync|mutex|atomic>\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    long long iterations = atoll(argv[2]);
    char* mode = argv[3];
    
    if (num_threads <= 0 || iterations <= 0) {
        fprintf(stderr, "Error: threads and iterations must be positive\n");
        return 1;
    }
    
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    counter = 0;
  
    for (int i = 0; i < num_threads; i++) {
        int create_result = 0;
        if (strcmp(mode, "unsync") == 0) {
            create_result = pthread_create(&threads[i], NULL, unsync_thread, &iterations);
        } else if (strcmp(mode, "mutex") == 0) {
            create_result = pthread_create(&threads[i], NULL, mutex_thread, &iterations);
        } else if (strcmp(mode, "atomic") == 0) {
            create_result = pthread_create(&threads[i], NULL, atomic_thread, &iterations);
        } else {
            fprintf(stderr, "Error: unknown mode '%s'. Use unsync|mutex|atomic\n", mode);
            free(threads);
            return 1;
        }
        
        if (create_result != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
       
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            return 1;
        }
    }
   
 
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
        }
    }
    
    long long expected = num_threads * iterations;
    printf("expected=%lld, actual=%lld\n", expected, counter);
    
    free(threads);
    return 0;
}
