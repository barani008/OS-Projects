#ifndef _TRACE_H_
#define _TRACE_H_

#include "trfs.h"

#define OPEN_FUNC 0
#define READ_FUNC 1
#define WRITE_FUNC 2
#define MKDIR_FUNC 3
#define RMDIR_FUNC 4
#define UNLINK_FUNC 5
#define CREATE_FUNC 6
#define LINK_FUNC 7
#define SYMLINK_FUNC 8
#define CLOSE_FUNC 9

struct record_header{
	unsigned int  rec_id;
	unsigned short rec_size;
	unsigned short rec_type;
};
struct open_trace {
	unsigned int p_id;
	unsigned long f_id;
	unsigned short perm_mode;
	unsigned short path_length;
	unsigned int open_flags;
	int ret_val;
	char path_name[1];
};
struct read_trace {
	unsigned int p_id;
	unsigned long f_id;
	long long buf_len;
	int ret_val;
	char buf[1];
};
struct write_trace {
	unsigned int p_id;
	unsigned long f_id;
	long long buf_len;
	int ret_val;
	char buf[1];
};
struct mkdir_trace {
	unsigned short perm_mode;
	unsigned short path_length;
	int ret_val;
	char path_name[1];
};
struct rmdir_trace {
	unsigned short path_length;
	int ret_val;
	char path_name[1];
};
struct unlink_trace {
	unsigned short path_length;
	int ret_val;
	char path_name[1];
};
struct create_trace {
	unsigned short perm_mode;
	unsigned short path_length;
	int ret_val;
	char path_name[1];
};
struct link_trace {
	unsigned short path_length1;
	unsigned short path_length2;
	int ret_val;
	char path_name[1];
};
struct symlink_trace {
	unsigned short path_length1;
	unsigned short path_length2;
	int ret_val;
	char path_name[1];
};
struct close_trace {
	unsigned int p_id;
	unsigned long f_id;
	int ret_val;
};

#endif 
