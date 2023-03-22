#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  pid_t pid;
  if ((pid = fork()) != 0) {
    kill(pid, SIGKILL); // kill immediately
  }
  sleep(20); // both sleep for 20 seconds
  wait(NULL);
  exit(EXIT_SUCCESS);
}