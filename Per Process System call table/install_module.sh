#!/bin/sh
set -x
# WARNING: this script doesn't check for errors, so you have to enhance it in case any of the commands
# below fail.
lsmod
rmmod wrap_syscalls_vector.ko
rmmod combo_syscalls_vector.ko
rmmod ioctl_support
rmmod vector_control
insmod vector_control.ko
insmod ioctl_support.ko
insmod combo_syscalls_vector.ko
insmod wrap_syscalls_vector.ko
lsmod
