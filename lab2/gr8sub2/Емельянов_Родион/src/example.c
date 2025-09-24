#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child1, child2;
    int status1, status2;
    
    printf("parent: PID=%d, starting...\n", getpid());
    fflush(stdout);
    
    child1 = fork();
    if (child1 == 0) {
        // Первый дочерний процесс
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);
        exit(0);
    }
    
    child2 = fork();
    if (child2 == 0) {
        // Второй дочерний процесс
        printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(1);
        exit(0);
    }
    
    // Родительский процесс ожидает завершения детей
    waitpid(child1, &status1, 0);
    waitpid(child2, &status2, 0);
    
    printf("parent: both children completed\n");
    printf("child_A exit code: %d\n", WEXITSTATUS(status1));
    printf("child_B exit code: %d\n", WEXITSTATUS(status2));
    
    return 0;
}