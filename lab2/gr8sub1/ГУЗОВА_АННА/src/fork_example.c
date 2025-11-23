#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("[parent] start, PID=%d\n", getpid());
    fflush(stdout);

    pid_t children[2];

    for (int i = 0; i < 2; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            printf("[child %c] PID=%d, PPID=%d\n",
                   'A' + i, getpid(), getppid());
            fflush(stdout);
            _exit(EXIT_SUCCESS);
        }
        children[i] = pid;
    }

    for (int i = 0; i < 2; ++i) {
        int status;
        pid_t pid = waitpid(children[i], &status, 0);
        if (pid == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        printf("[parent] child %d exited, status=%d\n", pid, WEXITSTATUS(status));
    }

    printf("[parent] done\n");
    return 0;
}
