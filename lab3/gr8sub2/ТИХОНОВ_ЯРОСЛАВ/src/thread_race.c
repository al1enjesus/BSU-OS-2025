#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <string.h>

#define DEFAULT_N 1000000
#define DEFAULT_M 4

unsigned long long counter_unsync = 0;
unsigned long long counter_mutex = 0;
atomic_ulong counter_atomic;

pthread_mutex_t mtx;

typedef struct {
    int id;
    unsigned long long *counter;
} thread_arg_t;

void *thread_func_unsync(void *arg) {
    unsigned long long M = *((unsigned long long*)arg);
    for (unsigned long long i = 0; i < M; i++) {
        counter_unsync++;
    }
    return NULL;
}

void *thread_func_mutex(void *arg) {
    unsigned long long M = *((unsigned long long*)arg);
    for (unsigned long long i = 0; i < M; i++) {
        pthread_mutex_lock(&mtx);
        counter_mutex++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

void *thread_func_atomic(void *arg) {
    unsigned long long M = *((unsigned long long*)arg);
    for (unsigned long long i = 0; i < M; i++) {
        atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int N = DEFAULT_M;
    unsigned long long M = DEFAULT_N;
    char mode[16] = "unsync";

    if (argc >= 2) N = atoi(argv[1]);
    if (argc >= 3) M = strtoull(argv[2], NULL, 10);
    if (argc >= 4) strncpy(mode, argv[3], sizeof(mode)-1);

    pthread_t *threads = malloc(sizeof(pthread_t) * N);
    if (!threads) { perror("malloc"); return 1; }

    struct timespec t0, t1, dt;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    if (strcmp(mode, "unsync") == 0) {
        for (int i = 0; i < N; i++) {
            if (pthread_create(&threads[i], NULL, thread_func_unsync, &M) != 0) {
                perror("pthread_create"); exit(1);
            }
        }
        for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);
    } else if (strcmp(mode, "mutex") == 0) {
        pthread_mutex_init(&mtx, NULL);
        for (int i = 0; i < N; i++) {
            if (pthread_create(&threads[i], NULL, thread_func_mutex, &M) != 0) {
                perror("pthread_create"); exit(1);
            }
        }
        for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);
        pthread_mutex_destroy(&mtx);
    } else if (strcmp(mode, "atomic") == 0) {
        atomic_init(&counter_atomic, 0);
        for (int i = 0; i < N; i++) {
            if (pthread_create(&threads[i], NULL, thread_func_atomic, &M) != 0) {
                perror("pthread_create"); exit(1);
            }
        }
        for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        free(threads);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    dt.tv_sec = t1.tv_sec - t0.tv_sec;
    dt.tv_nsec = t1.tv_nsec - t0.tv_nsec;
    if (dt.tv_nsec < 0) { dt.tv_sec--; dt.tv_nsec += 1000000000; }

    unsigned long long expected = N * M;
    unsigned long long actual = 0;
    if (strcmp(mode, "unsync") == 0) actual = counter_unsync;
    else if (strcmp(mode, "mutex") == 0) actual = counter_mutex;
    else if (strcmp(mode, "atomic") == 0) actual = atomic_load(&counter_atomic);

    printf("mode=%s N=%d M=%llu\n", mode, N, M);
    printf("expected = %llu\n", expected);
");
}