// lab3 Variant 1 — Part A: race condition and fixes
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int id;
    long long iters;         // how many increments this thread performs
} worker_arg_t;

static long long expected_total = 0;

// Shared state for different modes
static long long counter_unsync = 0;
static long long counter_mutex  = 0;
static atomic_llong counter_atomic;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static inline long long timespec_diff_ns(struct timespec a, struct timespec b) {
    long long s = (long long)(b.tv_sec - a.tv_sec);
    long long ns = (long long)(b.tv_nsec - a.tv_nsec);
    return s * 1000000000LL + ns;
}

static void *worker_unsync(void *arg_) {
    worker_arg_t *arg = (worker_arg_t*)arg_;
    for (long long i = 0; i < arg->iters; ++i) {
        // Deliberate race: ++ on a shared long long without synchronization
        counter_unsync++;
    }
    return NULL;
}

static void *worker_mutex(void *arg_) {
    worker_arg_t *arg = (worker_arg_t*)arg_;
    for (long long i = 0; i < arg->iters; ++i) {
        pthread_mutex_lock(&mtx);
        counter_mutex++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

static void *worker_atomic(void *arg_) {
    worker_arg_t *arg = (worker_arg_t*)arg_;
    for (long long i = 0; i < arg->iters; ++i) {
        atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <threads N> <increments M_total> <unsync|mutex|atomic>\n", prog);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 2;
    }
    char *end = NULL;
    long N = strtol(argv[1], &end, 10);
    if (*end != '\0' || N <= 0 || N > 1000000) {
        fprintf(stderr, "Invalid N: %s\n", argv[1]); return 2;
    }
    long long M_total = strtoll(argv[2], &end, 10);
    if (*end != '\0' || M_total < 0 || M_total > (long long)9e15) {
        fprintf(stderr, "Invalid M_total: %s\n", argv[2]); return 2;
    }
    const char *mode = argv[3];
    enum { MODE_UNSYNC, MODE_MUTEX, MODE_ATOMIC } m;
    if (strcmp(mode, "unsync") == 0) m = MODE_UNSYNC;
    else if (strcmp(mode, "mutex") == 0) m = MODE_MUTEX;
    else if (strcmp(mode, "atomic") == 0) m = MODE_ATOMIC;
    else { fprintf(stderr, "Unknown mode: %s\n", mode); usage(argv[0]); return 2; }

    expected_total = M_total;
    pthread_t *ths = calloc((size_t)N, sizeof(*ths));
    worker_arg_t *args = calloc((size_t)N, sizeof(*args));
    if (!ths || !args) { perror("calloc"); return 1; }

    // Evenly distribute M_total across N threads with remainder for first R threads
    long long base = (N == 0) ? 0 : (M_total / N);
    long long rem  = (N == 0) ? 0 : (M_total % N);

    atomic_store_explicit(&counter_atomic, 0, memory_order_relaxed);
    counter_mutex = 0;
    counter_unsync = 0;

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) { perror("clock_gettime"); return 1; }

    for (long i = 0; i < N; ++i) {
        args[i].id = (int)i;
        args[i].iters = base + (i < rem ? 1 : 0);
        int rc;
        void *(*fn)(void*) = NULL;
        switch (m) {
            case MODE_UNSYNC: fn = worker_unsync; break;
            case MODE_MUTEX:  fn = worker_mutex;  break;
            case MODE_ATOMIC: fn = worker_atomic; break;
        }
        rc = pthread_create(&ths[i], NULL, fn, &args[i]);
        if (rc != 0) {
            errno = rc;
            perror("pthread_create");
            return 1;
        }
    }
    for (long i = 0; i < N; ++i) {
        int rc = pthread_join(ths[i], NULL);
        if (rc != 0) { errno = rc; perror("pthread_join"); return 1; }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) { perror("clock_gettime"); return 1; }
    long long ns = timespec_diff_ns(t0, t1);
    double ms = ns / 1e6;

    long long actual = 0;
    switch (m) {
        case MODE_UNSYNC: actual = counter_unsync; break;
        case MODE_MUTEX:  actual = counter_mutex;  break;
        case MODE_ATOMIC: actual = atomic_load_explicit(&counter_atomic, memory_order_relaxed); break;
    }

    printf("mode=%s N=%ld M_total=%lld expected=%lld actual=%lld elapsed_ms=%.3f\n",
           mode, N, M_total, expected_total, actual, ms);

    pthread_mutex_destroy(&mtx);
    free(ths); free(args);
    return 0;
}
