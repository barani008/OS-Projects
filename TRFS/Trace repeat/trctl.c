#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "treplay.h"

#define TRFS_IOC_GETRSVSZ               _IOR('f', 1, long)
#define TRFS_IOC_SETRSVSZ               _IOW('f', 2, long)

//The getBitmap method retrieves the currently set bitmap from the super block
unsigned long getBitmap(char * const file){
	unsigned long result =0;
	int fd = open(file, O_RDONLY);
	if(fd == -1){
                        perror("Error while opening file\n");
                        return -1;
        }
	if(file==NULL){
		printf("empty/null params passed \n");
	}
	if(strcmp(file, "all")==0 || strcmp(file, "none")==0 || strncmp(file, "0x", 2)==0){
		printf("Enter the file path to get the bitmap value \n");
		close(fd);
		return -1;
	}else{
		//allocate the memory to save the value returned from ioctl call
		unsigned long* param = (unsigned long*) malloc(sizeof(unsigned long));
		result = ioctl(fd, TRFS_IOC_GETRSVSZ , param);
		if (result == -1)
			printf("Bitmap get failed: %s\n",strerror(errno));
		else{
			result = *param;
		}
		free(param);
	}
	close(fd);
	return result;
}

#ifdef EXTRA_CREDIT
unsigned int updateBitmap(char * op, unsigned long map){
        int op1 = 0;
	unsigned int bitmap = map;
        char * fun = op+1;
        if(op[0] == '+')
                op1 =1;
        if(strcasecmp(fun, "open")==0){
                if(op1 == 1)
                        bitmap |= 1<<OPEN;
                else
                        bitmap &= ~(1 << OPEN);
        }else if(strcasecmp(fun, "read")==0){
                if(op1 == 1)
                        bitmap |= 1<<READ;
                else
                        bitmap &= ~(1 << READ);
        }else if(strcasecmp(fun, "write")==0){
                if(op1 == 1)
                        bitmap |= 1<<WRITE;
                else
                        bitmap &= ~(1 << WRITE);
        }else if(strcasecmp(fun, "mkdir")==0){
                if(op1 == 1)
                        bitmap |= 1<<MKDIR;
                else
                        bitmap &= ~(1 << MKDIR);
        }else if(strcasecmp(fun, "RMDIR")==0){
                if(op1 == 1)
                        bitmap |= 1<<RMDIR;
                else
                        bitmap &= ~(1 << RMDIR);
        }else if(strcasecmp(fun, "UNLINK")==0){
                if(op1 == 1)
                        bitmap |= 1<<UNLINK;
                else
                        bitmap &= ~(1 << UNLINK);
        }else if(strcasecmp(fun, "create")==0){
                if(op1 == 1)
                        bitmap |= 1<<CREATE;
                else
                        bitmap &= ~(1 << CREATE);
        }else if(strcasecmp(fun, "link")==0){
                if(op1 == 1)
                        bitmap |= 1<<LINK;
                else
                        bitmap &= ~(1 << LINK);
        }else if(strcasecmp(fun, "symlink")==0){
                if(op1 == 1)
                        bitmap |= 1<<SYMLINK;
                else
                        bitmap &= ~(1 << SYMLINK);
        }else if(strcasecmp(fun, "CLOSE")==0){
                if(op1 == 1)
                        bitmap |= 1<<CLOSE;
                else
                        bitmap &= ~(1 << CLOSE);
        }
        return bitmap;
}
#endif

int main(int argc, char * const argv[])
{
	unsigned long hex=0, result =0;
	char * endptr;
	int fd = -1;
	if(argc <2){
		printf("Invalid number of arguments %d\n", argc);
		return -1;
	}
	if(argc == 2){
		printf("the currently set bitmap is %lx\n", getBitmap(argv[1]));	
	}else if(argc == 3){
		if(argv[1]==NULL || argv[2]==NULL){
			printf("empty/null params passed \n");
		}
		fd = open(argv[2], O_RDONLY);
                if(fd == -1){
                        perror("Error while opening file\n");
                        return -1;
                } 
		if(strcmp(argv[1], "all")==0){
			hex = 0xFFFFFFFF;	
		}else if(strcmp(argv[1], "none")==0){
			hex = 0;
		}else if(strncmp(argv[1], "0x", 2)==0){
			int len = strlen(argv[1])-2;
			endptr = (char *) malloc(len);
			hex = strtol(argv[1]+2,&endptr, 16);
		}
		#ifdef EXTRA_CREDIT
		else if(argv[1][0] == '+' || argv[1][0] == '-'){
			hex = getBitmap(argv[2]);
			if(hex == -1){
				printf("Invalid parameter passed \n");
				return -1;
			}
                        hex = updateBitmap(argv[1], hex);
                }
		#endif
		else{
			printf("Invalid parameter passed %s\n", argv[1]);
			return -1;
		}
		result = ioctl(fd, TRFS_IOC_SETRSVSZ , &hex);
                if (result == -1)
                        printf("Bitmap set failed: %s\n",strerror(errno));
	}
	#ifdef EXTRA_CREDIT
	else{
		int count = 1;
		fd = open(argv[argc-1], O_RDONLY);
		if(fd == -1){
			perror("Error while opening file\n");
			return -1;
		}
		hex = getBitmap(argv[argc-1]);
		while(count < argc-1){
			if(argv[count][0] == '+' || argv[count][0] == '-'){
				hex = updateBitmap(argv[count], hex);
			}else{
				printf("Invalid parameter passed %s\n", argv[count]);
				return -1;
			}
			count++;	
		}
                result = ioctl(fd, TRFS_IOC_SETRSVSZ , &hex);
                if (result == -1)
               		printf("Bitmap set failed: %s\n",strerror(errno));
      	}
	#else
	else{
		 printf("Invalid number of parameters passed %d\n", argc);
                 return -1;
	}
	#endif
	if(fd!=-1)
		close(fd);
	return 0;
}
