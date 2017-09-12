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

static int trfs_create(struct inode *dir, struct dentry *dentry,
			 umode_t mode, bool want_excl)
{

	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_create(d_inode(lower_parent_dentry), lower_dentry, mode,
			 want_excl);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	printk("***************CREATE OPERATION STARTS*****************\n");
	printk("create: current bitmap:%lx, create bit: %d\n",(get_trfs_trace_bitmap(dentry->d_sb)),
                                                                         (1<<CREATE_FUNC));
        if((get_trfs_trace_bitmap(dentry->d_sb)) & (1<<CREATE_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct create_trace * create_rec = NULL;
                char* full_path = NULL;
                char* end = NULL;
                int path_len = 0;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
                path_len = strlen(end)+1;
                totalLength = sizeof(struct record_header) + sizeof(struct create_trace)
                                                        + path_len;
                data_packet = kmalloc(totalLength, GFP_KERNEL);
                if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                create_rec = (struct create_trace*)(data_packet+sizeof(struct record_header));

                printk("full path : %s\n", end);
                printk("totalLength: %u\n", totalLength);
                printk("path length: %d\n",  path_len);
                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = CREATE_FUNC;
		
		printk("Perm Mode: %d\n", mode);
		create_rec->perm_mode = mode;
                create_rec->path_length = path_len;
                create_rec->ret_val = err;
                strncpy(create_rec->path_name,".",1);
                strcpy(create_rec->path_name+1, end);

                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("create_rec->perm mode : %d\n", create_rec->perm_mode);
                printk("path : %s\n", create_rec->path_name);
                printk("create_rec->ret_val : %d\n", create_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(dentry->d_sb), (void *)data_packet, totalLength);
                printk("***************CREATE OPERATION ENDS*****************\n");
out_exit:       if(full_path!=NULL)
                        kfree(full_path);
        }
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_link(struct dentry *old_dentry, struct inode *dir,
		       struct dentry *new_dentry)
{
	
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_dir_dentry;
	u64 file_size_save;
	int err;
	struct path lower_old_path, lower_new_path;
	file_size_save = i_size_read(d_inode(old_dentry));
	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_dir_dentry = lock_parent(lower_new_dentry);

	err = vfs_link(lower_old_dentry, d_inode(lower_dir_dentry),
		       lower_new_dentry, NULL);
	if (err || !d_inode(lower_new_dentry))
		goto out;

	err = trfs_interpose(new_dentry, dir->i_sb, &lower_new_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, d_inode(lower_new_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_new_dentry));
	set_nlink(d_inode(old_dentry),
		  trfs_lower_inode(d_inode(old_dentry))->i_nlink);
	i_size_write(d_inode(new_dentry), file_size_save);
out:
	printk("***************LINK OPERATION STARTS*****************\n");
	printk("link: current bitmap:%lx, link bit: %d\n",(get_trfs_trace_bitmap(old_dentry->d_sb)),
                                                                         (1<<LINK_FUNC));
        if((get_trfs_trace_bitmap(old_dentry->d_sb)) & (1<<LINK_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct link_trace * link_rec = NULL;
                char* full_path1 = NULL, *full_path2 = NULL;
                char* end1 = NULL, *end2 = NULL;
                int path_len1 = 0, path_len2 = 0;
                full_path1 = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path1 == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end1 = dentry_path_raw(old_dentry, full_path1, PATH_MAX);
                if(IS_ERR(end1)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end1);
                        goto out_exit;
                }
                path_len1 = strlen(end1)+1;
		full_path2 = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
		if(full_path2 == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end2 = dentry_path_raw(new_dentry, full_path2, PATH_MAX);
                if(IS_ERR(end2)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end2);
                        goto out_exit;
                }
                path_len2 = strlen(end2)+1;
                totalLength = sizeof(struct record_header) + sizeof(struct link_trace)
                                                        + path_len1 + path_len2 + 2;
                data_packet = kmalloc(totalLength, GFP_KERNEL);
		if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                link_rec = (struct link_trace*)(data_packet+sizeof(struct record_header));

                printk("full path1 : %s\n", end1);
		printk("full path2 : %s\n", end2);
                printk("totalLength: %u\n", totalLength);
                printk("path length: %d\n",  path_len1);
		printk("path length: %d\n",  path_len2);

                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = LINK_FUNC;

                link_rec->path_length1 = path_len1+1;
		link_rec->path_length2 = path_len2+1;
                link_rec->ret_val = err;
                strncpy(link_rec->path_name,".",1);
                strncpy(link_rec->path_name+1, end1, path_len1);
		strncpy(link_rec->path_name+path_len1+1,".",1);
                strcpy(link_rec->path_name+path_len1+2, end2);
	
                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("path1 : %s\n", link_rec->path_name);
                printk("path2 : %s\n", link_rec->path_name+link_rec->path_length1+1);
                printk("create_rec->ret_val : %d\n", link_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(old_dentry->d_sb), (void *)data_packet, totalLength);
                printk("***************LINK OPERATION ENDS*****************\n");
out_exit:       
		if(full_path1!=NULL)
                        kfree(full_path1);
		if(full_path2!=NULL)
			kfree(full_path2);
        }
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int trfs_unlink(struct inode *dir, struct dentry *dentry)
{
	
	int err;
	struct dentry *lower_dentry;
	struct inode *lower_dir_inode = trfs_lower_inode(dir);
	struct dentry *lower_dir_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_unlink(lower_dir_inode, lower_dentry, NULL);

	/*
	 * Note: unlinking on top of NFS can cause silly-renamed files.
	 * Trying to delete such files results in EBUSY from NFS
	 * below.  Silly-renamed files will get deleted by NFS later on, so
	 * we just need to detect them here and treat such EBUSY errors as
	 * if the upper file was successfully deleted.
	 */
	if (err == -EBUSY && lower_dentry->d_flags & DCACHE_NFSFS_RENAMED)
		err = 0;
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_inode);
	fsstack_copy_inode_size(dir, lower_dir_inode);
	set_nlink(d_inode(dentry),
		  trfs_lower_inode(d_inode(dentry))->i_nlink);
	d_inode(dentry)->i_ctime = dir->i_ctime;
	d_drop(dentry); /* this is needed, else LTP fails (VFS won't do it) */
out:
	printk("***************UNLINK OPERATION STARTS*****************\n");
        printk("unlink: current bitmap:%lx, unlink bit: %d\n",(get_trfs_trace_bitmap(dentry->d_sb)),
									 (1<<UNLINK_FUNC));
        if((get_trfs_trace_bitmap(dentry->d_sb)) & (1<<UNLINK_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct unlink_trace * unlink_rec = NULL;
                char* full_path = NULL;
                char* end = NULL;
                int path_len = 0;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
                path_len = strlen(end)+1;
                totalLength = sizeof(struct record_header) + sizeof(struct unlink_trace)
                                                        + path_len;
                data_packet = kmalloc(totalLength, GFP_KERNEL);
		if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                unlink_rec = (struct unlink_trace*)(data_packet+sizeof(struct record_header));

                printk("full path : %s\n", end);
                printk("totalLength: %u\n", totalLength);
                printk("path length: %d\n",  path_len);
                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = UNLINK_FUNC;

                unlink_rec->path_length = path_len;
                unlink_rec->ret_val = err;
                strncpy(unlink_rec->path_name,".",1);
                strcpy(unlink_rec->path_name+1, end);

                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
                printk("path : %s\n", unlink_rec->path_name);
                printk("unlink_rec->ret_val : %d\n", unlink_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(dentry->d_sb), (void *)data_packet, totalLength);
                printk("***************UNLINK OPERATION ENDS*****************\n");
out_exit:       if(full_path!=NULL)
                        kfree(full_path);
        }
	unlock_dir(lower_dir_dentry);
	dput(lower_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_symlink(struct inode *dir, struct dentry *dentry,
			  const char *symname)
{
	
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_symlink(d_inode(lower_parent_dentry), lower_dentry, symname);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	printk("***************SYMLINK OPERATION STARTS*****************\n");
        printk("SYMLINK: bitmap:%lx, symlink bit: %d\n",(get_trfs_trace_bitmap(dentry->d_sb)),
									 (1<<SYMLINK_FUNC));
        if((get_trfs_trace_bitmap(dentry->d_sb)) & (1<<SYMLINK_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct symlink_trace * symlink_rec = NULL;
                char* full_path = NULL;
                char* end = NULL;
                int path_len2 = 0, path_len1 = strlen(symname)+1;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
                path_len2 = strlen(end)+1;
                totalLength = sizeof(struct record_header) + sizeof(struct symlink_trace)
                                                        + path_len1 + path_len2 + 2;
                data_packet = kmalloc(totalLength, GFP_KERNEL);
                if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                symlink_rec = (struct symlink_trace*)(data_packet+sizeof(struct record_header));

                printk("source path : %s\n", symname);
                printk("dest path : %s\n", end);
                printk("totalLength: %u\n", totalLength);
                printk("path length: %d\n",  path_len1);
                printk("path length: %d\n",  path_len2);

                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = SYMLINK_FUNC;

                symlink_rec->path_length1 = path_len1+1;
                symlink_rec->path_length2 = path_len2+1;
                symlink_rec->ret_val = err;
                strncpy(symlink_rec->path_name,".",1);
                strncpy(symlink_rec->path_name+1, symname, path_len1);
		strncpy(symlink_rec->path_name+path_len1+1,".",1);
                strcpy(symlink_rec->path_name+path_len1+2, end);

                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
                printk("path1 : %s\n", symlink_rec->path_name);
                printk("path2 : %s\n", symlink_rec->path_name+symlink_rec->path_length1+1);
                printk("create_rec->ret_val : %d\n", symlink_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(dentry->d_sb), (void *)data_packet, totalLength);
                printk("***************SYMLINK OPERATION ENDS*****************\n");
out_exit:
                if(full_path!=NULL)
                        kfree(full_path);
        }

	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{

	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mkdir(d_inode(lower_parent_dentry), lower_dentry, mode);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;

	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));
	/* update number of links on parent directory */
	set_nlink(dir, trfs_lower_inode(dir)->i_nlink);

out:
	printk("***************MKDIR OPERATION STARTS*****************\n");
	printk("mkdir: bitmap:%lx, mkdir bit:%d\n",(get_trfs_trace_bitmap(dentry->d_sb)),
									 (1<<MKDIR_FUNC));
        //preparing struct for passing to Kthread
        if((get_trfs_trace_bitmap(dentry->d_sb)) & (1<<MKDIR_FUNC)){
                unsigned int totalLength = 0;
                void *data_packet = NULL;
                struct record_header * rec_header= NULL;
                struct mkdir_trace * mkdir_rec = NULL;
		char* full_path = NULL;
                char* end = NULL;
                int path_len = 0;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
                path_len = strlen(end)+1;
                totalLength = sizeof(struct record_header) + sizeof(struct mkdir_trace)
                                                        + path_len;
                data_packet = kmalloc(totalLength, GFP_KERNEL);
                if(data_packet == NULL){
                        printk("Unable to allocate memory\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                rec_header = (struct record_header *)data_packet;
                mkdir_rec = (struct mkdir_trace*)(data_packet+sizeof(struct record_header));

                printk("full path : %s\n", end);
                printk("totalLength: %u\n", totalLength);
                printk("path length: %d\n",  path_len);
                rec_header->rec_id = 0;
                rec_header->rec_size = totalLength;
                rec_header->rec_type = MKDIR_FUNC;

                mkdir_rec->perm_mode = mode;
                mkdir_rec->path_length = path_len;
		mkdir_rec->ret_val = err;
		strncpy(mkdir_rec->path_name,".",1);
                strcpy(mkdir_rec->path_name+1, end);

                printk("rec_header->rec_size : %d\n", rec_header->rec_size);
                printk("rec_header->rec_type : %d\n", rec_header->rec_type);
                printk("mkdir_rec->perm_mode : %u\n", mkdir_rec->perm_mode);
                printk("path : %s\n", mkdir_rec->path_name);
                printk("mkdir_rec->ret_val : %d\n", mkdir_rec->ret_val);
                trfs_kthread_enqueue(TRFS_SB(dentry->d_sb), (void *)data_packet, totalLength);
		printk("***************MKDIR OPERATION ENDS*****************\n");
out_exit:       if(full_path!=NULL)
                        kfree(full_path);
        }	
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_rmdir(struct inode *dir, struct dentry *dentry)
{

	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
	int err;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_rmdir(d_inode(lower_dir_dentry), lower_dentry);
	if (err)
		goto out;

	d_drop(dentry);	/* drop our dentry on success (why not VFS's job?) */
	if (d_inode(dentry))
		clear_nlink(d_inode(dentry));
	fsstack_copy_attr_times(dir, d_inode(lower_dir_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_dir_dentry));
	set_nlink(dir, d_inode(lower_dir_dentry)->i_nlink);

out:
	printk("***************RMDIR OPERATION STARTS*****************\n");
	printk("rmdir: bitmap:%lx, rmdir bit:%d\n",(get_trfs_trace_bitmap(dentry->d_sb)), 
									(1<<RMDIR_FUNC));
	if((get_trfs_trace_bitmap(dentry->d_sb)) & (1<<RMDIR_FUNC)){
		unsigned int totalLength = 0;
		void *data_packet = NULL;
		struct record_header * rec_header= NULL;
		struct rmdir_trace * rmdir_rec = NULL;
		char* full_path = NULL;
                char* end = NULL;
                int path_len = 0;
                full_path = (char *)kmalloc(PATH_MAX, GFP_KERNEL);
                if(full_path == NULL){
                        printk("Unable to allocate memory for path name\n");
                        err = -ENOMEM;
                        goto out_exit;
                }
                end = dentry_path_raw(dentry, full_path, PATH_MAX);
                if(IS_ERR(end)){
                        printk("Unable to generate full path\n");
                        err = PTR_ERR(end);
                        goto out_exit;
                }
		path_len = strlen(end)+1;
		totalLength = sizeof(struct record_header) + sizeof(struct rmdir_trace)	+ path_len;
		data_packet = kmalloc(totalLength, GFP_KERNEL);
		if(data_packet == NULL){
			printk("Unable to allocate memory\n");
			if(full_path == NULL)	
				kfree(full_path);
			err = -ENOMEM;
			goto out_exit;
		}
		rec_header = (struct record_header *)data_packet;
		rmdir_rec = (struct rmdir_trace*)(data_packet+sizeof(struct record_header));

		printk("full path : %s\n", end);
		printk("totalLength: %u\n", totalLength);
		rec_header->rec_id = 0;
		rec_header->rec_size = totalLength;
		rec_header->rec_type = RMDIR_FUNC;

		rmdir_rec->path_length = path_len;
		rmdir_rec->ret_val = err;
		strncpy(rmdir_rec->path_name,".",1);
		strcpy(rmdir_rec->path_name+1, end);
		printk("rec_header->rec_size : %d\n", rec_header->rec_size);
		printk("rec_header->rec_type : %d\n", rec_header->rec_type);
		printk("path : %s\n", rmdir_rec->path_name);
		printk("rmdir_rec->ret_val : %d\n", rmdir_rec->ret_val);
		printk("rmdir path length: %d\n",  path_len);
		trfs_kthread_enqueue(TRFS_SB(dentry->d_sb), (void *)data_packet, totalLength);
		printk("***************RMDIR OPERATION ENDS*****************\n");
out_exit:	if(full_path!=NULL)
                	kfree(full_path);
	}
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
			dev_t dev)
{
	
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mknod(d_inode(lower_parent_dentry), lower_dentry, mode, dev);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

/*
 * The locking rules in trfs_rename are complex.  We could use a simpler
 * superblock-level name-space lock for renames and copy-ups.
 */
static int trfs_rename(struct inode *old_dir, struct dentry *old_dentry,
			 struct inode *new_dir, struct dentry *new_dentry)
{
	
	int err = 0;
	struct dentry *lower_old_dentry = NULL;
	struct dentry *lower_new_dentry = NULL;
	struct dentry *lower_old_dir_dentry = NULL;
	struct dentry *lower_new_dir_dentry = NULL;
	struct dentry *trap = NULL;
	struct path lower_old_path, lower_new_path;
	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_old_dir_dentry = dget_parent(lower_old_dentry);
	lower_new_dir_dentry = dget_parent(lower_new_dentry);

	trap = lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	/* source should not be ancestor of target */
	if (trap == lower_old_dentry) {
		err = -EINVAL;
		goto out;
	}
	/* target should not be ancestor of source */
	if (trap == lower_new_dentry) {
		err = -ENOTEMPTY;
		goto out;
	}

	err = vfs_rename(d_inode(lower_old_dir_dentry), lower_old_dentry,
			 d_inode(lower_new_dir_dentry), lower_new_dentry,
			 NULL, 0);
	if (err)
		goto out;

	fsstack_copy_attr_all(new_dir, d_inode(lower_new_dir_dentry));
	fsstack_copy_inode_size(new_dir, d_inode(lower_new_dir_dentry));
	if (new_dir != old_dir) {
		fsstack_copy_attr_all(old_dir,
				      d_inode(lower_old_dir_dentry));
		fsstack_copy_inode_size(old_dir,
					d_inode(lower_old_dir_dentry));
	}

out:
	unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	dput(lower_old_dir_dentry);
	dput(lower_new_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int trfs_readlink(struct dentry *dentry, char __user *buf, int bufsiz)
{
	
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->readlink) {
		err = -EINVAL;
		goto out;
	}

	err = d_inode(lower_dentry)->i_op->readlink(lower_dentry,
						    buf, bufsiz);
	if (err < 0)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry), d_inode(lower_dentry));

out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static const char *trfs_get_link(struct dentry *dentry, struct inode *inode,
				   struct delayed_call *done)
{
	
	char *buf;
	int len = PAGE_SIZE, err;
	mm_segment_t old_fs;
	if (!dentry)
		return ERR_PTR(-ECHILD);

	/* This is freed by the put_link method assuming a successful call. */
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		return buf;
	}

	/* read the symlink, and then we will follow it */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = trfs_readlink(dentry, buf, len);
	set_fs(old_fs);
	if (err < 0) {
		kfree(buf);
		buf = ERR_PTR(err);
	} else {
		buf[err] = '\0';
	}
	set_delayed_call(done, kfree_link, buf);
	return buf;
}

static int trfs_permission(struct inode *inode, int mask)
{
	
	struct inode *lower_inode;
	int err;
	//printk("in trfs_permission %lu\n", inode->i_ino);
	lower_inode = trfs_lower_inode(inode);
	err = inode_permission(lower_inode, mask);
	return err;
}

static int trfs_setattr(struct dentry *dentry, struct iattr *ia)
{

	int err;
	struct dentry *lower_dentry;
	struct inode *inode;
	struct inode *lower_inode;
	struct path lower_path;
	struct iattr lower_ia;

	inode = d_inode(dentry);

	/*
	 * Check if user has permission to change inode.  We don't check if
	 * this user can change the lower inode: that should happen when
	 * calling notify_change on the lower inode.
	 */
	err = inode_change_ok(inode, ia);
	if (err)
		goto out_err;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_inode = trfs_lower_inode(inode);

	/* prepare our own lower struct iattr (with the lower file) */
	memcpy(&lower_ia, ia, sizeof(lower_ia));
	if (ia->ia_valid & ATTR_FILE)
		lower_ia.ia_file = trfs_lower_file(ia->ia_file);

	/*
	 * If shrinking, first truncate upper level to cancel writing dirty
	 * pages beyond the new eof; and also if its' maxbytes is more
	 * limiting (fail with -EFBIG before making any change to the lower
	 * level).  There is no need to vmtruncate the upper level
	 * afterwards in the other cases: we fsstack_copy_inode_size from
	 * the lower level.
	 */
	if (ia->ia_valid & ATTR_SIZE) {
		err = inode_newsize_ok(inode, ia->ia_size);
		if (err)
			goto out;
		truncate_setsize(inode, ia->ia_size);
	}

	/*
	 * mode change is for clearing setuid/setgid bits. Allow lower fs
	 * to interpret this in its own way.
	 */
	if (lower_ia.ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID))
		lower_ia.ia_valid &= ~ATTR_MODE;

	/* notify the (possibly copied-up) lower inode */
	/*
	 * Note: we use d_inode(lower_dentry), because lower_inode may be
	 * unlinked (no inode->i_sb and i_ino==0.  This happens if someone
	 * tries to open(), unlink(), then ftruncate() a file.
	 */
	inode_lock(d_inode(lower_dentry));
	err = notify_change(lower_dentry, &lower_ia, /* note: lower_ia */
			    NULL);
	inode_unlock(d_inode(lower_dentry));
	if (err)
		goto out;

	/* get attributes from the lower inode */
	fsstack_copy_attr_all(inode, lower_inode);
	/*
	 * Not running fsstack_copy_inode_size(inode, lower_inode), because
	 * VFS should update our inode size, and notify_change on
	 * lower_inode should update its size.
	 */

out:
	trfs_put_lower_path(dentry, &lower_path);
out_err:
	return err;
}

static int trfs_getattr(struct vfsmount *mnt, struct dentry *dentry,
			  struct kstat *stat)
{
	
	int err;
	struct kstat lower_stat;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_getattr(&lower_path, &lower_stat);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
	generic_fillattr(d_inode(dentry), stat);
	stat->blocks = lower_stat.blocks;
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		size_t size, int flags)
{

	int err; struct dentry *lower_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->setxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_setxattr(lower_dentry, name, value, size, flags);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t
trfs_getxattr(struct dentry *dentry, const char *name, void *buffer,
		size_t size)
{
	
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->getxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_getxattr(lower_dentry, name, buffer, size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t
trfs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{

	int err;
	struct dentry *lower_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->listxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_listxattr(lower_dentry, buffer, buffer_size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_removexattr(struct dentry *dentry, const char *name)
{

	int err;
	struct dentry *lower_dentry;
	struct path lower_path;
	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->removexattr) {
		err = -EINVAL;
		goto out;
	}
	err = vfs_removexattr(lower_dentry, name);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}
const struct inode_operations trfs_symlink_iops = {
	.readlink	= trfs_readlink,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.get_link	= trfs_get_link,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_dir_iops = {
	.create		= trfs_create,
	.lookup		= trfs_lookup,
	.link		= trfs_link,
	.unlink		= trfs_unlink,
	.symlink	= trfs_symlink,
	.mkdir		= trfs_mkdir,
	.rmdir		= trfs_rmdir,
	.mknod		= trfs_mknod,
	.rename		= trfs_rename,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_main_iops = {
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};
