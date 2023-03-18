#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  int n = atoi(argv[1]);
  int level = 0;
  pid_t pid;
  printf("Main Process ID: %d, level: %d\n", getpid(), level);
  for (int i = 0; i < n; i++) {
    pid = fork();
    if (pid == 0) {
      level++;
      printf("ID: %d, Parent ID: %d, Level: %d\n", getpid(), getppid(), level);
    } else {
      wait(NULL);
    }
  }
  exit(0);
}