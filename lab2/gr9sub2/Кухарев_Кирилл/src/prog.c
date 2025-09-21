#include<stdio.h>
#include <unistd.h>   
#include <sys/wait.h> 

int main() {
    printf("Родительский процесс (PID: %d) запущен.\n",getpid());

    pid_t first_child = fork();

    if (first_child == 0) {
        printf("    Первый дочерний (PID: %d)\n",getpid());
        sleep(1000);
        return 0;
    }else if(first_child == -1){
        printf("Возникла ошибка дочерний процесс 1 не запустился")
        return 0;
    }

    
    pid_t second_child = fork();

    if (second_child == 0) {
        printf("    Второй дочерний (PID: %d)\n",getpid());
        sleep(1000); // Имитация работы
        return 0;
    }else if(second_child == -1){
        printf("Возникла ошибка дочерний процесс 2 не запустился")
        return 0;
    }

    
    printf("Родитель ожидает завершения дочерних процессов...\n" );

    
    wait(NULL);
    wait(NULL);

    printf("Все дочерние процессы завершены. Родитель (PID %d) завершает работу.\n",getpid());

    return 0;
}