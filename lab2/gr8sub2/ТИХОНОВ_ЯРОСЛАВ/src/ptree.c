#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define MAX_LINE 256


// функция для чтения имени и PPid из /proc/<pid>/status
int read_status(int pid, char *name, int *ppid) {
char path[64];
snprintf(path, sizeof(path), "/proc/%d/status", pid);
FILE *f = fopen(path, "r");
if (!f) return -1;


char line[MAX_LINE];
*ppid = -1;
name[0] = '\0';


while (fgets(line, sizeof(line), f)) {
if (strncmp(line, "Name:", 5) == 0) {
sscanf(line, "Name:\t%255s", name);
} else if (strncmp(line, "PPid:", 5) == 0) {
sscanf(line, "PPid:\t%d", ppid);
}
}
fclose(f);
return 0;
}


int main(void) {
int pid = getpid();
char name[256];
int ppid;


while (1) {
if (read_status(pid, name, &ppid) != 0) break;
printf("%s(%d)", name, pid);
if (pid == 1 || ppid <= 0) break;
printf(" ← ");
pid = ppid;
}


printf("\n");
return 0;
}
