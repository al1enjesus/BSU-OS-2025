#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    printf("Parent: Starting, PID=%d\n", getpid());
    fflush(stdout);

    pid_t child1, child2;
    int status1, status2;
    char *endptr;
    const int sleep_time = (argc > 1) ? strtol(argv[1], &endptr, 10) : 10;
    if (argc > 1 && (*endptr != '\0' || sleep_time <= 0)) {
    fprintf(stderr, "Некорректное время ожидания\n");
    return 1;
    }

    child1 = fork();
    if (child1 == 0) {
        printf("child1: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(sleep_time);
        exit(0);
    }

    child2 = fork();
    if (child2 == 0) {
        printf("child2: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(sleep_time);
        exit(0);
    }
    sleep(1);

    printf("Parent: Waiting for children...\n");
    fflush(stdout);

    waitpid(child1, &status1, 0);
    printf("Parent: Child1 (PID=%d) exited with status: %d\n", child1, WEXITSTATUS(status1));
    fflush(stdout);

    waitpid(child2, &status2, 0);
    printf("Parent: Child2 (PID=%d) exited with status: %d\n", child2, WEXITSTATUS(status2));
    fflush(stdout);

    printf("Parent: All children completed. Exiting.\n");
    fflush(stdout);

    return 0;
}
