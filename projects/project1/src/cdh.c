#include <unistd.h>
#include "common.h"
#include "cdh.h"

const char* CDH_FILENAME = ".mishell_cdh";

void get_cdh_path(char* path) {
  char* home = getenv("HOME");
  strcpy(path, home);
  strcat(path, "/");
  strcat(path, CDH_FILENAME);
}

void cdh_init(cdhistory* hist) {
  hist->size = 0;
  hist->head = NULL;
  hist->tail = NULL;
}

cdhnode* cdh_new_node(char* path) {
  cdhnode* node = (cdhnode*) malloc(sizeof(cdhnode));
  strcpy(node->path, path);
  node->saved = false;
  node->next = NULL;
  node->prev = NULL;
  return node;
}

void cdh_add_head(cdhistory* hist, char* path) {

  // check if already in history
  cdhnode* node = hist->head;
  while (node != NULL) {
    if (strcmp(node->path, path) == 0) {
      return;
    }
    node = node->next;
  }
  
  // create new node
  node = cdh_new_node(path);

  // add to head
  if (hist->size == 0) {
    hist->head = node;
    hist->tail = node;
  } else {
    node->next = hist->head;
    hist->head->prev = node;
    hist->head = node;
  }
  hist->size++;
  
}

void cdh_load(cdhistory* hist) {
  
  // open file
  char cdh_path[PATH_MAX];
  get_cdh_path(cdh_path);
  FILE* cdh_file = fopen(cdh_path, "r");

  // return if file doesn't exist
  if (cdh_file == NULL) {
    return;
  }

  // read file
  char line[PATH_MAX];
  while (fgets(line, PATH_MAX, cdh_file) != NULL) {
    line[strcspn(line, "\r\n")] = 0;
    cdh_add(hist, line);
  }

  // close file
  fclose(cdh_file);

}

void cdh_add(cdhistory* hist, char* path) {

  // check if already in history, remove
  cdhnode* node = hist->head;
  while (node != NULL) {
    if (strcmp(node->path, path) == 0) {
      if (node->prev != NULL) {
        node->prev->next = node->next;
      }
      if (node->next != NULL) {
        node->next->prev = node->prev;
      }
      if (node == hist->head) {
        hist->head = node->next;
      }
      if (node == hist->tail) {
        hist->tail = node->prev;
      }
      hist->size--;
      break;
    }
    node = node->next;
  }

  // create new node
  node = cdh_new_node(path);

  // add to tail
  if (hist->size == 0) {
    hist->head = node;
    hist->tail = node;
  } else {
    hist->tail->next = node;
    node->prev = hist->tail;
    hist->tail = node;
  }
  hist->size++;

  // remove from head if too big
  if (hist->size > CDH_MAX) {
    cdhnode* old = hist->head;
    hist->head->next->prev = NULL;
    hist->head = hist->head->next;
    free(old);
    hist->size--;
  }

}

int min(int a, int b) {
  return a < b ? a : b;
}

int cdh_print(cdhistory* hist) {

  // determine how many to print
  int count = min(10, hist->size);

  if (count == 0) {
    err_msg("cdh: no history");
    return ERROR;
  }

  // go back from tail
  int i = 0;
  cdhnode* node = hist->tail;
  while ((++i) < count)
    node = node->prev;

  // print from node to tail
  char* pretty_path;
  while (node != NULL) {
    pretty_path = prettify_path(node->path);
    i--;
    printf(COLOR_CYAN " %c  %d)" COLOR_RESET "  %s\n", (97 + i), i, pretty_path);
    free(pretty_path);
    node = node->next;
  }

  return SUCCESS;
}

char* cdh_get(cdhistory* hist, int index) {

  // check bounds
  if (index < 0 || index >= hist->size) {
    return NULL;
  }

  // go back from tail
  cdhnode* node = hist->tail;
  for (int i = 0; i < index; i++) {
    node = node->prev;
  }

  return node->path;
}

void cdh_save(cdhistory* hist) {

  // open file
  char cdh_path[PATH_MAX];
  get_cdh_path(cdh_path);
  FILE* cdh_file = fopen(cdh_path, "w+");

  if (cdh_file == NULL) {
    err_msg("cdh: could not save history");
    return;
  }

  // write file
  cdhnode* node = hist->head;
  while (node != NULL) {
    fprintf(cdh_file, "%s\n", node->path);
    node = node->next;
  }

  // close file
  fclose(cdh_file);

}

void cdh_free(cdhistory* hist) {
  cdhnode* node = hist->head;
  cdhnode* next;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
}
