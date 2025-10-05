#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

pthread_mutex_t mtx;
atomic_ulong counter_atomic = 0;
unsigned long counter_unsync = 0;

typedef struct {
    int id;
    unsigned long M;
    const char *mode;
} thread_arg_t;

void *thread_func(void *arg) {
    thread_arg_t *a = arg;
    if (strcmp(a->mode, "unsync") == 0) {
        for (unsigned long i = 0; i < a->M; i++)
            counter_unsync++;
    } else if (strcmp(a->mode, "mutex") == 0) {
        for (unsigned long i = 0; i < a->M; i++) {
            pthread_mutex_lock(&mtx);
            counter_unsync++;
            pthread_mutex_unlock(&mtx);
        }
    } else if (strcmp(a->mode, "atomic") == 0) {
        for (unsigned long i = 0; i < a->M; i++)
            atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s N M mode\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    unsigned long M = strtoul(argv[2], NULL, 10);
    const char *mode = argv[3];

    pthread_t *threads = malloc(N * sizeof(pthread_t));
    thread_arg_t *args = malloc(N * sizeof(thread_arg_t));

    pthread_mutex_init(&mtx, NULL);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < N; i++) {
        args[i].id = i;
        args[i].M = M;
        args[i].mode = mode;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long long sec = t1.tv_sec - t0.tv_sec;
    long nsec = t1.tv_nsec - t0.tv_nsec;
    if (nsec < 0) { sec--; nsec += 1000000000L; }

    unsigned long expected = (unsigned long)N * M;
    unsigned long actual = (strcmp(mode, "atomic") == 0) ? counter_atomic : counter_unsync;

    printf("mode=%s N=%d M=%lu\n", mode, N, M);
    printf("expected=%lu actual=%lu\n", expected, actual);
    printf("time=%lld.%09ld s\n", sec, nsec);

    pthread_mutex_destroy(&mtx);
    free(threads);
    free(args);
    return 0;
}
