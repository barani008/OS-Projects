#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "treplay.h"

int main(int argc, char* const argv[]){

	int i = 0;
	int flagEntry = 0;
	int index = 0;
	int fdOpen = 0;
	int bytesreadGen = 0;
	int bytesreadOpen = 0;
	int bytesWriteTemp = 0;
	int bytesreadTemp = 0;
	int retValTemp = 0;
	int fdRead = 0;
	int fdTemp = -1;
	int inpFlag = 0;
	int counter = 0;
	char * fileName = NULL;
	char buffGenRead[100];
	char buffOpenRead[100];
	char buffTemp[100];
	void * buffWrite = NULL;
	char * s1 = NULL;
	char * s2 = NULL;
	int fd_list[100];
	unsigned long p_id_list[100];
	unsigned long f_id_list[100];
	struct stat statbuf;
	rheader_trace rh_trace; //header 
	ostruct_trace o_trace; //open
	rstruct_trace r_trace; //read
	wstruct_trace w_trace; //write
	cstruct_trace c_trace; //close
	mkstruct_trace mk_trace; //mkdir
	rmstruct_trace rm_trace; //rmdir
	ustruct_trace u_trace; //unlink
	crstruct_trace cr_trace; //create
	lstruct_trace l_trace; //link
	sstruct_trace s_trace; //symlink

	while ((flagEntry = getopt(argc, argv, "ns")) != -1)
		switch (flagEntry) {
			case 'n':
				inpFlag = 1;
				break;
			case 's':
				inpFlag = 2;
				break;
			default:
				printf("Wrong flags given\n");
				goto cleanExit;
				break;
		}
	for (index = optind; index < argc; index++) {
		fileName = argv[optind];
	}
	fdRead = open(fileName, O_RDONLY);
	bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
	if (bytesreadGen < 0) {
		printf("Invalid File!\n");
		goto cleanExit;
	}
	while (bytesreadGen != 0) {
		//Read the general header record
		memcpy((void * ) & rh_trace, buffGenRead, bytesreadGen);
		//Switch using the type of record received
		switch (rh_trace.rec_type) {
			case OPEN: 
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadTemp = read(fdRead, buffOpenRead, sizeof(o_trace));
				memcpy((void * ) & o_trace, buffOpenRead, bytesreadTemp);
				lseek(fdRead, -sizeof(o_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(o_trace) + o_trace.path_length);
				memcpy((void * ) & o_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: Open \nFILE PATH:%s\n", o_trace.path_name);
					printf("PERMISSION MODE:%d\n", o_trace.perm_mode);
					printf("FLAGS:%d\n",  o_trace.open_flags);
				} else {
					p_id_list[counter] = o_trace.p_id;
					f_id_list[counter] = o_trace.f_id;
					stat(o_trace.path_name, & statbuf);
					if (S_ISDIR(statbuf.st_mode) != 1) {
						fdOpen = open(o_trace.path_name, o_trace.open_flags, o_trace.perm_mode);
						fd_list[counter] = fdOpen;
					}
					//Check the operation performed
					if (fdOpen < 0) {
						if (o_trace.ret_val < 0) {
							printf("\nOPERATION: Open \nFILE PATH:%s\n", o_trace.path_name);
							printf("PERMISSION MODE:%d\n", o_trace.perm_mode);
							printf("FLAGS:%d\n",  o_trace.open_flags);
							printf("Result: No Deviation\n");
						} else {
							printf("\nOPERATION: Open \nFILE PATH:%s\n", o_trace.path_name);
							printf("PERMISSION MODE:%d\n", o_trace.perm_mode);
							printf("FLAGS:%d\n",  o_trace.open_flags);
							printf("Result: Deviation occured\n");
							if (inpFlag == 2)
								goto cleanExit;
							break;
						}
					} else {
						if (o_trace.ret_val >= 0) {
							printf("\nOPERATION: Open \nFILE PATH:%s\n", o_trace.path_name);
							printf("PERMISSION MODE:%d\n", o_trace.perm_mode);
							printf("FLAGS:%d\n",  o_trace.open_flags);
							printf("Result: No Deviation\n");
						} else {
							printf("\nOPERATION: Open \nFILE PATH:%s\n", o_trace.path_name);
							printf("PERMISSION MODE:%d\n", o_trace.perm_mode);
							printf("FLAGS:%d\n",  o_trace.open_flags);
							printf("Result: Deviation occured\n");
							if (inpFlag == 2)
								goto cleanExit;
							break;
						}
					}
					counter++;
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case READ:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(r_trace));
				memcpy((void * ) & r_trace, buffOpenRead, bytesreadOpen);
				lseek(fdRead, -sizeof(r_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(r_trace) + r_trace.buf_len);
				memcpy((void * ) & r_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: Read \n");
					printf("NUMBER OF BYTES:%lld\n",r_trace.buf_len);
					printf("BUFFER: %s \n", r_trace.buf );

				} else {
					for (i = 0; i < counter; i++) {
						if (p_id_list[i] == r_trace.p_id) {
							if (f_id_list[i] == r_trace.f_id) {
								fdTemp = fd_list[i];
							}
						}
					}
					bytesreadTemp = read(fdTemp, buffTemp, r_trace.buf_len);
					//Check the operation performed
					//Check  Return Value and Check number of bytes read
					if (bytesreadTemp == r_trace.ret_val && strcmp(buffTemp, r_trace.buf )==0) {
						printf("\nOPERATION: Read \n");
						printf("NUMBER OF BYTES:%lld\n",r_trace.buf_len);
						printf("BUFFER: %s \n", r_trace.buf );
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: Read \n");
						printf("NUMBER OF BYTES:%lld\n",r_trace.buf_len);
						printf("BUFFER: %s \n", r_trace.buf );
						printf("RESULT: Deviation occured\n");
						if (inpFlag == 2)
							goto cleanExit;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case WRITE:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(w_trace));
				memcpy((void * ) & w_trace, buffOpenRead, bytesreadOpen);
				lseek(fdRead, -sizeof(w_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(w_trace) + w_trace.buf_len);
				memcpy((void * ) & w_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: Write \n");
					printf("NUMBER OF BYTES:%lld\n",w_trace.buf_len);
					printf("BUFFER: %s \n", w_trace.buf );
				} else {
					for (i = 0; i < counter; i++) {
						if (p_id_list[i] == w_trace.p_id) {
							if (f_id_list[i] == w_trace.f_id) {
								fdTemp = fd_list[i];
							}
						}
					}
					buffWrite = & (w_trace.buf);
					//Perform operation 
					bytesWriteTemp = write(fdTemp, buffWrite, w_trace.buf_len);
					//Check the operation performed
					if (w_trace.ret_val == bytesWriteTemp) {
						printf("\nOPERATION: Write \n");
						printf("NUMBER OF BYTES:%lld\n",w_trace.buf_len);
						printf("BUFFER: %s \n", w_trace.buf );
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: Write \n");
						printf("NUMBER OF BYTES:%lld\n",w_trace.buf_len);
						printf("BUFFER: %s \n", w_trace.buf );
						printf("RESULT: Deviation occured\n");
						if (inpFlag == 2)
							goto cleanExit;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case CLOSE:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				//Perform close operation
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(c_trace));
				memcpy((void * ) & c_trace, buffOpenRead, bytesreadOpen);	
				if (inpFlag == 1) {
					printf("\nOPERATION: Close \n");
				} else {
					for (i = 0; i < counter; i++) {
						if (p_id_list[i] == c_trace.p_id) {
							if (f_id_list[i] == c_trace.f_id) {
								fdTemp = fd_list[i];
							}
						}
					}
					retValTemp = close(fdTemp);
					if (retValTemp == c_trace.ret_val) {
						printf("\nOPERATION: close \n");
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: close \n");
						printf("RESULT: Deviation occured\n");
						if (inpFlag == 2)
							goto cleanExit;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case MKDIR:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(mk_trace));
				memcpy((void * ) & mk_trace, buffOpenRead, bytesreadOpen);
				lseek(fdRead, -sizeof(mk_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(mk_trace) + mk_trace.path_length);
				memcpy((void * ) & mk_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: mkdir \n");
					printf("FILE PATH:%s\n",mk_trace.path_name);
					printf("PERMISSION MODE:%d\n",mk_trace.perm_mode);
				} else {
					//Perform mkdir
					retValTemp = mkdir(mk_trace.path_name, mk_trace.perm_mode);
					//Check if valid
					if (retValTemp == mk_trace.ret_val) {
						printf("\nOPERATION: mkdir \n");
						printf("FILE PATH:%s\n",mk_trace.path_name);
						printf("PERMISSION MODE:%d\n",mk_trace.perm_mode);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: mkdir \n");
						printf("FILE PATH:%s\n",mk_trace.path_name);
						printf("PERMISSION MODE:%d\n",mk_trace.perm_mode);
						printf("RESULT: Deviation Occured\n");
						if (inpFlag == 2)
							goto cleanExit;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case RMDIR:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(rm_trace));
				memcpy((void * ) & rm_trace, buffOpenRead, bytesreadOpen);
				lseek(fdRead, -sizeof(rm_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(rm_trace) + rm_trace.path_length);
				memcpy((void * ) & rm_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: rmdir \n");
					printf("FILE PATH:%s\n", rm_trace.path_name);
				} else {
					retValTemp = rmdir(rm_trace.path_name);
					if (retValTemp == rm_trace.ret_val) {
						printf("\nOPERATION: rmdir \n");
						printf("FILE PATH:%s\n", rm_trace.path_name);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: rmdir \n");
						printf("FILE PATH:%s\n", rm_trace.path_name);
						printf("RESULT: Deviation Occured\n");
						if (inpFlag == 2)
							goto cleanExit;
						break;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case UNLINK:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadTemp = read(fdRead, buffOpenRead, sizeof(u_trace));
				memcpy((void * ) & u_trace, buffOpenRead, bytesreadTemp);
				lseek(fdRead, -sizeof(u_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(u_trace) + u_trace.path_length);
				memcpy((void * ) & u_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: unlink \n");
					printf("FILE PATH:%s\n", u_trace.path_name);
				} else {
					//perform operation
					retValTemp = unlink(u_trace.path_name);
					//check if operation returns the same value
					if (retValTemp == u_trace.ret_val) {
						printf("\nOPERATION: unlink \n");
						printf("FILE PATH:%s\n", u_trace.path_name);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: unlink \n");
						printf("FILE PATH:%s\n", u_trace.path_name);
						printf("RESULT: Deviation Occured\n");
						if (inpFlag == 2)
							goto cleanExit;
						break;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case CREATE:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadTemp = read(fdRead, buffOpenRead, sizeof(cr_trace));
				memcpy((void * ) & cr_trace, buffOpenRead, bytesreadTemp);
				lseek(fdRead, -sizeof(cr_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(cr_trace) + cr_trace.path_length);
				memcpy((void * ) & cr_trace, buffOpenRead, bytesreadOpen);
				if (inpFlag == 1) {
					printf("\nOPERATION: create \n");
					printf("FILE PATH:%s\n", cr_trace.path_name);
					printf("PERMISSION MODE:%d\n", cr_trace.perm_mode);

				} else {
					//perform operation
					retValTemp = creat(cr_trace.path_name, cr_trace.perm_mode);
					//check if the operation has been performed successfully
					if ((retValTemp<0 && cr_trace.ret_val<0 && retValTemp== cr_trace.ret_val) ||
							(retValTemp > 0 && cr_trace.ret_val == 0)) {
						printf("\nOPERATION: create \n");
						printf("FILE PATH:%s\n", cr_trace.path_name);
						printf("PERMISSION MODE:%d\n", cr_trace.perm_mode);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: create \n");
						printf("FILE PATH:%s\n", cr_trace.path_name);
						printf("PERMISSION MODE:%d\n", cr_trace.perm_mode);
						printf("RESULT: Deviation Occured\n");
						if (inpFlag == 2)
							goto cleanExit;
						break;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case SYMLINK:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadTemp = read(fdRead, buffOpenRead, sizeof(s_trace));
				memcpy((void * ) & s_trace, buffOpenRead, bytesreadTemp);
				lseek(fdRead, -sizeof(s_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(s_trace) + s_trace.path_length1);
				memcpy((void * ) & s_trace, buffOpenRead, bytesreadOpen);
				s1 = s_trace.path_name;
				lseek(fdRead, -(sizeof(s_trace)+s_trace.path_length1), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(s_trace) 
							+ s_trace.path_length1 + s_trace.path_length2);
				memcpy((void * ) & s_trace, buffOpenRead, bytesreadOpen);
				s2 = (char *) (s_trace.path_name + s_trace.path_length1);
				if (inpFlag == 1) {
					printf("\nOPERATION: symlink \n");
					printf("FILE PATH 1:%s\n", s1);
					printf("FILE PATH 2:%s\n", s2);

				} else {
					//perform operation
					retValTemp = symlink(s1, s2);
					//check if the opration has been performed successfully
					if (retValTemp == s_trace.ret_val) {
						printf("\nOPERATION: symlink \n");
						printf("FILE PATH 1:%s\n", s1);
						printf("FILE PATH 2:%s\n", s2);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: symlink \n");
						printf("FILE PATH 1:%s\n", s1);
						printf("FILE PATH 2:%s\n", s2);
						printf("RESULT: Deviation Occured\n");
						if (inpFlag == 2)
							goto cleanExit;
						break;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			case LINK:
				printf("************************************************************************\n");
				printf("RECORD ID: %u\n", rh_trace.rec_id);
				bytesreadTemp = read(fdRead, buffOpenRead, sizeof(l_trace));
				memcpy((void * ) & l_trace, buffOpenRead, bytesreadTemp);
				lseek(fdRead, -sizeof(l_trace), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(l_trace) + l_trace.path_length1);
				memcpy((void * ) & l_trace, buffOpenRead, bytesreadOpen);
				s1 = l_trace.path_name;
				lseek(fdRead, -(sizeof(l_trace)+l_trace.path_length1), SEEK_CUR);
				bytesreadOpen = read(fdRead, buffOpenRead, sizeof(l_trace) 
						+ l_trace.path_length1 + l_trace.path_length2);
				memcpy((void * ) & l_trace, buffOpenRead, bytesreadOpen);
				s2 = (char *) (l_trace.path_name + l_trace.path_length1);
				if (inpFlag == 1) {
					printf("\nOPERATION: link \n");
					printf("FILE PATH 1:%s\n", s1);
					printf("FILE PATH 2:%s\n", s2);
				} else {
					//perform operation
					retValTemp = link(s1, s2);
					//check if the opration has been performed successfully
					if (retValTemp == l_trace.ret_val) {
						printf("\nOPERATION: link \n");
						printf("FILE PATH 1:%s\n", s1);
						printf("FILE PATH 2:%s\n", s2);
						printf("RESULT: No Deviation\n");
					} else {
						printf("\nOPERATION: link \n");
						printf("FILE PATH 1:%s\n", s1);
						printf("FILE PATH 2:%s\n", s2);
						printf("RESULT: Deviation occured\n");
						if (inpFlag == 2)
							goto cleanExit;
						break;
					}
				}
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
			default:
				printf("Unknown Record Type Error %d\n", rh_trace.rec_type);
				bytesreadGen = read(fdRead, buffGenRead, sizeof(rh_trace));
				break;
		}
	}
	printf("************************************************************************\n");
cleanExit: exit(0);
	   return 0;
}
