#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/pid.h>
#include "commonStructure.h"

#define PG_SIZE 4096
#define check_access(addr, size) \
	likely(!__range_not_ok(addr, size, user_addr_max()))
asmlinkage extern long (*sysptr)(void *arg);

asmlinkage long xmergesort(void *arg)
{
	int result = 0;				//Returns the Result 
	struct param * p= NULL;			//Structure used in kernel space for file op
	struct filename* infile1= NULL;		// input file1, used to store return val of getName
	struct filename* infile2= NULL;		// input file2, used to store return val of getName
	struct filename* outfile= NULL;		// output file, used to store the buffer to be written
	char c1 = '\0';				//char c1, c2 used for character comparison
	char c2 = '\0';
	bool caseInsensitive = false;		//flag for case sensitive merging, true means case Insensitive
	bool allowDuplicates = false;		// flag for allowing duplicates, false allows only unique records
	bool checkInputOrder = false;		//check the sort order in input file, not Implemented
	bool returnLineCount = false;		//flag for know whether to return written records count 
	unsigned int flag = 0;			
	short int bytes_read1 = PG_SIZE;	//used to keep count of the bytes inside buffer1
	short int bytes_read2 = PG_SIZE;	//used to keep count of the bytes inside buffer2
	char* buf1 = NULL;			//buf1, buf2 and outBuf used to read/write data from files
	char* buf2 = NULL;
	char* outBuf= NULL;
	struct file *filp1= NULL;		//filp1, filp2, outFilp file pointers are used to access
	struct file *filp2= NULL;		// input and output files
	struct file *outFilp= NULL;
	mm_segment_t oldfs = get_fs();		// storing old fs value, will be used while cleaning up
	unsigned int lines_written = 0;		//used to store return value of vfs_write for error checking
	unsigned int totalLines = 0;		// keeps count of the number of lines written to the output
	struct kstat inStat1, inStat2, outStat;	// used to check whether file exists in the system
	int locPointer1 = 0;			// loc pointers used while reading from input buffers
	int locPointer2 = 0;			//keeps track of where the current character is read from buffer
	int lastLinePtr1 = 0;			//last line ptrs keep track of where the current record started
	int lastLinePtr2 = 0;			//within the buffer.
	int blocksWritten = 0;          	//count the number of outBuffer written to output file
	int outBufPtr = 0;                      // denote the location where the next character has to be placed in outBuf
	struct task_struct *task = NULL;	
	//NULL check and bad address check
	if (arg == NULL || check_access(arg, sizeof(struct param))==0){
                printk("sys_mergesort Passed a null or invalid pointer in arg\n");
                result =  -EINVAL;      // NULL argument
                goto out;
	}else{					
		// Allocating kernel memory for struct and copying data from userland.
		unsigned long size = 0;
		p = (struct param*)kmalloc(sizeof(struct param), __GFP_NOFAIL);
		if(p==NULL){
			printk("sys_mergesort unable to allocate Memory \n");
                	result = -ENOMEM;
                	goto out;
		}
		size= copy_from_user(p, arg, sizeof(struct param));
		if(size!= 0){
			printk("sys_mergesort unable to copy argument from Userspace to Kernel might be bad pointer\n");
			result = size;
			goto out;
		}  
	}
	//checking for pointers within the struct pointer		
	if(p->infile1 == NULL || (p->infile2 == NULL) || (p->outfile == NULL ) || (p->data== NULL)){
                printk("sys_mergesort Passed a null pointer argument in arg\n");
                result = -EINVAL;
                goto out;
        }
	//checking for bad address 
	if( check_access(p->infile1 , sizeof(const char *))==0 || check_access(p->infile2 , sizeof(const char *))==0 ||
            check_access(p->outfile , sizeof(const char *))==0 || check_access(p->data , sizeof(unsigned int))==0){
		printk("sys_mergesort Passed bad memory address pointer arguments in arg\n");
                result = -EINVAL;
                goto out;
        }
	//Using getname to retrieve filename from userland to kernel 
        infile1 = getname(p->infile1);
        if(infile1 == NULL || IS_ERR(infile1)){
                printk("sys_mergesort unable to reference the first input file \n");
                result = (int) PTR_ERR(infile1);
                goto out;
        }
	infile2 = getname(p->infile2);
	if(infile2 == NULL || IS_ERR(infile2)){
                printk("sys_mergesort unable to reference the second input file \n");
                result = (int) PTR_ERR(infile2);
                goto out;
        }
	outfile = getname(p->outfile);
        if(outfile == NULL || IS_ERR(outfile)){
                printk("sys_mergesort unable to reference the output file \n");
                result = (int) PTR_ERR(outfile);
                goto out;
        }
	//allocating page size space for buffers in the kernel space
	buf1 = (char *) kmalloc(sizeof(char)*PG_SIZE, GFP_KERNEL);
	if(buf1 == NULL){
		printk("sys_mergesort unable to allocate Memory \n");
		result = -ENOMEM;
		goto out;
	}
        buf2 = (char *) kmalloc(sizeof(char)*PG_SIZE, __GFP_NOFAIL);
	if(buf2 == NULL){
                printk("sys_mergesort unable to allocate Memory \n");
                result = -ENOMEM;
                goto out;
        }
        outBuf = (char *) kmalloc(sizeof(char)*PG_SIZE, __GFP_NOFAIL);
	if(outBuf == NULL){
                printk("sys_mergesort unable to allocate Memory \n");
                result = -ENOMEM;
                goto out;
        }
	// dereferencing the flag variable from user space and checking max and minimum possible values
	if(p->flags <0 || p->flags >54){
		printk("sys_mergesort Invalid flag value \n");
                result = -EINVAL;
                goto out;
	}
	flag = p->flags;
	//printk("The pid to be tested %d\n", flag);
	//task = pid_task(find_vpid(flag), PIDTYPE_PID);
	//if(task!=NULL)
	//	printk("The task has pid is %d\n", task->pid);
	//setting appropriate flags
	if(flag>=32){
		printk("sys_mergesort Setting -d flag \n");
                returnLineCount = true;
                flag -=32;
        }if(flag>=16){
		printk("sys_mergesort Setting -t flag \n");
                checkInputOrder = true;
                flag -=16;
        }if(flag>=4){
		printk("sys_mergesort  Setting -i flag \n");
                caseInsensitive = true;
                flag -=4;
        }if(flag==3){
		printk("sys_mergesort Invalid flag value, Cannot set both -u and -a at the same time \n");
                result = -EINVAL;
                goto out;
        }if(flag == 0){
		printk("sys_mergesort Invalid flag value, both -u and -a are unset at the same time \n");
                result = -EINVAL;
                goto out;
	}if(flag==2){
		printk("sys_mergesort Setting -a flag \n");
                allowDuplicates = true;
                flag -=2;
        }if(flag==1){
		printk("sys_mergesort Setting -u flag \n");
                allowDuplicates = false;
                flag -=1;
        }
	 // setting fs to Kernel_DS
	set_fs(KERNEL_DS);
	//Check if the input files exists 
	if(vfs_stat(infile1->name, &inStat1)!=0 || vfs_stat(infile2->name, &inStat2)!=0){
		printk("sys_mergesort Invalid input file name, no such file exists \n");
                result = -EINVAL;
                goto out;
	}
	//Check to see if the input files are regular files.
	if(S_ISREG(inStat1.mode)==0){
                printk("sys_mergesort, The given input file1 is not a regular file \n");
                result = -EINVAL;
                goto out;
        }
        if(S_ISREG(inStat2.mode)==0){
                printk("sys_mergesort, The given input file2 is not a regular file \n");
                result = -EINVAL;
                goto out;
        }
	//check if the given input files are same
	if(inStat1.ino == inStat2.ino && inStat1.dev == inStat2.dev){
                printk("sys_mergesort Inodes are the same, input file1 and input file 2 are the same \n");
                result = -EINVAL;
                goto out;
        }
	// Accessing Input file pointer structures
	filp1 = filp_open(infile1->name, O_RDONLY, 0);
	if (!filp1 || IS_ERR(filp1)) {
		printk("sys_mergesort infile1 err %d %s\n", (int) PTR_ERR(filp1), infile1->name);
		result =  (int) PTR_ERR(filp1);
                goto out;
	}
	filp2 = filp_open(infile2->name, O_RDONLY, 0);
	if (!filp2 || IS_ERR(filp2)) {
		printk("sys_mergesort infile2 err %d %s\n", (int) PTR_ERR(filp2), infile2->name);
		result = (int) PTR_ERR(filp2);
		goto out;
	}
	if(vfs_stat(outfile->name, &outStat)==0){	//Condition where the output file already exists
		if(S_ISREG(outStat.mode)==0){
                	printk("sys_mergesort, The given output file is not a regular file \n");
                	result = -EINVAL;
                	goto out;
        	}
		//Check whether the input files are the same as the output file
		if(inStat1.ino == outStat.ino && inStat1.dev == outStat.dev){
                	printk("sys_mergesort Inodes are the same, input file1 and output file are the same \n");
                	result = -EINVAL;
                	goto out;
        	}
		if(inStat2.ino == outStat.ino && inStat2.dev == outStat.dev){
                	printk("sys_mergesort Inodes are the same, input file2 and output file are the same \n");
                	result = -EINVAL;
                	goto out;
        	}
        }
	//open already existing file and remove its contents()_WRONLY|O_TRUNC or create a new file(O_CREAT).
	//set the highest protection mode
	outFilp = filp_open(outfile->name, O_WRONLY|O_CREAT|O_TRUNC, inStat1.mode & inStat2.mode);
	if (!outFilp || IS_ERR(outFilp)) {
		printk("sys_mergesort output file err %d %s\n", (int) PTR_ERR(outFilp), outfile->name);
		result = (int) PTR_ERR(outFilp);
		goto out;
	}
	outFilp->f_inode->i_mode = inStat1.mode & inStat2.mode;		//
	filp1->f_pos = 0;                /* initializing file offset values */
	filp2->f_pos = 0;		
	outFilp->f_pos = 0;
	// Reading buffer from input files
	bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
	if(bytes_read1 < 0){
		printk("sys_mergesort Unable to read input file 1 \n");
		result = bytes_read1;
                goto out;
	}
	printk("sys_mergesort File1 read:%d \n", bytes_read1);
	bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
	if(bytes_read2 < 0){
                printk("sys_mergesort Unable to read input file 2 \n");
                result = bytes_read1;
                goto out;
        }
	printk("sys_mergesort File2 read:%d \n", bytes_read2);
	// This part of the code will try to read input one page at a time and compare records byte by byte
	//while loop will run until atleast one of the input files has been entirely read
	while((locPointer1< bytes_read1 && locPointer2<bytes_read2)){
		c1 = buf1[locPointer1];
		c2 = buf2[locPointer2];
		if(caseInsensitive){		//check if the case insensitive flag is set
			if(c1 >=97 && c1<=122){	// if the given alphabet is a small letter make it Caps and compare
				c1-=32;
			}
			if(c2 >=97 && c2<=122){
                                c2-=32;
                        }
		}		
		if(c1 == '\n' && c2!='\n'){	//case where one record ends before the other, the smaller one is written
			int i=0;
			for(i= lastLinePtr1; i<=locPointer1;i++){
				if(outBufPtr<PG_SIZE){	// check if the output buffer has reached its limit.
					outBuf[outBufPtr] = buf1[i];	//load record information 
					outBufPtr++;	
				}else{	// the buffer has reached page size so write the buffer into the file.
					loff_t locToWrite = (blocksWritten*PG_SIZE);	//get location to write
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);//write to file
					if(lines_written<0){
					 	printk("sys_mergesort Unable to write to Output file \n");
               		 		 	result = lines_written;
                				goto out;
        				}
					blocksWritten++;		
					outBufPtr = 0;	//reset buffer pointer
					i--;		// no buffer data was added to out,so go back and try to add again
				}
			}
			totalLines++;		// increment the line count to put inside 'data' field in struct param
			locPointer1++;		// increment the loc pointer to point to the next record in buffer
			lastLinePtr1 = locPointer1;//make the lastlineptr to point to the start of the current record.
			//check if loc pointer reached the end of buffer, this check is done in all the places where
			//the loc pointer 1 or loc pointer 2 is incremented. if the offset reached the end of buffer
			//reload the buffer with new data from the input file.
			if(locPointer1==bytes_read1){
                                bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos); //read buffer from file
				if(bytes_read1 < 0){		
         			       	printk("sys_mergesort Unable to read input file 1 \n");
                			result = bytes_read1;
                			goto out;
        			}
                                printk("sys_mergesort File1 read:%d \n", bytes_read1);
                                lastLinePtr1 = 0;		// reset input buffer pointers
				locPointer1 = 0;
                        }
			locPointer2 = lastLinePtr2;// make the loc pointer 2 point to the start of the current record. this is done so that, it can be used to compare current record with the next record in buf1.
		}else if(c1 != '\n' && c2=='\n'){	// the complementary case where record in buffer 2 ends first.
			int i=0;
			for(i = lastLinePtr2; i<=locPointer2;i++){
				if(outBufPtr<PG_SIZE){
					outBuf[outBufPtr] = buf2[i];
					outBufPtr++;
				}else{
					loff_t locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                                                printk("sys_mergesort Unable to write to Output file \n");
                                                result = lines_written;
                                                goto out;
                                        }
					blocksWritten++;
					outBufPtr = 0;
					i--;
				}
			}
			totalLines++;
			printk("The totalLines are %d\n", totalLines);
			locPointer2++;
			lastLinePtr2 = locPointer2;
			if(locPointer2==bytes_read2){
                                bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
                                if(bytes_read2 < 0){
                                        printk("sys_mergesort Unable to read input file 2 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
				printk("sys_mergesort File2 read:%d \n", bytes_read2);
                                lastLinePtr2 = 0;
				locPointer2 = 0;
                        }
			locPointer1 = lastLinePtr1;
		}else if(c1 =='\n' && c2 == '\n'){// case where both the records end simultaneously, duplicate records.
			int i=0;		  // in this case write first buffer and check duplicate flag to write
						  // the second buffer.
			for(i = lastLinePtr1; i<=locPointer1;i++){
				if(outBufPtr<PG_SIZE){
					outBuf[outBufPtr] = buf1[i];
					outBufPtr++;
				}else{
					loff_t locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                                                printk("sys_mergesort Unable to write to Output file \n");
                                                result = lines_written;
                                                goto out;
                                        }
					blocksWritten++;
					outBufPtr = 0;
					i--;
				}
			}
			totalLines++;
			locPointer1++;			//increment loc pointer to move to the next record
			lastLinePtr1 = locPointer1;	// assign lastlinePtr to the start of the new record
			if(locPointer1==bytes_read1){	
                                bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
                                if(bytes_read1 < 0){
                                        printk("sys_mergesort Unable to read input file 1 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
				printk("sys_mergesort File1 read:%d \n", bytes_read1);
                                lastLinePtr1 = 0;
				locPointer1=0;
                        }
			if(allowDuplicates){	// if flag is set, the duplicate record will be inserted into output.
				for(i = lastLinePtr2; i<=locPointer2;i++){
					if(outBufPtr<PG_SIZE){
						outBuf[outBufPtr] = buf2[i];
						outBufPtr++;
					}else{
						loff_t locToWrite = (blocksWritten*PG_SIZE);
						lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
						if(lines_written<0){
							printk("sys_mergesort Unable to write to Output file \n");
							result = lines_written;
							goto out;
						}					
						blocksWritten++;
						outBufPtr = 0;
						i--;
					}
				}
				totalLines++;
				printk("The totalLines are %d\n", totalLines);
			}
			locPointer2++;			//increment loc pointer to point to next record
			lastLinePtr2 = locPointer2;	//assign last pointer to start of the next record
			if(locPointer2==bytes_read2){
                                bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
				if(bytes_read2 < 0){
                                        printk("sys_mergesort Unable to read input file 2 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
                                printk("sys_mergesort File2 read:%d \n", bytes_read2);
                                lastLinePtr2 = 0;
				locPointer2 = 0;
                        }
		}else if(c1 == c2){		// case where both the records have the same character
			locPointer1++;		
			//special scenario where we still don't know which record is smaller but the buffer ends before
			//the end of the record. roll the file pointer to the start of the record being compared and load
			//the next page. since a record is less than page size, the current record will surely end in 
			//in this page. Efficiency is acheived by starting the comparison where we left off rather than
			//from the start of the record.
			if(locPointer1==bytes_read1){
				locPointer1 = (PG_SIZE - lastLinePtr1);		//determine offset value.
				//Record size Exceeds a page
				if(locPointer1 == PG_SIZE){
					printk("sys_mergesort unable to process, input record greater than page Size\n");
					result = -EINVAL;
					goto out;
				}
				// move the file pointer back by that offset.
				if(filp1->f_op->llseek(filp1,((-locPointer1)),SEEK_CUR) < 0){
					printk("sys_mergesort Unable to move file pointer \n");
					result = -EINVAL;
					goto out;
				}
				bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
				if(bytes_read1 < 0){
                                        printk("sys_mergesort Unable to read input file 1 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
				printk("current location pointer1:%d %c \n", locPointer1, buf1[locPointer1]);
				lastLinePtr1 = 0;	//reset starting point of record to start of buffer.
			}			//repeat for second buffer	
			locPointer2++;
			if(locPointer2==bytes_read2){
				locPointer2 = (PG_SIZE - lastLinePtr2);
				//Record size exceeds a page
				if(locPointer2 == PG_SIZE){
                                        printk("sys_mergesort unable to process, input record greater than page Size\n");
                                        result = -EINVAL;
                                        goto out;
                                }
				// move the file pointer back by that offset.
				if(filp2->f_op->llseek(filp2,((-locPointer2)),SEEK_CUR) < 0){
					printk("sys_mergesort Unable to move file pointer \n");
                                        result = -EINVAL;
                                        goto out;
				}
				bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
				if(bytes_read2 < 0){
                                        printk("sys_mergesort Unable to read input file 2 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
				printk("current location pointer2:%d %c\n", locPointer2, buf2[locPointer2]);
				lastLinePtr2 = 0;
			}	
		}else if(c1 <  c2){	//case where record in buf1 is lexographically smaller that record in buf2.
			int i=0;	//Write from start of the buf1 record till we encounter \n. may traverse buffer.
			for(i = lastLinePtr1;;){
				if(i==bytes_read1){	//check if we reached buf1 end before we get \n.
					bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
					if(bytes_read1 < 0){
                                        	printk("sys_mergesort Unable to read input file 1 \n");
                                        	result = bytes_read1;
                                        	goto out;
                                	}
					printk("sys_mergesort File1 read:%d \n", bytes_read1);
					i =0 ;					
				}
				if(outBufPtr<PG_SIZE){
					outBuf[outBufPtr] = buf1[i];
					outBufPtr++;
				}else{
					loff_t locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                                                printk("sys_mergesort Unable to write to Output file \n");
                                                result = lines_written;
                                                goto out;
                                        }
					blocksWritten++;
					outBufPtr = 0;
					i--;
				} 
				if(buf1[i]=='\n'){ //reached \n now move to the next record and break the loop.
					i++;	
					break;
				}
				i++;
			}
			totalLines++;	
			locPointer1=i;		//assign i to the loc pointer, now loc pointer points to next reocrd
			lastLinePtr1 = locPointer1;
			if(locPointer1==bytes_read1){ //check loc pointer reached the end of buf1
                                bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
				if(bytes_read1 < 0){
                                        printk("sys_mergesort Unable to read input file 1 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
                                printk("sys_mergesort File1 read:%d \n", bytes_read1);
                                lastLinePtr1 = 0;
                                locPointer1=0;
                        }
			locPointer2 = lastLinePtr2;			//reset loc pointer2 to start of current record
		}else if(c1 >  c2){	//complementary case to the above, where record from buf2 is smaller.
			int i=0;
			printk("sys_mergesort file2 to be written:%c, %d \n", buf1[locPointer1], locPointer1);
			printk("sys_mergesort file2 to be written:%c , %d\n", buf2[locPointer2], locPointer2);
			for(i = lastLinePtr2;;){
				if(i==bytes_read2){
					bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
					if(bytes_read2 < 0){
                                        	printk("sys_mergesort Unable to read input file 2 \n");
                                        	result = bytes_read1;
                                        	goto out;
                                	}
					printk("sys_mergesort File2 read:%d \n", bytes_read2);
					i=0;
				}
				if(outBufPtr<PG_SIZE){
					outBuf[outBufPtr] = buf2[i];
					outBufPtr++;
				}else{
					loff_t locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                                                printk("sys_mergesort Unable to write to Output file \n");
                                                result = lines_written;
                                                goto out;
                                        }	
					blocksWritten++;
					outBufPtr = 0;
					i--;
				}
				if(buf2[i]=='\n'){
					i++;
                                        break;
				}           
				i++;                    
			}
			totalLines++;
			printk("The totalLines are %u\n", totalLines);
			locPointer2 = i;
                        printk("File1 buffer added to the output, cur location: %d %c\n", locPointer2, buf2[locPointer2]);
			lastLinePtr2 = locPointer2;
			if(locPointer2==bytes_read2){
                                bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
				if(bytes_read2 < 0){
                                        printk("sys_mergesort Unable to read input file 2 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
                                printk("sys_mergesort File2 read:%d \n", bytes_read2);
                                lastLinePtr2 = 0;
                                locPointer2 = 0;
                        }
			locPointer1 = lastLinePtr1;
		}
	}
	//While loop exited with one or both files being fully read and merged, now add all the records from the remaining
	//file(if any).
	if(locPointer1>=bytes_read1 && locPointer2<bytes_read2){// input file 1 has ended, input file2 should be written.
		loff_t locToWrite = 0;
		printk("sys_mergesort file1 ended writing from file2 \n");
		while(bytes_read2!=0){					//while we read an empty buffer
			while(locPointer2<bytes_read2){
				if(buf2[locPointer2] == '\n'){
					totalLines++;			// count the number of lines written
				}
				if(outBufPtr<PG_SIZE){			// add evverything to output buffer.
					outBuf[outBufPtr] = buf2[locPointer2];
					outBufPtr++;
					locPointer2++;
				}else{
					locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                                                printk("sys_mergesort Unable to write to Output file \n");
                                                result = lines_written;
                                                goto out;
                                        }
					blocksWritten++;
					outBufPtr = 0;
				}
			}
			if(locPointer2==bytes_read2){//reload buf2 if the current buffer ended
                                bytes_read2 = vfs_read(filp2, buf2, PG_SIZE, &filp2->f_pos);
				if(bytes_read2 < 0){
                                        printk("sys_mergesort Unable to read input file 2 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
                                printk("sys_mergesort File2 read:%d \n", bytes_read2);
                                locPointer2 = 0;
                        }
		}
		//this is write whatever there is left inside the output buffer into the file.No more writes after this.
		locToWrite = (blocksWritten*PG_SIZE);
        	lines_written = vfs_write(outFilp, outBuf, outBufPtr, &locToWrite);
		if(lines_written<0){
        		printk("sys_mergesort Unable to write to Output file \n");
                	result = lines_written;
              		goto out;
        	}
	}
	else if(locPointer1< bytes_read1 && locPointer2 >= bytes_read2){//complementary to the above case
		loff_t locToWrite = 0;
		printk("sys_mergesort file2 ended writing from file1 \n");
                while(bytes_read1!=0){
                        while(locPointer1<bytes_read1){
				if(buf1[locPointer1] == '\n'){
                                        totalLines++;
					printk("The totalLines are %u\n", totalLines);
				}
                                if(outBufPtr<PG_SIZE){
                                        outBuf[outBufPtr] = buf1[locPointer1];
                                        outBufPtr++;
					locPointer1++;				
                                }else{
                                        locToWrite = (blocksWritten*PG_SIZE);
					lines_written = vfs_write(outFilp, outBuf, PG_SIZE, &locToWrite);
					if(lines_written<0){
                        			printk("sys_mergesort Unable to write to Output file \n");
                        			result = lines_written;
                        			goto out;
                			}
                                        blocksWritten++;
                                        outBufPtr = 0;
                                }
                        }
                        if(locPointer1==bytes_read1){
                                bytes_read1 = vfs_read(filp1, buf1, PG_SIZE, &filp1->f_pos);
				if(bytes_read1 < 0){
                                        printk("sys_mergesort Unable to read input file 1 \n");
                                        result = bytes_read1;
                                        goto out;
                                }
                                printk("sys_mergesort File1 read:%d \n", bytes_read1);
                                locPointer1= 0;
                        }
                }
		locToWrite = (blocksWritten*PG_SIZE);
        	lines_written = vfs_write(outFilp, outBuf, outBufPtr, &locToWrite);
		if(lines_written<0){
                        printk("sys_mergesort Unable to write to Output file \n");
                        result = lines_written;
                        goto out;
                }
        }else{	// case where both the files have ended simultaneously, write whatever there is left in the output buffer.
		loff_t locToWrite = (blocksWritten*PG_SIZE);
                lines_written = vfs_write(outFilp, outBuf, outBufPtr, &locToWrite);
                if(lines_written<0){
                        printk("sys_mergesort Unable to write to Output file \n");
                        result = lines_written;
                        goto out;
                }
	}
	if(returnLineCount){	// incase if the number of records written needs to be returned.
		int bytes_failed = copy_to_user(p->data, &totalLines, sizeof(totalLines));//write line count to userland
		printk("TotalLines : %d\n", totalLines);
		printk("bytes_failed : %d\n", bytes_failed);		
		if(bytes_failed != 0){
			printk("sys_mergesort Error in writing(returning) the number of lines written to output file\n");
			result = -EFAULT;
		}
	}
out: // perform clean up, if something is not null, it has to be cleaned up.
	set_fs(oldfs);
	if(p!=NULL){
		kfree(p);
		p = NULL;
	}
	if(buf1!=NULL){
		kfree(buf1);
		buf1= NULL;
	}
	if(buf2!=NULL){
		kfree(buf2);
		buf2 = NULL;
	}
	if(outBuf!=NULL){
		kfree(outBuf);
		outBuf = NULL;
	}
	if(result != 0 && outFilp!=NULL){
		struct dentry * outDentry = outFilp->f_path.dentry;
                if(vfs_unlink(outDentry->d_parent->d_inode, outDentry, NULL)!=0){
                        printk("Unable to remove the partially created output file\n");
                }
        }
	if(filp1!=NULL)
                filp_close(filp1, NULL);
        if(filp2!=NULL)
                filp_close(filp2, NULL);
        if(outFilp!=NULL)
                filp_close(outFilp, NULL);
	if(infile1!=NULL)
		putname(infile1);
	if(infile2!=NULL)
		putname(infile2);
	if(outfile!=NULL)
		putname(outfile);
	return result;
}

static int __init init_sys_xmergesort(void)
{
	printk("installed new sys_xmergesort module\n");
	if (sysptr == NULL)
		sysptr = xmergesort;
	return 0;
}
static void  __exit exit_sys_xmergesort(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_xmergesort module\n");
}
module_init(init_sys_xmergesort);
module_exit(exit_sys_xmergesort);

MODULE_AUTHOR("GANGABARNI BALAKRISHNAN");
MODULE_DESCRIPTION(" Merge Two sorted files in the proper sorted order");
MODULE_LICENSE("GPL");
