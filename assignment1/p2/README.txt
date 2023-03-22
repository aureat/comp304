Compile and run either by
$ cd .. && make && ./bin/p2 5 ls -la
Or by
$ gcc -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE -Wall -Werror -std=c99 p2.c -o ../bin/p2 -lrt && ../bin/p2 5 ls -la