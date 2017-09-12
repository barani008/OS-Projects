#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(){
	int ret = 0;
	printf("Enter any value to run mkdir\n");
	getchar();
	printf("rmdir : %d\n", rmdir("/tmp/newDir"));
        ret = mkdir("/tmp/newDir", 0664);
        printf("Executed the mkdir %d\n", ret);
	return 0;
}
