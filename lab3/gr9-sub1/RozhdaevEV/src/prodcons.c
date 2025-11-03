#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#define POISON_PILL INT_MIN

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t free_slots;
    sem_t used_slots;
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
    if (!rb->data) {
        fprintf(stderr, "Failed to allocate buffer memory\n");
        exit(1);
    }
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->producers_active = producers_total;
    
    if (pthread_mutex_init(&rb->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        exit(1);
    }
    if (sem_init(&rb->free_slots, 0, (unsigned int)capacity) != 0) {
        fprintf(stderr, "Failed to initialize semaphore\n");
        exit(1);
    }
    if (sem_init(&rb->used_slots, 0, 0) != 0) {
        fprintf(stderr, "Failed to initialize semaphore\n");
        exit(1);
    }
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&rb->free_slots);
    sem_destroy(&rb->used_slots);
}

static void rb_push(ring_buffer_t* rb, int value) {
    sem_wait(&rb->free_slots);
    pthread_mutex_lock(&rb->mutex);
    
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&rb->used_slots);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    if (sem_trywait(&rb->used_slots) != 0) {
        pthread_mutex_lock(&rb->mutex);
        int active = rb->producers_active;
        pthread_mutex_unlock(&rb->mutex);
        if (active == 0) {
            return 0;
        }
        sem_wait(&rb->used_slots);
    }
    
    pthread_mutex_lock(&rb->mutex);
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    pthread_mutex_unlock(&rb->mutex);
    
    sem_post(&rb->free_slots);
    return 1;
}

static void rb_push_poison_pill(ring_buffer_t* rb) {
    for (int i = 0; i < rb->capacity; i++) {
        rb_push(rb, POISON_PILL);
    }
}

static void rb_producer_done(ring_buffer_t* rb) {
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    if (rb->producers_active == 0) {
        rb_push_poison_pill(rb);
    }
    pthread_mutex_unlock(&rb->mutex);
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (int i = 0; i < a->items_to_produce; i++) {
        int value = (a->producer_index + 1) * 1000000 + i;
        rb_push(a->rb, value);
    }
    rb_producer_done(a->rb);
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int value;
    while (1) {
        if (!rb_pop(a->rb, &value)) {
            break;
        }
        if (value == POISON_PILL) {
            break;
        }
        a->consumed_sum += value;
        a->consumed_count++;
    }
    return NULL;
}

static void usage(const char* prog) {
    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items_total> -B <buffer_size>\n", prog);
    fprintf(stderr, "Limits: P,C: 1-64, B: 1-100000, N: 0-1000000000\n");
}

int main(int argc, char** argv) {
    int P = 2, C = 2, B = 64;
    long long N = 100000;

    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch (opt) {
            case 'P': 
                P = atoi(optarg); 
                if (P <= 0 || P > 64) {
                    fprintf(stderr, "Invalid producers count: %d (must be 1-64)\n", P);
                    return 1;
                }
                break;
            case 'C': 
                C = atoi(optarg); 
                if (C <= 0 || C > 64) {
                    fprintf(stderr, "Invalid consumers count: %d (must be 1-64)\n", C);
                    return 1;
                }
                break;
            case 'N': 
                N = atoll(optarg); 
                if (N < 0 || N > 1000000000LL) {
                    fprintf(stderr, "Invalid items count: %lld (must be 0-1000000000)\n", N);
                    return 1;
                }
                break;
            case 'B': 
                B = atoi(optarg); 
                if (B <= 0 || B > 100000) {
                    fprintf(stderr, "Invalid buffer size: %d (must be 1-100000)\n", B);
                    return 1;
                }
                break;
            default: 
                usage(argv[0]); 
                return 1;
        }
    }

    ring_buffer_t rb;
    rb_init(&rb, B, P);

    pthread_t* pt = (pthread_t*)calloc((size_t)P, sizeof(pthread_t));
    pthread_t* ct = (pthread_t*)calloc((size_t)C, sizeof(pthread_t));
    producer_args_t* pargs = (producer_args_t*)calloc((size_t)P, sizeof(producer_args_t));
    consumer_args_t* cargs = (consumer_args_t*)calloc((size_t)C, sizeof(consumer_args_t));
    
    if (!pt || !ct || !pargs || !cargs) {
        fprintf(stderr, "Memory allocation failed\n");
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

    printf("[prodcons] P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld\n",
           P, C, N, B, produced_total, consumed_total, consumed_sum);

    free(pt);
    free(ct);
    free(pargs);
    free(cargs);
    rb_destroy(&rb);
    return 0;
}
