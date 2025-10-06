#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

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
    
    // ЧИСТАЯ ГОНКА ДАННЫХ - только инкремент
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

static sync_mode_t parse_mode(const char* s) {
    if (strcmp(s, "unsync") == 0) return MODE_UNSYNC;
    if (strcmp(s, "mutex") == 0) return MODE_MUTEX;
    if (strcmp(s, "atomic") == 0) return MODE_ATOMIC;
    fprintf(stderr, "Unknown mode: %s (use: unsync|mutex|atomic)\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_threads> <iterations_per_thread> <unsync|mutex|atomic>\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s 32 10000000 unsync   # демонстрация гонки данных\n", argv[0]);
        fprintf(stderr, "  %s 32 10000000 mutex    # корректная синхронизация\n", argv[0]);
        fprintf(stderr, "  %s 32 10000000 atomic   # атомарные операции\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[1]);
    long long iters = atoll(argv[2]);
    sync_mode_t mode = parse_mode(argv[3]);

    if (num_threads <= 0 || iters < 0) {
        fprintf(stderr, "Invalid arguments: threads=%d, iterations=%lld\n", num_threads, iters);
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

    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("ТЕСТ ГОНКИ ДАННЫХ\n");
    printf("Потоков: %d, Итераций на поток: %lld, Режим: %s\n", 
           num_threads, iters, (mode==MODE_UNSYNC?"unsync":mode==MODE_MUTEX?"mutex":"atomic"));
    printf("═══════════════════════════════════════════════════════════════════\n");

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
            fprintf(stderr, "pthread_create failed для потока %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(tids[i], NULL) != 0) {
            fprintf(stderr, "pthread_join failed для потока %d\n", i);
        }
    }

    long long end_ms = now_monotonic_ms();
    long long expected = (long long)num_threads * iters;
    long long actual = 0;
    
    if (mode == MODE_UNSYNC) actual = shared_counter_unsync;
    else if (mode == MODE_MUTEX) actual = shared_counter_mutex;
    else actual = atomic_load_explicit(&shared_counter_atomic, memory_order_relaxed);
    printf("\nРЕЗУЛЬТАТЫ ТЕСТА:\n");
    printf("  Ожидаемое значение: %lld\n", expected);
    printf("  Полученное значение: %lld\n", actual);
    printf("  Расхождение: %lld операций\n", expected - actual);
    printf("  Процент потерь: %.2f%%\n", (expected - actual) * 100.0 / expected);
    printf("  Время выполнения: %lld мс\n", (end_ms - start_ms));
    
    if (expected == actual) {
        printf("  Статус: ✓ КОРРЕКТНО (все операции выполнены)\n");
        if (mode == MODE_UNSYNC) {
            printf("\n  Примечание: Гонка данных не проявилась.\n");
            printf("  Рекомендации для демонстрации гонки:\n");
            printf("    - Увеличить количество потоков (64+)\n");
            printf("    - Увеличить количество итераций (20+ миллионов)\n");
            printf("    - Запустить на многоядерном процессоре\n");
        }
    } else {
        printf("  Статус: ✗ ГОНКА ДАННЫХ (потеряно %lld операций!)\n", expected - actual);
        printf("\n  Анализ гонки данных:\n");
        printf("    Операция shared_counter_unsync++ не атомарна\n");
        printf("    Состоит из: 1) чтение 2) инкремент 3) запись\n");
        printf("    Между шагами другие потоки перезаписывают значение\n");
    }
    
    printf("═══════════════════════════════════════════════════════════════════\n");

    free(tids);
    free(args);
    
    return 0;
}
