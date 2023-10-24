#ifndef SLEEP_HH
#define SLEEP_HH
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


 /******************************************************************************
  pthread_sleep takes an integer number of seconds to pause the current thread
  original by Yingwu Zhu
  updated by Muhammed Nufail Farooqi
  *****************************************************************************/
int pthread_sleep (int seconds);

#endif
