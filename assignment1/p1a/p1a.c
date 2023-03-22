#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

  // get number of children
  int n = atoi(argv[1]);

  // print parent process id and level
  int level = 0;
  printf("Main Process ID: %d, level: %d\n", getpid(), level);

  // recursively create n children
  pid_t pid;
  for (int i = 0; i < n; i++) {
    if ((pid = fork()) == 0) {
      printf("Process ID: %d, Parent ID: %d, level: %d\n", getpid(), getppid(), ++level);
    }
  }

  // wait for all children and exit
  wait(NULL);
  exit(EXIT_SUCCESS);

}