TRFS_VERSION="0.1"

EXTRA_CFLAGS += -DTRFS_VERSION=\"$(TRFS_VERSION)\"

obj-m += trfs.o

trfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o kthread.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: trfs

trfs:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

