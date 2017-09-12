#ifndef __VECTOR_CONTROL_H_
#define __VECTOR_CONTROL_H_

#define MAX_VECTOR_NAME_LEN 256
#define MAX_BUFFER_SIZE 4096

typedef struct vector_sys_call {
        int syscall_no;
        unsigned long function_ptr;
        struct vector_sys_call* nxt;
}vector_syscall;

typedef struct new_vector {
	unsigned int vector_id;
	char vector_name[MAX_VECTOR_NAME_LEN];
	vector_syscall* vect_syscall_head;
	int ref_count;
	struct module *module;
	struct new_vector *nxt;
}vector;


#endif
