#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#define BUF_SIZE 30

#define READ_END 0
#define WRITE_END 1

double average(double* arr, int n) {
  double sum = 0;
  for (int i = 0; i < n; i++)
    sum += arr[i];
  return sum / n;
}

double maximum(double* arr, int n) {
  double max = arr[0];
  for (int i = 1; i < n; i++)
    if (arr[i] > max)
      max = arr[i];
  return max;
}

double minimum(double* arr, int n) {
  double min = arr[0];
  for (int i = 1; i < n; i++)
    if (arr[i] < min)
      min = arr[i];
  return min;
}

void bench_exec(char** argv, int* p_st, double* tbench) {

  // direct stream to /dev/null
  int devnull = open("/dev/null", O_WRONLY);
  dup2(devnull, 1);

  // set to error initially
  *p_st = 1;

  // execute command
  int status;
  pid_t pid;
  struct timeval t_start, t_end;

  if ((pid = fork()) == 0) {
    execvp(argv[0], argv); // child executes command
    exit(EXIT_FAILURE); // reports failure if execvp fails
  } else {

    // calculate execution time
    gettimeofday(&t_start, NULL);
    waitpid(pid, &status, 0);
    gettimeofday(&t_end, NULL);

    // if child exited normally, set status and elapsed time
    if (status == 0) {
      *p_st    = 0;
      *tbench  = (t_end.tv_sec - t_start.tv_sec) * 1000.0;
      *tbench += (t_end.tv_usec - t_start.tv_usec) / 1000.0;
    }

  }

}

int main(int argc, char* argv[]) {

  if (argc < 3) { // print usage if not enough arguments
    fprintf(stderr, "Usage: %s <num-processes> <command> <option1> ...\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int n = atoi(argv[1]); // get number of children

  char *newargv[argc - 1]; // create new argv for execvp
  for (int i = 2; i < argc; i++)
    newargv[i - 2] = argv[i]; // copy whole command
  newargv[argc - 2] = NULL;

  // read-write pipe
  int fd[2];
  char write_buf[BUF_SIZE];
  char read_buf[BUF_SIZE];
  pipe(fd);

  // execution times
  double exec_time, exec_times[n];

  // run n simultaneous benchmarks
  pid_t cur_pid;
  for (int i = 0; i < n; i++) {
    if ((cur_pid = fork()) == 0) {

      // close read-end for child
      close(fd[READ_END]);

      // execute and get elapsed time
      int bench_status;
      double t_elapsed;
      bench_exec(newargv, &bench_status, &t_elapsed);

      // if benchmark was successful,
      // report index and elapsed time
      if (bench_status == 0) {
        sprintf(write_buf, "%d,%.5f", i, t_elapsed);
        write(fd[WRITE_END], write_buf, BUF_SIZE);
        close(fd[WRITE_END]);
        exit(EXIT_SUCCESS);
      }

      // report failure
      close(fd[WRITE_END]);
      exit(EXIT_FAILURE);

    }
  }

  // close write-end for parent
  close(fd[WRITE_END]);

  // receive execution times as children finish
  // print execution times
  int status, pid_index;
  while ((cur_pid = wait(&status)) > 0) {
    if (status == 0) {

      // read execution time from pipe
      read(fd[READ_END], read_buf, BUF_SIZE);
      pid_index = atoi(strtok(read_buf, ","));
      exec_time = atof(strtok(NULL, ","));

      // store execution time and print
      exec_times[pid_index] = exec_time;
      printf("Child %d Executed in %.2f millis\n", (pid_index + 1), exec_time);

    } else {
      // kill all children if one fails
      close(fd[READ_END]);
      signal(SIGQUIT, SIG_IGN);
      kill(0, SIGQUIT);
      fprintf(stderr, "command failed!\n");
      exit(EXIT_FAILURE);
    }
  }

  // close read-end for parent
  close(fd[READ_END]);

  // print stats and exit
  printf("Max: %.2f millis\n", maximum(exec_times, n));
  printf("Min: %.2f millis\n", minimum(exec_times, n));
  printf("Average: %.2f millis\n", average(exec_times, n));
  exit(EXIT_SUCCESS);

}