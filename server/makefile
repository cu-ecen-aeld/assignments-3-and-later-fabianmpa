.PHONY: all clean

all: aesdsocket.o
			$(CC) aesdsocket.o -o aesdsocket
			rm aesdsocket.o

aesdsocket.o: aesdsocket.c
			$(CC) -g -O1 -c aesdsocket.c -Wall -Werror

clean:
		rm -f *.o aesdsocket
