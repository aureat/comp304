#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

#define MAX_NUMS 1000

int main(int argc, char* argv[]) {

  // get args from command line
  int x = atoi(argv[1]);
  int n = atoi(argv[2]);

  // reads a newline-delimited sequence of at most 1000 numbers from stdin
  // and parses them into an array
  char buf[MAX_NUMS];
  int nums[MAX_NUMS], nums_length = 0;
  while (fgets(buf, 1000, stdin) != NULL) {
    nums[nums_length++] = atoi(buf);
  }

  // shared memory segment
  int shm_fd;
  void* shm_ptr;
  const int SHMSIZE = MAX_NUMS * sizeof(int);
  const char* name = "comp304";

  // open shared memory segment
  shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}

  // restrict shared memory size
  ftruncate(shm_fd, SHMSIZE);

  // map shared memory segment
  shm_ptr = mmap(0, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    printf("Map failed");
    exit(EXIT_FAILURE);
  }

  // copy nums to shared memory
  memcpy(shm_ptr, nums, (nums_length)*sizeof(int));

  // keep track of pids
  pid_t cur_pid, pids[n];

  // fork n child processes
  for (int i = 0; i < n; i++) {

    if ((pids[i] = fork()) == 0) {

      // iterate over nums_length/n elements its responsible for
      int start = (i * nums_length) / n;
      int end = ((i + 1) * nums_length) / n;

      // get the remainder if nums_length is not divisible by n
      if (i == n - 1) end += nums_length % n;

      // iterate and search for x
      for (int j = start; j < end; j++) {
        if (nums[j] == x) {
          printf("Found %d at index %d\n", x, j);
          exit(0); // exit with status 0 if x is found
        }
      }

      // if x is not found, exit with status 1
      exit(1);
    }

  }

  // wait for children to finish and check their exit status
  int status;
  while ((cur_pid = wait(&status)) > 0) {

    // check if x is found
    if (status == 0) {

      // kill all children and exit with status 0
      for (int i = 0; i < n; i++)
        kill(pids[i], SIGTERM);
      exit(0);
      
    }

  }

  // close and unlink shared memory
  if (shm_unlink(name) == -1) {
		printf("Error removing %s\n", name);
		exit(-1);
	}

  // if x is not found, print "Not found" and exit with status 1
  printf("Not found\n");
  exit(1);
}