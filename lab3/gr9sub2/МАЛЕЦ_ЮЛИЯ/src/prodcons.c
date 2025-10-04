#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    int *buffer;
    int capacity;
    int count;
    int head;
    int tail;
    int done_producing;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} ring_buffer_t;

typedef struct {
    ring_buffer_t *rb;
    int items_to_produce;
    int id;
} producer_arg_t;

typedef struct {
    ring_buffer_t *rb;
    int id;
} consumer_arg_t;

int rb_init(ring_buffer_t *rb, int capacity) {
    rb->buffer = malloc(sizeof(int) * capacity);
    if (!rb->buffer) {
        return -1;
    }
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->done_producing = 0;
    
    if (pthread_mutex_init(&rb->mutex, NULL) != 0) {
        free(rb->buffer);
        return -1;
    }
    
    if (pthread_cond_init(&rb->not_full, NULL) != 0) {
        pthread_mutex_destroy(&rb->mutex);
        free(rb->buffer);
        return -1;
    }
    
    if (pthread_cond_init(&rb->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&rb->mutex);
        pthread_cond_destroy(&rb->not_full);
        free(rb->buffer);
        return -1;
    }
    
    return 0;
}

void rb_destroy(ring_buffer_t *rb) {
    free(rb->buffer);
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_full);
    pthread_cond_destroy(&rb->not_empty);
}

void *producer(void *arg) {
    producer_arg_t *parg = (producer_arg_t *)arg;
    for (int i = 0; i < parg->items_to_produce; ++i) {
        if (pthread_mutex_lock(&parg->rb->mutex) != 0) {
            break;
        }
        
        while (parg->rb->count == parg->rb->capacity) {
            if (pthread_cond_wait(&parg->rb->not_full, &parg->rb->mutex) != 0) {
                pthread_mutex_unlock(&parg->rb->mutex);
                break;
            }
        }

        int item = i + parg->id * 1000;
        parg->rb->buffer[parg->rb->tail] = item;
        parg->rb->tail = (parg->rb->tail + 1) % parg->rb->capacity;
        parg->rb->count++;

        printf("Producer %d produced %d\n", parg->id, item);

        pthread_cond_signal(&parg->rb->not_empty);
        pthread_mutex_unlock(&parg->rb->mutex);
    }

    if (pthread_mutex_lock(&parg->rb->mutex) == 0) {
        parg->rb->done_producing++;
        pthread_cond_broadcast(&parg->rb->not_empty);
        pthread_mutex_unlock(&parg->rb->mutex);
    }

    printf("Producer %d finished\n", parg->id);
    return NULL;
}

void *consumer(void *arg) {
    consumer_arg_t *carg = (consumer_arg_t *)arg;
    while (1) {
        if (pthread_mutex_lock(&carg->rb->mutex) != 0) {
            break;
        }
        
        while (carg->rb->count == 0) {
            if (carg->rb->done_producing > 0) {
                pthread_mutex_unlock(&carg->rb->mutex);
                printf("Consumer %d finished\n", carg->id);
                return NULL;
            }
            if (pthread_cond_wait(&carg->rb->not_empty, &carg->rb->mutex) != 0) {
                pthread_mutex_unlock(&carg->rb->mutex);
                return NULL;
            }
        }

        int item = carg->rb->buffer[carg->rb->head];
        carg->rb->head = (carg->rb->head + 1) % carg->rb->capacity;
        carg->rb->count--;

        pthread_cond_signal(&carg->rb->not_full);
        pthread_mutex_unlock(&carg->rb->mutex);

        printf("Consumer %d consumed %d\n", carg->id, item);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int P = 0, C = 0, N = 0, B = 0;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'P':
                    if (i + 1 < argc) {
                        P = atoi(argv[++i]);
                    } else {
                        fprintf(stderr, "Option -P requires an argument\n");
                        return 1;
                    }
                    break;
                case 'C':
                    if (i + 1 < argc) {
                        C = atoi(argv[++i]);
                    } else {
                        fprintf(stderr, "Option -C requires an argument\n");
                        return 1;
                    }
                    break;
                case 'N':
                    if (i + 1 < argc) {
                        N = atoi(argv[++i]);
                    } else {
                        fprintf(stderr, "Option -N requires an argument\n");
                        return 1;
                    }
                    break;
                case 'B':
                    if (i + 1 < argc) {
                        B = atoi(argv[++i]);
                    } else {
                        fprintf(stderr, "Option -B requires an argument\n");
                        return 1;
                    }
                    break;
                default:
                    fprintf(stderr, "Unknown option: %s\n", argv[i]);
                    fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer_size>\n", argv[0]);
                    return 1;
            }
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer_size>\n", argv[0]);
            return 1;
        }
    }
    
    if (P <= 0 || C <= 0 || N <= 0 || B <= 0) {
        fprintf(stderr, "Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer_size>\n", argv[0]);
        fprintf(stderr, "All values must be positive integers\n");
        return 1;
    }

    printf("Starting producer-consumer simulation:\n");
    printf("  Producers: %d\n", P);
    printf("  Consumers: %d\n", C);
    printf("  Items per producer: %d\n", N / P);
    printf("  Total items: %d\n", N);
    printf("  Buffer size: %d\n", B);
    printf("\n");

    ring_buffer_t rb;
    if (rb_init(&rb, B) != 0) {
        fprintf(stderr, "Failed to initialize ring buffer\n");
        return 1;
    }

    pthread_t producers[P], consumers[C];
    producer_arg_t pargs[P];
    consumer_arg_t cargs[C];

    for (int i = 0; i < P; ++i) {
        pargs[i].rb = &rb;
        pargs[i].items_to_produce = N / P;
        pargs[i].id = i;
        if (pthread_create(&producers[i], NULL, producer, &pargs[i]) != 0) {
            fprintf(stderr, "Failed to create producer thread %d\n", i);
            rb.done_producing = P;
            pthread_cond_broadcast(&rb.not_empty);
            
            for (int j = 0; j < i; ++j) {
                pthread_join(producers[j], NULL);
            }
            for (int j = 0; j < C; ++j) {
                pthread_join(consumers[j], NULL);
            }
            rb_destroy(&rb);
            return 1;
        }
    }

    for (int i = 0; i < C; ++i) {
        cargs[i].rb = &rb;
        cargs[i].id = i;
        if (pthread_create(&consumers[i], NULL, consumer, &cargs[i]) != 0) {
            fprintf(stderr, "Failed to create consumer thread %d\n", i);
            rb.done_producing = P;
            pthread_cond_broadcast(&rb.not_empty);
            
            for (int j = 0; j < P; ++j) {
                pthread_join(producers[j], NULL);
            }
            for (int j = 0; j < i; ++j) {
                pthread_join(consumers[j], NULL);
            }
            for (int j = i; j < C; ++j) {
                pthread_cancel(consumers[j]);
            }
            rb_destroy(&rb);
            return 1;
        }
    }

    for (int i = 0; i < P; ++i) {
        pthread_join(producers[i], NULL);
    }

    for (int i = 0; i < C; ++i) {
        pthread_join(consumers[i], NULL);
    }

    rb_destroy(&rb);
    
    printf("\nAll producers and consumers finished successfully.\n");
    return 0;
}
