#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

static long long counter = 0;
static _Atomic long long counter_atomic = 0;
pthread_mutex_t lock;

typedef struct 
{
	long long niters;
	const char *mode;
} thread_arg_t;

void *worker(void *arg) 
{
	thread_arg_t *a = (thread_arg_t *)arg;
	long long n = a->niters;
	const char *mode = a->mode;

	if (strcmp(mode, "unsync") == 0) 
	{
		for (long long i = 0; i < n; i++)
			counter++;
	} 
	else if (strcmp(mode, "mutex") == 0) 
	{
		for (long long i = 0; i < n; i++) 
		{
			pthread_mutex_lock(&lock);
			counter++;
			pthread_mutex_unlock(&lock);
		}
	} 
	else if (strcmp(mode, "atomic") == 0) 
	{
		for (long long i = 0; i < n; i++) 
		{
			atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) 
{
	if (argc < 4) 
	{
		fprintf(stderr, "Usage: %s <threads> <iters> <mode>\n", argv[0]);
		return 1;
	}

	int nthreads = atoi(argv[1]);
	long long niters = atoll(argv[2]);
	const char *mode = argv[3];

	if (nthreads <= 0 || niters <= 0) 
	{
		fprintf(stderr, "Error: number of threads and iterations must be positive.\n");
		return 1;
	}

	if (strcmp(mode, "unsync") != 0 && strcmp(mode, "mutex") != 0 && strcmp(mode, "atomic") != 0) 
	{
		fprintf(stderr, "Error: mode must be one of: unsync | mutex | atomic\n");
		return 1;
	}

	pthread_t *threads = malloc(nthreads * sizeof(pthread_t));
	if (!threads) 
	{
		perror("malloc");
		return 1;
	}

	pthread_mutex_init(&lock, NULL);

	thread_arg_t arg = { .niters = niters, .mode = mode };

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0; i < nthreads; i++) 
	{
		pthread_create(&threads[i], NULL, worker, &arg);
	}
	for (int i = 0; i < nthreads; i++) 
	{
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

	long long expected = nthreads * niters;
	long long actual = (strcmp(mode, "atomic") == 0) ? atomic_load(&counter_atomic) : counter;

	printf("Threads: %d, Iters/thread: %lld, Mode: %s\n", nthreads, niters, mode);
	printf("Expected: %lld, Actual: %lld\n", expected, actual);
	printf("Elapsed: %.4f sec\n", elapsed);

	pthread_mutex_destroy(&lock);
	free(threads);
	return 0;
}
