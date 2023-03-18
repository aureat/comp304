#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  pid_t pid = fork();
  if (pid != 0) {
    kill(pid, SIGSEGV);
  }
  sleep(20);
  wait(NULL);
  exit(0);
}