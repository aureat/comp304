#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "common.h"
#include "cdh.h"
#include "cloc.h"
#include "psvis.h"
#include "command.h"


int command_cdh(Command* command, cdhistory* hist) {
  
  // return if too many args
  if (command->arg_count > 3) {
    return err_cmd(command, "too many arguments");
  }

  int r;
  char c;

  // print history and wait for selection if no args
  if (command->arg_count == 2) {
    if (cdh_print(hist) == ERROR)
      return ERROR;
    printf("Select directory by letter or number: ");
    c = getchar();
  }

  // print specific entry if 1 arg
  if (command->arg_count == 3) {

    // check if it's a single char
    if (strlen(command->args[1]) != 1) {
      return err_cmd(command, "invalid index");
    }

    // get index from char
    c = command->args[1][0];

  }

  // get index from char
  int index;
  if (isalpha(c))
    index = tolower(c) - 97;
  else if (isdigit(c))
    index = c - 48;
  else
    return err_cmd(command, "invalid index");
  
  // get path from index
  char* path = cdh_get(hist, index);
  if (path == NULL) {
    return err_cmd(command, "invalid index");
  }

  // change to path
  if ((r = chdir(path)) == -1) {
    return err_errno(command);
  }

  printf("trying to change path\n");
  printf("path: %s\n", path);

  // add to cd history if successful
  cdh_add(hist, path);

  return SUCCESS;
}

int command_cd(Command* command, cdhistory* hist) {

  // file path
  char path[PATH_MAX];
  char abs_path[PATH_MAX];
  char* home = getenv("HOME");

  // return if too many args
  if (command->arg_count > 3) {
    return err_cmd(command, "too many arguments");
  }

  // change to home if no args
  if (command->arg_count == 2) {
    strcpy(path, home);
  }
  
  // change to dir if 1 arg
  if (command->arg_count == 3) {
    // handle ~ as home
    if (strncmp(command->args[1], "~", sizeof(char)) == 0) {
      strcpy(path, home);
      strcat(path, (command->args[1] + 1));
    } else {
      strcpy(path, command->args[1]);
    }
  }

  // get absolute path
  realpath(path, abs_path);

  // report error if failed
  int r;
  if ((r = chdir(path)) == -1) {
    return err_errno(command);
  }

  // add to cd history if successful
  cdh_add(hist, abs_path);

  return SUCCESS;
}

int command_ls(Command* command) {

  // final path
  char path[PATH_MAX];

  // return if too many args
  if (command->arg_count > 3) {
    return err_cmd(command, "too many arguments");
  }

  // Use current directory if no argument is provided
  if (command->arg_count == 2) {
    strcpy(path, ".");
  }

  // Use the provided path
  if (command->arg_count == 3) {
    if (strncmp(command->args[1], "~", sizeof(char)) == 0) {
      strcpy(path, getenv("HOME"));
      strcat(path, (command->args[1] + 1));
      printf("%s\n", path);
    } else
      strcpy(path, command->args[1]);
  }

  DIR *dir;
  struct dirent *entry;

  // Open the directory
  dir = opendir(path);
  if (dir == NULL) {
    return err_errno(command);
  }

  int max_name_len = 0, num_columns = 0, num_rows = 0;

  // Get max name length and num rows
  while ((entry = readdir(dir)) != NULL) {
    num_rows++;
    if ((int)strlen(entry->d_name) > max_name_len) {
      max_name_len = strlen(entry->d_name);
    }
  }

  struct winsize ws;
  // Determine the number of columns to use based on the terminal width
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    num_columns = 1;
  } else {
    num_columns = ws.ws_col / (max_name_len + 2);
  }
  num_rows = (num_rows + num_columns - 1) / num_columns;

  // Print the entries in multiple columns
  rewinddir(dir);
  char fullpath[PATH_MAX];
  int i, j, k;
  for (i = 0; i < num_rows; i++) {
    for (j = 0; j < num_columns; j++) {
      k = i + j * num_rows;
      entry = readdir(dir);
      if (entry == NULL)
        break;
      snprintf(fullpath, PATH_MAX, "%s/%s", path, entry->d_name);
      printf("%-*s", max_name_len, entry->d_name);
      if (j < num_columns - 1 && k + num_rows < num_rows * num_columns) {
          printf("  ");
      }
    }
    printf("\n");
  }

  if (closedir(dir) == -1) {
    return err_errno(command);
  }

  return SUCCESS;
}

int command_roll(Command* command) {

  if (command->arg_count != 3 || strchr(command->args[1], 'd') == NULL) {
    return err_msg("usage: roll [number of rolls]d<number of sides>");
  }

  char *num_rolls_str, *num_sides_str;
  if (strncmp(command->args[1], "d", sizeof(char)) == 0) {
    num_rolls_str = "1";
    num_sides_str = command->args[1] + 1;
  } else {
    num_rolls_str = strtok(command->args[1], "d");
    num_sides_str = strtok(NULL, "d");
  }

  if (num_rolls_str == NULL || num_sides_str == NULL) {
    return err_cmd(command, "invalid dice format");
  }

  int num_rolls = atoi(num_rolls_str);
  int num_sides = atoi(num_sides_str);
  
  if (num_rolls < 1 || num_sides < 1) {
    return err_cmd(command, "invalid dice format");
  }
  
  srand(time(NULL));
  
  int rolls[num_rolls];
  int sum = 0;
  
  for (int i = 0; i < num_rolls; i++) {
    int roll = rand() % num_sides + 1;
    sum += roll;
    rolls[i] = roll;
  }
  
  printf("Rolled %d (", sum);
  for (int i = 0; i < num_rolls - 1; i++) {
    printf("%d + ", rolls[i]);
  }
  printf("%d)\n", rolls[num_rolls - 1]);
  
  return SUCCESS;
}

int command_cloc(Command* command) {

  // final path
  char path[PATH_MAX];

  // return if too many args
  if (command->arg_count > 3) {
    return err_cmd(command, "too many arguments");
  }

  // Use current directory if no argument is provided
  if (command->arg_count == 2) {
    strcpy(path, ".");
  }

  // Use the provided path
  if (command->arg_count == 3) {
    if (strncmp(command->args[1], "~", sizeof(char)) == 0) {
      strcpy(path, getenv("HOME"));
      strcat(path, (command->args[1] + 1));
      // printf("%s\n", path);
    } else
      strcpy(path, command->args[1]);
  }

  // create stats struct
  LangStats stats;
  cloc_init(&stats);

  // search path and look for errors
  if (cloc_directory(path, &stats) == ERROR) {
    return err_errno(command);
  }

  // print stats 
  cloc_print(&stats);

  return SUCCESS;
}

int command_psvis(Command* command) {

  if (command->arg_count != 3) {
    return err_msg("usage: psvis [pid]");
  }

  // get pid
  int pid;
  if (sscanf(command->args[1], "%d", &pid) != 1) {
    return err_cmd(command, "invalid pid");
  }

  // insert psvis module
  if (system("sudo insmod " PSVIS_MODULE_FILENAME) < 0) {
    return err_errno(command);
  }

  // change permissions
  if (system("sudo chmod 666 /dev/" PSVIS_DEVICE_FILENAME) < 0) {
    return err_errno(command);
  }

  // visualize process tree
  if (visualize_pstree(command->args[1]) != SUCCESS) {
    return err_cmd(command, "failed to visualize process tree");
  }

  // remove psvis module
  if (system("sudo rmmod " PSVIS_MODULE_FILENAME) < 0) {
    return err_errno(command);
  }

  return SUCCESS;
}

