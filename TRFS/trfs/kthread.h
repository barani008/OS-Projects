#ifndef KTHREAD_H
#define KTHREAD_H

#include "trfs.h"
#include "trace.h"

struct trfs_open_req {
        void* req_record;
        struct file* filp;
        loff_t req_len;
        struct completion done;
        struct list_head kthread_ctl_list;
};

struct trfs_kthread_ctl {
#define TRFS_KTHREAD_ZOMBIE 0x00000001
        u32 flags;
        struct mutex mux;
        struct list_head req_list;
        wait_queue_head_t wait;
};

int trfs_threadfn(void *ctl);
int trfs_init_kthread(struct trfs_sb_info* sb_info);
void trfs_destroy_kthread(struct trfs_sb_info* sb_info);
int trfs_kthread_enqueue(struct trfs_sb_info* sb_info, void * req_record, int req_len);

#endif
