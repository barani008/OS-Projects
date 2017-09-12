INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: ioctl_control show_vectors

ioctl_control: ioctl_control.c
	gcc -o ioctl_control ioctl_control.c

show_vectors: show_vectors.c
	gcc -o show_vectors show_vectors.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

