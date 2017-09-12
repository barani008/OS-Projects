obj-m += vector_control.o
obj-m += ioctl_support.o
obj-m += combo_syscalls_vector.o
obj-m += wrap_syscalls_vector.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: vector_control ioctl_support xcombo_syscalls_vector wrap_syscalls_vector

ioctl_support:
	make  -Wall -Werror -C /lib/modules/$(shell uname -r)/build -I/usr/src/hw3-cse506p32/include M=$(PWD) modules

vector_control:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build -I/usr/src/hw3-cse506p32/include M=$(PWD) modules

xcombo_syscalls_vector:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build -I/usr/src/hw3-cse506p32/include M=$(PWD) modules

wrap_syscalls_vector:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build -I/usr/src/hw3-cse506p32/include M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
