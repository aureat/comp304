/*
  Simple example for process creation and getpid
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


int main()
{
	pid_t pid;

	pid = fork();

	printf("The value of pid %d\n", pid);

	if (pid == 0) { /* child process */
	  printf("Child process id %d\n", getpid());
	  return 0;
	}
	else if (pid > 0) { /* parent process */
	  wait(NULL);
	  printf("Parent process id %d\n", getpid());
	  return 0;
	}
}
