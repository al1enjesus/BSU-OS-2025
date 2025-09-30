#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;
    int status;

    printf("parent: starting, PID=%d, PPID=%d\n", getpid(), getppid());
    fflush(stdout);  

    pid1 = fork();
    if (pid1 == 0) {
        printf("child[0]: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);  
        exit(0);
    }

    pid2 = fork();
    if (pid2 == 0) {
        printf("child[1]: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);  
        exit(0);
    }

    sleep(1);
    
    printf("\n------ Дерево процессов: -----------------\n");
    char command[100];
    sprintf(command, "pstree -p %d", getpid());
    system(command);
    printf("\n-----------------------------------------\n");
    

    waitpid(pid1, &status, 0);
    printf("parent: child[0] finished with status %d\n", WEXITSTATUS(status));
    
    waitpid(pid2, &status, 0);
    printf("parent: child[1] finished with status %d\n", WEXITSTATUS(status));
    
    printf("parent: all children finished\n");
    
    return 0;
}