#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

/**************************************************************************
 * 
 * Sleep for n microseconds
 *
 *************************************************************************/
void sleep_a_little(int n)
{
  struct timeval value;
  
  if (n <= 0)
    return;
  
  value.tv_usec = n % 1000000;
  value.tv_sec = n / 1000000;
  
  (void) select(1, 0, 0, 0, &value);
}


