// lab3 Variant 1 — Part B: Producer–Consumer with bounded buffer (mutex + condvar)
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

typedef struct {
    int *buf;
    size_t cap;
    size_t head, tail, count;
    pthread_mutex_t mtx;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool producers_done;
} ring_t;

typedef struct {
    int tid;
    long long start_val;   // inclusive
    long long end_val;     // inclusive; 0 if producer has no work
    ring_t *rb;
} producer_arg_t;

typedef struct {
    int tid;
    ring_t *rb;
    long long local_count;
    long long local_sum;
} consumer_arg_t;

static inline long long timespec_diff_ns(struct timespec a, struct timespec b) {
    long long s = (long long)(b.tv_sec - a.tv_sec);
    long long ns = (long long)(b.tv_nsec - a.tv_nsec);
    return s * 1000000000LL + ns;
}

static void ring_init(ring_t *r, size_t cap) {
    r->buf = (int*)malloc(cap * sizeof(int));
    if (!r->buf) { perror("malloc"); exit(1); }
    r->cap = cap; r->head = r->tail = r->count = 0;
    r->producers_done = false;
    pthread_mutex_init(&r->mtx, NULL);
    pthread_cond_init(&r->not_full, NULL);
    pthread_cond_init(&r->not_empty, NULL);
}

static void ring_destroy(ring_t *r) {
    pthread_cond_destroy(&r->not_empty);
    pthread_cond_destroy(&r->not_full);
    pthread_mutex_destroy(&r->mtx);
    free(r->buf);
}

static void ring_push(ring_t *r, int v) {
    pthread_mutex_lock(&r->mtx);
    while (r->count == r->cap) {
        pthread_cond_wait(&r->not_full, &r->mtx);
    }
    r->buf[r->tail] = v;
    r->tail = (r->tail + 1) % r->cap;
    r->count++;
    pthread_cond_signal(&r->not_empty);
    pthread_mutex_unlock(&r->mtx);
}

static bool ring_pop(ring_t *r, int *out) {
    pthread_mutex_lock(&r->mtx);
    while (r->count == 0 && !r->producers_done) {
        pthread_cond_wait(&r->not_empty, &r->mtx);
    }
    if (r->count == 0 && r->producers_done) {
        pthread_mutex_unlock(&r->mtx);
        return false; // graceful shutdown
    }
    *out = r->buf[r->head];
    r->head = (r->head + 1) % r->cap;
    r->count--;
    pthread_cond_signal(&r->not_full);
    pthread_mutex_unlock(&r->mtx);
    return true;
}

static void *producer(void *arg_) {
    producer_arg_t *a = (producer_arg_t*)arg_;
    if (a->end_val < a->start_val) return NULL; // no work
    for (long long v = a->start_val; v <= a->end_val; ++v) {
        ring_push(a->rb, (int)v);
    }
    return NULL;
}

static void *consumer(void *arg_) {
    consumer_arg_t *a = (consumer_arg_t*)arg_;
    int val;
    while (ring_pop(a->rb, &val)) {
        a->local_count++;
        a->local_sum += val;
    }
    return NULL;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <total_items> -B <buffer_capacity>\n", prog);
}

int main(int argc, char **argv) {
    long P = 0, C = 0;
    long long N_total = 0;
    long B = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-P") == 0 && i+1 < argc) { P = strtol(argv[++i], NULL, 10); }
        else if (strcmp(argv[i], "-C") == 0 && i+1 < argc) { C = strtol(argv[++i], NULL, 10); }
        else if (strcmp(argv[i], "-N") == 0 && i+1 < argc) { N_total = strtoll(argv[++i], NULL, 10); }
        else if (strcmp(argv[i], "-B") == 0 && i+1 < argc) { B = strtol(argv[++i], NULL, 10); }
        else { usage(argv[0]); return 2; }
    }
    if (P <= 0 || C <= 0 || N_total < 0 || B <= 0) { usage(argv[0]); return 2; }

    ring_t rb; ring_init(&rb, (size_t)B);

    pthread_t *pth = calloc((size_t)P, sizeof(*pth));
    pthread_t *cth = calloc((size_t)C, sizeof(*cth));
    producer_arg_t *pargs = calloc((size_t)P, sizeof(*pargs));
    consumer_arg_t *cargs = calloc((size_t)C, sizeof(*cargs));
    if (!pth || !cth || !pargs || !cargs) { perror("calloc"); return 1; }

    // Distribute 1..N_total across P producers without overlap
    long long base = (P == 0) ? 0 : (N_total / P);
    long long rem  = (P == 0) ? 0 : (N_total % P);
    long long cur = 1;
    for (long i = 0; i < P; ++i) {
        long long quota = base + (i < rem ? 1 : 0);
        pargs[i].tid = (int)i;
        pargs[i].rb = &rb;
        pargs[i].start_val = (quota > 0) ? cur : 1;
        pargs[i].end_val   = (quota > 0) ? (cur + quota - 1) : 0;
        cur += quota;
    }
    for (long i = 0; i < C; ++i) {
        cargs[i].tid = (int)i;
        cargs[i].rb = &rb;
        cargs[i].local_count = 0;
        cargs[i].local_sum = 0;
    }

    struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);

    for (long i = 0; i < P; ++i) {
        int rc = pthread_create(&pth[i], NULL, producer, &pargs[i]);
        if (rc != 0) { errno = rc; perror("pthread_create producer"); return 1; }
    }
    for (long i = 0; i < C; ++i) {
        int rc = pthread_create(&cth[i], NULL, consumer, &cargs[i]);
        if (rc != 0) { errno = rc; perror("pthread_create consumer"); return 1; }
    }

    // Join producers, then mark done and wake all consumers
    for (long i = 0; i < P; ++i) {
        int rc = pthread_join(pth[i], NULL);
        if (rc != 0) { errno = rc; perror("pthread_join producer"); return 1; }
    }
    pthread_mutex_lock(&rb.mtx);
    rb.producers_done = true;
    pthread_cond_broadcast(&rb.not_empty);
    pthread_mutex_unlock(&rb.mtx);

    for (long i = 0; i < C; ++i) {
        int rc = pthread_join(cth[i], NULL);
        if (rc != 0) { errno = rc; perror("pthread_join consumer"); return 1; }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (double)timespec_diff_ns(t0, t1) / 1e6;

    long long total_count = 0, total_sum = 0;
    for (long i = 0; i < C; ++i) {
        total_count += cargs[i].local_count;
        total_sum += cargs[i].local_sum;
    }
    long long expected_sum = (N_total * (N_total + 1)) / 2;

    printf("P=%ld C=%ld N=%lld B=%ld elapsed_ms=%.3f consumed_count=%lld expected_count=%lld sum=%lld expected_sum=%lld %s\n",
           P, C, N_total, B, ms, total_count, N_total, total_sum, expected_sum,
           (total_count == N_total && total_sum == expected_sum) ? "OK" : "MISMATCH");

    free(pth); free(cth); free(pargs); free(cargs);
    ring_destroy(&rb);
    return 0;
}
