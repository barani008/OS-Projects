/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "trfs.h"
#include "trace.h"
#include "kthread.h"
#include <asm/uaccess.h>

#define CHECK_ACCESS(ADDR, SIZE) \
        likely(!__range_not_ok(ADDR, SIZE, user_addr_max()))

static ssize_t trfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	// Lower file operation 
	lower_file = trfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	if (err >= 0){
                fsstack_copy_attr_atime(d_inode(dentry),
                                        file_inode(lower_file));
        }
	printk("*****************************READ CALL STARTS**************************\n");
	printk("read: bitmap:%lx, read bit:%d\n",(get_trfs_trace_bitmap(file->f_inode->i_sb)), 
										(1<<READ_FUNC));
	//preparing struct for passing to Kthread
	if((get_trfs_trace_bitmap(file->f_inode->i_sb)) & (1<<READ_FUNC)){
		unsigned int totalLength = sizeof(struct record_header) + sizeof(struct read_trace)+ err;
		void * data_packet = kmalloc(totalLength, GFP_KERNEL);
		struct record_header * rec_header = (struct record_header *)data_packet;
		struct read_trace * read_rec = (struct read_trace*)(data_packet+sizeof(struct record_header));

		printk("totalLength: %d\n", totalLength);
		printk("buff length: %d\n",  err);

		rec_header->rec_id = 0;
		rec_header->rec_size = totalLength;
		rec_header->rec_type = READ_FUNC;

		read_rec->p_id = current->pid;
		read_rec->f_id = (unsigned long)file;
		/* update our inode atime upon a successful lower read */
		if (err >= 0){
			read_rec->buf_len = err;
			strcpy(read_rec->buf, buf);
			strncpy(read_rec->buf + read_rec->buf_len,"\0",1);
		}else{
			strncpy(read_rec->buf,"\0",1);
			read_rec->buf_len = 0;
		}
		read_rec->ret_val = err;

		printk("rec_header->rec_size : %d\n", rec_header->rec_size);
		printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("read_rec->p_id : %d\n",read_rec->p_id);
		printk("file pointer: %p\n",file);
		printk("read_rec->f_id : %lu\n",read_rec->f_id);
		printk("read_rec->buf_len : %lld\n",read_rec->buf_len);
		printk("buf : %s\n", read_rec->buf);
		printk("read_rec->ret_val : %d\n", read_rec->ret_val);
		trfs_kthread_enqueue(TRFS_SB(file->f_inode->i_sb), (void *)data_packet, totalLength);
		 printk("*****************************READ CALL ENDS**************************\n");
	}
	return err;
}

static ssize_t trfs_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	
	lower_file = trfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
        if (err >= 0) {
                fsstack_copy_inode_size(d_inode(dentry),
                                        file_inode(lower_file));
                fsstack_copy_attr_times(d_inode(dentry),
                                        file_inode(lower_file));
        }
	printk("*****************************WRITE CALL STARTS**************************\n");
	printk("write: bitmap:%lx, write bit:%d\n",(get_trfs_trace_bitmap(file->f_inode->i_sb)), 
										(1<<WRITE_FUNC));
	//preparing struct for passing to Kthread
	if((get_trfs_trace_bitmap(file->f_inode->i_sb)) & (1<<WRITE_FUNC)){
		unsigned int totalLength = sizeof(struct record_header) + sizeof(struct write_trace)+ count;
		void *data_packet = kmalloc(totalLength, GFP_KERNEL);
		struct record_header * rec_header = (struct record_header *)data_packet;
		struct write_trace * write_rec = (struct write_trace*)(data_packet+sizeof(struct record_header));
		printk("totalLength: %d\n", totalLength);
		printk("buf length: %d\n",  err);

		rec_header->rec_id = 0;
		rec_header->rec_size = totalLength;
		rec_header->rec_type = WRITE_FUNC;

		write_rec->p_id = current->pid;
		write_rec->f_id = (unsigned long)file;
		write_rec->buf_len = count;
		strcpy(write_rec->buf, buf);
		strncpy(write_rec->buf + write_rec->buf_len,"\0",1);
		write_rec->ret_val = err;

		printk("rec_header->rec_size : %d\n", rec_header->rec_size);
		printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("write_rec->p_id : %d\n",write_rec->p_id);
		printk("file pointer: %p\n",file);
		printk("write_rec->f_id : %lu\n",write_rec->f_id);
		printk("write_rec->buf_len : %lld\n",write_rec->buf_len);
		printk("buf : %s\n", write_rec->buf);
		printk("write_rec->ret_val : %d\n", write_rec->ret_val);
		trfs_kthread_enqueue(TRFS_SB(file->f_inode->i_sb), (void *)data_packet, totalLength);
		 printk("*****************************WRITE CALL ENDS**************************\n");
	}
	return err;
}

static int trfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	printk("trfs_readdir %s\n", dentry->d_iname);
	lower_file = trfs_lower_file(file);
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	return err;
}

static long trfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	lower_file = trfs_lower_file(file);
	if(cmd == TRFS_IOC_GETRSVSZ || cmd == TRFS_IOC_SETRSVSZ){
		if(CHECK_ACCESS((unsigned long*) arg, sizeof(unsigned long*))==0){
			printk("Invalid memory address passed \n");
			err = -EFAULT;
			goto out;
		}else{
			unsigned long result=0;
			struct super_block * sb = file->f_path.dentry->d_sb; 
			if(cmd == TRFS_IOC_GETRSVSZ){
				result = get_trfs_trace_bitmap(sb);
				printk("result %lu \n",result);
				if(copy_to_user((unsigned long*)arg, &result, sizeof(result))!=0){
					printk("Not all bytes were written %lu\n", result);
				}
				return 0;
			}
			else{
				unsigned long bitmap = *(unsigned long*) arg;
				printk("Setting value for Bitmap %lu \n", bitmap);
				set_trfs_trace_bitmap(sb, bitmap);
				return 0;
			}
		}
	}
	printk("trfs_unlocked_ioctl %s\n", file->f_path.dentry->d_iname);
	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

	/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
	if (!err)
		fsstack_copy_attr_all(file_inode(file),
				      file_inode(lower_file));
out:
	return err;
}

#ifdef CONFIG_COMPAT
static long trfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = trfs_lower_file(file);
	printk("trfs_compat_ioctl %s\n", file->f_path.dentry->d_iname);
	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int trfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = trfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "trfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!TRFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "trfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &trfs_vm_ops;

	file->f_mapping->a_ops = &trfs_aops; /* set our aops */
	if (!TRFS_F(file)->lower_vm_ops) /* save for our ->fault */
		TRFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}

static int trfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;

	/* don't open unhashed/deleted files */
        if (d_unhashed(file->f_path.dentry)) {
                err = -ENOENT;
                goto out_err;
        }

        file->private_data =
                kzalloc(sizeof(struct trfs_file_info), GFP_KERNEL);
        if (!TRFS_F(file)) {
                err = -ENOMEM;
                goto out_err;
        }

        /* open lower object and link trfs's file struct to lower's */
        trfs_get_lower_path(file->f_path.dentry, &lower_path);
        lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
        path_put(&lower_path);
        if (IS_ERR(lower_file)) {
                err = PTR_ERR(lower_file);
                lower_file = trfs_lower_file(file);
                if (lower_file) {
                        trfs_set_lower_file(file, NULL);
                        fput(lower_file); /* fput calls dput for lower_dentry */
                }
        } else {
                trfs_set_lower_file(file, lower_file);
        }

        if (err)
                kfree(TRFS_F(file));
        else
                fsstack_copy_attr_all(inode, trfs_lower_inode(inode));
out_err:
	printk("*****************************OPEN CALL STARTS**************************\n");
	printk("open: bitmap:%lx, open bit:%d\n",(get_trfs_trace_bitmap(file->f_inode->i_sb)), 
										(1<<OPEN_FUNC));
	//preparing struct for passing to Kthread
	if((get_trfs_trace_bitmap(file->f_inode->i_sb)) & (1<<OPEN_FUNC)){
		unsigned int totalLength = 0;
		void *data_packet = NULL;
		struct record_header * rec_header= NULL;
		struct open_trace * open_rec = NULL;
		char* full_path = NULL;
                char* end = NULL;
                int path_len = 0;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(file->f_path.dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
                path_len = strlen(end)+1;				//for mentioning the current directory
		totalLength = sizeof(struct record_header) + sizeof(struct open_trace) 
							+ path_len;	
		data_packet = kmalloc(totalLength, GFP_KERNEL);
		if(data_packet == NULL){
			printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
		}
		rec_header = (struct record_header *)data_packet;
		open_rec = (struct open_trace*)(data_packet+sizeof(struct record_header));
	
		printk("full path : %s\n", end);
		printk("totalLength: %u\n", totalLength);
		printk("path length: %d\n",  path_len);

		rec_header->rec_id = 0;
		rec_header->rec_size = totalLength;
		rec_header->rec_type = OPEN_FUNC;

		open_rec->p_id = current->pid;
		open_rec->f_id = (unsigned long)file;
		open_rec->perm_mode = inode->i_mode;
		open_rec->open_flags = file->f_flags;
		open_rec->path_length = path_len;
		strncpy(open_rec->path_name, ".", 1);
		strcpy(open_rec->path_name+1, end);

		printk("rec_header->rec_size : %d\n", rec_header->rec_size);
		printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("open_rec->p_id : %d\n",open_rec->p_id);
		printk("file pointer: %p\n",file);
		printk("open_rec->f_id : %lu\n",open_rec->f_id);
		printk("open_rec->perm_mode : %u\n", open_rec->perm_mode);
		printk("open_rec->open_flags : %d\n",  open_rec->open_flags);
		printk("open_rec->path_length : %d\n",open_rec->path_length);
		printk("path : %s\n", open_rec->path_name);
		open_rec->ret_val = err;
                printk("open_rec->ret_val : %d\n", open_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(inode->i_sb), (void *)data_packet, totalLength);
out_exit:	if(full_path!=NULL)
                	kfree(full_path);
		printk("*****************************OPEN CALL ENDS**************************\n");
	}
	return err;
}

static int trfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int trfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = trfs_lower_file(file);
	if (lower_file) {
		trfs_set_lower_file(file, NULL);
		fput(lower_file);
	}
	
	printk("****************************CLOSE CALL STARTS**************************\n");
        printk("close: bitmap:%lx, close bit:%d\n",(get_trfs_trace_bitmap(file->f_inode->i_sb)), 
										(1<<CLOSE_FUNC));
        //preparing struct for passing to Kthread
        if((get_trfs_trace_bitmap(file->f_inode->i_sb)) & (1<<CLOSE_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct close_trace * close_rec = NULL;
		totalLength = sizeof(struct record_header) + sizeof(struct close_trace);
                data_packet = kmalloc(totalLength, GFP_KERNEL);
                if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                close_rec = (struct close_trace*)(data_packet+sizeof(struct record_header));

                printk("totalLength: %u\n", totalLength);

                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = CLOSE_FUNC;

                close_rec->p_id = current->pid;
                close_rec->f_id = (unsigned long)file;
		
                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
                printk("close_rec->p_id : %d\n",close_rec->p_id);
                printk("file pointer: %p\n",file);
                printk("close_rec->f_id : %lu\n",close_rec->f_id);
                close_rec->ret_val = 0;
                printk("close_rec->ret_val : %d\n", close_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(inode->i_sb), (void *)data_packet, totalLength);
                printk("*****************************CLOSE CALL ENDS**************************\n");
        }
out_exit:
	kfree(TRFS_F(file));
	return 0;
}

static int trfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = trfs_lower_file(file);
	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	trfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int trfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * TRfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t trfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = trfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * TRfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
trfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	return err;
}

/*
 * TRfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
trfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
					file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations trfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= trfs_read,
	.write		= trfs_write,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.mmap		= trfs_mmap,
	.open		= trfs_open,
	.flush		= trfs_flush,
	.release	= trfs_file_release,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
	.read_iter	= trfs_read_iter,
	.write_iter	= trfs_write_iter,
};

/* trimmed directory options */
const struct file_operations trfs_dir_fops = {
	.llseek		= trfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= trfs_readdir,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.open		= trfs_open,
	.release	= trfs_file_release,
	.flush		= trfs_flush,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
};
