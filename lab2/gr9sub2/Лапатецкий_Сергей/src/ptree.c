#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int pid = -1;
	if(argc != 2)
	{
		printf("Enter pid:");
		int r = scanf("%d", &pid);
		if(pid <= 0 || r == 0 || r == EOF)
		{
			printf("Error: invalid input\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		pid = atoi(argv[1]);
		if(pid <= 0)
		{
			printf("Error: invalid input\n");
			exit(EXIT_FAILURE);
		}
	}

	char path[255];
	char* field = NULL;
	char* name = NULL;

	do
	{
		sprintf(path, "/proc/%d/status", pid);

		//printf("%s\n", path);

		FILE* in = fopen(path, "r");

		if(!in)
		{
			printf("Error: cannot open status\n");
			exit(EXIT_FAILURE);
		}
		size_t n = 0;

		int v = -1;

		int rpid = 0;
		int rppid = 0;

		while(getline(&field, &n, in) != -1 && v == -1)
		{
			strtok(field, ":");
			while(field != NULL)
			{
				//printf("%s|",field);
				if(strcmp(field, "Name")==0)
				{
					field = strtok(NULL, ":");
					char *temp = (char*)realloc(name, (strlen(field) + 1) * sizeof(char));
					if (temp == NULL)
					{
						if(name != NULL)
							free(name);
						if(field != NULL)
							free(field);
						fclose(in);
						fprintf(stderr, "Error: memory allocation failed\n");
						exit(EXIT_FAILURE);
					}
					else
					{
						name = temp;
					}
					strcpy(name, field);
					//printf("(%s)\n", name);
				}
				else if(strcmp(field, "Pid")==0)
				{
					field = strtok(NULL, ":");
					rpid = atoi(field);
					//printf("(%d)\n", rpid);
				}
				else if(strcmp(field, "PPid")==0)
				{
					field = strtok(NULL, ":");
					rppid = atoi(field);
					//printf("(%d)\n", rppid);
					v = 0;
				}
				field = strtok(NULL, ":");
			}
		}
		int namen = strspn(name, " \t");
		name[strlen(name)-1] = '\0';
		if(rppid >= 1)
		{
			printf("%s(%d) <- ", name+namen, rpid);
		}
		else
		{
			printf("%s(%d)\n", name+namen, rpid);
		}

		pid = rppid;

		fclose(in);
	}while(pid != 0);

	if(name != NULL)
		free(name);

	if(field != NULL)
		free(field);

	exit(EXIT_SUCCESS);

}
