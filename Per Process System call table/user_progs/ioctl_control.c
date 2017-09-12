#include <stdio.h>
#include "../ioctl_support.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

int main(int argc,char** args){
	char device_file[] = "/dev/ioctl_syscall"; 
	int mode;
	int pid;
	int vid, ret;
	struct ioctl_params *ioctl_param = NULL;
	int fd;
	ioctl_param = malloc(sizeof(ioctl_param));
	printf("Enter IOCTL command type, REMOVE(0), SET_VECTOR(1) and GET_VECTOR(2)  \n");
	scanf("%d", &mode);
	printf("Enter the pid \n");
	scanf("%d", &pid);
	if(kill(pid,0) != 0){	
		ret = -EINVAL;
		goto out;
	}
	ioctl_param->pid = pid;
	if(mode == 1){
		printf("Enter vector id \n");
		scanf("%d",&vid);
		ioctl_param->vid = vid;
	}
	fd = open(device_file,O_CREAT,777);
	if(mode == 1){
		ret = ioctl(fd,IOCTL_SET_VECTOR,(unsigned long)ioctl_param);
		if(ret >= 0){
			printf(" Vector Id %d was set for Pid %d\n ", vid, pid);
		}else{
			printf("Error occured while setting vid, check input %d\n");
			ret = -EINVAL;
			goto out;
		}
	}
	else if(mode == 2){
		ret = ioctl(fd,IOCTL_GET_VECTOR,(unsigned long)ioctl_param);
		if(ret >= 0){
			printf("Vector Id of the PID %d is %d\n", pid, ret);
		}
	}
	else{
		ret = ioctl(fd,IOCTL_REMOVE,(unsigned long)ioctl_param);
		if(ret >= 0){
		printf("Vector Id was removed for the PID %d. Back to default mode\n", pid); 
		}
	}
out:
	printf(strerror(errno));
	printf("\n");
	return 0;
}
