#include <stdio.h>
#include <stdlib.h>  /* For exit() function */
#include <fcntl.h>
#include <sys/stat.h>

int main()
{
	printf("Enter any key to start test\n");
        //getchar();
        sleep(40);
	int fd = creat("/tmp/mntTest1/test/new.c", O_RDWR);
	char buf[30];
	fd = open("/tmp/mntTest1/test/new.c", O_RDWR, 4096);
	if(fd == -1)
	{
		printf("Error!");
		exit(1);
	}
	printf("write:%d\n",write(fd, "take this and work please" , 30));
	printf("close:%d\n",close(fd));
	fd = open("/tmp/mntTest1/test/new.c", O_RDWR, 4096);
	printf("read:%d\n",read(fd,buf, 30));  
	printf("%s\n",buf);
	printf("close:%d\n",close(fd));
	printf("unlink:%d\n",unlink("/tmp/mntTest1/test/new.c"));
	printf("mkdir:%d\n",mkdir("/tmp/mntTest1/test/newDir", 0664));
	fd = creat("/tmp/mntTest1/test/newDir/223.c", O_RDWR);
        printf("write:%d\n",write(fd, " please please", 30));
        printf("close:%d\n",close(fd));
	fd = open("/tmp/mntTest1/test/newDir/223.c", O_RDWR);
	printf("read:%d\n",read(fd,buf, 30));
        printf("%s\n",buf);
        printf("close:%d\n",close(fd));
	printf("symlink:%d\n",symlink("/tmp/mntTest1/test/newDir/223.c", "/tmp/mntTest1/link.c"));
	printf("link:%d\n",link("/tmp/mntTest1/223.c", "/tmp/mntTest1/link1.c"));
	printf("unlink:%d\n",unlink("/tmp/mntTest1/link1.c"));
	printf("unlink:%d\n",unlink("/tmp/mntTest1/223.c"));
	printf("rmdir:%d\n",rmdir("/tmp/mntTest1/test/newDir"));
	printf("unlink:%d\n",unlink("/tmp/mntTest1/link1.c"));
	printf("getpid:%d\n", getpid());
	printf("getppid:%d\n", getppid());
	return 0;
}
