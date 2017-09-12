#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <linux/vector_control.h>
#include <trace/events/syscalls.h>


extern void deregister_vector(vector_syscall* vect_syscall_head);
extern int register_vector(char* vector_name, int vector_id, vector_syscall* vect_syscall_head, struct module* module);

vector_syscall* vector_head = NULL;
vector_syscall* vector_tail = NULL;

char vector_name[MAX_VECTOR_NAME_LEN] = "wrap_syscall_vector";
int vector_id = 2;
extern asmlinkage long sys_getpid(void);

int write_sys_call_no = 1;
int read_sys_call_no = 0;
int link_sys_call_no = 86;
int getpid_sys_call_no = 39;
int getppid_sys_call_no = 110;	

asmlinkage long write_wrap_syscall(int fd, const void *buf, size_t count){
	int ret = 0;
	char* kern_buffer = NULL;
	kern_buffer = kmalloc(PAGE_SIZE,GFP_KERNEL);
	if(kern_buffer == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_buffer)){
		ret = PTR_ERR(kern_buffer);
		goto end;
	}
	memset(kern_buffer,0, PAGE_SIZE);
	ret = copy_from_user(kern_buffer,(char*)buf,PAGE_SIZE);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	printk(KERN_INFO "Wrap vector write syscall\n");
        printk(KERN_INFO "Fd %d\n",fd);
	printk(KERN_INFO "Buffer %s",kern_buffer);
	printk(KERN_INFO "Size %ld", count);
	ret = CALL_DEFAULT;
end:
	if(kern_buffer != NULL){
		kfree(kern_buffer);
	}
	return ret;
}


asmlinkage long getpid_wrap_syscall(void){
	int ret = 0;
	printk(KERN_INFO "Wrap vector Get pid wrap syscall");
	ret = CALL_DEFAULT;
	return ret;
}

asmlinkage long link_wrap_syscall(const char *target, const char *linkpath){
	int ret = 0;
	char* kern_target = NULL;
	char* kern_link = NULL;
	kern_target = kmalloc(PAGE_SIZE,GFP_KERNEL);
	kern_link = kmalloc(PAGE_SIZE,GFP_KERNEL);
	if(kern_target == NULL || kern_link == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_target)){
		ret = PTR_ERR(kern_target);
		goto end;
	}
	if(IS_ERR(kern_link)){
		ret = PTR_ERR(kern_link);
		goto end;
	}
	memset(kern_target,0,PAGE_SIZE);
	memset(kern_link,0,PAGE_SIZE);
	ret = copy_from_user(kern_target,target,PAGE_SIZE);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	ret = copy_from_user(kern_link,kern_link,PAGE_SIZE);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	printk(KERN_INFO "Wrap vector Link syscall \n");
	printk(KERN_INFO "Target %s\n",kern_target);
	printk(KERN_INFO "Link %s\n",kern_link);
	ret = CALL_DEFAULT; 
end:
	if(kern_target != NULL){
		kfree(kern_target);
	}
	if(kern_link !=NULL){
		kfree(kern_link);
	}
	return ret;
}

asmlinkage long read_wrap_syscall(int fd, const void *buf, size_t count){
	int ret = 0;
	char* kern_buffer = NULL;
	kern_buffer = kmalloc(PAGE_SIZE,GFP_KERNEL);
	if(kern_buffer == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_buffer)){
		ret = PTR_ERR(kern_buffer);
		goto end;
	}
	memset(kern_buffer,0, PAGE_SIZE);
	ret = copy_from_user(kern_buffer,(char*)buf,PAGE_SIZE);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	printk(KERN_INFO "Wrap vector wrap read syscall\n");
        printk(KERN_INFO "Fd %d\n",fd);
	printk(KERN_INFO "Buffer %s",kern_buffer);
	printk(KERN_INFO "Size %ld", count);
	ret = CALL_DEFAULT;
end:
	if(kern_buffer != NULL){
		kfree(kern_buffer);
	}
	return ret;
}
asmlinkage long getppid_wrap_syscall(void){
	int ret = 0;
	printk(KERN_INFO "Wrap vector Get parent pid wrap syscall");
	ret = CALL_DEFAULT;
	return ret;
}

static int add_syscall_to_vector(int sys_call_no, unsigned long function_ptr){
	int ret = 0;
	vector_syscall* new_vector_syscall = NULL;
	new_vector_syscall = kmalloc(sizeof(vector_syscall),GFP_KERNEL);
	if(new_vector_syscall == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(new_vector_syscall)){
		ret = PTR_ERR(new_vector_syscall);
		if(new_vector_syscall != NULL){
        	        kfree(new_vector_syscall);
	        }

		goto end;
	}
	memset(new_vector_syscall,0,sizeof(vector_syscall));
	new_vector_syscall->syscall_no = sys_call_no;
	new_vector_syscall->function_ptr = function_ptr;
	new_vector_syscall->nxt = NULL;
	if(vector_tail == NULL){
		vector_tail = new_vector_syscall;
		vector_head = vector_tail;
	}
	else{
		vector_tail->nxt = new_vector_syscall;
		vector_tail = new_vector_syscall;
	}
end:
	new_vector_syscall = NULL;
	return ret;

}

static int initialize_vector(void){
	int ret = 0;

	ret = add_syscall_to_vector(read_sys_call_no,(unsigned long) read_wrap_syscall);
	if(ret < 0){
		goto end;
	}
	ret = add_syscall_to_vector(write_sys_call_no,(unsigned long) write_wrap_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(getpid_sys_call_no,(unsigned long) getpid_wrap_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(getppid_sys_call_no,(unsigned long) getppid_wrap_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(link_sys_call_no,(unsigned long) link_wrap_syscall);
        if(ret < 0){
                goto end;
        }
	ret = register_vector(vector_name,vector_id,vector_head,THIS_MODULE);
end:
	return ret;
}


static void delete_vector_syscalls(void){
	vector_syscall* temp = NULL;
	temp = vector_head;
	while(temp != NULL){
		vector_head = vector_head->nxt;
		kfree(temp);		
		temp = vector_head;
	}
	vector_tail = NULL;
	vector_head = NULL;
}


static int __init init_combo_syscalls_vector(void){
	int ret = 0;
	ret = initialize_vector();		
	if( ret < 0){
		delete_vector_syscalls();
	}
	return ret;
}

static void  __exit exit_combo_syscalls_vector(void)
{
	deregister_vector(vector_head);
        printk("removed wrap vector module\n");

}

module_init(init_combo_syscalls_vector);
module_exit(exit_combo_syscalls_vector);
