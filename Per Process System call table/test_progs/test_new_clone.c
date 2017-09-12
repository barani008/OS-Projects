#define _GNU_SOURCE

#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <asm-generic/errno.h>
#include <sched.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifndef __NR_cloneSVT
#error cloneSVT system call not defined
#endif


#define errExit(msg)	 do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

int main(int argc, const char *argv[])
{

	int childId = 0;
	unsigned long vid;
	printf("Enter System Vector ID :");
	scanf("%lu", &vid);

	childId = syscall(__NR_cloneSVT, CLONE_FILES | SIGCHLD, 0, NULL, NULL, 0, vid);

	if ( childId == 0 )
	{
		printf ( "Child : Hello I am the child process\n");
		printf ( "Child : Child PID: %d\n", getpid());
		printf ( "Child : Parent PID: %d\n", getppid());
		mkdir("/usr/src/child", 0644);
		rmdir("/usr/src/child");
		printf("Bye from Child!\n");

	}
	else
	{
		sleep(2);
		printf ( "Parent : Hello I am the parent process\n" ) ;
		printf ( "Parent : Parent PID: %d\n", getpid());
		printf ( "Parent : Child PID: %d\n", childId);
		mkdir("/usr/src/parent", 0644);
		rmdir("/usr/src/parent");
		waitpid(childId, NULL, 0);
		printf("Bye from parent!\n");
	} 

	return 0;
}

