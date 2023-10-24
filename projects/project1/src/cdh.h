#ifndef CDH_H
#define CDH_H

#include "common.h"

#define CDH_MAX 10

struct cdh_node_t {
  bool saved;
  char path[PATH_MAX];
  struct cdh_node_t *next;
  struct cdh_node_t *prev;
};

struct cdh_t {
  int size;
  struct cdh_node_t *head;
  struct cdh_node_t *tail;
};


void cdh_init(cdhistory* hist);
void cdh_load(cdhistory* hist);
void cdh_free(cdhistory* hist);
void cdh_save(cdhistory* hist);
void cdh_add(cdhistory* hist, char* path);
int cdh_print(cdhistory* hist);
char* cdh_get(cdhistory* hist, int index);

#endif
