#ifndef CLOC_H
#define CLOC_H

#define LINE_MAX 1024
#define LANGS 5

typedef enum {
  FILE_TEXT,
  FILE_PYTHON,
  FILE_C,
  FILE_CPP,
  FILE_HEADER
} FileType;

typedef struct {
  int files;
  int blank;
  int comment;
  int code;
} FileStats;

typedef struct {
  int ignored;
  int ignored_dirs;
  int unique;
  int fails;
  FileStats types[LANGS];
} LangStats;

void cloc_init(LangStats* stats);
int cloc_directory(char *path, LangStats* stats);
void cloc_print(LangStats* stats);

#endif
