#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>


int main()
{
    pid_t pid1, pid2;

    printf("Parent started. PID: %d\n\n", getpid());
    fflush(stdout);

    pid1 = fork();
    if (pid1 < 0) // error
    {
        perror("fork");
        exit(1);
    }
    if (pid1 == 0) // child proc
    {
        printf("Child[0]: PID - %d, PPID - %d\n", getpid(), getppid());
        fflush(stdout);
        exit(0);
    }

    pid2 = fork();
    if (pid2 < 0) // error
    {
        perror("fork");
        exit(1);
    }
    if (pid2 == 0) // child proc
    {
        printf("Child[1]: PID - %d, PPID - %d\n", getpid(), getppid());
        fflush(stdout);
        exit(0);
    }

    int status;
    for (int i = 0; i < 2; ++i)
    {
        pid_t wpid = wait(&status);
        if (wpid < 0) // error
        {
            perror("wait");
            exit(1);
        }

        if (WIFEXITED(status))
        {
            printf("Parent: Chlild %d finished with code %d\n", wpid, WEXITSTATUS(status));
        }
        else
        {
            printf("Parent: Chlild %d finished abnormaly\n", wpid);
        }
        fflush(stdout);
    }

    printf("\nParent finished. PID: %d\n", getpid());

    return 0;
}