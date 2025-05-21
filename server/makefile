CROSS_COMPILE=
GCC=gcc
.PHONY: all clean

all: aesdsocket.o
			$(GCC) aesdsocket.o -o aesdsocket
			rm aesdsocket.o

aesdsocket.o: aesdsocket.c
			$(GCC) -g -O1 -c aesdsocket.c -Wall -Werror

clean:
		rm -f *.o aesdsocket

ifeq ($(CROSS_COMPILE),true)
GCC=aarch64-none-linux-gnu-gcc
endif