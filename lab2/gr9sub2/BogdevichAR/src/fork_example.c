#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("parent: Starting\\n");
    fflush(stdout);

    pid_t child1 = fork();
    if (child1 == 0) {
        printf("child[0]: PID=%d\\n", getpid());
        sleep(1);
        exit(0);
    }

    pid_t child2 = fork();
    if (child2 == 0) {
        printf("child[1]: PID=%d\\n", getpid());
        sleep(1);
        exit(0);
    }

    wait(NULL);
    wait(NULL);
    printf("parent: All children completed\\n");
    return 0;
}
