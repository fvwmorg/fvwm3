#include <signal.h>
#include <stdlib.h>

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  exit(0);
}


void SetupPipeHandler(void)
{
  signal(SIGPIPE,DeadPipe);
}
