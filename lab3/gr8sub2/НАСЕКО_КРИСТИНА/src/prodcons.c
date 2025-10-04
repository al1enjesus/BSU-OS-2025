```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>

//кольцевой буфер
typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
    int producers_active;
    int consumers_total;
} ring_buffer_t;

//аргументы для производителя
typedef struct {
    ring_buffer_t* rb;
    int items_to_produce;
    int producer_index;
    long long start_index;
} producer_args_t;

//аргументы для потребителя
typedef struct {
    ring_buffer_t* rb;
    long long consumed_sum;
    long long consumed_count;
    int consumer_index;
} consumer_args_t;

//инициализация кольцевого буфера
static void rb_init(ring_buffer_t* rb, int capacity, int producers_total, int consumers_total) {
    rb->data = (int*)malloc(sizeof(int)*capacity);
    if (!rb->data) exit(1);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->producers_active = producers_total;
    rb->consumers_total = consumers_total;
    pthread_mutex_init(&rb->mutex, NULL);
    sem_init(&rb->empty, 0, capacity);
    sem_init(&rb->full, 0, 0);
}

//освобождение ресурсов буфера
static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&rb->empty);
    sem_destroy(&rb->full);
}

//добавление элемента в буфер
static void rb_push(ring_buffer_t* rb, int value) {
    sem_wait(&rb->empty);
    pthread_mutex_lock(&rb->mutex);
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->full);
}

//извлечение элемента из буфера
static int rb_pop(ring_buffer_t* rb, int* value) {
    sem_wait(&rb->full);
    pthread_mutex_lock(&rb->mutex);
    if (rb->producers_active == 0 && rb->count == 0) {
        pthread_mutex_unlock(&rb->mutex);
        return 0; 
    }
    if (rb->count > 0) {
        *value = rb->data[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
        rb->count--;
        pthread_mutex_unlock(&rb->mutex);
        sem_post(&rb->empty);
        return 1;
    }
    pthread_mutex_unlock(&rb->mutex);
    return 0;
}

//уведомление о завершении работы производителя
static void rb_producer_done(ring_buffer_t* rb) {
    int post_count = 0;
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    if (rb->producers_active == 0) post_count = rb->consumers_total;
    pthread_mutex_unlock(&rb->mutex);
    for (int i = 0; i < post_count; i++) sem_post(&rb->full);
}

//функция производителя
static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (int)(a->start_index + i);
        rb_push(a->rb, value);
    }
    rb_producer_done(a->rb);
    return NULL;
}

//функция потребителя
static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int v;
    while (1) {
        if (!rb_pop(a->rb, &v)) break;  // Завершаем когда буфер пуст
        a->consumed_sum += v;
        a->consumed_count += 1;
    }
    return NULL;
}

//вывод справки по использованию
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
    
    pthread_t* pt = calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = calloc((size_t)C, sizeof(consumer_args_t));
    if (!pt || !ct || !pargs || !cargs) return 1;
    
    //распределение работы между производителями
    int per_producer = (int)(N / P);
    int remainder = (int)(N % P);
    long long next_start = 0;
    for (int i = 0; i < P; i++) {
        int items = per_producer + (i < remainder ? 1 : 0);
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = items;
        pargs[i].start_index = next_start;
        next_start += items;
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) return 1;
    }
    
    //создание потребителей
    for (int i = 0; i < C; i++) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&ct[i], NULL, consumer_thread, &cargs[i]) != 0) return 1;
    }
    
    //ожидание завершения производителей
    long long produced_total = 0;
    for (int i = 0; i < P; i++) {
        pthread_join(pt[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }
    
    //ожидание завершения потребителей
    for (int i = 0; i < C; i++) pthread_join(ct[i], NULL);
    
    //подсчет и проверка результатов
    long long consumed_total = 0, consumed_sum = 0;
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }
    
    long long expected_sum = 0;
    for (long long i = 0; i < N; i++) expected_sum += i;
    
    printf("P=%d C=%d N=%lld B=%d\n", P, C, N, B);
    printf("Produced: %lld items\n", produced_total);
    printf("Consumed: %lld items\n", consumed_total);
    printf("Expected sum: %lld\n", expected_sum);
    printf("Actual sum: %lld\n", consumed_sum);
    printf("Status: %s\n", (produced_total == consumed_total && expected_sum == consumed_sum) ? "PASS" : "FAIL");
    
    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
    return 0;
}
```
