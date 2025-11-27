#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("parent: PID=%d, PPID=%d - Starting\n", getpid(), getppid());
    fflush(stdout);

    pid_t child1 = fork();
    
    if (child1 == 0) {
        printf("child[0]: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);
        exit(0);
    } else if (child1 < 0) {
        perror("fork failed");
        exit(1);
    }

    pid_t child2 = fork();
    
    if (child2 == 0) {
        printf("child[1]: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(1);
        exit(0);
    } else if (child2 < 0) {
        perror("fork failed");
        exit(1);
    }

    int status;
    pid_t waited_pid;
    
    printf("parent: Waiting for children to complete...\n");
    fflush(stdout);
    
    waited_pid = waitpid(child1, &status, 0);
    if (waited_pid == child1) {
        if (WIFEXITED(status)) {
            printf("parent: Child[0] (PID=%d) exited with code %d\n", child1, WEXITSTATUS(status));
        }
        fflush(stdout);
    }

    waited_pid = waitpid(child2, &status, 0);
    if (waited_pid == child2) {
        if (WIFEXITED(status)) {
            printf("parent: Child[1] (PID=%d) exited with code %d\n", child2, WEXITSTATUS(status));
        }
        fflush(stdout);
    }

    printf("parent: All children completed. Exiting.\n");
    fflush(stdout);
    
    return 0;
}
