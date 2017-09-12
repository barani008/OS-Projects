#ifndef IOCTL_SUPPORT_H
#define IOCTL_SUPPORT_H

#include <linux/ioctl.h>
#define DEVICE_NUM 121

#define IOCTL_SET_VECTOR _IOR(DEVICE_NUM, 1, unsigned long)
#define IOCTL_REMOVE _IOR(DEVICE_NUM, 2, unsigned long)
#define IOCTL_GET_VECTOR _IOR(DEVICE_NUM, 3, unsigned long)

struct ioctl_params{
	int pid;
	int vid;
};

#endif
