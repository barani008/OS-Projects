#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "commonStructure.h"

#ifndef __NR_xmergesort
#error xmergesort system call not defined
#endif

int main(int argc, char * const argv[])
{
	int rc=0;
	struct param* p = (struct param*) malloc(sizeof(struct param));
	//if(argc<5){
	//	printf("Invalid Number of Arguments\n");
	//}
	p->data = (unsigned int *) malloc(sizeof(unsigned int));
	int flag =0;
	p->flags = 0;
	while (( flag = getopt(argc, argv,"uaidt")) != -1) {
        switch (flag) {
		case 'u' : p->flags|=1;
                	break;
             	case 'a' : p->flags|=2;
                 	break;
             	case 'i' : p->flags|=4; 
                	break;
             	case 'd' : p->flags|=16;
                 	break;
		case 't' : p->flags|=32;
                        break;
             	default:  
                 	printf("Invalid Flags set");
			return -7;
        	}
    	}
	if(argv[optind] == NULL || argv[optind+1]== NULL || argv[optind+2] == NULL || argv[optind+3] !=NULL){
		printf("Invalid Number of Arguments\n");
		return -1;
	}
	//p->flags = 6000;		
	p->outfile = argv[optind++];
	p->infile1 = argv[optind++];
	p->infile2 = argv[optind];
	void *dummy = (void *) p;
  	rc = syscall(__NR_xmergesort, dummy);
	if (rc == 0)
		printf("Lines Written %d\n", *(p->data));
	else{
		perror("Error!!!");
		/*printf("syscall returned %d (errno=%d)\n", rc, errno);
		if(errno == 1)
			printf("EPERM error, Operation not Permitted!!!");
		else if(errno == 2)
                        printf("ENOENT error, No such file or directory!!!\n");
		else if(errno == 3)
			printf("some of the parameters passed in the struct argument are null!!!\n");
		else if(errno == 4)
			printf("Bad address is passed to some the parameter pointers in the struct!!!\n");
		else if(errno == 5)
                        printf("Invalid flags, flags u and a are set at the same time!!!\n");
		else if(errno == 6)
                        printf("invalid flags, flags u and a are both unset at the same time!!!\n");
		else if(errno == 7)
                        printf("invalid flag parameter passed!!!\n");
		else if(errno == 8)
                        printf("unable to retrieve Dentry/inode info for the given files!!!\n");
		else if(errno == 9)
                        printf("EBADF Bad file Number!!!\n");
		else if(errno == 10)
                        printf("ECHILD, No child process!!!\n");
		else if(errno == 11)
                        printf("Unable to find the input file(s), file(s) missing!!!\n");
		else if(errno == 12)
                        printf("ENOMEM, Error no physical memory availble for process!!!\n");
		else if(errno == 13)
                        printf("EACCESS, Access denied to the file!!!\n");
		else if(errno == 14)
                        printf("EFAULT, Memory error!!!\n ");
		else if(errno == 15)
			printf("Null Argument has been passed to the System Call!!!\n");
		else if(errno == 16)
                        printf("File Error, Unable to move file pointer position!!!\n");
		else if(errno == 17)
                        printf("The output file is not a regular one!!!\n");
		else if(errno == 18)
                        printf("The given input and output files are the same!!!\n");
		else if(errno == 19)
                        printf("Bad address passed, unable to copy user input from userland!!!\n");
		else if(errno == 20)
                        printf("Given input files are not regular!!!\n");
		else if(errno == 21)
                        printf("The given input files are the same!!!\n");
		else if(errno == 22)
			printf("EINVAL, Error invalid Argument!!!\n");
		else if(errno == 78)
                        printf("ENAMETOOLONG, file name too long!!!\n");
		else if(errno == 116)
			printf("ESTALE, Stale file handle!!!\n");*/
	}
	exit(rc);
}
