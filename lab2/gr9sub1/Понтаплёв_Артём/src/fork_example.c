#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
printf("parent: start with PID=%d\n", getpid());
fflush(stdout);
for (int i = 0; i < 2; i++)
{
pid_t pid = fork();
if (pid == 0)
{
printf("child[%d]: PID=%d, PPID=%d\n", i, getpid(), getppid());
fflush(stdout);
sleep(5);
exit(0);
}
else if (pid < 0)
{
perror("fork failed");
exit(1);
}
}
sleep(7);
for (int i = 0; i < 2; i++)
{
int status;
pid_t child_pid = wait(&status);
if (child_pid > 0)
{
printf("parent: child %d exited with status %d\n", child_pid, WEXITSTATUS(status));
fflush(stdout);
}
}
printf("parent: all children completed\n");
fflush(stdout);
return 0;
}
