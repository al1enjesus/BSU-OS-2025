#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

void print_memory_metrics(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    char line[256];
    unsigned long vm_size = 0, vm_rss = 0, vm_data = 0, vm_stk = 0;

    while (fgets(line, sizeof(line), f)) {
		if(strncmp(line, "VmSize:", 7) == 0){
			sscanf(line, "VmSize: %lu kB", &vm_size);
		}else if(strncmp(line, "VmRSS:", 6) == 0){
			sscanf(line, "VmRSS: %lu kB", &vm_rss);
		}else if(strncmp(line, "VmData:", 7) == 0){
			sscanf(line, "VmData: %lu kB", &vm_data);
		}else if(strncmp(line, "VmStk:", 6) == 0){
			sscanf(line, "VmStk: %lu kB", &vm_stk);
		}
    }
    fclose(f);
    printf("Memory Metrics for PID %d:\n", pid);
    printf("  VSZ (Virtual):  %lu KB (%.1f MB)\n", vm_size, vm_size/1024.0);
    printf("  RSS (Resident): %lu KB (%.1f MB)\n", vm_rss, vm_rss/1024.0);
    printf("  Data/Heap:      %lu KB (%.1f MB)\n", vm_data, vm_data/1024.0);
    printf("  Stack:          %lu KB (%.1f MB)\n", vm_stk, vm_stk/1024.0);
}

void print_memory_map(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen failed");
        return;
    }

    printf("\nMemory Map:\n");
    
	printf("---------------------------------------------------------------------------------------------------------------\n");
	printf("| %-26s| %-5s| %-15s| %-55s |\n", "Address Range", "Perms", "Size", "Path");
    printf("---------------------------------------------------------------------------------------------------------------\n");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long start, end;
        char perms[5], path_str[256] = "";

        int field = sscanf(line, "%lx-%lx %s %*s %*s %*s %[^\n]", &start, &end, perms, path_str);
	
		if(field >= 3)
		{
			unsigned long size_bytes = end - start;
        	unsigned long size_kb = (end - start) / 1024;
			double size_mb = size_kb/ 1024.0;

			char size[256];
			if(size_mb >= 1.0)
			{
				snprintf(size, sizeof(size), "%.1f MB", size_mb);
			}else {
				snprintf(size, sizeof(size), "%lu KB", size_kb);
			}

			if(field < 4 || path_str[0] == '\0')
			{
				if(strstr(line, "[heap]")){
					strcpy(path_str, "[heap]");
				}else if(strstr(line, "[stack]")){
					strcpy(path_str, "[stack]");
				}else if(strstr(line,"[vdso]")){
					strcpy(path_str, "[vdso]");
				}else if(strstr(line, "[vvar]")){
					strcpy(path_str, "[vvar]");
				}else if(perms[2] == 'x'){
					strcpy(path_str, "[code/anonymous]");
				}else
					strcpy(path_str, "[anonymous]");
			}
			
			printf("| %-12lx-%-12lx | %-4s | %-14s | %-55s | \n", start, end, perms,size, path_str);

		}			
			
    }
    printf("---------------------------------------------------------------------------------------------------------------\n");

    fclose(f);
}

void demonstrate_memory_types() {
    printf("\n=== Demonstrating Different Memory Types ===\n\n");

    // 1. Стек (stack)
    char stack_var[1024];  // Локальная переменная
    memset(stack_var, 'S', sizeof(stack_var));
    printf("1. Stack variable allocated: 1 KB at %p\n", (void*)stack_var);

    // 2. Heap (через malloc)
    size_t heap_size = 10 * 1024 * 1024;  // 10 MB
    char *heap_var = malloc(heap_size);
    if (!heap_var) {
        perror("malloc failed");
        return;
    }

    memset(heap_var, 'H', heap_size);

    printf("2. Heap allocated: 10 MB at %p\n", (void*)heap_var);

    size_t mmap_size = 50 * 1024 * 1024;  // 50 MB
    void *mmap_var = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap failed");
        free(heap_var);
        return;
    }

    printf("3. Anonymous mmap: 50 MB at %p\n", mmap_var);

	const char *filename = "/tmp/memory_info_demo.dat";
	int fd = open(filename, O_RDWR | O_CREAT, 0664);
	if(fd >= 0)
	{
		ftruncate(fd, 2 * 1024 * 1024);
		
		size_t file_mmap_size = 2 * 1024 * 1024;
		void *file_mmap_var = mmap(NULL, file_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if(file_mmap_var != MAP_FAILED)
		{
			memset(file_mmap_var, 'F', file_mmap_size);
			printf("4. File-backed mmap: 2 MB at %p (file: %s)\n", file_mmap_var, filename);
			munmap(file_mmap_var, file_mmap_size);
		} else {
			perror("file mmap failed");
		}
		close(fd);
		unlink(filename);
	} else {
		perror("failed to create temp file");
	}



    printf("\nMemory allocated. Check /proc/%d/maps to see different regions.\n", getpid());
    printf("Press Enter to see memory info and map...\n");
    getchar();

    print_memory_metrics(getpid());
    print_memory_map(getpid());

    printf("\nPress Enter to free memory and exit...\n");
    getchar();

    free(heap_var);
    munmap(mmap_var, mmap_size);
}


int main(int argc, char *argv[]) {
    if (argc == 2) {
        pid_t pid = atoi(argv[1]);
        printf("Analyzing process %d\n\n", pid);
        print_memory_metrics(pid);
        print_memory_map(pid);
    } else {
        printf("Memory Info Demo\n");
        printf("================\n\n");
        printf("No PID specified. Running demonstration mode.\n");
        printf("This will allocate different types of memory and show the results.\n\n");

        demonstrate_memory_types();
    }

    return 0;
}

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте TODO в функциях print_memory_metrics() и print_memory_map()
 *
 * 2. Добейтесь корректного вывода метрик и карты памяти
 *
 * 3. Дополните demonstrate_memory_types():
 *    - Заполните heap_var данными (чтобы страницы реально выделились)
 *    - Создайте file-backed mmap (откройте файл и отобразите его)
 *
 * 4. Запустите программу и сравните вывод с системными утилитами:
 *    $ ./memory_info &
 *    $ PID=$!
 *    $ ps -o pid,vsz,rss -p $PID
 *    $ cat /proc/$PID/status | grep ^Vm
 *
 * 5. Проанализируйте:
 *    - Почему VSZ больше RSS?
 *    - Где находятся stack, heap, mmap в адресном пространстве?
 *    - Что означают разные права доступа (r-xp, rw-p)?
 *
 * 6. Дополнительно (*):
 *    - Добавьте вывод PSS из /proc/[PID]/smaps_rollup
 *    - Добавьте цветной вывод для разных типов памяти
 *    - Реализуйте группировку сегментов (все библиотеки вместе)
 */
