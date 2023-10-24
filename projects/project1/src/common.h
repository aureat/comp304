#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PATH_MAX 4096

typedef struct command_t Command;
typedef struct cdh_node_t cdhnode;
typedef struct cdh_t cdhistory;

enum return_codes {
	ERROR = -1,
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

char* prettify_path(const char* path);
int err_msg(const char *msg);
int err_cmd(Command* command, const char* msg);
int err_errno(Command* command);


#endif
