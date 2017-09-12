#define _GNU_SOURCE  
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

static inline int getcpu() {
    #ifdef SYS_getcpu
    int cpu, status;
    status = syscall(SYS_getcpu, &cpu, NULL, NULL);
    return status;
    #else
    return -1; // unavailable
    #endif
}

int main(){
	printf("Enter any key to start test\n");
        getchar();
	printf("%d \n", getcpu());
	return 0;
}
