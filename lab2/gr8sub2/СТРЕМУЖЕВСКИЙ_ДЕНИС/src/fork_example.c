#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("parent: start PID=%ld\n", (long)getpid());
    fflush(stdout);

    pid_t kids[2];
    for (int i = 0; i < 2; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            printf("child[%d]: PID=%ld, PPID=%ld\n",
                   i, (long)getpid(), (long)getppid());
            fflush(stdout);
            sleep(10);
            _exit(10 + i);
        } else {
            kids[i] = pid;
        }
    }

    for (int i = 0; i < 2; ++i) {
        int status = 0;
        pid_t w;
        do {
            w = waitpid(kids[i], &status, 0);
        } while (w == -1 && errno == EINTR);

        if (w == -1) {
            perror("waitpid");
            continue;
        }

        if (WIFEXITED(status)) {
            printf("parent: child[%d] pid=%ld exited code=%d\n",
                   i, (long)w, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("parent: child[%d] pid=%ld killed by signal %d\n",
                   i, (long)w, WTERMSIG(status));
        } else {
            printf("parent: child[%d] pid=%ld ended with unusual status=0x%x\n",
                   i, (long)w, status);
        }
        fflush(stdout);
    }

    printf("parent: done PID=%ld\n", (long)getpid());
    fflush(stdout);
    return 0;
}
