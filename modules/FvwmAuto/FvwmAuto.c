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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"

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


static RETSIGTYPE TerminateHandler(int signo);

/***********************************************************************
 *
 *  Procedure:
 *  Termination procedure : *not* a signal handler
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  (void)nonsense;
  myfprintf((stderr,"Leaving via DeadPipe\n"));
  exit(0);
}

/***********************************************************************
 *
 *  Procedure:
 *  Signal handler that tells the module to quit
 *
 ***********************************************************************/
static RETSIGTYPE
TerminateHandler(int signo)
{
  fvwmSetTerminate(signo);
}

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int
main(int argc, char **argv)
{
  char *enter_fn="Silent Raise";	/* default */
  char *leave_fn=NULL;
  char *buf;
  int len;
  unsigned long m_mask;
  unsigned long last_win = 0;	/* last window handled */
  unsigned long focus_win = 0;	/* current focus */
  unsigned long raised_win = 0;
  fd_set_size_t fd_width;
  int fd[2];
  int timeout;
  int sec = 0;
  int usec = 0;
  int n;
  struct timeval value;
  struct timeval *delay;
  fd_set in_fdset;
  char raise_immediately;
#ifdef DEBUG
  int count = 0;
  char big_int_area[32];
#endif
  Bool do_pass_id = False;

#ifdef DEBUGTOFILE
  freopen(".FvwmAutoDebug","w",stderr);
#endif

  if(argc < 7 || argc > 10)
  {
    fprintf(stderr,"FvwmAuto can use one to four arguments.\n");
    exit(1);
  }

  /* Dead pipes mean fvwm died */
#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGTERM);
#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
# else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
  }
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
		     sigmask(SIGINT)  |
		     sigmask(SIGHUP)  |
		     sigmask(SIGQUIT) |
		     sigmask(SIGTERM) );
#endif

  signal(SIGPIPE, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 0);
  siginterrupt(SIGINT, 0);
  siginterrupt(SIGHUP, 0);
  siginterrupt(SIGQUIT, 0);
  siginterrupt(SIGTERM, 0);
#endif
#endif

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  if ((timeout = atoi(argv[6])))
  {
    sec = timeout / 1000;
    usec = (timeout % 1000) * 1000;
    delay = &value;
    raise_immediately = 0;
  }
  else
  {
    raise_immediately = 1;
    delay = NULL;
  }

  n = 7;
  if (argv[n])			/* if specified */
  {
    char *token;

    /* -passid option */
    if (*argv[n] && StrEquals(argv[n], "-passid"))
    {
       do_pass_id = True;
       n++;
    }
	/*** enter command ***/
    if (*argv[n] && !StrEquals(argv[n],"NOP")) /* not empty */
    {
      enter_fn = argv[n];		/* override default */
      n++;
    }
    else
    {
      enter_fn = NULL;		/* nop */
    }
    /* This is a hack to prevent user interaction with old configs. */
    if (enter_fn)
    {
      token = PeekToken(enter_fn, NULL);
      if (!StrEquals(token, "Silent"))
	enter_fn = safestrdup(CatString2("Silent ", enter_fn));
    }
    /*** leave command ***/
    if (argv[n] && *argv[n] && !StrEquals(argv[n],"NOP"))
    {
      /* leave function specified */
      leave_fn=argv[n];
      n++;
    }
    if (leave_fn)
    {
      token = PeekToken(leave_fn, NULL);
      if (!StrEquals(token, "Silent"))
	leave_fn = safestrdup(CatString2("Silent ", leave_fn));
    }
  }

  /* Exit if nothing to do. */
  if (!enter_fn && !leave_fn)
    return -1;

  m_mask = M_FOCUS_CHANGE;
  /* Disable special raise/lower support on general actions. *
   * This works as expected in most of cases. */
  if (matchWildcards("*Raise*", CatString2(enter_fn, leave_fn)) ||
      matchWildcards("*Lower*", CatString2(enter_fn, leave_fn)))
    m_mask |= M_RAISE_WINDOW | M_LOWER_WINDOW;

  /* migo (04/May/2000): It is simply incorrect to listen to raise/lower
   * packets and change the state if the action itself has no raise/lower.
   * Detecting whether to listen or not by the action name is good enough.
  m_mask = M_FOCUS_CHANGE | M_RAISE_WINDOW | M_LOWER_WINDOW;
   */

  SetMessageMask(fd, m_mask);
  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);
  /* tell fvwm that we want to be lock on send */
  SetSyncMask(fd, m_mask);

  fd_width = fd[1] + 1;
  FD_ZERO(&in_fdset);

  /* create the command buffer */
  len = 0;
  if (enter_fn != 0)
  {
    len = strlen(enter_fn);
  }
  if (leave_fn != NULL)
  {
    len = max(len, strlen(leave_fn));
  }
  if (do_pass_id)
  {
    len += 32;
  }
  buf = safemalloc(len);

  while( !isTerminated )
  {
    char raise_window_now;
    static char have_new_window = 0;

    FD_SET(fd[1], &in_fdset);

    myfprintf((stderr, "\nstart %d (ri = %d, hnw = %d, usec = %d)\n",
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
    sprintf(big_int_area, "%d usecs", (int)delay->tv_usec);
    myfprintf((stderr, "select: delay = %s\n",
	       (have_new_window) ? big_int_area : "infinite" ));
#endif
    if (fvwmSelect(fd_width,
		   &in_fdset, NULL, NULL,
		   (have_new_window) ? delay : NULL) == -1)
    {
      myfprintf((stderr, "select: error! (%s)\n", strerror(errno)));
      break;
    }

    raise_window_now = 0;
    if (FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket *packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
      {
	myfprintf((stderr, "Leaving because of null packet\n"));
	break;
      }

      myfprintf((stderr, "pw = 0x%x, fw=0x%x, rw = 0x%x, lw=0x%x\n",
		 (int)packet->body[0], (int)focus_win, (int)raised_win,
		 (int)last_win));

      switch (packet->type)
      {
      case M_FOCUS_CHANGE:
	/* it's a focus package */
	focus_win = packet->body[0];
	myfprintf((stderr, "focus change\n"));

	if (focus_win != raised_win)
	{
	  myfprintf((stderr, "its a new window\n"));
	  have_new_window = 1;
	  raise_window_now = raise_immediately;
	}
#ifdef DEBUG
	else fprintf(stderr, "no new window\n");
#endif
	break;

      case M_RAISE_WINDOW:
	myfprintf((stderr, "raise packet 0x%x\n", (int)packet->body[0]));
	raised_win = packet->body[0];
	if (have_new_window && focus_win == raised_win)
	{
	  myfprintf((stderr, "its the old window: don't raise\n"));
	  have_new_window = 0;
	}
	break;

      case M_LOWER_WINDOW:
	myfprintf((stderr, "lower packet 0x%x\n", (int)packet->body[0]));
	if (have_new_window && focus_win == packet->body[0])
	{
	  myfprintf((stderr,
		     "window was explicitly lowered, don't raise it again\n"));
	  have_new_window = 0;
	}
	break;
      } /* switch */
      SendUnlockNotification(fd);
    }
    else
    {
      if (have_new_window)
      {
	myfprintf((stderr, "must raise now\n"));
	raise_window_now = 1;
      }
    }

    if (raise_window_now)
    {
      myfprintf((stderr, "raising 0x%x\n", (int)focus_win));

      if (last_win && leave_fn)
      {
	/* if focus_win isn't the root */
	if (do_pass_id)
	{
	  sprintf(buf, "%s 0x%x\n", leave_fn, (int)last_win);
	}
	else
	{
	  sprintf(buf, "%s\n", leave_fn);
	}
	SendInfo(fd, buf, last_win);
      }

      if (focus_win && enter_fn)
      {
	/* if focus_win isn't the root */
	if (do_pass_id)
	{
	  sprintf(buf, "%s 0x%x\n", enter_fn, (int)focus_win);
	}
	else
	{
	  sprintf(buf, "%s\n", enter_fn);
	}
	SendInfo(fd, buf, focus_win);
	raised_win = focus_win;
      }

      /* switch to wait mode again */
      last_win = focus_win;
      have_new_window = 0;
    }
  } /* while */

  return 0;
}
