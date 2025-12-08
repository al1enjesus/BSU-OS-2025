#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child_a, child_b;
    int status;

    printf("parent: started with PID=%d\n", getpid());
    fflush(stdout); 

    child_a = fork();
    if (child_a == 0) {
        printf("child_A: my PID=%d, parent PID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);
        exit(0);
    }
	
    child_b = fork();
    if (child_b == 0) {
        printf("child_B: my PID=%d, parent PID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(1);
        exit(0);
    }

    waitpid(child_a, &status, 0);
    waitpid(child_b, &status, 0);

    printf("parent: all children finished, exiting\n");
    return 0;
}
