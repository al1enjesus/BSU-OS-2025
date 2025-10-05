#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    sem_t empty;   // свободные слоты
    sem_t full;    // занятые слоты
    int producers_active;
    int consumers_total;
} ring_buffer_t; // Структура адаптирована для семафоров.

typedef struct {
    ring_buffer_t* rb;
    int items_to_produce;
    int producer_index;
} producer_args_t;

typedef struct {
    ring_buffer_t* rb;
    long long consumed_sum;
    long long consumed_count;
    int consumer_index;
} consumer_args_t;

static void rb_init(ring_buffer_t* rb, int capacity, int producers_total, int consumers_total) {
    rb->data = (int*)malloc(sizeof(int) * capacity);
    if (!rb->data) {
        fprintf(stderr, "Buffer allocation failed\n");
        exit(1);
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->producers_active = producers_total;
    rb->consumers_total = consumers_total;

    if (pthread_mutex_init(&rb->mutex, NULL) != 0) {
        fprintf(stderr, "mutex init failed\n");
        exit(1);
    }
    if (sem_init(&rb->empty, 0, (unsigned int)capacity) != 0) {
        fprintf(stderr, "sem_init(empty) failed\n");
        exit(1);
    }
    if (sem_init(&rb->full, 0, 0) != 0) {
        fprintf(stderr, "sem_init(full) failed\n");
        exit(1);
    }
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&rb->empty);
    sem_destroy(&rb->full);
}

static void rb_push(ring_buffer_t* rb, int value) {
    // Ждём свободный слот и добавляем элемент под защитой mutex.
    sem_wait(&rb->empty);
    pthread_mutex_lock(&rb->mutex);

    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;

    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->full);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    // Ждём доступный элемент (или "разбуживание" при завершении).
    sem_wait(&rb->full);
    pthread_mutex_lock(&rb->mutex);

    // Если элементы отсутствуют и производителей уже нет — завершаемся.
    if (rb->count == 0 && rb->producers_active == 0) {
        pthread_mutex_unlock(&rb->mutex);
        // Не возвращаем слот в empty: это "пустое разбуживание" для завершения.
        return 0;
    }

    // Иначе забираем элемент.
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;

    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->empty);
    return 1;
}

static void rb_producer_done(ring_buffer_t* rb) {
    // Отмечаем завершение производителя. Когда последний завершится — разбудим всех потребителей.
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    int should_wake_consumers = (rb->producers_active == 0);
    pthread_mutex_unlock(&rb->mutex);

    if (should_wake_consumers) {
        // Разбудим всех потребителей, чтобы они могли корректно выйти, если буфер пуст.
        for (int i = 0; i < rb->consumers_total; i++) {
            sem_post(&rb->full);
        }
    }
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + i; // пример кодирования
        rb_push(a->rb, value);
    }
    rb_producer_done(a->rb);
    // getchar();
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int v;
    while (rb_pop(a->rb, &v)) {
        a->consumed_sum += v;
        a->consumed_count += 1;
    }
    //getchar();
    return NULL;
}

static void usage(const char* prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_total> -B <buffer_size>\n", prog);
}

int main(int argc, char** argv) {
    int P = 2, C = 2, B = 64;
    long long N = 100000;

    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt) {
            case 'P': P = atoi(optarg); break;
            case 'C': C = atoi(optarg); break;
            case 'N': N = atoll(optarg); break;
            case 'B': B = atoi(optarg); break;
            default: usage(argv[0]); return 1;
        }
    }
    if (P <= 0 || C <= 0 || B <= 0 || N < 0) {
        usage(argv[0]);
        return 1;
    }

    ring_buffer_t rb;
    rb_init(&rb, B, P, C);

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Allocation failed\n");
        free(pt); free(ct); free(pargs); free(cargs);
        rb_destroy(&rb);
        return 1;
    }

    int per_producer = (int)(N / P);
    int remainder = (int)(N % P);

    for (int i = 0; i < P; i++) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = per_producer + (i < remainder ? 1 : 0);
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) {
            fprintf(stderr, "pthread_create producer failed\n");
            free(pt); free(ct); free(pargs); free(cargs);
            rb_destroy(&rb);
            return 1;
        }
    }

    for (int i = 0; i < C; i++) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&ct[i], NULL, consumer_thread, &cargs[i]) != 0) {
            fprintf(stderr, "pthread_create consumer failed\n");
            free(pt); free(ct); free(pargs); free(cargs);
            rb_destroy(&rb);
            return 1;
        }
    }

    long long produced_total = 0;
    for (int i = 0; i < P; i++) {
        pthread_join(pt[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }
    for (int i = 0; i < C; i++) {
        pthread_join(ct[i], NULL);
    }

    long long consumed_total = 0;
    long long consumed_sum = 0;
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    printf("[prodcons] (semaphores variant) P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld\n",
           P, C, N, B, produced_total, consumed_total, consumed_sum);

    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
     // getchar();
    return 0;
}
