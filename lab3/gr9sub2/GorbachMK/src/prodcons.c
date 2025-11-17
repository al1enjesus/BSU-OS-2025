#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

typedef struct 
{
	int *data;
	int size;
	int head;
	int tail;
	int count;
	pthread_mutex_t mutex;
	pthread_cond_t not_full;
	pthread_cond_t not_empty;
} buffer_t;

buffer_t buffer;
int done = 0;

void buffer_init(buffer_t *b, int size) 
{
	b->data = malloc(size * sizeof(int));
	if (!b->data) 
	{
		perror("malloc");
		exit(1);
	}
	b->size = size;
	b->head = b->tail = b->count = 0;
	pthread_mutex_init(&b->mutex, NULL);
	pthread_cond_init(&b->not_full, NULL);
	pthread_cond_init(&b->not_empty, NULL);
}

void buffer_destroy(buffer_t *b) 
{
	free(b->data);
	pthread_mutex_destroy(&b->mutex);
	pthread_cond_destroy(&b->not_full);
	pthread_cond_destroy(&b->not_empty);
}

void buffer_put(buffer_t *b, int value) 
{
	pthread_mutex_lock(&b->mutex);
	while (b->count == b->size)
		pthread_cond_wait(&b->not_full, &b->mutex);
	b->data[b->tail] = value;
	b->tail = (b->tail + 1) % b->size;
	b->count++;
	pthread_cond_signal(&b->not_empty);
	pthread_mutex_unlock(&b->mutex);
}

int buffer_get(buffer_t *b) 
{
	pthread_mutex_lock(&b->mutex);
	while (b->count == 0 && !done)
		pthread_cond_wait(&b->not_empty, &b->mutex);

	if (b->count == 0 && done) 
	{
		pthread_mutex_unlock(&b->mutex);
		return -1;
	}

	int value = b->data[b->head];
	b->head = (b->head + 1) % b->size;
	b->count--;
	pthread_cond_signal(&b->not_full);
	pthread_mutex_unlock(&b->mutex);
	return value;
}

void *producer(void *arg) 
{
	int n = *(int *)arg;
	for (int i = 0; i < n; i++) 
	{
		buffer_put(&buffer, i);
	}
	return NULL;
}

void *consumer(void *arg) 
{
	(void)arg;
	while (1) 
	{
		int val = buffer_get(&buffer);
		if (val == -1) break;
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

	if (P <= 0 || C <= 0 || N <= 0 || B <= 0) 
	{
		fprintf(stderr, "Error: all parameters (-P, -C, -N, -B) must be positive integers.\n");
		return 1;
	}

	buffer_init(&buffer, B);

	pthread_t *prod = malloc(P * sizeof(pthread_t));
	pthread_t *cons = malloc(C * sizeof(pthread_t));
	if (!prod || !cons) 
	{
		perror("malloc");
		return 1;
	}

	int per_prod = N / P;
	for (int i = 0; i < P; i++) 
	{
		pthread_create(&prod[i], NULL, producer, &per_prod);
	}
	for (int i = 0; i < C; i++) 
	{
		pthread_create(&cons[i], NULL, consumer, NULL);
	}

	for (int i = 0; i < P; i++) pthread_join(prod[i], NULL);

	pthread_mutex_lock(&buffer.mutex);
	done = 1;
	pthread_cond_broadcast(&buffer.not_empty);
	pthread_mutex_unlock(&buffer.mutex);

	for (int i = 0; i < C; i++) pthread_join(cons[i], NULL);

	buffer_destroy(&buffer);
	free(prod);
	free(cons);

	printf("Done. Produced and consumed %d items successfully.\n", N);
	return 0;
}
