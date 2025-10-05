#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

typedef enum { MODE_UNSYNC, MODE_MUTEX, MODE_ATOMIC } mode_t;

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
    // TODO: реализуйте небезопасное инкрементирование общего счетчика без синхронизации
    // Пример цели: shared_counter_unsync++ внутри цикла для демонстрации гонки
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        // TODO: shared_counter_unsync++;
    }
    return NULL;
}

static void* worker_mutex(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    // TODO: реализуйте корректное инкрементирование под защитой mutex
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        // TODO:
        // pthread_mutex_lock(&counter_mutex);
        // shared_counter_mutex++;
        // pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

static void* worker_atomic(void* arg) {
    thread_args_t* a = (thread_args_t*)arg;
    // TODO: реализуйте корректное инкрементирование через атомики (stdatomic.h)
    for (long long i = 0; i < a->iterations_per_thread; i++) {
        // TODO: atomic_fetch_add_explicit(&shared_counter_atomic, 1, memory_order_relaxed);
    }
    return NULL;
}

static mode_t parse_mode(const char* s) {
    if (strcmp(s, "unsync") == 0) return MODE_UNSYNC;
    if (strcmp(s, "mutex") == 0) return MODE_MUTEX;
    if (strcmp(s, "atomic") == 0) return MODE_ATOMIC;
    fprintf(stderr, "Unknown mode: %s (use: unsync|mutex|atomic)\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_threads> <iterations_per_thread> <unsync|mutex|atomic>\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[1]);
    long long iters = atoll(argv[2]);
    mode_t mode = parse_mode(argv[3]);

    if (num_threads <= 0 || iters < 0) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    pthread_t* tids = (pthread_t*)calloc((size_t)num_threads, sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)calloc((size_t)num_threads, sizeof(thread_args_t));
    if (!tids || !args) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    atomic_store(&shared_counter_atomic, 0);
    shared_counter_unsync = 0;
    shared_counter_mutex = 0;

    fprintf(stderr, "[thread_race] Внимание: это скелет (samples). Реализуйте TODO для осмысленных результатов.\n");

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


