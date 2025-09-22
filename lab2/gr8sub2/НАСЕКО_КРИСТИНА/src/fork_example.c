#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("parent: PID=%d\n", getpid());
    fflush(stdout);

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            
            printf("child_%c: PID=%d, PPID=%d\n", 'A' + i, getpid(), getppid());
            fflush(stdout);
            _exit(0); 
        } else if (pid < 0) {
            perror("fork");
            return 1;
        }
    }

    
    for (int i = 0; i < 2; i++) {
        int status;
        wait(&status);
        printf("child exited with code: %d\n", WEXITSTATUS(status));
    }

    return 0;
}
