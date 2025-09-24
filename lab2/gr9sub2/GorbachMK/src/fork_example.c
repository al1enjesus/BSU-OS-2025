#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	printf("Parent start: PID=%d\n",getpid());
	fflush(stdout);

	for (int i = 0; i < 2; i++) {
		pid_t pid = fork();

		if (pid == 0) {
			printf("child[%d]: PID=%d,PPID=%d\n", i, getpid(), getppid());
			fflush(stdout);
			while(1) sleep(1);
		}
	}

	for (int i = 0; i < 2; i++) {
		wait(NULL);
	}

	printf("Parent finish\n");
	return 0;
}
