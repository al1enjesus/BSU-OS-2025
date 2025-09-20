#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Максимальная глубина дерева процессов для предотвращения зацикливания
#define MAX_DEPTH 1000

// Функция для получения имени процесса по PID через /proc/<pid>/comm
void get_process_name(pid_t pid, char* buffer, size_t buffer_size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    
    FILE* f = fopen(path, "r");
    if (f) {
        if (fgets(buffer, buffer_size, f)) {
            // Убираем перевод строки
            buffer[strcspn(buffer, "\n")] = 0;
        }
        fclose(f);
    } else {
        strncpy(buffer, "unknown", buffer_size);
    }
}

// Функция для получения PPID через /proc/<pid>/status
pid_t get_parent_pid(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
    pid_t ppid = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            ppid = atoi(line + 5);
            break;
        }
    }
    fclose(f);
    
    return ppid;
}

void print_chain(pid_t pid) {
    char proc_name[256];
    int depth = 0;
    int first = 1;

    // Основной цикл обхода цепочки процессов
    while (pid != 0 && pid != 1 && depth < MAX_DEPTH) {
        get_process_name(pid, proc_name, sizeof(proc_name));

        if (first) {
            printf("%s(%d)", proc_name, pid);
            first = 0;
        } else {
            printf(" ← %s(%d)", proc_name, pid);
        }

        pid = get_parent_pid(pid);
        depth++;
    }

    // Обработка корневого процесса (init)
    if (pid == 1) {
        char init_name[256];
        get_process_name(1, init_name, sizeof(init_name));
        printf(" ← %s(1)", init_name);
    }
    // Защита от превышения максимальной глубины
    else if (depth >= MAX_DEPTH) {
        printf(" ... (max depth reached)");
    }

    printf("\n");
}

int main() {
    print_chain(getpid());
    return 0;
}
