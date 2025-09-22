#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* defines */

#define MAX_CHAIN_LEN 100
#define PROC_PATH "/proc"

typedef struct 
{
    pid_t pid;
    pid_t ppid;
    char name[256];
} procces_info_t;

/* utils */

int get_process_info(procces_info_t *pinf, pid_t pid)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%d/status", PROC_PATH, pid);

    FILE *file = fopen(path, "r");
    if (file == NULL)
        return -1;
    
    pinf->pid = pid;
    pinf->ppid = -1;
    pinf->name[0] = '\0';


    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line + 5, "%255s", pinf->name);
        }
        else if (strncmp(line, "PPid:", 5) == 0) {
            sscanf(line + 5, "%d", &pinf->ppid);
        }
        
        if (pinf->ppid != -1 && pinf->name[0] != '\0') {
            break;
        }
    }

    fclose(file);
    return 0;
}

int get_procces_chain(procces_info_t *pinfs)
{
    procces_info_t current_proc_info;
    int chain_len = 0;
    pid_t current_pid = getpid();

    while (chain_len < MAX_CHAIN_LEN)
    {
        if (get_process_info(&current_proc_info, current_pid) == -1)
            break;
        
        pinfs[chain_len] = current_proc_info;
        chain_len++;

        current_pid = current_proc_info.ppid;
        if (current_pid <= 0)
            break;
    }

    return chain_len;
}

void print_procces_chain(procces_info_t *chain, int len)
{
    for (int i = len - 1; i >= 0; --i)
    {
        printf("%s(%d)", chain[i].name, chain[i].pid);

        if (i > 0)
            printf(" <- ");
    }
    printf("\n");
}

/* main */

int main()
{
    procces_info_t pinfs[MAX_CHAIN_LEN];
    int chain_len = get_procces_chain(pinfs);

    if (chain_len > 0)
        print_procces_chain(pinfs, chain_len);
    else
    {
        perror("Empty chain");
        return 1;
    }

    return 0;
}