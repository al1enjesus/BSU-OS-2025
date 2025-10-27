#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    printf("PID: %d\n", getpid());
    
    
    char stack_var[1024];
    memset(stack_var, 'S', sizeof(stack_var));

    char *heap_var = malloc(1024 * 1024); // 1 MB
    memset(heap_var, 'H', 1024 * 1024);
   
    char *mmap_var = mmap(NULL, 1024 * 1024, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(mmap_var, 'M', 1024 * 1024);
    
    printf("Память выделена. Нажми Enter для продолжения...\n");
    getchar();
    
    free(heap_var);
    munmap(mmap_var, 1024 * 1024);
    return 0;
}
