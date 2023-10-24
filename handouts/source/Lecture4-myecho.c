/* myecho.c */

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
  int j=0;

  printf("\nChild process is executing this program\n");

  for (j = 0; j < argc; j++)
    printf("argv[%d]: %s\n", j, argv[j]);
  
  exit(EXIT_SUCCESS);
}
