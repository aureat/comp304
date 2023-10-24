#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "common.h"
#include "cloc.h"


void cloc_init(LangStats* stats) {
  stats->ignored = 0;
  stats->ignored_dirs = 0;
  stats->unique = 0;
  stats->fails = 0;
  for (int i = 0; i < LANGS; i++) {
    stats->types[i].files = 0;
    stats->types[i].blank = 0;
    stats->types[i].comment = 0;
    stats->types[i].code = 0;
  }
}

int getfiletype(char *fname) {

  char filename[PATH_MAX];
  strcpy(filename, fname);

  char* ext = strrchr(filename, '.');
  if (ext == NULL) {
    return FILE_TEXT;
  }

  // Python files
  if (strcmp(ext, ".py") == 0) {
    return FILE_PYTHON;
  }

  // C files
  if (strcmp(ext, ".c") == 0) {
    return FILE_C;
  }

  // C++ files
  if (strcmp(ext, ".cc") == 0 || strcmp(ext, ".cpp") == 0 || 
      strcmp(ext, ".cxx") == 0)
  {
    return FILE_CPP;
  }

  // C/C++ header files
  if (strcmp(ext, ".h") == 0 || strcmp(ext, ".hh") == 0 ||
      strcmp(ext, ".hpp") == 0 || strcmp(ext, ".hxx") == 0) 
  {
    return FILE_HEADER;
  }

  // by default assume text file
  return FILE_TEXT;
}

int cloc_file(char* path, LangStats* stats) {

  ssize_t read;
  size_t len = 0;
  int in_comment = 0;
  char* line = malloc(LINE_MAX * sizeof(char));

  // open file
  FILE* fp = fopen(path, "r");
  if (fp == NULL) {
    return ERROR;
  }
  
  // determine file type from extension
  FileType type = getfiletype(path);

  // set comment delimiters
  const char* single_comment, *multi_comment_start, *multi_comment_end;
  if (type == FILE_PYTHON) {
    single_comment = "#";
    multi_comment_start = "\"\"\"";
    multi_comment_end = "\"\"\"";
  } else {
    single_comment = "//";
    multi_comment_start = "/*";
    multi_comment_end = "*/";
  }

  // go through lines
  while ((read = getline(&line, &len, fp)) != -1) {

    // count blank lines
    if (read == 1) {
      stats->types[type].blank++;
      continue;
    }

    // for a text file, count all lines as code
    if (type == FILE_TEXT) {
      stats->types[type].code++;
      continue;
    }

    // check if inside a comment block
    if (in_comment) {
      stats->types[type].comment++;
      if (strstr(line, multi_comment_end) != NULL) {
        in_comment = 0;
      }
      continue;
    } 

    // check for comments and count code lines
    if (strstr(line, single_comment) != NULL || strstr(line, multi_comment_start) != NULL) {
      stats->types[type].comment++;
      continue;
    }

    // anything else is code
    stats->types[type].code++;

  }

  // close file
  fclose(fp);

  // increment file count and return
  stats->unique++;
  stats->types[type].files++;
  return SUCCESS;

}

int cloc_directory(char *path, LangStats* stats) {

  DIR *dir;
  struct dirent *entry;
  struct stat file_stat;

  // create local copy of path
  char filepath[PATH_MAX];
  strcpy(filepath, path);

  // buffer for current path
  char curpath[PATH_MAX];

  // open and report error if failed
  if ((dir = opendir(filepath)) == NULL) {
    return ERROR;
  }

  // read directory entries
  while ((entry = readdir(dir)) != NULL) {

    // Add path
    sprintf(curpath, "%s/%s", filepath, entry->d_name);

    // Continue if failed
    if (stat(curpath, &file_stat) == ERROR) {
      stats->fails++;
      continue;
    }

    // Recurse into subdirectory if directory and not dotfile
    if (S_ISDIR(file_stat.st_mode)) {
      if (entry->d_name[0] == '.') {
        stats->ignored_dirs++;
        continue;
      }
      cloc_directory(curpath, stats);
      continue;
    }

    // Ignore dotfiles, binaries, drafts, and non-regular files
    if ((entry->d_name[0] == '.') || (entry->d_name[0] == '~') || 
        (access(curpath, X_OK) == 0) || (!S_ISREG(file_stat.st_mode)))
    {
      stats->ignored++;
      continue;
    }

    // Process file and report error
    if (cloc_file(curpath, stats) == ERROR) {
      stats->fails++;
      continue;
    }

  }

  // close directory
  closedir(dir);

  return SUCCESS;

}

const char* get_lang_name(int type) {
  switch (type) {
    case FILE_TEXT:
      return "Text";
    case FILE_PYTHON:
      return "Python";
    case FILE_C:
      return "C Source Files";
    case FILE_CPP:
      return "C++ Source Files";
    case FILE_HEADER:
      return "C/C++ Header Files";
    default:
      return "Unknown";
  }
}

void print_dashed_line(int n) {
  for (int i = 0; i < n; i++)
    printf("-");
  printf("\n");
}

void cloc_print(LangStats* stats) {

  // vertical sum
  int totals[] = {0, 0, 0};
  for (int i = 0; i < LANGS; i++) {
    totals[0] += stats->types[i].blank;
    totals[1] += stats->types[i].comment;
    totals[2] += stats->types[i].code;
  }

  // window width
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  // print file counts
  printf("%8d text files.\n", stats->unique + stats->ignored);
  printf("%8d unique files.\n", stats->unique);
  printf("%8d files ignored.\n", stats->ignored);
  printf("%8d directories ignored.\n", stats->ignored_dirs);
  printf("%8d files not accessible.\n", stats->fails);

  // dashed line across the terminal
  print_dashed_line(w.ws_col);
  
  // column names
  printf("  %-20s %20s %15s %15s %15s\n", "Language", "Files", "Blank", "Comment", "Code");

  // dashed line across the terminal
  print_dashed_line(w.ws_col);

  // print file counts
  // left-most column aligned left, right columns aligned right
  for (int i = 0; i < LANGS; i++) {
    printf("  %-20s %20d %15d %15d %15d\n", get_lang_name(i), stats->types[i].files,
        stats->types[i].blank, stats->types[i].comment, stats->types[i].code);
  }

  // dashed line across the terminal
  print_dashed_line(w.ws_col);
  printf("  %-20s %20d %15d %15d %15d\n", "SUM", stats->unique, totals[0], totals[1], totals[2]);
  print_dashed_line(w.ws_col);

}

