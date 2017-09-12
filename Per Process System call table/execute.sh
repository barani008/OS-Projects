make
cd test_progs/
make
cd ./../user_progs/
make
cd ..
rm -f /dev/ioctl_syscall
mknod /dev/ioctl_syscall c 121 212
sh install_module.sh
cd ./user_progs/
./ioctl_control
