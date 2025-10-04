
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
    
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    
    counter = 0;
    
    for (int i = 0; i < num_threads; i++) {
        if (strcmp(mode, "unsync") == 0) {
            pthread_create(&threads[i], NULL, unsync_thread, &iterations);
        } else if (strcmp(mode, "mutex") == 0) {
            pthread_create(&threads[i], NULL, mutex_thread, &iterations);
        } else if (strcmp(mode, "atomic") == 0) {
            pthread_create(&threads[i], NULL, atomic_thread, &iterations);
        }
    }
   
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
  
    long long expected = num_threads * iterations;
    printf("expected=%lld, actual=%lld\n", expected, counter);
    
    free(threads);
    return 0;
}
