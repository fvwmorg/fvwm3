/* This module, and the entire FvwmAuto program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 * reworked by A.Kadlec (albrecht@auto.tuwien.ac.at) 09/96
 * to support an arbitrary enter_fn & leave_fn (command line arguments)
 * completely reworked, while I was there.
 *
 * Modified by John Aughey (jha@cs.purdue.edu) 11/96
 * to not perform any action when entering the root window
 *
 * Minor patch by C. Hines 12/96.
 *
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#define TRUE 1
#define FALSE

#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "libs/Module.h"
#include "libs/fvwmlib.h"

/* here is the old double parens trick. */
/* #define DEBUG */
#ifdef DEBUGTOFILE
#define DEBUG
#endif
#ifdef DEBUG
#define myfprintf(X) \
  fprintf X;\
  fflush (stderr);
#else
#define myfprintf(X)
#endif

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  myfprintf((stderr,"Leaving via DeadPipe\n"));
  exit(0);
}


/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *enter_fn="Silent Raise";	/* default */
  char *leave_fn=NULL;
  unsigned long last_win = 0;	/* last window handled */
  unsigned long focus_win = 0;	/* current focus */
  unsigned long raised_win = 0;
  fd_set_size_t fd_width;
  int fd[2];
  int timeout;
  int sec = 0;
  int usec = 0;
  struct timeval value;
  struct timeval *delay;
  fd_set in_fdset;
  char raise_immediately;
#ifdef DEBUG
  int count = 0;
  char big_int_area[32];
#endif

#ifdef DEBUGTOFILE
  freopen(".FvwmAutoDebug","w",stderr);
#endif

  if(argc < 7 || argc > 9)
  {
    fprintf(stderr,"FvwmAuto can use one to three arguments.\n");
    exit(1);
  }

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  if ((timeout = atoi(argv[6])))
  {
    sec = timeout / 1000;
    usec = (timeout % 1000) * 1000;
    delay=&value;
    raise_immediately = 0;
  }
  else
  {
    raise_immediately = 1;
    delay = NULL;
  }

  if (argv[7])			/* if specified */
  {
    if (*argv[7] && !StrEquals(argv[7],"NOP")) /* not empty */
      enter_fn=argv[7];		/* override default */
    else
      enter_fn=NULL;		/* nop */

    if (argv[8] && *argv[8] && !StrEquals(argv[8],"NOP"))
      /* leave function specified */
      leave_fn=argv[8];
  }

  fd_width = GetFdWidth();
  
  SetMessageMask(fd,M_FOCUS_CHANGE|M_RAISE_WINDOW);
  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);
  /* tell fvwm that we want to be lock on send */
  SetSyncMask(fd, M_FOCUS_CHANGE|M_RAISE_WINDOW);

  while(1)
  {
    char raise_window_now;
    static char have_new_window = 0;

    FD_ZERO(&in_fdset);
    FD_SET(fd[1],&in_fdset);

    myfprintf((stderr,"\nstart %d (ri = %d, hnw = %d, usec = %d)\n",
               count++, raise_immediately, have_new_window, usec));
    if (!raise_immediately)
    {
      /* fill in struct - modified by select() */
      delay->tv_sec = sec;
      delay->tv_usec = usec;
    }
    else
    {
      /* delay is already a NULL pointer */
    }
#ifdef DEBUG
    sprintf(big_int_area,"%d usecs",delay->tv_usec);
#endif
    myfprintf((stderr,"select: delay = %s\n",
               (have_new_window) ? big_int_area : "infinite" ));
    select(fd_width, SELECT_FD_SET_CAST &in_fdset, 0, 0,
	   (have_new_window) ? delay : NULL);

    raise_window_now = 0;
    if (FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket *packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
      {
        myfprintf((stderr,"Leaving because of null packet\n"));
	exit(0);
      }
      myfprintf((stderr,"pw = 0x%x, fw=0x%x, rw = 0x%x, lw=0x%x\n",
                packet->body[0],focus_win,raised_win,last_win));
      switch (packet->type)
      {
      case M_FOCUS_CHANGE:
	/* it's a focus package */
	focus_win = packet->body[0];
        myfprintf((stderr,"focus change\n"));
	if (focus_win != raised_win)
	{
          myfprintf((stderr,"its a new window\n"));
	  have_new_window = 1;
	  raise_window_now = raise_immediately;
	}
#ifdef DEBUG
        else myfprintf((stderr,"no new window\n"));
#endif
	break;
      case M_RAISE_WINDOW:
        myfprintf((stderr,"raise packet 0x%x\n", packet->body[0]));
	raised_win = packet->body[0];
	if (have_new_window && focus_win == raised_win)
	{
          myfprintf((stderr,"its the old window: don't raise\n"));
	  have_new_window = 0;
	}
	break;
      }
      SendInfo(fd, "UNLOCK", 0);
    }
    else
    {
      if (have_new_window)
      {
        myfprintf((stderr,"must raise now\n"));
	raise_window_now = 1;
      }
    }
    if (raise_window_now)
    {
      myfprintf((stderr,"raising 0x%x\n", focus_win));
      if (last_win && leave_fn)
      {
	/* if focus_win isn't the root */
	SendInfo(fd,leave_fn,last_win);
      }
      if (focus_win && enter_fn)
      {
	/* if focus_win isn't the root */
	SendInfo(fd,enter_fn,focus_win);
	raised_win = focus_win;
      }
      /* switch to wait mode again */
      last_win = focus_win;
      have_new_window = 0;
    }
  }
  return 0;
}



