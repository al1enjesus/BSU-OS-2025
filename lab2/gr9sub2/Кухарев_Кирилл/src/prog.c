#include<stdio.h>
#include <unistd.h>   // для fork(), getpid(), getppid()
#include <sys/wait.h> // для wait()

int main() {
    printf("Родительский процесс (PID: %d) запущен.\n",getpid());

    // Создаем первого дочернего процесса
    pid_t first_child = fork();

    if (first_child == 0) {
        // Код первого дочернего процесса
        printf("    Первый дочерний (PID: %d)\n",getpid());
        sleep(1000);
        return 0;
    }

    // Создаем второго дочернего процесса (только в родителе)
    pid_t second_child = fork();

    if (second_child == 0) {
        // Код второго дочернего процесса
        printf("    Второй дочерний (PID: %d)\n",getpid());
        sleep(1000); // Имитация работы
        return 0;
    }

    // Код только для родительского процесса
    printf("Родитель ожидает завершения дочерних процессов...\n" );

    // Ожидаем завершения обоих дочерних процессов
    wait(nullptr); // Ждем первого завершившегося
    wait(nullptr); // Ждем второго завершившегося

    printf("Все дочерние процессы завершены. Родитель (PID %d) завершает работу.\n",getpid());

    return 0;
}