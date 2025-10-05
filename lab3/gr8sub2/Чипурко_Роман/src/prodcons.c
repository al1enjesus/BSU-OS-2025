#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

typedef struct {
    int* data;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
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

static sem_t sem_slots;
static sem_t sem_items;
static int consumers_global = 0;

static void rb_init(ring_buffer_t* rb, int capacity, int producers_total, int consumers_total) {
    rb->data = (int*)malloc(sizeof(int) * capacity);
    if (!rb->data) { fprintf(stderr,"Failed to allocate buffer\n"); exit(1); }
    rb->capacity = capacity;
    rb->head = rb->tail = rb->count = 0;
    rb->producers_active = producers_total;
    pthread_mutex_init(&rb->mutex,NULL);
    sem_init(&sem_slots, 0, capacity);
    sem_init(&sem_items, 0, 0);
    consumers_global = consumers_total;
}

static void rb_destroy(ring_buffer_t* rb) {
    free(rb->data);
    pthread_mutex_destroy(&rb->mutex);
    sem_destroy(&sem_slots);
    sem_destroy(&sem_items);
}

static void rb_push(ring_buffer_t* rb, int value) {
    sem_wait(&sem_slots);
    pthread_mutex_lock(&rb->mutex);
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    pthread_mutex_unlock(&rb->mutex);
    sem_post(&sem_items);
}

static int rb_pop(ring_buffer_t* rb, int* value) {
    sem_wait(&sem_items);
    pthread_mutex_lock(&rb->mutex);
    if (rb->count == 0 && rb->producers_active == 0) {
        pthread_mutex_unlock(&rb->mutex);
        return 0;
    }
    if (rb->count > 0) {
        *value = rb->data[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
        rb->count--;
        pthread_mutex_unlock(&rb->mutex);
        sem_post(&sem_slots);
        return 1;
    } else {
        pthread_mutex_unlock(&rb->mutex);
        return 0;
    }
}

static void rb_producer_done(ring_buffer_t* rb) {
    pthread_mutex_lock(&rb->mutex);
    rb->producers_active--;
    int remaining = rb->producers_active;
    if (remaining == 0) {
        for (int i=0; i<consumers_global; i++) sem_post(&sem_items);
    }
    pthread_mutex_unlock(&rb->mutex);
}

static void* producer_thread(void* arg) {
    producer_args_t* a = (producer_args_t*)arg;
    for (int i=0;i<a->items_to_produce;i++) {
        int value = (a->producer_index+1)*1000000 + i;
        rb_push(a->rb,value);
        usleep(100);
    }
    rb_producer_done(a->rb);
    return NULL;
}

static void* consumer_thread(void* arg) {
    consumer_args_t* a = (consumer_args_t*)arg;
    int v;
    while (rb_pop(a->rb,&v)) {
        a->consumed_sum += v;
        a->consumed_count++;
        usleep(100);
    }
    return NULL;
}

static double timespec_diff_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec)/1e9;
}

int main(int argc, char** argv) {
    int P=2,C=2,B=64;
    long long N=100000;

    int opt;
    while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
        switch(opt) {
            case 'P': P=atoi(optarg); break;
            case 'C': C=atoi(optarg); break;
            case 'N': N=atoll(optarg); break;
            case 'B': B=atoi(optarg); break;
            default: fprintf(stderr,"Usage: %s -P <producers> -C <consumers> -N <items> -B <buffer>\n",argv[0]); return 1;
        }
    }

    ring_buffer_t rb;
    rb_init(&rb,B,P,C);

    pthread_t* pt = calloc(P,sizeof(pthread_t));
    pthread_t* ct = calloc(C,sizeof(pthread_t));
    producer_args_t* pargs = calloc(P,sizeof(producer_args_t));
    consumer_args_t* cargs = calloc(C,sizeof(consumer_args_t));

    int per_producer = (int)(N/P);
    int remainder = (int)(N%P);

    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC,&start);

    for (int i=0;i<P;i++) {
        pargs[i].rb = &rb;
        pargs[i].producer_index = i;
        pargs[i].items_to_produce = per_producer + (i<remainder?1:0);
        pthread_create(&pt[i],NULL,producer_thread,&pargs[i]);
    }
    for (int i=0;i<C;i++) {
        cargs[i].rb = &rb;
        cargs[i].consumer_index = i;
        cargs[i].consumed_count = 0;
        cargs[i].consumed_sum = 0;
        pthread_create(&ct[i],NULL,consumer_thread,&cargs[i]);
    }

    long long produced_total = 0;
    for (int i=0;i<P;i++) {
        pthread_join(pt[i],NULL);
        produced_total += pargs[i].items_to_produce;
    }
    for (int i=0;i<C;i++) pthread_join(ct[i],NULL);

    long long consumed_total=0, consumed_sum=0;
    for (int i=0;i<C;i++){
        consumed_total += cargs[i].consumed_count;
        consumed_sum += cargs[i].consumed_sum;
    }

    clock_gettime(CLOCK_MONOTONIC,&end);
    double elapsed = timespec_diff_sec(start,end);

    printf("Producers: %d | Consumers: %d | Buffer: %d\n",P,C,B);
    printf("Produced:  %lld items\n",produced_total);
    printf("Consumed:  %lld items\n",consumed_total);
    printf("Total sum: %lld\n",consumed_sum);
    printf("Elapsed time: %.3f s (%.3f µs per item)\n", elapsed, (elapsed/(double)consumed_total)*1e6);

    sleep(30);

    free(pt); free(ct); free(pargs); free(cargs);
    rb_destroy(&rb);
    return 0;
}
