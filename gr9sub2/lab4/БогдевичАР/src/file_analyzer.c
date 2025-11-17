#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>  // Добавляем для getpid()

int main() {
    printf("📁 File Analyzer - Базовая версия\n");
    printf("==================================\n");
    
    // Простая программа для демонстрации
    printf("Эта программа может быть расширена для анализа файлов\n");
    printf("Текущий PID: %d\n", getpid());
    
    // Демонстрация работы с файлами
    printf("\n--- Анализ текущего каталога ---\n");
    system("ls -la | head -10");
    
    printf("\n--- Информация о памяти процесса ---\n");
    char command[256];
    snprintf(command, sizeof(command), "cat /proc/%d/status | grep -E 'VmSize|VmRSS'", getpid());
    system(command);
    
    return 0;
}
