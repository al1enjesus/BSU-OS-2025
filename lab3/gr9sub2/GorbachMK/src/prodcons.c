#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

typedef struct 
{
	int *buf;
	int size;
	int in, out, count;
	pthread_mutex_t mtx;
	pthread_cond_t not_full, not_empty;
	int done;
} buffer_t;

buffer_t buf;
int produced_sum = 0;
int consumed_sum = 0;
pthread_mutex_t sum_lock;

void buffer_init(buffer_t *b, int size) 
{
	b->buf = malloc(size * sizeof(int));
	b->size = size;
	b->in = b->out = b->count = 0;
	b->done = 0;
	pthread_mutex_init(&b->mtx, NULL);
	pthread_cond_init(&b->not_full, NULL);
	pthread_cond_init(&b->not_empty, NULL);
}

void buffer_destroy(buffer_t *b) 
{
	free(b->buf);
	pthread_mutex_destroy(&b->mtx);
	pthread_cond_destroy(&b->not_full);
	pthread_cond_destroy(&b->not_empty);
}

void buffer_put(buffer_t *b, int val) 
{
	pthread_mutex_lock(&b->mtx);
	while (b->count == b->size)
		pthread_cond_wait(&b->not_full, &b->mtx);
	b->buf[b->in] = val;
	b->in = (b->in + 1) % b->size;
	b->count++;
	pthread_cond_signal(&b->not_empty);
	pthread_mutex_unlock(&b->mtx);
}

int buffer_get(buffer_t *b, int *val) 
{
	pthread_mutex_lock(&b->mtx);
	while (b->count == 0 && !b->done)
		pthread_cond_wait(&b->not_empty, &b->mtx);
	if (b->count == 0 && b->done) 
	{
		pthread_mutex_unlock(&b->mtx);
		return 0;
	}
	*val = b->buf[b->out];
	b->out = (b->out + 1) % b->size;
	b->count--;
	pthread_cond_signal(&b->not_full);
	pthread_mutex_unlock(&b->mtx);
	return 1;
}

void *producer(void *arg) 
{
	int n = *(int *)arg;
	for (int i = 0; i < n; i++) 
	{
		buffer_put(&buf, i + 1);
		pthread_mutex_lock(&sum_lock);
		produced_sum += i + 1;
		pthread_mutex_unlock(&sum_lock);
	}
	return NULL;
}

void *consumer(void *arg) 
{
	int val;
	while (buffer_get(&buf, &val)) 
	{
		pthread_mutex_lock(&sum_lock);
		consumed_sum += val;
		pthread_mutex_unlock(&sum_lock);
	}
	return NULL;
}

int main(int argc, char *argv[]) 
{
	int P = 2, C = 2, N = 100000, B = 64;

	for (int i = 1; i < argc; i++) 
	{
		if (strcmp(argv[i], "-P") == 0) P = atoi(argv[++i]);
		else if (strcmp(argv[i], "-C") == 0) C = atoi(argv[++i]);
		else if (strcmp(argv[i], "-N") == 0) N = atoi(argv[++i]);
		else if (strcmp(argv[i], "-B") == 0) B = atoi(argv[++i]);
	}

	buffer_init(&buf, B);
	pthread_mutex_init(&sum_lock, NULL);

	pthread_t prod[P], cons[C];
	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, &N);
	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer, NULL);

	for (int i = 0; i < P; i++)
		pthread_join(prod[i], NULL);

	pthread_mutex_lock(&buf.mtx);
	buf.done = 1;
	pthread_cond_broadcast(&buf.not_empty);
	pthread_mutex_unlock(&buf.mtx);

	for (int i = 0; i < C; i++)
		pthread_join(cons[i], NULL);

	printf("Produced sum = %d | Consumed sum = %d\n", produced_sum, consumed_sum);
	buffer_destroy(&buf);
	pthread_mutex_destroy(&sum_lock);
	return 0;
}
