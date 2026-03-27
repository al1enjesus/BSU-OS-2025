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
char path[64];
FILE *f;
char line[100];
char name[50] = "unknown";
long ppid = -1;
char *endptr;
if ((int)snprintf(path, sizeof(path), "/proc/%d/comm", pid) >= (int)sizeof(path))
{
strcpy(name, "unknown");
}
else
{
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
char *newline = strchr(name, '\n');
if (newline)
{
*newline = '\0';
}
}
fclose(f);
}
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
if ((int)snprintf(path, sizeof(path), "/proc/%d/status", pid) >= (int)sizeof(path))
{
break;
}
f = fopen(path, "r");
if (!f)
{
break;
}
int found = 0;
while (fgets(line, sizeof(line), f))
{
if (strncmp(line, "PPid:", 5) == 0)
{
ppid = strtol(line + 5, &endptr, 10);
if (endptr == line + 5 || *endptr != '\n')
{
ppid = -1;
}
found = 1;
break;
}
}
fclose(f);
if (!found || ppid <= 0)
{
break;
}
pid = (int)ppid;
}
printf("\n");
return 0;
}
