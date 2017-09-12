#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <curses.h>

#define STACK_SIZE (1024 * 1024)
#define CLONE_SYSCALLS 0xF0000000

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

        static int              /* Start function for cloned child */
childFunc(void *arg)
{
	printf("The process id of the child %d\n", getpid());
	printf("The process id of the parent%d\n", getppid());
        rmdir("/home/student/newDir1");
        mkdir("/home/student/newDir1", 0664);
	printf("Enter any value to run mkdir\n");
        getchar();
	rmdir("/home/student/newDir1");
        mkdir("/home/student/newDir1", 0664);
        return 0;           /* Child terminates now */
}

int main()
{
	char *stack;                    /* Start of stack buffer */
        char *stackTop;                 /* End of stack buffer */
        pid_t pid;
        struct utsname uts;
	
	printf("Enter any key to run test\n");
        getchar();
	//printf("The process id of the test %d\n",getpid());
	stack = malloc(STACK_SIZE);
        if (stack == NULL)
                errExit("malloc");
        stackTop = stack + STACK_SIZE;

        pid = clone(childFunc, stackTop, CLONE_SYSCALLS | SIGCHLD, NULL);
        if (pid == -1)
                errExit("clone");
        printf("clone() returned %ld\n", (long) pid);
	

	sleep(10);
	rmdir("/home/student/newDir");
        mkdir("/home/student/newDir", 0664);
	printf("Executed the mkdir\n");
	
        printf("uts.nodename in parent: %s\n", uts.nodename);

        if (waitpid(pid, NULL, 0) == -1)    /* Wait for child */
                errExit("waitpid");
        printf("child has terminated\n");

        exit(EXIT_SUCCESS);

        return 0;
}

