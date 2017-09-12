cd combo_syscalls_vector
rmmod combo_syscalls_vector.ko
cd ..
rm -f /dev/ioctl_syscall
mknod /dev/ioctl_syscall c 121 212
make clean;make
sh install_module.sh
cp Module.symvers combo_syscalls_vector/
cd combo_syscalls_vector
make
sh install_module.sh
cd ../user_progs
gcc -o ioctl_control ioctl_control.c
./ioctl_control

