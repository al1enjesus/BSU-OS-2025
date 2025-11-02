#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>  // Добавьте эту строку

#define MAX_LINE 1024

typedef struct {
    unsigned long vsz;
    unsigned long rss;
    unsigned long min_flt;
    unsigned long maj_flt;
    char comm[256];
} ProcessInfo;

int read_proc_file(pid_t pid, const char* filename, char* buffer, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/%s", pid, filename);
    
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    
    if (fgets(buffer, size, f) == NULL) {
        fclose(f);
        return 0;
    }
    
    fclose(f);
    return 1;
}

int get_process_info(pid_t pid, ProcessInfo* info) {
    char buffer[MAX_LINE];
    
    if (read_proc_file(pid, "stat", buffer, sizeof(buffer))) {
        char* token = strtok(buffer, " ");
        int field = 0;
        
        while (token != NULL) {
            field++;
            switch (field) {
                case 2:
                    strncpy(info->comm, token, sizeof(info->comm) - 1);
                    if (info->comm[0] == '(') {
                        memmove(info->comm, info->comm + 1, strlen(info->comm));
                    }
                    if (info->comm[strlen(info->comm) - 1] == ')') {
                        info->comm[strlen(info->comm) - 1] = '\0';
                    }
                    break;
                case 10:
                    info->min_flt = strtoul(token, NULL, 10);
                    break;
                case 12:
                    info->maj_flt = strtoul(token, NULL, 10);
                    break;
            }
            token = strtok(NULL, " ");
        }
    }
    
    if (read_proc_file(pid, "status", buffer, sizeof(buffer))) {
        char* line = strtok(buffer, "\n");
        while (line) {
            if (strncmp(line, "VmSize:", 7) == 0) {
                sscanf(line, "VmSize: %lu kB", &info->vsz);
            } else if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %lu kB", &info->rss);
            }
            line = strtok(NULL, "\n");
        }
    }
    
    return 1;
}

void print_process_info(const ProcessInfo* info) {
    printf("Процесс: %s\n", info->comm);
    printf("VSZ:  %.2f MB\n", info->vsz / 1024.0);
    printf("RSS:  %.2f MB\n", info->rss / 1024.0);
    printf("Page Faults:\n");
    printf("  Minor: %lu\n", info->min_flt);
    printf("  Major: %lu\n", info->maj_flt);
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    FILE* f = fopen(path, "r");
    if (!f) {
        printf("Не могу прочитать карту памяти для процесса %d\n", pid);
        return;
    }
    
    printf("\n=== Карта памяти (первые 15 строк) ===\n");
    
    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 15) {
        printf("%s", line);
        count++;
    }
    
    if (count == 15) {
        printf("... (обрезано)\n");
    }
    
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Использование: %s <PID>\n", argv[0]);
        printf("  PID: ID процесса для анализа\n");
        printf("Пример: %s %d\n", argv[0], getpid());
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    
    ProcessInfo info = {0};
    if (get_process_info(pid, &info)) {
        printf("=== Memory Profiler ===\n");
        print_process_info(&info);
        print_memory_map(pid);
    } else {
        printf("Не могу получить доступ к процессу %d.\n", pid);
        return 1;
    }
    
    return 0;
}
