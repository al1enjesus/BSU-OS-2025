#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() 
{
int pid = getppid();
int first = 1;
while (pid > 0)
{
char path[50];
FILE *f;
char line[100];
char name[50] = "unknown";
int ppid = -1;
sprintf(path, "/proc/%d/comm", pid);
f = fopen(path, "r");
if (f == NULL)
{
strcpy(name, "unknown");
}
else
{
if (fgets(name, sizeof(name), f) == NULL)
{
strcpy(name, "unknown");
}
else
{
name[strcspn(name, "\n")] = '\0';
}
fclose(f);
}
if (first)
{
printf("%s(%d)", name, pid);
first = 0;
}
else
{
printf(" <-%s(%d)", name, pid);
}
if (pid == 1)
{
break;
}
sprintf(path, "/proc/%d/status", pid);
f = fopen(path, "r");
if (!f)
{
break;
}
while (fgets(line, sizeof(line), f))
{
if (strncmp(line, "PPid:", 5) == 0)
{
ppid = atoi(line + 5);
break;
}
}
fclose(f);
if (ppid <= 0)
{
break;
}
pid = ppid;
}
printf("\n");
return 0;
}
