
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // fork(), getpid(), getppid()
#include <sys/wait.h> // wait()

int main() {
    printf("parent: PID=%d start\n", getpid());
    fflush(stdout);

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        else if (pid == 0) {
            printf("child[%d]: PID=%d, PPID=%d\n", i, getpid(), getppid());
            fflush(stdout);
            _exit(0);
        }
   
    }

    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("parent: child PID=%d exited, code=%d\n",
                   wpid, WEXITSTATUS(status));
        }
        fflush(stdout);
    }

    printf("parent: PID=%d finished\n", getpid());
    fflush(stdout);

    return 0;
}
