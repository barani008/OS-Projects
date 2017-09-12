#include <stdio.h>
#include <stdlib.h>


int main(){
	FILE *f = fopen("/proc/syscall_vectors", "r");

	char c;
	c = fgetc(f);
	while (c != EOF ){
		printf("%c", c);
		c = fgetc(f);
	}
  	return 0;
}	
