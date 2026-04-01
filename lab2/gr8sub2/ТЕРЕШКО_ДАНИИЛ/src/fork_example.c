#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;
    int status;

    printf("parent: PID=%d\n", getpid());
    
    // Первый дочерний процесс
    pid1 = fork();
    if (pid1 == 0) {
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        _exit(0);
    }
    
    // Второй дочерний процесс
    pid2 = fork();
    if (pid2 == 0) {
        printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
        _exit(0);
    }
    
    // Ожидание завершения обоих детей
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    
    return 0;
}