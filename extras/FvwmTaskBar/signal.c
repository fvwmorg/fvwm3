/* Trying to handle signals */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

void alarm() {
  printf("Alarm !\n");
}

void main() {
  static struct itimerval timeout;
  static struct sigaction action;
  int i;

  timeout.it_interval.tv_sec=1;
  timeout.it_interval.tv_usec = 0;
  timeout.it_value.tv_sec=1;
  timeout.it_value.tv_usec = 0;
  action.sa_handler = (void (*)(void))alarm;
  sigemptyset(&action.sa_mask);
  action.sa_flags=SA_RESTART;
  sigaction(SIGALRM,&action,NULL); 
  setitimer(ITIMER_REAL,&timeout,NULL); 
  
  printf("Signal Test :\n");
  i = 0;
  while (1) {
    printf(".");
  }

}
