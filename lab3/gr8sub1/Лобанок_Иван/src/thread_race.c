#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/time.h>

typedef enum { MODE_MUTEX, MODE_ATOMIC } race_mode_t;

long long counter = 0;          // общий счетчик для mutex
pthread_mutex_t mutex;

atomic_llong atomic_counter = 0; // атомарный счетчик

typedef struct {
    long long n_iter;           // количество инкрементов для этого потока
    race_mode_t mode;
} thread_arg_t;

// функция потока
void* thread_func(void* arg) {
    thread_arg_t* a = (thread_arg_t*)arg;
    for (long long i = 0; i < a->n_iter; i++) {
        if (a->mode == MODE_MUTEX) {
            pthread_mutex_lock(&mutex);
            counter++;
            pthread_mutex_unlock(&mutex);
        } else { // MODE_ATOMIC
            atomic_fetch_add_explicit(&atomic_counter, 1, memory_order_relaxed);
        }
    }
    return NULL;
}

// парсинг режима
race_mode_t parse_mode(const char* s) {
    if (strcmp(s, "mutex") == 0) return MODE_MUTEX;
    if (strcmp(s, "atomic") == 0) return MODE_ATOMIC;
    fprintf(stderr, "Unknown mode: %s (use: mutex|atomic)\n", s);
    exit(1);
}

// измерение времени
double get_time_sec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s N M mode\n", argv[0]);
        fprintf(stderr, "Modes: mutex | atomic\n");
        return 1;
    }

    int N = atoi(argv[1]);          // количество потоков
    long long M = atoll(argv[2]);   // общее количество инкрементов
    race_mode_t mode = parse_mode(argv[3]);

    pthread_t* threads = malloc(N * sizeof(pthread_t));
    thread_arg_t* args = malloc(N * sizeof(thread_arg_t));

    if (!threads || !args) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    // делим M итераций между потоками
    long long base_iter = M / N;
    long long remainder = M % N;
    for (int i = 0; i < N; i++) {
        args[i].n_iter = base_iter + (i < remainder ? 1 : 0);
        args[i].mode = mode;
    }

    // обнуляем счетчики
    counter = 0;
    atomic_store(&atomic_counter, 0);

    pthread_mutex_init(&mutex, NULL);

    double t1 = get_time_sec();

    for (int i = 0; i < N; i++) {
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    double t2 = get_time_sec();

    long long expected = M;
    long long actual = (mode == MODE_ATOMIC) ? atomic_load_explicit(&atomic_counter, memory_order_relaxed) : counter;

    printf("Mode: %s\n", argv[3]);
    printf("Threads: %d, Total increments: %lld\n", N, expected);
    printf("Expected: %lld, Actual: %lld\n", expected, actual);
    printf("Time: %.6f sec\n", t2 - t1);

    pthread_mutex_destroy(&mutex);
    free(threads);
    free(args);
    return 0;
}

