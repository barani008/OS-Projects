#ifndef TREPLAY_H
#define TREPLAY_H

#define OPEN 0 
#define READ 1
#define WRITE 2
#define MKDIR 3
#define RMDIR 4
#define UNLINK 5
#define CREATE 6
#define LINK 7
#define SYMLINK 8
#define CLOSE 9

typedef struct record_header{
	unsigned int  rec_id;
	unsigned short rec_size;
	unsigned short rec_type;
}rheader_trace;

typedef struct open_trace {
	unsigned int p_id;
	unsigned long f_id;
	unsigned short perm_mode;
	unsigned short path_length;
	unsigned int open_flags;
	int ret_val;
	char path_name[1];
}ostruct_trace;

typedef struct read_trace {
	unsigned int p_id;
	unsigned long f_id;
	long long buf_len;
	int ret_val;
	char  buf[1];
}rstruct_trace;

typedef struct symlink_trace {
	unsigned short path_length1;
	unsigned short path_length2;
	int ret_val;
	char path_name[1];
}sstruct_trace;

typedef struct link_trace {
	unsigned short path_length1;
	unsigned short path_length2;
	int ret_val;
	char path_name[1];
}lstruct_trace;

typedef struct unlink_trace {
	unsigned short path_length;
	int ret_val;
	char path_name[1];
}ustruct_trace;

typedef struct write_trace {
	unsigned int p_id;
	unsigned long f_id;
	long long buf_len;
	int ret_val;
	char  buf[1];
}wstruct_trace;

typedef struct close_trace {
	unsigned int p_id;
	unsigned long f_id;
	int ret_val;
}cstruct_trace;

typedef struct make_dir {
	unsigned short perm_mode;
	unsigned short path_length;
	int ret_val;
	char path_name[1];
}mkstruct_trace;

typedef struct rem_dir {
	unsigned short path_length;
	int ret_val;
	char path_name[1];
}rmstruct_trace;

typedef struct creat_e {
	unsigned short perm_mode;
	unsigned short path_length;
	unsigned int ret_val;
	char path_name[1];
}crstruct_trace;

#endif
