#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("parent: PID=%d, starting\n", getpid());
    fflush(stdout);

    pid_t child1 = fork();
    
    if (child1 == 0) {
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        exit(0);
    }
    
    pid_t child2 = fork();
    
    if (child2 == 0) {
        printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        exit(0);
    }
    
    int status;
    pid_t terminated_pid;
    
    terminated_pid = waitpid(child1, &status, 0);
    if (terminated_pid != -1) {
        printf("parent: child_A (PID=%d) exited with status %d\n", terminated_pid, WEXITSTATUS(status));
        fflush(stdout);
    }
    
    terminated_pid = waitpid(child2, &status, 0);
    if (terminated_pid != -1) {
        printf("parent: child_B (PID=%d) exited with status %d\n", terminated_pid, WEXITSTATUS(status));
        fflush(stdout);
    }
    
    printf("parent: PID=%d, all children completed\n", getpid());
    fflush(stdout);
    
    return 0;
}
