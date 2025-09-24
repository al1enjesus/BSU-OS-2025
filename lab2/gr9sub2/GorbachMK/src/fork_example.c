#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	printf("Parent start: PID=%d\n", getpid());
	fflush(stdout);

	for (int i = 0; i < 2; i++) {
	pid_t pid = fork();

		if (pid == 0) {
		printf("child[%d]: PID=%d, PPID=%d\n", i, getpid(), getppid());
		fflush(stdout);
		return 0;
		}
	}

	for (int i = 0; i < 2; i++) {
	int status;
	pid_t child_pid = wait(&status);
	printf("Child with PID=%d exited, status=%d\n", child_pid, WEXITSTATUS(status));
	fflush(stdout);
	}

	printf("Parent finish: PID=%d\n", getpid());
	return 0;
}
