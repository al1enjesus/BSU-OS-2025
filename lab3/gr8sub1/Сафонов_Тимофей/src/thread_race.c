#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

typedef enum { MODE_UNSYNC, MODE_MUTEX, MODE_ATOMIC } sync_mode_t;

typedef struct {
    int thread_index;
    long long iterations_per_thread;
} thread_args_t;

static long long shared_counter_unsync = 0;
static long long shared_counter_mutex = 0;
static atomic_llong shared_counter_atomic;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline long long now_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void* worker_unsync(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        shared_counter_unsync++;
    }
    return NULL;
}

static void* worker_mutex(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        pthread_mutex_lock(&counter_mutex);
        shared_counter_mutex++;
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

static void* worker_atomic(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        atomic_fetch_add_explicit(&shared_counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

static int parse_mode(const char* s, sync_mode_t* out_mode) {
    if (strcmp(s, "unsync") == 0) {
        *out_mode = MODE_UNSYNC;
        return 0;
    }
    if (strcmp(s, "mutex") == 0) {
        *out_mode = MODE_MUTEX;
        return 0;
    }
    if (strcmp(s, "atomic") == 0) {
        *out_mode = MODE_ATOMIC;
        return 0;
    }
    
    fprintf(stderr, "Unknown mode: %s (use: unsync|mutex|atomic)\n", s);
    return -1;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_threads> <iterations_per_thread> <unsync|mutex|atomic>\n", argv[0]);
        return 1;
    }
    
    char* endptr;
    errno = 0;
    long num_threads_long = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || num_threads_long <= 0 || num_threads_long > INT_MAX) {
        fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
        return 1;
    }
    int num_threads = (int)num_threads_long;
    
    errno = 0;
    long long iters = strtoll(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || iters < 0) {
        fprintf(stderr, "Invalid iterations count: %s\n", argv[2]);
        return 1;
    }

    sync_mode_t mode;
    if (parse_mode(argv[3], &mode) != 0) {
        return 1;
    }

    pthread_t* tids = (pthread_t*)calloc((size_t)num_threads, sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)calloc((size_t)num_threads, sizeof(thread_args_t));
    if (tids == NULL || args == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    atomic_store(&shared_counter_atomic, 0);
    shared_counter_unsync = 0;
    shared_counter_mutex = 0;

    long long start_ms = now_monotonic_ms();

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_index = i;
        args[i].iterations_per_thread = iters;
        void* (*fn)(void*) = NULL;
        switch (mode) {
            case MODE_UNSYNC: fn = worker_unsync; break;
            case MODE_MUTEX: fn = worker_mutex; break;
            case MODE_ATOMIC: fn = worker_atomic; break;
        }
        if (pthread_create(&tids[i], NULL, fn, &args[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(tids[i], NULL);
    }

    long long end_ms = now_monotonic_ms();
    long long expected = (long long)num_threads * iters;
    long long actual = 0;
    if (mode == MODE_UNSYNC) actual = shared_counter_unsync;
    else if (mode == MODE_MUTEX) actual = shared_counter_mutex;
    else actual = atomic_load_explicit(&shared_counter_atomic, memory_order_relaxed);

    printf("mode=%s threads=%d iters_per_thread=%lld expected=%lld actual=%lld time_ms=%lld\n",
           (mode==MODE_UNSYNC?"unsync":mode==MODE_MUTEX?"mutex":"atomic"),
           num_threads, iters, expected, actual, (end_ms - start_ms));

    free(tids);
    free(args);
    return 0;
}