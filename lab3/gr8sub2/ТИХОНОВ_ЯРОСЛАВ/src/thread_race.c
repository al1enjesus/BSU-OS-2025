#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <string.h>

long N;
int M;

unsigned long long counter_unsync = 0;
unsigned long long counter_mutex = 0;
atomic_ulong counter_atomic;
pthread_mutex_t mtx;

void* thread_func_unsync(void* arg) {
    for (long i = 0; i < M; ++i) {
        counter_unsync++;
    }
    return NULL;
}

void* thread_func_mutex(void* arg) {
    for (long i = 0; i < M; ++i) {
        pthread_mutex_lock(&mtx);
        counter_mutex++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

void* thread_func_atomic(void* arg) {
    for (long i = 0; i < M; ++i) {
        atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <threads> <increments_per_thread> <mode: unsync|mutex|atomic>\n", argv[0]);
        return 1;
    }

    int threads_count = atoi(argv[1]);
    M = atoi(argv[2]);
    char* mode = argv[3];

    pthread_t* threads = malloc(sizeof(pthread_t) * threads_count);
    if (!threads) { perror("malloc"); return 2; }

    struct timespec t0, t1, dt;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    if (strcmp(mode, "unsync") == 0) {
        for (int i = 0; i < threads_count; ++i)
            if (pthread_create(&threads[i], NULL, thread_func_unsync, NULL) != 0) { perror("pthread_create"); return 3; }
        for (int i = 0; i < threads_count; ++i) pthread_join(threads[i], NULL);
    } else if (strcmp(mode, "mutex") == 0) {
        pthread_mutex_init(&mtx, NULL);
        for (int i = 0; i < threads_count; ++i)
            if (pthread_create(&threads[i], NULL, thread_func_mutex, NULL) != 0) { perror("pthread_create"); return 3; }
        for (int i = 0; i < threads_count; ++i) pthread_join(threads[i], NULL);
        pthread_mutex_destroy(&mtx);
    } else if (strcmp(mode, "atomic") == 0) {
        atomic_init(&counter_atomic, 0);
        for (int i = 0; i < threads_count; ++i)
            if (pthread_create(&threads[i], NULL, thread_func_atomic, NULL) != 0) { perror("pthread_create"); return 3; }
        for (int i = 0; i < threads_count; ++i) pthread_join(threads[i], NULL);
    } else {
        fprintf(stderr, "Unknown mode %s\n", mode);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    dt.tv_sec = t1.tv_sec - t0.tv_sec;
    dt.tv_nsec = t1.tv_nsec - t0.tv_nsec;
    if (dt.tv_nsec < 0) { dt.tv_sec--; dt.tv_nsec += 1000000000L; }

    unsigned long long expected = (unsigned long long)threads_count * M;
    unsigned long long actual = 0;
    if (strcmp(mode, "unsync") == 0) actual = counter_unsync;
    if (strcmp(mode, "mutex") == 0) actual = counter_mutex;
    if (strcmp(mode, "atomic") == 0) actual = atomic_load(&counter_atomic);

    printf("mode=%s threads=%d M=%d\n", mode, threads_count, M);
    printf("expected = %llu\n", expected);
");