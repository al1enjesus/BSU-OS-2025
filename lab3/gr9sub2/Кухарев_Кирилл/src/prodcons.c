#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

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
    rb->count = 0;
    rb->producers_active = producers_total;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_full, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
    
    printf("Инициализирован кольцевой буфер: capacity=%d, producers=%d\n", 
           capacity, producers_total);
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_full);
    pthread_cond_destroy(&rb->not_empty);
}

static void rb_push(ring_buffer_t* rb, int value) {
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
    pthread_mutex_lock(&rb->mutex);
    while (rb->count == 0 && rb->producers_active > 0) { 
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }
    
    if (rb->count == 0 && rb->producers_active == 0) {
        pthread_mutex_unlock(&rb->mutex);
        return 0; 
    }
    
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    return 1;
}

static void rb_producer_done(ring_buffer_t* rb) {
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    printf("Производитель завершил работу. Осталось активных: %d\n", rb->producers_active);
    
    if (rb->producers_active == 0) {
        pthread_cond_broadcast(&rb->not_empty);
        printf("Все производители завершили. Уведомление потребителей...\n");
    }
    pthread_mutex_unlock(&rb->mutex);
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    printf("Производитель %d запущен: произведет %d элементов\n", 
           a->producer_index, a->items_to_produce);
    
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + i;
        rb_push(a->rb, value);
    }
    
    printf("Производитель %d завершил работу\n", a->producer_index);
    rb_producer_done(a->rb);
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    printf("Потребитель %d запущен\n", a->consumer_index);
    
    int value;
    int consumed = 0;
    
    while (rb_pop(a->rb, &value)) {
        a->consumed_sum += value;
        a->consumed_count += 1;
        consumed++;
        
    }
    
    printf("Потребитель %d завершил: обработал %lld элементов, сумма=%lld\n", 
           a->consumer_index, a->consumed_count, a->consumed_sum);
    return NULL;
}

static void usage(const char* prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_total> -B <buffer_size>\n", prog);
    fprintf(stderr, "Example: %s -P 2 -C 2 -N 100000 -B 64\n", prog);
}

static inline long long now_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
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

    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("PRODUCER-CONSUMER TEST\n");
    printf("Производители: %d, Потребители: %d, Элементов: %lld, Буфер: %d\n", 
           P, C, N, B);
    printf("═══════════════════════════════════════════════════════════════════\n");

    long long start_ms = now_monotonic_ms();

    ring_buffer_t rb;
    rb_init(&rb, B, P);

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 1;
    }

    int per_producer = (int)(N / P);
    int remainder = (int)(N % P);
    long long distributed_total = 0;
    
    printf("Распределение работы между производителями:\n");
    for (int i = 0; i < P; i++) {
        pargs[i].items_to_produce = per_producer + (i < remainder ? 1 : 0);
        distributed_total += pargs[i].items_to_produce;
        printf("  Производитель %d: %d элементов\n", i, pargs[i].items_to_produce);
    }
    printf("Всего распределено: %lld элементов\n", distributed_total);

    for (int i = 0; i < P; i++) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        if (pthread_create(&pt[i], NULL, producer_thread, &pargs[i]) != 0) {
            fprintf(stderr, "Ошибка создания производителя %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < C; i++) {
        cargs[i].rb = &rb;
        cargs[i].consumed_sum = 0;
        cargs[i].consumed_count = 0;
        cargs[i].consumer_index = i;
        if (pthread_create(&ct[i], NULL, consumer_thread, &cargs[i]) != 0) {
            fprintf(stderr, "Ошибка создания потребителя %d\n", i);
            return 1;
        }
    }

    long long produced_total = 0;
    for (int i = 0; i < P; i++) {
        pthread_join(pt[i], NULL);
        produced_total += pargs[i].items_to_produce;
    }
    printf("Все производители завершили работу\n");

    for (int i = 0; i < C; i++) {
        pthread_join(ct[i], NULL);
    }
    printf("Все потребители завершили работу\n");

    long long end_ms = now_monotonic_ms();

    long long consumed_total = 0;
    long long consumed_sum = 0;
    
    for (int i = 0; i < C; i++) {
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
        printf("Потребитель %d: %lld элементов, сумма=%lld\n", 
               i, cargs[i].consumed_count, cargs[i].consumed_sum);
    }

    long long expected_sum = 0;
    for (int i = 0; i < P; i++) {
        int start = (i + 1) * 1000000;
        int count = pargs[i].items_to_produce;
        expected_sum += (long long)count * start + (long long)count * (count - 1) / 2;
    }

    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("ФИНАЛЬНЫЕ РЕЗУЛЬТАТЫ:\n");
    printf("  Произведено:    %lld элементов\n", produced_total);
    printf("  Потреблено:     %lld элементов\n", consumed_total);
    printf("  Полученная сумма: %lld\n", consumed_sum);
    printf("  Ожидаемая сумма:  %lld\n", expected_sum);
    printf("  Корректность данных: %s\n", 
           (produced_total == consumed_total && consumed_sum == expected_sum) ? 
           "✓ ВЕРНО" : "✗ ОШИБКА");
    printf("  Время выполнения: %lld мс\n", (end_ms - start_ms));
    
    if (produced_total != consumed_total) {
        printf("  ОШИБКА: Потеряно элементов: %lld\n", produced_total - consumed_total);
    }
    if (consumed_sum != expected_sum) {
        printf("  ОШИБКА: Расхождение суммы: %lld\n", expected_sum - consumed_sum);
    }
    printf("═══════════════════════════════════════════════════════════════════\n");


    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);

    return 0;
}