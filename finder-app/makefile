.PHONY: all clean

all: writer.o
			$(CC) $(CFLAGS) $(INCLUDE) $(LD_FLAGS)  \
			writer.o -o writer

writer.o: writer.c
			$(CC) -c writer.c

clean:
		rm -f *.o writer

