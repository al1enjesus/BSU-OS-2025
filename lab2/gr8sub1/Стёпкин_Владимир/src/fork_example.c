#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    printf("parent(PID=%d) started\n", getpid());
    fflush(stdout);

    pid_t child_pids[2];

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Дочерний процесс
            printf("child_%d: PID=%d, PPID=%d\n", i, getpid(), getppid());
            fflush(stdout);
            exit(0);  // дочерний завершается
        } else {
            // Родительский процесс
            child_pids[i] = pid;
        }
    }

    // Родитель ждёт дочерних процессов
    for (int i = 0; i < 2; i++) {
        int status;
        pid_t wpid = waitpid(child_pids[i], &status, 0);
        if (wpid > 0) {
            if (WIFEXITED(status)) {
                printf("child with PID %d exited with code %d\n", wpid, WEXITSTATUS(status));
            } else {
                printf("child with PID %d terminated abnormally\n", wpid);
            }
            fflush(stdout);
        }
    }

    printf("parent(PID=%d) finished\n", getpid());
    fflush(stdout);

    return 0;
}
