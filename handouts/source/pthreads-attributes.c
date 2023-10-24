#include <pthread.h>
#include <stdio.h>
#define NUM THREADS 5

/* Each thread will begin control in this function */
void *runner(void *param){
    
    /* do some work ... */
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    
    int i, scope, policy;
    
    pthread_t tid[NUM THREADS];
    pthread_attr_t attr;
    
    /* get the default attributes */
    pthread_attr_...........(&attr);
    
    /* first inquire on the current scope */
    if (pthread_attr_...........(&attr, &scope) != 0)
        fprintf(stderr, "Unable to get scheduling scope\n");
    else {
        if (scope == PTHREAD_SCOPE_PROCESS)
            printf("PTHREAD SCOPE PROCESS");
        else if (scope == PTHREAD_SCOPE_SYSTEM)
            printf("PTHREAD SCOPE SYSTEM");
        else
            fprintf(stderr, "Illegal scope value.\n");
    }
    /* get the current scheduling policy */
    if (pthread_attr_.............(&attr, &policy) != 0)
        fprintf(stderr, "Unable to get policy.\n");
    else {
        if (policy == SCHED_OTHER) printf("SCHED OTHER\n");
        else if (policy == SCHED_RR) printf("SCHED RR\n");
        else if (policy == SCHED_FIFO) printf("SCHED FIFO\n");
    }
    
    /* set the scheduling policy - FIFO, RR, or OTHER */
    if (pthread_attr_.............(&attr, SCHED_FIFO) != 0)
        fprintf(stderr, "Unable to set policy.\n");
    
    /* set the scheduling algorithm to PCS or SCS */
    pthread_attr_............(&attr, PTHREAD_SCOPE_SYSTEM);
    
    /* create the threads */
    for (i = 0; i < NUM THREADS; i++)
        pthread_............(........, ........, ........, ........);
    
    /* now join on each thread */   
    for (i = 0; i < NUM THREADS; i++)
        pthread_........(......., ........);
}



/******************************************************************************
 * FILE: hello_arg1.c
 * DESCRIPTION:
 *   A "hello world" Pthreads program which demonstrates one safe way
 *   to pass arguments to threads during thread creation.
 * AUTHOR: Blaise Barney
 * LAST REVISED: 01/29/09
 ******************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS8

char *messages[NUM_THREADS];

void *PrintHello(void *threadid)
{
  int *id_ptr, taskid;

  sleep(1);
  id_ptr = (int *) threadid;
  taskid = *id_ptr;
  printf("Thread %d: %s\n", taskid, messages[taskid]);
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS];
  int *taskids[NUM_THREADS];
  int rc, t;

  messages[0] = "English: Hello World!";
  messages[1] = "French: Bonjour, le monde!";
  messages[2] = "Spanish: Hola al mundo";
  messages[3] = "Turkish: Merhaba Dunya!";
  messages[4] = "German: Guten Tag, Welt!"; 
  messages[5] = "Russian: Zdravstvytye, mir!";
  messages[6] = "Japan: Sekai e konnichiwa!";
  messages[7] = "Latin: Orbis, te saluto!";

  for(t=0;t<NUM_THREADS;t++) {
    taskids[t] = (int *) malloc(sizeof(int));
    *taskids[t] = t;
    printf("Creating thread %d\n", t);
    rc = pthread_create(&threads[t], NULL, PrintHello, (void *) taskids[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  pthread_exit(NULL);
}

