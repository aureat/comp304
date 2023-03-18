Compile and run either by
$ cd .. && make && shuf -i 1-1000 | ./bin/p3 98 3
Or by
$ gcc -std=c99 p3.c -o ../bin/p3 && shuf -i 1-1000 | ../bin/p3 98 3