#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

#define EXIT_ERROR -1

#define MAX_NUMS 1000
#define MAX_LINE 64

int main(int argc, char* argv[]) {

  // get args from command line
  int x = atoi(argv[1]);
  int n = atoi(argv[2]);

  // shared memory segment
  int shm_fd;
  const int SHMSIZE = MAX_NUMS * sizeof(int);
  const char* name = "comp304";

  // open shared memory segment
  shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (shm_fd == EXIT_ERROR) {
		perror("shm_open failed");
		exit(EXIT_ERROR);
	}

  // restrict shared memory size
  ftruncate(shm_fd, SHMSIZE);

  // map shared memory segment for read-write
  void* shm_ptr = mmap(0, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    exit(EXIT_ERROR);
  }

  // nums array
  int* nums = (int*) shm_ptr;
  int nums_length = 0;

  // buffer for reading from stdin
  size_t buf_len = MAX_LINE * sizeof(char);
  char* line_buf = malloc(buf_len);
  if (line_buf == NULL) {
    perror("buffer allocation failed");
    exit(EXIT_ERROR);
  }

  // read newline-delimited sequence of at most 1000 numbers from stdin
  // store in shared memory
  while (getline(&line_buf, &buf_len, stdin) != -1 && \
         nums_length < MAX_NUMS) 
  {
    nums[nums_length++] = atoi(line_buf);
  }

  // free buffer
  free(line_buf);

  // keep track of pids
  pid_t cur_pid, pids[n];

  // fork n child processes
  for (int i = 0; i < n; i++) {
    if ((pids[i] = fork()) == 0) {

      // iterate over nums_length/n elements its responsible for
      int start = (i * nums_length) / n;
      int end = ((i + 1) * nums_length) / n;

      // get the remainder for the last child if nums_length is not divisible by n
      if (i == n - 1)
        end += nums_length % n;

      // iterate and search for x
      for (int j = start; j < end; j++) {
        if (nums[j] == x) {
          printf("Found %d at index %d\n", x, j);
          exit(EXIT_SUCCESS); // exit with status 0 if x is found
        }
      }

      // exit with status 1 if child does not find x
      exit(EXIT_FAILURE);
    }
  }

  // wait for children to finish and check their exit status
  int status;
  int found = 0;
  while ((cur_pid = wait(&status)) > 0) {
    if (status == EXIT_SUCCESS) { // check if x is found
      for (int i = 0; i < n; i++)
        kill(pids[i], SIGTERM); // kill all remaining children politely
      found = 1;
      break;
    }
  }

  // unmap shared memory segment
  if (munmap(shm_ptr, SHMSIZE) == EXIT_ERROR) {
    perror("munmap failed");
    exit(EXIT_ERROR);
  }

  // close shared memory descriptor
  if (close(shm_fd) == EXIT_ERROR) {
    perror("failed to close shared memory");
    exit(EXIT_ERROR);
  }

  // close and unlink shared memory
  if (shm_unlink(name) == EXIT_ERROR) {
		perror("shm_unlink failed");
		exit(EXIT_ERROR);
	}

  // exit with status 0 if x is found
  if (found) {
    exit(EXIT_SUCCESS);
  }

  fprintf(stderr, "number not found!\n");
  exit(EXIT_FAILURE);

}