#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	fflush(stdout);

	printf("parent(PID=%d)\n", getpid());
	for(int i = 0; i < 2; ++i)
	{

		pid_t child = fork();

		switch(child)
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

	wait(NULL);

	return 0;
}

