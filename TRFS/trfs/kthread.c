/**
 * eCryptfs: Linux filesystem encryption layer
 *
 * Copyright (C) 2008 International Business Machines Corp.
 *   Author(s): Michael A. Halcrow <mahalcro@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/mount.h>
#include "trfs.h"
#include "trace.h"
#include "kthread.h"


/**
 * trfs_threadfn
 *
 * The trfs kernel thread that has the responsibility of writing
 * in the trace_file
 *
 * Returns zero on success; non-zero otherwise
 */
int trfs_threadfn(void *ctl)
{
	set_freezable();
	while (1)  {
		struct trfs_open_req *req;
		struct trfs_kthread_ctl* trfs_kthread_ctl = (struct trfs_kthread_ctl*)ctl;
		int rc = 0;

		wait_event_freezable(
			trfs_kthread_ctl->wait,
			(!list_empty(&trfs_kthread_ctl->req_list)
			 || kthread_should_stop()));
		mutex_lock(&trfs_kthread_ctl->mux);
		if (trfs_kthread_ctl->flags & TRFS_KTHREAD_ZOMBIE) {
			mutex_unlock(&trfs_kthread_ctl->mux);
			goto out;
		}
		while (!list_empty(&trfs_kthread_ctl->req_list)) {
			req = list_first_entry(&trfs_kthread_ctl->req_list,
					       struct trfs_open_req,
					       kthread_ctl_list);
			list_del(&req->kthread_ctl_list);
			rc = vfs_write(req->filp, req->req_record, req->req_len,&req->filp->f_pos);
			kfree(req->req_record);
			printk("Written to file %d bytes\n", rc);
			complete(&req->done);
		}
		mutex_unlock(&trfs_kthread_ctl->mux);
	}
out:
	
	return 0;
}

int trfs_init_kthread(struct trfs_sb_info* sb_info)
{
	int rc = 0;
	struct trfs_kthread_ctl *trfs_kthread_ctl = (struct trfs_kthread_ctl*) kmalloc(sizeof(struct trfs_kthread_ctl), 
						GFP_KERNEL);
	struct task_struct *trfs_kthread;
	mutex_init(&trfs_kthread_ctl->mux);
	init_waitqueue_head(&trfs_kthread_ctl->wait);
	INIT_LIST_HEAD(&trfs_kthread_ctl->req_list);
	trfs_kthread = kthread_run(&trfs_threadfn, trfs_kthread_ctl,
				       "trfs-kthread");
	sb_info->ctl = trfs_kthread_ctl;
	sb_info->threadId = trfs_kthread;
	if (IS_ERR(trfs_kthread)) {
		rc = PTR_ERR(trfs_kthread);
		printk(KERN_ERR "%s: Failed to create kernel thread; rc = [%d]"
		       "\n", __func__, rc);
	}
	return rc;
}

void trfs_destroy_kthread(struct trfs_sb_info* sb_info)
{
	struct trfs_open_req *req, *tmp;
	struct trfs_kthread_ctl *trfs_kthread_ctl = sb_info->ctl;
	struct task_struct *trfs_kthread = sb_info->threadId;

	mutex_lock(&trfs_kthread_ctl->mux);
	trfs_kthread_ctl->flags |= TRFS_KTHREAD_ZOMBIE;
	if(sb_info->filp!=NULL)
                filp_close(sb_info->filp, NULL);
	list_for_each_entry_safe(req, tmp, &trfs_kthread_ctl->req_list,
				 kthread_ctl_list) {
		list_del(&req->kthread_ctl_list);
		complete(&req->done);
	}
	mutex_unlock(&trfs_kthread_ctl->mux);
	kthread_stop(trfs_kthread);
	wake_up(&trfs_kthread_ctl->wait);
}

/**
 * trfs_kthread_enqueue
 *
 * This function adds the record to be written into the queue.
 *
 * Returns zero on success; non-zero otherwise
 */
int trfs_kthread_enqueue(struct trfs_sb_info* sb_info, void * req_record, int req_len)
{
	struct trfs_open_req* req = (struct trfs_open_req*) kmalloc(sizeof(struct trfs_open_req), 
						GFP_KERNEL);
	int rc = 0;
	struct trfs_kthread_ctl* trfs_kthread_ctl;
	struct record_header *header = NULL;
	init_completion(&req->done);
	trfs_kthread_ctl = sb_info->ctl;
	mutex_lock(&trfs_kthread_ctl->mux);
	if (trfs_kthread_ctl->flags & TRFS_KTHREAD_ZOMBIE) {
		rc = -EIO;
		mutex_unlock(&trfs_kthread_ctl->mux);
		printk(KERN_ERR "%s: We are in the middle of shutting down; "
		       "aborting request to write to trace file\n",
			__func__);
		goto out;
	}
        req->req_len = req_len;
        req->filp = sb_info->filp;
        header = (struct record_header *)req_record;
        header->rec_id = get_trfs_trace_record_id(sb_info);
        req->req_record = req_record;
        printk("Generated record_id:  %u\n", header->rec_id);
	list_add_tail(&req->kthread_ctl_list, &trfs_kthread_ctl->req_list);
	printk("Added the write request record id : %d\n", header->rec_id);
	mutex_unlock(&trfs_kthread_ctl->mux);
	wake_up(&trfs_kthread_ctl->wait);
	wait_for_completion(&req->done);
out:
	return rc;
}
