Compile and run either by
$ cd .. && make && ./bin/p1b & && ps -l | grep 'Z'
Or by
$ gcc -std=c99 p1b.c -o ../bin/p1b && ../bin/p1b & && ps -l | grep 'Z'