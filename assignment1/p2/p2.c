#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 30
#define READ_END 0
#define WRITE_END 1

double average(double *arr, int n) {
  double sum = 0;
  for (int i = 0; i < n; i++)
    sum += arr[i];
  return sum / n;
}

double maximum(double *arr, int n) {
  double max = arr[0];
  for (int i = 1; i < n; i++)
    if (arr[i] > max)
      max = arr[i];
  return max;
}

double minimum(double *arr, int n) {
  double min = arr[0];
  for (int i = 1; i < n; i++)
    if (arr[i] < min)
      min = arr[i];
  return min;
}

double bench_exec(char **argv) {
  int exec_pid;
  struct timeval t_start, t_end;
  double t_elapsed;

  // direct stream to /dev/null
  int devnull = open("/dev/null", O_WRONLY);
  dup2(devnull, 1);

  // capture start time
  gettimeofday(&t_start, NULL);

  // execute command
  if ((exec_pid = fork()) == 0) {
    execvp(argv[0], argv);
    perror("execve");
    exit(EXIT_FAILURE);
  }

  // wait for execvp to return
  waitpid(exec_pid, NULL, 0);

  // capture end time
  gettimeofday(&t_end, NULL);

  // calculate elapsed time and return
  return (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
         (t_end.tv_usec - t_start.tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {

  // check args
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <num-processes> <command> <option1> ...\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  // get args
  int n = atoi(argv[1]);
  char *newargv[argc - 1];
  for (int i = 2; i < argc; i++)
    newargv[i - 2] = argv[i];
  newargv[argc - 2] = NULL;

  // read-write pipe
  int fd[2];
  char write_buf[BUF_SIZE];
  char read_buf[BUF_SIZE];

  // execution times
  double t_elapsed, exec_time, exec_times[n];

  // children & args to execute
  int cur_pid, pid_index;

  // create pipe
  pipe(fd);

  // run n simultaneous benchmarks
  // send process index and elapsed time to parent
  for (int i = 0; i < n; i++) {
    if ((cur_pid = fork()) == 0) {
      close(fd[READ_END]);
      t_elapsed = bench_exec(newargv);
      sprintf(write_buf, "%d,%.5f", i, t_elapsed);
      write(fd[WRITE_END], write_buf, BUF_SIZE);
      close(fd[WRITE_END]);
      exit(0);
    }
  }

  close(fd[WRITE_END]);

  // receive execution times as children finish
  // print execution times
  while ((cur_pid = wait(NULL)) > 0) {
    read(fd[READ_END], read_buf, BUF_SIZE);
    pid_index = atoi(strtok(read_buf, ","));
    exec_time = atof(strtok(NULL, ","));
    exec_times[pid_index] = exec_time;
    printf("Child %d Executed in %.2f millis\n", (pid_index + 1), exec_time);
  }

  close(fd[READ_END]);

  // print stats
  printf("Max: %.2f millis\n", maximum(exec_times, n));
  printf("Min: %.2f millis\n", minimum(exec_times, n));
  printf("Average: %.2f millis\n", average(exec_times, n));

  exit(0);
}