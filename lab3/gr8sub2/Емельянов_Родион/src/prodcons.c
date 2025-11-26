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
    sem_t empty;    // Свободные слоты
    sem_t full;     // Занятые слоты
    pthread_mutex_t mutex; // Мьютекс для доступа к буферу
    int producers_active;
} ring_buffer_t;

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
    rb->data = (int*)malloc(sizeof(int) * capacity);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->producers_active = producers_total;
    
    // Инициализация семафоров
    sem_init(&rb->empty, 0, capacity);  // Изначально все слоты свободны
    sem_init(&rb->full, 0, 0);          // Изначально нет занятых слотов
    pthread_mutex_init(&rb->mutex, NULL);
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    sem_destroy(&rb->empty);
    sem_destroy(&rb->full);
    pthread_mutex_destroy(&rb->mutex);
}

static void rb_push(ring_buffer_t* rb, int value) {
    // Ждем свободный слот
    sem_wait(&rb->empty);
    
    // Захватываем мьютекс для доступа к буферу
    pthread_mutex_lock(&rb->mutex);
    
    // Добавляем элемент в буфер
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    
    pthread_mutex_unlock(&rb->mutex);
    
    // Увеличиваем счетчик занятых слотов
    sem_post(&rb->full);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    // Ждем занятый слот или завершение производителей
    while (1) {
        // Пробуем получить элемент
        if (sem_trywait(&rb->full) == 0) {
            pthread_mutex_lock(&rb->mutex);
            
            *value = rb->data[rb->head];
            rb->head = (rb->head + 1) % rb->capacity;
            
            pthread_mutex_unlock(&rb->mutex);
            
            // Освобождаем слот
            sem_post(&rb->empty);
            return 1;
        }
        
        // Проверяем, есть ли еще активные производители
        pthread_mutex_lock(&rb->mutex);
        int active = rb->producers_active > 0;
        pthread_mutex_unlock(&rb->mutex);
        
        if (!active) {
            return 0; // Больше элементов не будет
        }
        
        // Краткая пауза перед повторной попыткой
        usleep(1000);
    }
}

static void rb_producer_done(ring_buffer_t* rb) {
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    pthread_mutex_unlock(&rb->mutex);
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + i;
        rb_push(a->rb, value);
        
        // Небольшая пауза для демонстрации работы
        if (i % 10000 == 0) {
            usleep(1000);
        }
    }
    rb_producer_done(a->rb);
    printf("Producer %d finished, produced %d items\n", 
           a->producer_index, a->items_to_produce);
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int value;
    int count = 0;
    
    while (rb_pop(a->rb, &value)) {
        a->consumed_sum += value;
        a->consumed_count += 1;
        count++;
        
        // Небольшая пауза для демонстрации работы
        if (count % 10000 == 0) {
            usleep(500);
        }
    }
    
    printf("Consumer %d finished, consumed %lld items\n", 
           a->consumer_index, a->consumed_count);
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

    printf("Starting producer-consumer: P=%d C=%d N=%lld B=%d\n", P, C, N, B);

    // Запуск производителей
    for (int i = 0; i < P; i++) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = per_producer + (i < remainder ? 1 : 0);
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) {
            fprintf(stderr, "pthread_create producer failed\n");
            return 1;
        }
    }

    // Запуск потребителей
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

    // Ожидание завершения производителей
    long long produced_total = 0;
    for (int i = 0; i < P; i++) {
        pthread_join(pt[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }

    // Ожидание завершения потребителей
    for (int i = 0; i < C; i++) {
        pthread_join(ct[i], NULL);
    }

    // Подсчет результатов
    long long consumed_total = 0;
    long long consumed_sum = 0;
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    printf("[prodcons] (semaphore version) P=%d C=%d N=%lld B=%d\n", P, C, N, B);
    printf("Produced: %lld items\n", produced_total);
    printf("Consumed: %lld items\n", consumed_total);
    printf("Total sum: %lld\n", consumed_sum);
    
    // Проверка корректности
    if (produced_total == consumed_total) {
        printf("✓ SUCCESS: All items produced and consumed correctly\n");
    } else {
        printf("✗ ERROR: Item count mismatch! Produced: %lld, Consumed: %lld\n", 
               produced_total, consumed_total);
    }

    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
    return 0;
}