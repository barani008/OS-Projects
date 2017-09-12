EXTRA_CFLAGS += -D EXTRA_CREDIT

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all:	trctl treplay

trctl: trctl.c treplay.h
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi trctl.c -o trctl
	#gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi -D EXTRA_CREDIT trctl.c -o trctl	

treplay: treplay.c treplay.h
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi treplay.c -o treplay

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f trctl treplay

