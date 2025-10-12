#include stdio.h
#include stdlib.h
#include pthread.h
#include stdatomic.h
#include string.h
#include time.h

#define THREAD_LIMIT 8

typedef struct {
    long long plain_counter;
    pthread_mutex_t lock;
    atomic_long atomic_val;
    int thread_count;
    long long iterations;
    char mode[16];
} shared_data_t;

void thread_task(void arg) {
    shared_data_t data = (shared_data_t)arg;

    if (strcmp(data-mode, unsync) == 0) {
        for (long long i = 0; i  data-iterations; i++) {
            data-plain_counter++;
        }
    } 
    else if (strcmp(data-mode, mutex) == 0) {
        for (long long i = 0; i  data-iterations; i++) {
            pthread_mutex_lock(&data-lock);
            data-plain_counter++;
            pthread_mutex_unlock(&data-lock);
        }
    } 
    else if (strcmp(data-mode, atomic) == 0) {
        for (long long i = 0; i  data-iterations; i++) {
            atomic_fetch_add_explicit(&data-atomic_val, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

int main(int argc, char argv[]) {
    if (argc != 4) {
        fprintf(stderr, Usage %s threads increments unsyncmutexatomicn, argv[0]);
        return EXIT_FAILURE;
    }

    int n_threads = atoi(argv[1]);
    long long per_thread = atoll(argv[2]);
    char mode = argv[3];

    if (n_threads = 0  n_threads  THREAD_LIMIT) {
        fprintf(stderr, Thread count must be between 1 and %dn, THREAD_LIMIT);
        return EXIT_FAILURE;
    }

    shared_data_t data;
    data.thread_count = n_threads;
    data.iterations = per_thread;
    strcpy(data.mode, mode);
    data.plain_counter = 0;
    atomic_init(&data.atomic_val, 0);

    if (strcmp(mode, mutex) == 0) {
        pthread_mutex_init(&data.lock, NULL);
    }

    pthread_t threads[THREAD_LIMIT];
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i  n_threads; i++) {
        if (pthread_create(&threads[i], NULL, thread_task, &data) != 0) {
            perror(pthread_create);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i  n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);

    if (strcmp(mode, mutex) == 0) {
        pthread_mutex_destroy(&data.lock);
    }

    double elapsed = (finish.tv_sec - start.tv_sec) +
                     (finish.tv_nsec - start.tv_nsec)  1e9;

    long long expected = n_threads  per_thread;
    long long observed = (strcmp(mode, atomic) == 0)
                              atomic_load(&data.atomic_val)
                              data.plain_counter;

    printf(Execution mode %sn, mode);
    printf(Threads %d, Increments %lldn, n_threads, per_thread);
    printf(Expected %lld, Observed %lldn, expected, observed);
    printf(Elapsed %.6f secondsn, elapsed);
    printf(Result %sn, (expected == observed)  CORRECT  INCORRECT);

    return 0;
}
