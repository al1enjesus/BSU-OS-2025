#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(void) {
pid_t pid1, pid2;


printf("parent: start, PID=%d\n", getpid());
fflush(stdout);


pid1 = fork();
if (pid1 < 0) {
perror("fork");
exit(EXIT_FAILURE);
}


if (pid1 == 0) {
printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
fflush(stdout);
sleep(1);
return 42;
}


pid2 = fork();
if (pid2 < 0) {
perror("fork");
exit(EXIT_FAILURE);
}


if (pid2 == 0) {
printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
fflush(stdout);
sleep(2);
return 84;
}


printf("parent: spawned children %d and %d, waiting...\n", pid1, pid2);
fflush(stdout);


int status;
pid_t w;
while ((w = wait(&status)) > 0) {
if (WIFEXITED(status)) {
printf("parent: child %d exited with code %d\n", w, WEXITSTATUS(status));
} else if (WIFSIGNALED(status)) {
printf("parent: child %d killed by signal %d\n", w, WTERMSIG(status));
} else {
printf("parent: child %d ended with status 0x%x\n", w, status);
}
fflush(stdout);
}


printf("parent: all children handled, exiting\n");
fflush(stdout);
return 0;
}
