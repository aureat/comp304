CC = gcc
CCFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE -Wall -Werror -std=c99
LIBS = -lrt

all: makebin
	$(CC) $(CCFLAGS) p1a/p1a.c -o bin/p1a $(LIBS)
	$(CC) $(CCFLAGS) p1b/p1b.c -o bin/p1b $(LIBS)
	$(CC) $(CCFLAGS) p2/p2.c -o bin/p2 $(LIBS)
	$(CC) $(CCFLAGS) p3/p3.c -o bin/p3 $(LIBS)

makebin:
	mkdir bin

clean:
	rm -r bin
