#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>

long long counter_unsync = 0;
long long counter_mutex = 0;
atomic_llong counter_atomic = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    long long n_iters;
    char sync_mode[8]; // "unsync", "mutex", "atomic"
} thread_arg_t;

void* increment(void* arg) {
    thread_arg_t* t_arg = (thread_arg_t*)arg;
    long long n = t_arg->n_iters;

    if (strcmp(t_arg->sync_mode, "unsync") == 0) {
        for (long long i = 0; i < n; ++i) {
            counter_unsync++; // гонка!
        }
    } else if (strcmp(t_arg->sync_mode, "mutex") == 0) {
        for (long long i = 0; i < n; ++i) {
            pthread_mutex_lock(&lock);
            counter_mutex++;
            pthread_mutex_unlock(&lock);
        }
    } else if (strcmp(t_arg->sync_mode, "atomic") == 0) {
        for (long long i = 0; i < n; ++i) {
            atomic_fetch_add(&counter_atomic, 1);
        }
    } else {
        fprintf(stderr, "Unknown sync mode: %s\n", t_arg->sync_mode);
        pthread_exit(NULL);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_threads> <num_iterations> <sync_mode>\n", argv[0]);
        fprintf(stderr, "sync_mode: unsync | mutex | atomic\n");
        return 1;
    }

    int N = atoi(argv[1]);
    long long M = atoll(argv[2]);
    char* mode = argv[3];

    if (N <= 0 || M <= 0) {
        fprintf(stderr, "Thread count and iteration count must be positive.\n");
        return 1;
    }

    if (strcmp(mode, "unsync") != 0 && strcmp(mode, "mutex") != 0 && strcmp(mode, "atomic") != 0) {
        fprintf(stderr, "Invalid sync mode: %s\n", mode);
        return 1;
    }

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t* threads = malloc(sizeof(pthread_t) * N);
    thread_arg_t arg;
    arg.n_iters = M / N;
    strncpy(arg.sync_mode, mode, sizeof(arg.sync_mode) - 1);
    arg.sync_mode[sizeof(arg.sync_mode) - 1] = '\0';

    counter_unsync = 0;
    counter_mutex = 0;
    atomic_store(&counter_atomic, 0);

    for (int i = 0; i < N; ++i) {
        pthread_create(&threads[i], NULL, increment, &arg);
    }
    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    long long result = 0;
    if (strcmp(mode, "unsync") == 0) {
        result = counter_unsync;
    } else if (strcmp(mode, "mutex") == 0) {
        result = counter_mutex;
    } else if (strcmp(mode, "atomic") == 0) {
        result = counter_atomic;
    }

    printf("Expected: %lld, Actual: %lld\n", M, result);

    free(threads);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1e9;
    printf("Elapsed: %.5f s\n", elapsed);

    return 0;
}
