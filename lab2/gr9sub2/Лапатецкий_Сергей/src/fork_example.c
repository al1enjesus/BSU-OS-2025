#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	fflush(stdout);

	printf("parent(PID=%d)\n", getpid());

	pid_t pid[2];

	for(int i = 0; i < 2; ++i)
	{

		pid[i] = fork();

		switch(pid[i])
		{
			case -1:
				printf("Error\n");
			break;

			case 0:
				printf("%cchild[%d](PID=%d, PPID=%d)\n", '-', i, getpid(), getppid());
				sleep(5);
				exit(EXIT_SUCCESS);
			break;
		}
	}

	for(int i = 0; i < 2; ++i)
		wait(NULL);

	return 0;
}

