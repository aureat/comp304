Compile and run either by
$ cd .. && make && shuf -i 1-1000 | ./bin/p3 98 3
Or by
$ gcc -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE -Wall -Werror -std=c99 p3.c -o ../bin/p3 -lrt && shuf -i 1-1000 | ../bin/p3 98 3