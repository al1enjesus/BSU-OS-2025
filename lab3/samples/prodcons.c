#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    int producers_active;
} ring_buffer_t; // TODO: это готовая структура. Не меняйте поля без необходимости.

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

static void rb_init(ring_buffer_t* rb, int capacity, int producers_total) {
    // TODO: выделите буфер, инициализируйте индексы и синхронизацию
    rb->data = (int*)malloc(sizeof(int)*capacity);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->producers_active = producers_total;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_full, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_full);
    pthread_cond_destroy(&rb->not_empty);
}

static void rb_push(ring_buffer_t* rb, int value) {
    // TODO: реализуйте вставку в кольцевой буфер под mutex с ожиданием not_full
    pthread_mutex_lock(&rb->mutex);
    while (rb->count == rb->capacity) {
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    // TODO: реализуйте извлечение из кольцевого буфера под mutex с ожиданием not_empty
    pthread_mutex_lock(&rb->mutex);
    while (rb->count == 0 && rb->producers_active > 0) {
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }
    if (rb->count == 0 && rb->producers_active == 0) {
        pthread_mutex_unlock(&rb->mutex);
        return 0; // no more items will arrive
    }
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    return 1;
}

static void rb_producer_done(ring_buffer_t* rb) {
    // TODO: сообщайте потребителям об окончании производителей
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    if (rb->producers_active == 0) {
        pthread_cond_broadcast(&rb->not_empty);
    }
    pthread_mutex_unlock(&rb->mutex);
}

static void* producer_thread(void* arg) {
    // TODO: генерируйте элементы и кладите их в буфер
    producer_args_t* a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + i; // пример кодирования
        rb_push(a->rb, value);
    }
    rb_producer_done(a->rb);
    return NULL;
}

static void* consumer_thread(void* arg) {
    // TODO: извлекайте элементы пока доступны и агрегируйте метрики
    consumer_args_t* a = (consumer_args_t*)arg;
    int v;
    while (rb_pop(a->rb, &v)) {
        a->consumed_sum += v;
        a->consumed_count += 1;
    }
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
    rb_init(&rb, B, P);

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Allocation failed\n");
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

    printf("[prodcons] (samples skeleton) P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld\n",
           P, C, N, B, produced_total, consumed_total, consumed_sum);

    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
    return 0;
}


