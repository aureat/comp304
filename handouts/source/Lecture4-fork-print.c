
#include <stdio.h>
#include <unistd.h>

int main()
{
  printf("Print 1: %d\n",getpid());
  fork();

  printf("Print 2: %d\n",getpid());
  
  fork();
  printf("Print 3: %d\n",getpid());

  fork();
  printf("Print 4: %d\n",getpid());

  return 0;
}
