CROSS_COMPILE=
GCC=gcc

build: writer.o
			$(GCC) writer.o -o writer

writer.o: writer.c
			$(GCC) -c writer.c


ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
GCC=aarch64-none-linux-gnu-gcc
endif

clean:
		rm -f *.o writer




