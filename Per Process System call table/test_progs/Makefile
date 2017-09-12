INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: test_combo test_256 test_wrapper test_clone test_new_clone 

test_clone: test_clone.c	
	gcc -Wall -o test_clone test_clone.c

test_combo: test_combo.c
	gcc -Wall -o test_combo test_combo.c

test_256: test_256.c
	gcc -Wall -o test_256 test_256.c

test_wrapper: test_wrapper.c
	gcc -Wall -o test_wrapper test_wrapper.c

test_new_clone: test_new_clone.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi test_new_clone.c -o test_new_clone 
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
