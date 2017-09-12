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

extern void deregister_vector(vector_syscall* vect_syscall_head);
extern int register_vector(char* vector_name, int vector_id, vector_syscall* vect_syscall_head, struct module* module);

vector_syscall* vector_head = NULL;
vector_syscall* vector_tail = NULL;

char vector_name[MAX_VECTOR_NAME_LEN] = "combo_syscall_vector";
int vector_id = 1;

int open_sys_call_no = 2;
int mkdir_sys_call_no = 83;
int close_sys_call_no = 3;
int unlink_sys_call_no = 87;
int symlink_sys_call_no = 88;	

asmlinkage long open_combo_syscall(const char* __user file_name, int flags, umode_t mode){
	int ret = 0;
	char* kern_file_name = NULL;
	kern_file_name = kmalloc(1024,GFP_KERNEL);
	if(kern_file_name == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_file_name)){
		ret = PTR_ERR(kern_file_name);
		goto end;
	}
	memset(kern_file_name,0,1024);
	ret = copy_from_user(kern_file_name,file_name,1024);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	//printk(sys_getpid(void));
	printk(KERN_INFO "Combo Vector Restricted open syscall \n");
	printk(KERN_INFO "File name %s\n",kern_file_name);
	printk(KERN_INFO "Mode %d\n",mode);
	printk(KERN_INFO "Flags %d\n",flags);
	if(strstr(kern_file_name,".virus") == NULL){
		ret = CALL_DEFAULT;
	}
	else{
		ret = -ENOTSUPP;
	}
end:
	if(kern_file_name != NULL){
		kfree(kern_file_name);
	}
	return ret;
}


asmlinkage long close_combo_syscall(int fd){
	int ret = 0;
	printk(KERN_INFO "Combo Vector wrap close syscall");
        printk(KERN_INFO "Fd %d",fd);
	ret = CALL_DEFAULT;
	return ret;
}

asmlinkage long unlink_combo_syscall(const char *pathname){
	int ret = 0;
	char* kern_path_name = NULL;
	kern_path_name = kmalloc(1024,GFP_KERNEL);
	if(kern_path_name == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_path_name)){
		ret = PTR_ERR(kern_path_name);
		goto end;
	}
	memset(kern_path_name,0,1024);
	ret = copy_from_user(kern_path_name,pathname,1024);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	printk(KERN_INFO "Combo vector restricted unlink syscall\n");
        printk(KERN_INFO "Filename %s\n",kern_path_name);
	if(strstr(kern_path_name,".protected") == NULL){
		ret = CALL_DEFAULT;
	}
	else{
		ret = -ENOTSUPP;
	}
end:
	if(kern_path_name != NULL){
		kfree(kern_path_name);
	}
	return ret;
}

asmlinkage long mkdir_combo_syscall(const char* __user path_name,umode_t mode){
	int ret = 0;
	char* kern_path_name;
	struct task_struct* tsk = get_current();
	kern_path_name = kmalloc(1024,GFP_KERNEL);
	if(kern_path_name == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(kern_path_name)){
		ret = PTR_ERR(kern_path_name);
		goto end;
	}
	ret = copy_from_user(kern_path_name,path_name,1024);
	if(ret < 0){
		ret = -EFAULT;
		goto end;
	}
	//printk(sys_getpid(void));
	printk(KERN_INFO "Combo vector restricted mkdir syscall\n");
	printk(KERN_INFO "File name %s\n",kern_path_name);
	printk(KERN_INFO "Mode %d\n",mode);
	if(strcmp("httpd", tsk->comm)==0){
		printk("This is operation is not supported\n");
		ret = -ENOTSUPP;
	}else{
		ret = CALL_DEFAULT;	
	}
end:
	if(kern_path_name != NULL){
		kfree(kern_path_name);
	}
	printk("Return Value %d \n", ret);
	return ret;

}

asmlinkage long symlink_combo_syscall(const char *target, const char *linkpath){
	int ret = -83000;
	printk("Combo vector Symlink syscall disabled");
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

	ret = add_syscall_to_vector(open_sys_call_no,(unsigned long) open_combo_syscall);
	if(ret < 0){
		goto end;
	}
	ret = add_syscall_to_vector(mkdir_sys_call_no,(unsigned long) mkdir_combo_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(close_sys_call_no,(unsigned long) close_combo_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(unlink_sys_call_no,(unsigned long) unlink_combo_syscall);
        if(ret < 0){
                goto end;
        }
	ret = add_syscall_to_vector(symlink_sys_call_no,(unsigned long) symlink_combo_syscall);
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
        printk("removed combo vector module\n");

}

module_init(init_combo_syscalls_vector);
module_exit(exit_combo_syscalls_vector);
