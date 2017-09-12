#!/bin/bash

rm -rf /tmp/tfile
umount /mnt/trfs
rmmod trfs.ko
insmod trfs.ko

