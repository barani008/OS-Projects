#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vector_control.h>
#include <linux/seq_file.h>
#include <linux/linkage.h>
#include <linux/moduleloader.h>

#define AUTHOR "PRASANTH"
#define DESCRIPTION "VECTOR CONTROL MODULE"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
static const char filename[] = "syscall_vectors";
struct proc_dir_entry* pde = NULL;

extern int (*increase)(int arg);
extern int (*reduce)(void *arg);
extern int (*get_addr)(void* arg);
extern void* (*get_vect)(int arg);
extern void* (*get_table)(int arg);

struct mutex vector_list_lock;
vector* vector_list_head = NULL;
vector* vector_list_tail = NULL;

char buffer[4096];

//static int show_vectors(char* user_buffer,char** buffer
static ssize_t show_vectors(struct file *file, char __user *buf, size_t size, loff_t *ppos){
	int ret = 0;
	char *current_ptr;
	vector *temp;
	int vec_len, temp_count;
	int digits = 0;
	if(*ppos > 0){
		ret = 0;
		goto out;
	}
	temp = NULL;
	current_ptr = NULL;
	memset(buffer, 0, MAX_BUFFER_SIZE);
	current_ptr = buffer;
	if(vector_list_head == NULL) {
		goto out;
	}

	temp = vector_list_head;
	while ((temp != NULL) && (ret < MAX_BUFFER_SIZE)){
		digits = 0;
		vec_len = strlen(temp->vector_name);
		memcpy(current_ptr, temp->vector_name, vec_len);
		current_ptr[vec_len] = '(';
		current_ptr = current_ptr + vec_len + 1;
		temp_count = temp->ref_count;
		if(temp_count == 0){
			digits = 1;
		}	
		else{
			while(temp_count > 0){
				temp_count = temp_count/10;
				digits++;
			}
		}
		snprintf(current_ptr, digits+1, "%d", temp->ref_count);
		current_ptr[digits] = ')';
		current_ptr[digits+1] = '\n';
		current_ptr = current_ptr + digits + 2;
		ret = ret + vec_len + digits + 4;	
		*ppos = ret;
		temp = temp->nxt;
	}
	memcpy(buf, buffer, ret);
out:
	return ret;
}

static const struct file_operations vector_proc_fops = {
	.read = show_vectors
};

void* get_vector_head(int vector_id){
	vector_syscall* ret = NULL;
	vector* temp = NULL;
	mutex_lock(&vector_list_lock);
	temp = vector_list_head;
	if(vector_list_head == NULL || vector_id == 0){
		goto out;
	}
	while(temp != NULL){
       		if(vector_id == temp->vector_id){
			ret = temp->vect_syscall_head;
			goto out;
		}
		temp = temp->nxt;
	}
out:	
	temp = NULL;
	mutex_unlock(&vector_list_lock);
	if(ret!=NULL)
		return (void *)ret;
	else
		return NULL;
}
EXPORT_SYMBOL(get_vector_head);

void* get_vector_table(int vector_id){
        vector* ret = NULL;
        vector* temp = NULL;
        mutex_lock(&vector_list_lock);
        temp = vector_list_head;
        if(vector_list_head == NULL || vector_id <= 0){
                goto out;
        }
        while(temp != NULL){
                if(vector_id == temp->vector_id){
                        ret = temp;
                        goto out;
                }
                temp = temp->nxt;
        }
out:
        temp = NULL;
        mutex_unlock(&vector_list_lock);
        if(ret!=NULL)
                return (void *)ret;
        else
                return NULL;
}
EXPORT_SYMBOL(get_vector_table);

int get_vectorid_by_address_head(void* vec){
	vector_syscall * vect = (vector_syscall *)vec;
	int ret = 0;
	vector* temp = NULL;
	mutex_lock(&vector_list_lock);
	temp = vector_list_head;
	if(vector_list_head == NULL){
		goto out;
	}
	while(temp != NULL){
       		if(vect == temp->vect_syscall_head){
			ret = temp->vector_id;
			goto out;
		}
		temp = temp->nxt;
	}
out:
	temp = NULL;
	mutex_unlock(&vector_list_lock);
	return ret;
}
EXPORT_SYMBOL(get_vectorid_by_address_head);

int reduce_ref_count(void* vec){
	vector_syscall * vect = (vector_syscall *)vec;
	int ret = 0;
	vector* temp = NULL;
        mutex_lock(&vector_list_lock);
        temp = vector_list_head;
        if(vector_list_head == NULL){
                goto out;
        }
        while(temp != NULL){
       		if(vect == temp->vect_syscall_head){
			if(temp->ref_count > 0){
                		temp->ref_count = temp->ref_count - 1;
			}
			if(temp->ref_count == 0){
				module_put(temp->module);
			}
			ret = temp->ref_count;
			goto out;
                }
		temp = temp->nxt;
        }
out:
	temp = NULL;
	mutex_unlock(&vector_list_lock);
        return ret;
}
EXPORT_SYMBOL(reduce_ref_count);

int increase_ref_count(int vector_id){
	int ret = 0;
	vector* temp = NULL;
	
        mutex_lock(&vector_list_lock);
        temp = vector_list_head;
        if(vector_list_head == NULL){
                goto out;
        }
        while(temp != NULL){
       		if(vector_id == temp->vector_id){
	                temp->ref_count = temp->ref_count + 1;
		
			if(temp->ref_count == 1){
				try_module_get(temp->module);
			}
			ret = temp->ref_count;
			goto out;
		}
		temp = temp->nxt;
        }
out:
	temp = NULL;
	mutex_unlock(&vector_list_lock);
        return ret;
}
EXPORT_SYMBOL(increase_ref_count);

static int add_vector(char* vector_name, int vector_id, vector_syscall* vector_syscall_head, struct module* module){
	int ret = 0;
	vector* new_vector = NULL;
	new_vector = kmalloc(sizeof(vector),GFP_KERNEL);
	if(new_vector == NULL){
		ret = -ENOMEM;
		goto end;
	}
	if(IS_ERR(new_vector)){
		ret = PTR_ERR(new_vector);
		goto end;
	}
	memset(new_vector,0,sizeof(vector));	
	memcpy(new_vector->vector_name, vector_name, strlen(vector_name)+1);
	new_vector->vect_syscall_head = vector_syscall_head;
	new_vector->vector_id = vector_id;
	new_vector->list1 = 0xFFFFFFFFFFFFFFFF;
	new_vector->list2 = 0xFFFFFFFFFFFFFFFF;
	new_vector->list3 = 0xFFFFFFFFFFFFFFFF;
	new_vector->list4 = 0xFFFFFFFFFFFFFFFF;	
	new_vector->module = module;
	new_vector->nxt = NULL;
	if(vector_list_head == NULL){
		vector_list_head = new_vector;
		vector_list_tail = vector_list_head;
	}
	else{
		vector_list_tail->nxt = new_vector;
		vector_list_tail = new_vector;
	}
end:
	new_vector = NULL;
	return ret;	
}

int register_vector(char* vector_name, int vector_id, vector_syscall* vect, struct module* vector_module){
	int ret = 0;
	mutex_lock(&vector_list_lock);
	ret = add_vector(vector_name, vector_id, vect,vector_module);
	if(ret < 0){
		goto end;
	}
end:
	mutex_unlock(&vector_list_lock);
	return ret;
}
EXPORT_SYMBOL(register_vector);

static int delete_vector(vector* vect){
	int ret = 0;
        vector* temp = vector_list_head;
        vector* before_temp = NULL;
        if(vector_list_head == NULL){
                ret = -EFAULT;
                goto end;
        }
        //else if(vector_list_head->nxt == NULL && vector_list_head->vector_id == vector_list_head->vector_id){
        //        goto ref_count_check;
        //}
        //else if(vector_list_head->nxt != NULL){
        else{
                while(temp != NULL){
                        if(temp->vector_id == vect->vector_id){
                                goto ref_count_check;
                        }
			before_temp = temp;
			temp = temp->nxt;
                }
        }
        ret = -EFAULT;
        goto end;
ref_count_check:
        if(temp->ref_count > 0){
                ret = -1000; //Cannot remove vector since there are live running processes using this vector
                goto end;
        }
        if(temp == vector_list_head){
		vector_list_head = temp->nxt;
		if(temp!=NULL && vector_list_tail == temp){
			kfree(temp);
			vector_list_tail = NULL;
		}else if(temp!=NULL){
			kfree(temp);
		}
        }
        else{
                before_temp->nxt = temp->nxt;
                if(temp == vector_list_tail){
                        vector_list_tail = before_temp;
                }
                kfree(temp);	
        }
end:
        before_temp = NULL;
	temp = NULL;
        return ret;

}

void deregister_vector(vector* vect){
	int ret = 0;
	mutex_lock(&vector_list_lock);
	ret = delete_vector(vect);
	if(ret < 0){
		goto end;
	}
end:
	mutex_unlock(&vector_list_lock);
	return;
} 
EXPORT_SYMBOL(deregister_vector);


static int __init init_vector_control(void){
	int ret = 0;
	pde = proc_create(filename,0,NULL,&vector_proc_fops);
	if(IS_ERR(pde)){
		ret = PTR_ERR(pde);
		goto end;
	}
	if (increase == NULL)
             	increase = increase_ref_count;
	if (reduce == NULL)
                reduce = reduce_ref_count;
	if (get_addr == NULL)
                get_addr = get_vectorid_by_address_head;
	if(get_vect == NULL)
		get_vect = get_vector_head;
	if(get_table == NULL)
		get_table = get_vector_table;
	mutex_init(&vector_list_lock);
end:
	return ret; 
}

static void  __exit exit_vector_control(void)
{
	if (increase != NULL)
                increase = NULL;
        if (reduce != NULL)
                reduce = NULL;
        if (get_addr != NULL)
                get_addr = NULL;
	if(get_vect != NULL)
		get_vect = NULL;
	if(get_table !=NULL)
		get_table = NULL;
	remove_proc_entry(filename,0);
}
module_init(init_vector_control);
module_exit(exit_vector_control);
