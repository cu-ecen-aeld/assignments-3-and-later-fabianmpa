.PHONY: all clean

all: aesdsocket.o
			$(CC) aesdsocket.o -o aesdsocket -lexplain -L/usr/include
			rm aesdsocket.o

aesdsocket.o: aesdsocket.c
			$(CC) -g -O1 -c aesdsocket.c -Wall -Werror

clean:
		rm -f *.o aesdsocket
