#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t stop;

void notify() {
    system("zenity --info --text='Telegram был завершён.' &");
}

void kill_telegram() {
    FILE *fp = popen("pgrep Telegram", "r");
    if (!fp) return;

    char pid[16];
    while (fgets(pid, sizeof(pid), fp)) {
        int pid_num = atoi(pid);
        if (pid_num > 0) {
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "kill -KILL %d", pid_num);
            system(cmd);
	    notify();
            printf("[!] Telegram process %d terminated.\n", pid_num);
        }
    }
    pclose(fp);
}

void handle_int(int signum) {
    stop = 1;
}

int main() {
   signal(SIGINT, handle_int); 
    printf("[*] Watchdog запущен. Ожидание Telegram...\n");
    while (!stop) {
        kill_telegram();
        sleep(1);
    }
    return 0;
}

