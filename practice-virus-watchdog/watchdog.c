#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define PROCESS_NAME "Telegram"
#define CHECK_INTERVAL 5
#define MAX_PID_LENGTH 16
#define MAX_BUFFER_SIZE 1024

int find_process_pid(const char *process_name) {
    FILE *fp;
    char command[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    int pid = -1;
    
    snprintf(command, sizeof(command), "ps aux | grep -i telegram | grep -v grep | grep -v watchdog | grep -v 'telegram.*--' | head -1");
    fp = popen(command, "r");
    
    if (fp == NULL) {
        return -1;
    }
    
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char *token = strtok(buffer, " \t");
        for (int i = 1; i < 2 && token != NULL; i++) {
            token = strtok(NULL, " \t");
        }
        if (token != NULL) {
            pid = atoi(token);
        }
    }
    
    pclose(fp);
    return pid;
}

int kill_process(int pid, int signal_type) {
    if (kill(pid, 0) == -1) {
        printf("Процесс %d уже завершен\n", pid);
        return -1;
    }
    if (kill(pid, signal_type) == 0) {
        printf("Процесс %d завершен сигналом %d\n", pid, signal_type);
        return 0;
    } else {
        perror("Ошибка при завершении процесса");
        return -1;
    }
}

void show_gui_notification(const char *title, const char *message) {
    pid_t pid = fork();
    
    if (pid == 0) {
        char command[MAX_BUFFER_SIZE];
        snprintf(command, sizeof(command), 
                 "zenity --warning --title=\"%s\" --text=\"%s\" --width=400", 
                 title, message);
        system(command);
        exit(0);
    }
}

void turn_off_monitor() {
    int ret = system("xset dpms force off 2>/dev/null");
    (void)ret; 
}

volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

void log_event(const char *message) {
    FILE *log_file = fopen("/tmp/telegram_blocker.log", "a");
    if (log_file) {
        time_t now = time(NULL);
        char time_buf[64];
        struct tm *tm_info = localtime(&now);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(log_file, "[%s] %s\n", time_buf, message);
        fclose(log_file);
    }
}

int main() {
    printf("Запуск монитора процессов...\n");
    printf("Целевой процесс: %s\n", PROCESS_NAME);
    printf("Интервал проверки: %d секунд\n", CHECK_INTERVAL);
    printf("Для остановки нажмите Ctrl+C\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    log_event("Монитор процессов запущен");
    
    int telegram_pid;
    int monitor_off = 0;
    
    while (keep_running) {
        telegram_pid = find_process_pid(PROCESS_NAME);
        printf("Проверка... найден PID: %d\n", telegram_pid);
        
        if (telegram_pid > 0) {
            printf("Обнаружен процесс %s с PID: %d\n", PROCESS_NAME, telegram_pid);
            
            show_gui_notification(
                "Блокировка приложения",
                "Приложение Telegram заблокировано по соображениям безопасности!\n\n"
                "Доступ к мессенджеру ограничен."
            );
                system("xrandr --output $(xrandr | grep ' connected' | head -1 | cut -d' ' -f1) --brightness 0.1 2>/dev/null");
            
            printf("Завершаем процесс %d...\n", telegram_pid);
            
            if (kill_process(telegram_pid, SIGTERM) == -1) {
                printf("Пробуем принудительное завершение...\n");
                kill_process(telegram_pid, SIGKILL);
            }
            
            if (!monitor_off) {
                printf("Выключаем монитор...\n");
                turn_off_monitor();
                monitor_off = 1;
            }
            
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), 
                     "Заблокирован процесс Telegram (PID: %d)", telegram_pid);
            log_event(log_msg);
            
            sleep(CHECK_INTERVAL);
                system("xrandr --output $(xrandr | grep ' connected' | head -1 | cut -d' ' -f1) --brightness 1.0 2>/dev/null");
        }
        
        sleep(CHECK_INTERVAL);
    }
    
    printf("Монитор процессов остановлен\n");
    log_event("Монитор процессов остановлен");
    
    return 0;
}