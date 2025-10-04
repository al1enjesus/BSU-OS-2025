#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

long long counter = 0;
pthread_mutex_t lock;

typedef struct 
{
	long long niter;
	char mode[16];
} thread_arg_t;

void *worker(void *arg) 
{
	thread_arg_t *targ = (thread_arg_t *)arg;
	for (long long i = 0; i < targ->niter; i++) 
	{
		if (strcmp(targ->mode, "unsync") == 0) 
		{
			counter++;
		} 
		else if (strcmp(targ->mode, "mutex") == 0) 
		{
			pthread_mutex_lock(&lock);
			counter++;
			pthread_mutex_unlock(&lock);
		} 
		else if (strcmp(targ->mode, "atomic") == 0) 
		{
			atomic_fetch_add_explicit((_Atomic long long *)&counter, 1, memory_order_relaxed);
		}
	}
	return NULL;
}

double now_sec() 
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) 
{
	if (argc < 4) 
	{
		fprintf(stderr, "Usage: %s <threads> <iters> <mode>\n", argv[0]);
		fprintf(stderr, "Modes: unsync | mutex | atomic\n");
		return 1;
	}

	int nthreads = atoi(argv[1]);
	long long iters = atoll(argv[2]);
	char *mode = argv[3];

	pthread_t threads[nthreads];
	thread_arg_t args[nthreads];

	if (strcmp(mode, "mutex") == 0)
		pthread_mutex_init(&lock, NULL);

	double t0 = now_sec();
	for (int i = 0; i < nthreads; i++) 
	{
		args[i].niter = iters;
		strcpy(args[i].mode, mode);
		pthread_create(&threads[i], NULL, worker, &args[i]);
	}
	for (int i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);
	double t1 = now_sec();

	long long expected = nthreads * iters;
	printf("Mode: %s | Threads: %d | Iters/thread: %lld\n", mode, nthreads, iters);
	printf("Expected: %lld | Actual: %lld | Time: %.6f s\n", expected, counter, t1 - t0);

	if (strcmp(mode, "mutex") == 0)
		pthread_mutex_destroy(&lock);
	return 0;
}
