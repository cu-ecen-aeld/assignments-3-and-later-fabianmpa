CC ?= gcc 
CFLAGS ?= -Wall
LDFLAGS ?=
.PHONY: all clean

all: aesdsocket.o
			$(CC) $(LDFLAGS) aesdsocket.o -o aesdsocket
			rm aesdsocket.o

aesdsocket.o: aesdsocket.c
			$(CC) $(CFLAGS) -c aesdsocket.c

clean:
		rm -f *.o aesdsocket
