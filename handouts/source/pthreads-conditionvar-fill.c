
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS  3
#define TCOUNT 10
#define COUNT_LIMIT 12

int     count = 0;
pthread_mutex_t ....................;
pthread_cond_t ....................;
void *inc_count(void *t);
void *watch_count(void *t);
int main(int argc, char *argv[])
{
  int i, rc; 
  long t1=1, t2=2, t3=3;
  pthread_t threads[3];
  pthread_attr_t attr;

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&count_mutex, NULL);
  pthread_cond_init (&count_threshold_cv, NULL);

  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
  pthread_create(..........., ..........., ..........., ...........);  
  pthread_create(..........., ..........., ..........., ...........);
  pthread_create(..........., ..........., ..........., ...........);
    
  /* Wait for all threads to complete */
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_.........(threads[i], NULL);
  }
  printf ("Main(): Waited and joined with %d threads. Final value of count = %d. Done.\n", 
          NUM_THREADS, count);

  /* Clean up and exit */
  pthread_attr_destroy(&attr);
    
  pthread_...............(&count_mutex);
 
  pthread_...............(&count_threshold_cv);
    
  pthread_exit (NULL);
}














void *inc_count(void *t)
{
    int i;
    long my_id = (long)t;
    for (i=0; i < TCOUNT; i++) {
        pthread_.................(.............);
        count++;
        
        /*
         Check the value of count and signal waiting thread when condition is
         reached.  Note that this occurs while mutex is locked.*/
        if (count == COUNT_LIMIT) {
            printf("inc_count(): thread %ld, count = %d  Threshold reached. ",
                   my_id, count);
            
            pthread_.............(................);
            printf("Just sent signal.\n");
        }
        printf("inc_count(): thread %ld, count = %d, unlocking mutex\n",
               my_id, count);
        pthread_..................(................);
        
        /* Do some work so threads can alternate on mutex lock */
        sleep(1);
    }
    pthread_exit(NULL);    
}
void *watch_count(void *t) {
    
    long my_id = (long)t;
    printf("Starting watch_count(): thread %ld\n", my_id);
    
    /* Lock mutex and wait for signal.  Note that the pthread_cond_wait routine
     will automatically and atomiscally unlock mutex while it waits.
     Also, note that if COUNT_LIMIT is reached before this routine is run by
     the waiting thread, the loop will be skipped to prevent pthread_cond_wait
     from never returning. */
    
    pthread_................(...............);
    
    while (count < COUNT_LIMIT) {
        printf("watch_count(): thread %ld Count= %d. Going into wait...\n", my_id,count);
        
        pthread_............(.............., ..............);
        
        printf("watch_count(): thread %ld Condition signal received. Count= %d\n", my_id,count);
        printf("watch_count(): thread %ld Updating the value of count...\n", my_id,count);
        count += 125;
        printf("watch_count(): thread %ld count now = %d.\n", my_id, count);
    }
    printf("watch_count(): thread %ld Unlocking mutex.\n", my_id);  
    pthread_................(............);
    pthread_exit(NULL);
}
