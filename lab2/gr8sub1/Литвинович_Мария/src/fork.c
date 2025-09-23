#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t child1, child2;
    int status1, status2;
    
    printf("=== Лабораторная работа 2 - Создание процессов ===\n");
    printf("Родительский процесс запущен: PID=%d, PPID=%d\n", getpid(), getppid());
    fflush(stdout);
    
    // Создаем первого дочернего процесса
    child1 = fork();
    
    if (child1 == -1) {
        perror("Ошибка при создании первого дочернего процесса");
        exit(1);
    }
    
    if (child1 == 0) {
        // Код первого дочернего процесса
        printf("Дочерний процесс 1: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);  // Имитация работы
        printf("Дочерний процесс 1 завершается\n");
        fflush(stdout);
        exit(10);  // Завершаем с кодом 10
    }
    
    // Создаем второго дочернего процесса
    child2 = fork();
    
    if (child2 == -1) {
        perror("Ошибка при создании второго дочернего процесса");
        exit(1);
    }
    
    if (child2 == 0) {
        // Код второго дочернего процесса
        printf("Дочерний процесс 2: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(1);  // Имитация работы
        printf("Дочерний процесс 2 завершается\n");
        fflush(stdout);
        exit(20);  // Завершаем с кодом 20
    }
    
    // Код родительского процесса
    printf("Родительский процесс ожидает завершения дочерних...\n");
    fflush(stdout);
    
    // Ожидаем завершения первого дочернего процесса
    waitpid(child1, &status1, 0);
    if (WIFEXITED(status1)) {
        printf("Дочерний процесс 1 завершился с кодом: %d\n", WEXITSTATUS(status1));
    }
    
    // Ожидаем завершения второго дочернего процесса
    waitpid(child2, &status2, 0);
    if (WIFEXITED(status2)) {
        printf("Дочерний процесс 2 завершился с кодом: %d\n", WEXITSTATUS(status2));
    }
    
    printf("Родительский процесс завершает работу\n");
    return 0;
}
