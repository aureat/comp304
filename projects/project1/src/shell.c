#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <libgen.h>
#include "common.h"
#include "command.h"
#include "cloc.h"
#include "cdh.h"

// shell name
const char *sysname = "mishell";

// cd history
cdhistory cdhist;

// path to executable
char path_to_exec[PATH_MAX];

/**
 * Prints an error message
 * @param msg simple error message
 */
int err_msg(const char *msg) {
  fprintf(stderr, "%s\n", msg);
	return ERROR;
}

/**
 * Prints an error message with command name
 * @param command command struct
 * @param msg simple error message
 */
int err_cmd(Command* command, const char* msg) {
	fprintf(stderr, "%s: %s\n", command->name, msg);
	return ERROR;
}

/**
 * Prints an error message from error code
 * @param command command struct
 */
int err_errno(Command* command) {
	fprintf(stderr, "%s: %s\n", command->name, strerror(errno));
	return ERROR;
}

/**
 * Prints a command struct
 * @param command command struct
 */
void print_command(Command* command) {
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n",
		   command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");

	for (i = 0; i < 3; i++) {
		printf("\t\t%d: %s\n", i,
			   command->redirects[i] ? command->redirects[i] : "N/A");
	}

	printf("\tArguments (%d):\n", command->arg_count);

	for (i = 0; i < command->arg_count; ++i) {
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	}

	if (command->next) {
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param command command struct
 * @return 0
 */
int free_command(Command* command) {
	if (command->arg_count) {
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}

	for (int i = 0; i < 3; ++i) {
		if (command->redirects[i])
			free(command->redirects[i]);
	}

	if (command->next) {
		free_command(command->next);
		command->next = NULL;
	}

	free(command->name);
	free(command);
	return SUCCESS;
}

/**
 * Truncate a path string to the first letter of each directory
 * @param path Current working directory path
 * @return New prettified path string
 */
char* prettify_path(const char* path) {
  
	// old path
  char* old_path = strdup(path);
	char* p = old_path;

	// new path
  size_t path_len = (strlen(path) + 1) * sizeof(char);
  char* new_path = (char*)malloc(path_len);
	new_path[0] = '\0';
  
  char* home_path = getenv("HOME");
  if (strncmp(p, home_path, strlen(home_path)) == 0) {
    strcat(new_path, "~");
    p += strlen(home_path);
  }

  if (p != NULL && *p == '/') {
    strcat(new_path, "/");
    p++;
  }

  if (*p) {
    char* prev_token;
    char* token = strtok(p, "/");
    while (token != NULL) {
      prev_token = token;
      token = strtok(NULL, "/");
      if (token != NULL) {
        char pth[] = {prev_token[0], '/', '\0'};
        strcat(new_path, pth);
      }
    }

    strcat(new_path, prev_token);
  }

  free(old_path);
  return new_path;
}

/**
 * Show the command prompt
 * @return 0
 */
int show_prompt(void) {

	// get current working directory and prettify it
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	char* new_path = prettify_path(cwd);

	// print prompt
	printf(COLOR_CYAN "%s> " COLOR_YELLOW "%s" COLOR_RESET " $ ", sysname, new_path);
	free(new_path);

	return SUCCESS;
}

/**
 * Parse a command string into a command struct
 * @param  buf		  input string
 * @param  command  empty command struct
 * @return          0
 */
int parse_command(char *buf, Command *command) {
  const char *splitters = " \t"; // split at whitespace
  int index, len;
  len = strlen(buf);

  // trim left whitespace
  while (len > 0 && strchr(splitters, buf[0]) != NULL) {
    buf++;
    len--;
  }

  while (len > 0 && strchr(splitters, buf[len - 1]) != NULL) {
    // trim right whitespace
    buf[--len] = 0;
  }

  // auto-complete
  if (len > 0 && buf[len - 1] == '?') {
    command->auto_complete = true;
  }

  // background
  if (len > 0 && buf[len - 1] == '&') {
    command->background = true;
  }

  char *pch = strtok(buf, splitters);
  if (pch == NULL) {
    command->name = (char *)malloc(1);
    command->name[0] = 0;
  } else {
    command->name = (char *)malloc(strlen(pch) + 1);
    strcpy(command->name, pch);
  }

  command->args = (char **)malloc(sizeof(char *));

  int redirect_index = -1;
  int arg_index = 0;
  char temp_buf[1024], *arg;

  while (1) {
    // tokenize input on splitters
    pch = strtok(NULL, splitters);
    if (!pch)
      break;
    arg = temp_buf;
    strcpy(arg, pch);
    len = strlen(arg);

    // empty arg, go for next
    if (len == 0) {
      continue;
    }

    // trim left whitespace
    while (len > 0 && strchr(splitters, arg[0]) != NULL) {
      arg++;
      len--;
    }

    // trim right whitespace
    while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) {
      arg[--len] = 0;
    }

    // empty arg, go for next
    if (len == 0) {
      continue;
    }

    if (redirect_index != -1) {
      command->redirects[redirect_index] = malloc(len + 1);
      strcpy(command->redirects[redirect_index], arg);
      redirect_index = -1;
      continue;
    }

    // piping to another command
    if (strcmp(arg, "|") == 0) {
      Command *c = malloc(sizeof(Command));
      int l = strlen(pch);
      pch[l] = splitters[0]; // restore strtok termination
      index = 1;
      while (pch[index] == ' ' || pch[index] == '\t')
        index++; // skip whitespaces

      parse_command(pch + index, c);
      pch[l] = 0; // put back strtok termination
      command->next = c;
      continue;
    }

    // background process
    if (strcmp(arg, "&") == 0) {
      // handled before
      continue;
    }

    // handle input redirection
    redirect_index = -1;
    if (arg[0] == '<') {
      arg++;
      len--;
      redirect_index = 0;
    } else if (arg[0] == '>') {
      redirect_index = 1;
      arg++;
      len--;
      if (len > 0 && arg[0] == '>') {
        redirect_index = 2;
        arg++;
        len--;
      }
    }

    if (len > 0 && redirect_index != -1) {
      command->redirects[redirect_index] = malloc(len + 1);
      strcpy(command->redirects[redirect_index], arg);
      redirect_index = -1;
      continue;
    }

    if (redirect_index != -1)
      continue;

    // normal arguments
    if (len > 2 &&
        ((arg[0] == '"' && arg[len - 1] == '"') ||
         (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
    {
      arg[--len] = 0;
      arg++;
    }

    command->args =
        (char **)realloc(command->args, sizeof(char *) * (arg_index + 1));

    command->args[arg_index] = (char *)malloc(len + 1);
    strcpy(command->args[arg_index++], arg);
  }
  command->arg_count = arg_index;

  if (redirect_index != -1) {
    command->redirects[redirect_index] = malloc(1);
    command->redirects[redirect_index][0] = 0;
  }

  // increase args size by 2
  command->args = (char **)realloc(command->args,
                                   sizeof(char *) * (command->arg_count += 2));

  // shift everything forward by 1
  for (int i = command->arg_count - 2; i > 0; --i) {
    command->args[i] = command->args[i - 1];
  }

  // set args[0] as a copy of name
  command->args[0] = strdup(command->name);

  // set args[arg_count-1] (last) to NULL
  command->args[command->arg_count - 1] = NULL;

  return SUCCESS;
}

void prompt_backspace(void) {
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(Command* command) {
	size_t index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &=
		~(ICANON |
		  ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		// handle tab
		if (c == 9) {
			buf[index++] = '?'; // autocomplete
			break;
		}

		// handle backspace
		if (c == 127) {
			if (index > 0) {
				prompt_backspace();
				index--;
			}
			continue;
		}

		if (c == 27 || c == 91 || c == 66 || c == 67 || c == 68) {
			continue;
		}

		// up arrow
		if (c == 65) {
			while (index > 0) {
				prompt_backspace();
				index--;
			}

			char tmpbuf[4096];
			printf("%s", oldbuf);
			strcpy(tmpbuf, buf);
			strcpy(buf, oldbuf);
			strcpy(oldbuf, tmpbuf);
			index += strlen(buf);
			continue;
		}

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') {
		index--;
	}

	// null terminate string
	buf[index++] = '\0';

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(Command* command);

void rainbow_text(char* text) {
	int i = 0;
	while (*text) {
		printf("\033[38;5;%dm%c", 196 + i, *text);
		text++;
		i++;
	}
	printf("\033[0m");
}

int main(int argc, char** argv) {

	cdh_init(&cdhist);
	cdh_load(&cdhist);

	realpath(dirname(argv[0]), path_to_exec);

	printf("COMP304 - Operating Systems - Project 1\n");
	printf("\033[1m");
	rainbow_text("Welcome to Mishell fam!\n");
	printf("\033[0m");
	printf("Type " COLOR_GREEN "help" COLOR_RESET " for a list of commands\n");

	while (1) {
		Command* command = malloc(sizeof(Command));

		// set all bytes to 0
		memset(command, 0, sizeof(Command));

		int code;
		code = prompt(command);
		if (code == EXIT) {
			break;
		}

		code = process_command(command);
		if (code == EXIT) {
			break;
		}

		free_command(command);
	}

	cdh_save(&cdhist);
	cdh_free(&cdhist);

	return 0;
}

void print_help(char* name, char* description) {
	printf(" " COLOR_GREEN "%-12s" COLOR_RESET " %s\n", name, description);
}

int resolve_exec_path(Command* command) {

	// check if file exists and is executable
	if (access(command->args[0], X_OK) == 0) {
		return SUCCESS;
	}

	// get PATH environment variable
	char PATH[PATH_MAX];
	strcpy(PATH, getenv("PATH"));

	// split PATH by :
	char* dir = strtok(PATH, ":");
	while (dir != NULL) {

		// construct path
		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s/%s", dir, command->args[0]);

		// check if file exists and is executable
		if (access(buf, X_OK) == 0) {
			command->args[0] = strdup(buf);
			return SUCCESS;
		}

		// get next dir
		dir = strtok(NULL, ":");
	}

	return ERROR;
}

int execute_michelle(Command* command) {
	// repurpose command
	command->args = realloc(command->args, sizeof(char*) * 3);

	// set python path
	command->args[0] = strdup("python");
	if (resolve_exec_path(command) == ERROR) {
		command->args[0] = strdup("python3");
		if (resolve_exec_path(command) == ERROR) {
			err_msg("michelle: failed to locate python");
			return ERROR;
		}
	}

	// get path to michelle.py
	char buf[PATH_MAX];
	strcpy(buf, path_to_exec);
	strcat(buf, "/");
	strcat(buf, "michelle/michelle.py");

	// set path to michelle.py
	command->args[1] = strdup(buf);
	command->args[2] = NULL;

	// fork and execute command
	pid_t pid;
	if ((pid = fork()) == 0) {
		execv(command->args[0], command->args);
		err_msg("michelle: something went wrong");
		exit(SUCCESS);
	}

	// wait for child process to finish
	waitpid(pid, NULL, 0);
	return SUCCESS;
}

int process_command(Command* command) {

	int r;

	if (strcmp(command->name, "") == 0) {
		return SUCCESS;
	}

	if (strcmp(command->name, "exit") == 0) {
		return EXIT;
	}

	if (strcmp(command->name, "cd") == 0) {
		return command_cd(command, &cdhist);
	}

	if (strcmp(command->name, "cdh") == 0) {
		return command_cdh(command, &cdhist);
	}

	if (strcmp(command->name, "help") == 0) {
		print_help("michelle", "smart interactive shell");
		print_help("cd", "change directory");
		print_help("cdh", "change directory history");
		print_help("exit", "exit the shell");
		print_help("help", "print this help message");
		print_help("clear", "clear the screen");
		print_help("roll", "roll a dice");
		print_help("cloc", "count lines of code");
		print_help("psvis", "visualize processes");
		return SUCCESS;
	}

	if (strcmp(command->name, "cloc") == 0) {
		return command_cloc(command);
	}

	if (strcmp(command->name, "clear") == 0) {
		printf("\033[2J\033[1;1H");
		return SUCCESS;
	}

	if (strcmp(command->name, "roll") == 0) {
		return command_roll(command);
	}

	if (strcmp(command->name, "michelle") == 0) {
		return execute_michelle(command);
	}

	if (strcmp(command->name, "psvis") == 0) {
		return command_psvis(command);
	}

	if ((r = resolve_exec_path(command)) != SUCCESS) {
		err_cmd(command, "command not found");
		return UNKNOWN;
	}

	pid_t background_pid;
	if ((background_pid = fork()) == 0) {

		// open read redirect file
		int rd_read;
		if (command->redirects[0] != NULL) {
			rd_read = open(command->redirects[0], O_RDONLY);
			if (rd_read == ERROR) {
				err_cmd(command, "failed to open input redirect file");
				exit(ERROR);
			}
		}

		// open write redirect file
		int rd_write;
		if (command->redirects[1] != NULL) {
			rd_write = open(command->redirects[1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
			if (rd_write == ERROR) {
				err_cmd(command, "failed to open write redirect file");
				exit(ERROR);
			}
		}

		// open append redirect file
		int rd_append;
		if (command->redirects[2] != NULL) {
			rd_append = open(command->redirects[2], O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
			if (rd_append == ERROR) {
				err_cmd(command, "failed to open append redirect file");
				exit(ERROR);
			}
		}

		// execute command
		pid_t exec_pid;
		if ((exec_pid = fork()) == 0) {

			// redirect read
			if (command->redirects[0] != NULL) {
				dup2(rd_read, STDIN_FILENO);
				close(rd_read);
			}

			// redirect write
			if (command->redirects[1] != NULL) {
				dup2(rd_write, STDOUT_FILENO);
				close(rd_write);
			}

			// redirect append
			if (command->redirects[2] != NULL) {
				dup2(rd_append, STDOUT_FILENO);
				close(rd_append);
			}

			execv(command->args[0], command->args);
			err_cmd(command, "command failed to execute");
			exit(EXIT);
		}

		// wait for exec_pid to finish and report
		waitpid(exec_pid, NULL, 0);

		// close file descriptors
		close(rd_read);
		close(rd_write);
		close(rd_append);

		// print if background
		if (command->background) {
			printf("\nTask with PID=%d finished\n", exec_pid);
		}

		exit(SUCCESS);
	}
		
	// return if background
	if (command->background) {
		return SUCCESS;
	}

	waitpid(background_pid, NULL, 0);
	return SUCCESS;
}
