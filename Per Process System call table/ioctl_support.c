#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "vector_control.h"
#include "ioctl_support.h"

#define DEVICE_NAME "ioctl_syscall"
#define AUTHOR "GAYATHRI S"
#define DESCRIPTION "IOCTL SUPPORT FOR PER PROCESS TABLE"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);

struct mutex ioctl_mut_lock;
extern int reduce_ref_count(vector_syscall* vect);
extern vector_syscall* get_vector_head(int vid);
extern int increase_ref_count(int vid);
extern int get_vectorid_by_address_head(vector_syscall* vec);

static int add_syscall_vect(unsigned int vid, unsigned int pid){
	int ret = 0;
	struct task_struct * task = pid_task(find_vpid(pid), PIDTYPE_PID);
	vector_syscall* sys_vec = NULL;
	sys_vec = get_vector_head(vid);

	printk("Retrieved Struct task from pid \n");
	if(IS_ERR(sys_vec)){
		ret = PTR_ERR(sys_vec);
		goto out;
	}
	if(sys_vec == NULL){
		ret = -EINVAL;
		printk("Invalid VID!!! \n");
		goto out;
	}
	printk("The Pid: %u to be assigned vector id: %u \n", pid, vid);
	if(task->syscall_vector_head == NULL) {
                task->syscall_vector_head = (void *)sys_vec;
		printk("Added vector address to void* syscall_vector_head field\n");
		ret = increase_ref_count(vid);
        }else{
		ret = reduce_ref_count((vector_syscall *)task->syscall_vector_head);
		if(ret < 0)
			goto out;
		task->syscall_vector_head = (void *)sys_vec;
                printk("Added vector address to void* syscall_vector_head field\n");
                ret = increase_ref_count(vid);	
	}
out:
	return ret;
}

static int remove_syscall_vect(int pid) {
        int ret = 0;
	struct task_struct * task = pid_task(find_vpid(pid), PIDTYPE_PID);

        if(task->syscall_vector_head != NULL) {
                printk("Removing vector address from void* syscall_vector_head field");               
		ret = reduce_ref_count((vector_syscall *)task->syscall_vector_head);
	 	if(ret < 0)
                        goto out;
		task->syscall_vector_head = NULL;
        }
out:
	return ret;
}

static long syscall_ioctl(struct file *file,
                 unsigned int ioctl_num,
                 unsigned long input_param)
{
        int ret = 0;
       	struct ioctl_params* param = NULL;
	struct task_struct* tsk = NULL;
	vector_syscall* sys_vec = NULL;
        mutex_lock(&ioctl_mut_lock);
        param = kmalloc(sizeof(struct ioctl_params*), GFP_KERNEL);
        if(param == NULL || IS_ERR(param)) {
                ret = -ENOMEM;
                goto out;
        }
        ret = copy_from_user(param,(struct ioctl_params*) input_param, 													sizeof(struct ioctl_params*));
	if(ret < 0) {
		ret = -1;
		goto out;
	}
	printk("PROCESS_ID_RECEIVED is: %d\n", param->pid);
	if(pid_task(find_vpid(param->pid), PIDTYPE_PID) == NULL ) {
		ret = -EINVAL;
		printk(" PROCESS ID does not exist ");
		goto out;
	}
	switch (ioctl_num) {
        case IOCTL_SET_VECTOR:
          //      try_module_get(THIS_MODULE);
                printk("VECTOR_ID_RECEIVED is: %d\n", param->vid);
                ret = add_syscall_vect(param->vid, param->pid);
                if(ret < 0) {
			ret = -EINVAL;
              //          module_put(THIS_MODULE);
                }
		printk("The IOCTL_SET_VECTOR called ret: %d \n", ret);
                break;
	case IOCTL_REMOVE:
	//	try_module_get(THIS_MODULE);
                printk(" PROCESS_ID_RECEIVED is: %d", param->pid);
                ret = remove_syscall_vect(param->pid);
		if(ret < 0) {
                        ret = -EINVAL;
                }
            //    module_put(THIS_MODULE);
		printk("The IOCTL_REMOVE called ret: %d \n", ret);
                break;
        case IOCTL_GET_VECTOR:
		tsk = pid_task(find_vpid(param->pid), PIDTYPE_PID);
		sys_vec = tsk->syscall_vector_head;
		if(sys_vec != NULL){
			ret = get_vectorid_by_address_head(sys_vec);
			if(ret < 0){
                 		ret = -1;
                	}
			goto out;
		}	
		ret = 0; //Default vector id	
		break;
	default:
                ret = -1;
        }
out:
	printk("The value to be returned ret: %d \n", ret);
	if(param!=NULL)
        	kfree(param);
        mutex_unlock(&ioctl_mut_lock);
        return (long)ret;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
        .unlocked_ioctl = syscall_ioctl,
};

static int __init init_ioctl_module(void)
{
        int ret = 0;
        ret = register_chrdev(DEVICE_NUM, DEVICE_NAME, &fops);
        if (ret < 0) {
                printk(KERN_ALERT "%s Unable to %d\n",
                        "Register the character device ", ret);
                goto out;
        }
        mutex_init(&ioctl_mut_lock);
out:
        return ret;
}


static void __exit exit_ioctl_module(void)
{
        unregister_chrdev(DEVICE_NUM, DEVICE_NAME);
}

module_init(init_ioctl_module);
module_exit(exit_ioctl_module);

