#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child1, child2;
    int status1, status2;
    
    printf("=== Process Creation Example ===\n");
    printf("parent: starting, PID=%d, PPID=%d\n", getpid(), getppid());
    fflush(stdout);
    
    // Создаем первого потомка
    child1 = fork();
    
    if (child1 == 0) {
        // Код первого потомка
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        printf("child_A: sleeping for 2 seconds...\n");
        fflush(stdout);
        sleep(2);
        printf("child_A: finished\n");
        fflush(stdout);
        exit(10);
    } else if (child1 > 0) {
        // Код родителя
        printf("parent: created child_A with PID=%d\n", child1);
        fflush(stdout);
        
        // Создаем второго потомка
        child2 = fork();
        
        if (child2 == 0) {
            // Код второго потомка
            printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            printf("child_B: sleeping for 1 second...\n");
            fflush(stdout);
            sleep(1);
            printf("child_B: finished\n");
            fflush(stdout);
            exit(20);
        } else if (child2 > 0) {
            // Код родителя
            printf("parent: created child_B with PID=%d\n", child2);
            fflush(stdout);
            
            printf("parent: waiting for children to finish...\n");
            fflush(stdout);
            
            // Ожидаем завершения обоих потомков
            waitpid(child1, &status1, 0);
            printf("parent: child_A (PID=%d) finished with status %d\n", 
                   child1, WEXITSTATUS(status1));
            fflush(stdout);
            
            waitpid(child2, &status2, 0);
            printf("parent: child_B (PID=%d) finished with status %d\n", 
                   child2, WEXITSTATUS(status2));
            fflush(stdout);
            
            printf("parent: all children finished, exiting\n");
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
