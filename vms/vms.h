/*----------------------------------------------------------------------------------------------------------------------------------
    Module : vms.h for Fvwm on Open VMS
    Author : Fabien Villard (Villard_F@Decus.Fr)
    Date   : 5-JAN-1999
    Action : specials functions and constants for running Fvwm with OpenVms.
----------------------------------------------------------------------------------------------------------------------------------*/
#include "ftime.h"

#ifndef DBG
  #define DBG  -1
  #define INFO 0
  #define WARN 1
  #define ERR  2
#endif

#define VMS_MAX_ARGUMENT_IN_CMD 255     /* - Maximum number in a command passed to builtin functions - */

void VMS_msg(int type,char *id,char *msg,...);
void VMS_SplitCommand(char *cmd, char **argums, int maxArgums, int *nbArgums);
int VMS_select_pipes(int nbPipes, fd_set *readFds, fd_set *writeFds, fd_set *filler, struct timeval *timeoutVal);

