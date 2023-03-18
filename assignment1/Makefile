CC = gcc
CCFLAGS = -Wall -Werror -std=c99

all: makebin
	$(CC) $(CCFLAGS) p1a/p1a.c -o bin/p1a
	$(CC) $(CCFLAGS) p1b/p1b.c -o bin/p1b
	$(CC) $(CCFLAGS) p2/p2.c -o bin/p2
	$(CC) $(CCFLAGS) p3/p3.c -o bin/p3

makebin:
	mkdir bin

clean:
	rm -r bin