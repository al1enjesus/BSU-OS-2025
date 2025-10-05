#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  int *data;
  int capacity;
  int head;
  int tail;
  int count;
  pthread_mutex_t mutex;
  sem_t empty_slots;
  sem_t full_slots;
} ring_buffer_t;

typedef struct {
  ring_buffer_t *rb;
  int items_to_produce;
  int producer_index;
} producer_args_t;

typedef struct {
  ring_buffer_t *rb;
  long long consumed_sum;
  long long consumed_count;
  int consumer_index;
} consumer_args_t;

static void rb_init(ring_buffer_t *rb, const int capacity) {
  rb->data = (int *)malloc(sizeof(int) * (size_t)capacity);
  if (!rb->data) {
    perror("malloc");
    exit(1);
  }
  rb->capacity = capacity;
  rb->head = 0;
  rb->tail = 0;
  pthread_mutex_init(&rb->mutex, NULL);
  if (sem_init(&rb->empty_slots, 0, (unsigned int)capacity) != 0) {
    perror("sem_init(empty_slots)");
    exit(1);
  }
  if (sem_init(&rb->full_slots, 0, 0) != 0) {
    perror("sem_init(full_slots)");
    exit(1);
  }
}

static void rb_destroy(ring_buffer_t *rb) {
  free(rb->data);
  pthread_mutex_destroy(&rb->mutex);
  sem_destroy(&rb->empty_slots);
  sem_destroy(&rb->full_slots);
}

static void rb_push(ring_buffer_t *rb, const int value) {
  while (sem_wait(&rb->empty_slots) == -1 && errno == EINTR) {
  }
  pthread_mutex_lock(&rb->mutex);
  rb->data[rb->tail] = value;
  rb->tail = (rb->tail + 1) % rb->capacity;
  pthread_mutex_unlock(&rb->mutex);
  sem_post(&rb->full_slots);
}

static int rb_pop(ring_buffer_t *rb) {
  while (sem_wait(&rb->full_slots) == -1 && errno == EINTR) {
  }
  pthread_mutex_lock(&rb->mutex);
  const int val = rb->data[rb->head];
  rb->head = (rb->head + 1) % rb->capacity;
  pthread_mutex_unlock(&rb->mutex);
  sem_post(&rb->empty_slots);
  return val;
}

static void *producer_thread(void *arg) {
  producer_args_t *a = (producer_args_t *)arg;
  for (int i = 0; i < a->items_to_produce; i++) {
    int value = (a->producer_index + 1) * 1000000 + i;
    rb_push(a->rb, value);
  }
  return NULL;
}

static void *consumer_thread(void *arg) {
  consumer_args_t *a = (consumer_args_t *)arg;
  for (;;) {
    int v = rb_pop(a->rb);
    if (v == -1) {
      break;
    }
    a->consumed_sum += v;
    a->consumed_count += 1;
  }
  return NULL;
}

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s -P <producers> -C <consumers> -N <items_total> -B "
          "<buffer_size>\n",
          prog);
  fprintf(stderr, "Example: %s -P 2 -C 2 -N 100000 -B 64\n", prog);
}

int main(int argc, char **argv) {
  int P = 2, C = 2, B = 64;
  long long N = 100000;

  int opt;
  while ((opt = getopt(argc, argv, "P:C:N:B:")) != -1) {
    switch (opt) {
    case 'P':
      P = atoi(optarg);
      break;
    case 'C':
      C = atoi(optarg);
      break;
    case 'N':
      N = atoll(optarg);
      break;
    case 'B':
      B = atoi(optarg);
      break;
    default:
      usage(argv[0]);
      return 1;
    }
  }
  if (P <= 0 || C <= 0 || B <= 0 || N < 0) {
    usage(argv[0]);
    return 1;
  }

  ring_buffer_t rb;
  rb_init(&rb, B);

  pthread_t *pt = (pthread_t *)calloc((size_t)P, sizeof(pthread_t));
  pthread_t *ct = (pthread_t *)calloc((size_t)C, sizeof(pthread_t));
  producer_args_t *pargs =
      (producer_args_t *)calloc((size_t)P, sizeof(producer_args_t));
  consumer_args_t *cargs =
      (consumer_args_t *)calloc((size_t)C, sizeof(consumer_args_t));
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

  for (int k = 0; k < C; k++) {
    rb_push(&rb, -1);
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

  printf(
      "[prodcons] P=%d C=%d N=%lld B=%d produced=%lld consumed=%lld sum=%lld\n",
      P, C, N, B, produced_total, consumed_total, consumed_sum);

  int ok = 1;
  if (produced_total != N) {
    fprintf(stderr, "[ERR] produced_total (%lld) != N (%lld)\n", produced_total,
            N);
    ok = 0;
  }
  if (consumed_total != N) {
    fprintf(stderr, "[ERR] consumed_total (%lld) != N (%lld)\n", consumed_total,
            N);
    ok = 0;
  }

  long long expected_sum = 0;
  for (int i = 0; i < P; i++) {
    long long k = pargs[i].items_to_produce;
    long long base = (long long)(i + 1) * 1000000LL;
    expected_sum += k * base + (k * (k - 1)) / 2;
  }
  if (consumed_sum != expected_sum) {
    fprintf(stderr, "[WARN] consumed_sum (%lld) != expected_sum (%lld)\n",
            consumed_sum, expected_sum);
  } else {
    fprintf(stderr, "[OK] sum check passed: %lld\n", consumed_sum);
  }

  if (!ok) {
    return 2;
  }

  free(pt);
  free(ct);
  free(pargs);
  free(cargs);
  rb_destroy(&rb);
  return 0;
}
