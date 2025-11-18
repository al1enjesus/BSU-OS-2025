#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;
    int status1, status2;

    printf("parent: PID=%d, PPID=%d\n", getpid(), getppid());
    fflush(stdout);

   
    pid1 = fork();
    if (pid1 == 0) {
     
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);
        exit(0);
    } else if (pid1 > 0) {
       
       
        pid2 = fork();
        if (pid2 == 0) {
          
            printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            sleep(1);
            exit(0);
        } else if (pid2 > 0) {
            printf("parent: waiting for children...\n");
            fflush(stdout);
            
            waitpid(pid1, &status1, 0);
            printf("parent: child_A (PID=%d) finished with status %d\n", pid1, WEXITSTATUS(status1));
            fflush(stdout);
            
            waitpid(pid2, &status2, 0);
            printf("parent: child_B (PID=%d) finished with status %d\n", pid2, WEXITSTATUS(status2));
            fflush(stdout);
            
            printf("parent: all children finished\n");
            fflush(stdout);
        } else {
            perror("fork failed for child_B");
            exit(1);
        }
    } else {
        perror("fork failed for child_A");
        exit(1);
    }

    return 0;
}
