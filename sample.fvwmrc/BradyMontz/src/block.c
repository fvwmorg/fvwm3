#include <stdio.h>
#include <signal.h>

int main (void)
{
  sigset_t set;
  
  sigprocmask (SIG_UNBLOCK, NULL, &set);
  sigsuspend (&set);
  return 0;
}
