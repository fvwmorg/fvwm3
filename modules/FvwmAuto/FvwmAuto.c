/* -*-c-*- */
/* This module, and the entire FvwmAuto program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "libs/ftime.h"

#include <unistd.h>
#include <ctype.h>
#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/wild.h"

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

/*
 *
 *  Procedure:
 *  Termination procedure : *not* a signal handler
 *
 */
RETSIGTYPE DeadPipe(int nonsense)
{
	(void)nonsense;
	myfprintf((stderr,"Leaving via DeadPipe\n"));
	exit(0);
	SIGNAL_RETURN;
}

/*
 *
 *  Procedure:
 *  Signal handler that tells the module to quit
 *
 */
static RETSIGTYPE
TerminateHandler(int signo)
{
	fvwmSetTerminate(signo);
	SIGNAL_RETURN;
}

/*
 *
 *  Procedure:
 *      main - start of module
 *
 */
int
main(int argc, char **argv)
{
	/* The struct holding the module info */
	static ModuleArgs* module;
	char *enter_fn="Silent Raise";        /* default */
	char *leave_fn=NULL;
	char *buf;
	int len;
	unsigned long m_mask;
	unsigned long mx_mask;
	unsigned long last_win = 0;   /* last window handled */
	unsigned long focus_win = 0;  /* current focus */
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
	Bool do_pass_id = False;
	Bool use_enter_mode = False;
	Bool use_leave_mode = False;
#ifdef DEBUG
	int count = 0;
#endif

#ifdef DEBUGTOFILE
	freopen(".FvwmAutoDebug","w",stderr);
#endif

	module = ParseModuleArgs(argc,argv,0); /* no alias in this module */
	if (module==NULL)
	{
		fprintf(stderr,"FvwmAuto Version "VERSION" should only be executed by fvwm!\n");
		exit(1);
	}


	if (module->user_argc < 1 || module->user_argc > 5)
	{
		fprintf(stderr,"FvwmAuto can use one to five arguments.\n");
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

	fd[0] = module->to_fvwm;
	fd[1] = module->from_fvwm;

	if ((timeout = atoi(module->user_argv[0]) ))
	{
		sec = timeout / 1000;
		usec = (timeout % 1000) * 1000;
	}
	else
	{
		sec = 0;
		usec = 1000;
	}
	delay = &value;

	n = 1;
	if (n < module->user_argc && module->user_argv[n])
	{
		char *token;

		/* -passid option */
		if (n < module->user_argc && *module->user_argv[n] && StrEquals(module->user_argv[n], "-passid"))
		{
			do_pass_id = True;
			n++;
		}
		if (n < module->user_argc && *module->user_argv[n] && StrEquals(module->user_argv[n], "-menterleave"))
		{
			/* enterleave mode */
			use_leave_mode = True;
			use_enter_mode = True;
			n++;
		}
		else if (n < module->user_argc && *module->user_argv[n] && StrEquals(module->user_argv[n], "-menter"))
		{
			/* enter mode */
			use_leave_mode = False;
			use_enter_mode = True;
			n++;
		}
		else if (n < module->user_argc && *module->user_argv[n] && StrEquals(module->user_argv[n], "-mfocus"))
		{
			/* focus mode */
			use_leave_mode = False;
			use_enter_mode = False;
			n++;
		}
		/*** enter command ***/
		if (n < module->user_argc && *module->user_argv[n] && StrEquals(module->user_argv[n], "Nop"))
		{
			/* nop */
			enter_fn = NULL;
			n++;
		}
		else if (n < module->user_argc)
		{
			/* override default */
			enter_fn = module->user_argv[n];
			n++;
		}
		/* This is a hack to prevent user interaction with old configs.
		 */
		if (enter_fn)
		{
			token = PeekToken(enter_fn, NULL);
			if (!StrEquals(token, "Silent"))
			{
				enter_fn = xstrdup(
					CatString2("Silent ", enter_fn));
			}
		}
		/*** leave command ***/
		if (n < module->user_argc && module->user_argv[n] && *module->user_argv[n] &&
		    StrEquals(module->user_argv[n], "Nop"))
		{
			/* nop */
			leave_fn = NULL;
			n++;
		}
		else if (n < module->user_argc)
		{
			/* leave function specified */
			leave_fn=module->user_argv[n];
			n++;
		}
		if (leave_fn)
		{
			token = PeekToken(leave_fn, NULL);
			if (!StrEquals(token, "Silent"))
			{
				leave_fn = xstrdup(
					CatString2("Silent ", leave_fn));
			}
		}
	}

	/* Exit if nothing to do. */
	if (!enter_fn && !leave_fn)
	{
		return -1;
	}

	if (use_enter_mode)
	{
		m_mask = 0;
		mx_mask = MX_ENTER_WINDOW | MX_LEAVE_WINDOW | M_EXTENDED_MSG;
	}
	else
	{
		mx_mask = M_EXTENDED_MSG;
		m_mask = M_FOCUS_CHANGE;
	}
	/* Disable special raise/lower support on general actions. *
	 * This works as expected in most of cases. */
	if (matchWildcards("*Raise*", CatString2(enter_fn, leave_fn)) ||
	    matchWildcards("*Lower*", CatString2(enter_fn, leave_fn)))
	{
		m_mask |= M_RAISE_WINDOW | M_LOWER_WINDOW;
	}

	/* migo (04/May/2000): It is simply incorrect to listen to raise/lower
	 * packets and change the state if the action itself has no raise/lower.
	 * Detecting whether to listen or not by the action name is good enough.
	 m_mask = M_FOCUS_CHANGE | M_RAISE_WINDOW | M_LOWER_WINDOW;
	*/

	SetMessageMask(fd, m_mask);
	SetMessageMask(fd, mx_mask);
	/* tell fvwm we're running */
	SendFinishedStartupNotification(fd);
	/* tell fvwm that we want to be lock on send */
	SetSyncMask(fd, m_mask);
	SetSyncMask(fd, mx_mask);

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
	buf = xmalloc(len);

	while (!isTerminated)
	{
		char raise_window_now;
		static char have_new_window = 0;

		FD_SET(fd[1], &in_fdset);

		myfprintf(
			(stderr, "\nstart %d (hnw = %d, usec = %d)\n", count++,
			 have_new_window, usec));
		/* fill in struct - modified by select() */
		delay->tv_sec = sec;
		delay->tv_usec = usec;
#ifdef DEBUG
		{
			char tmp[32];

			sprintf(tmp, "%d usecs", (delay) ?
				(int)delay->tv_usec : -1);
			myfprintf((stderr, "select: delay = %s\n",
				   (have_new_window) ? tmp : "infinite" ));
		}
#endif
		if (fvwmSelect(fd_width, &in_fdset, NULL, NULL,
			       (have_new_window) ? delay : NULL) == -1)
		{
			myfprintf(
				(stderr, "select: error! (%s)\n",
				 strerror(errno)));
			break;
		}

		raise_window_now = 0;
		if (FD_ISSET(fd[1], &in_fdset))
		{
			FvwmPacket *packet = ReadFvwmPacket(fd[1]);

			if (packet == NULL)
			{
				myfprintf(
					(stderr,
					 "Leaving because of null packet\n"));
				break;
			}

			myfprintf(
				(stderr,
				 "pw = 0x%x, fw=0x%x, rw = 0x%x, lw=0x%x\n",
				 (int)packet->body[0], (int)focus_win,
				 (int)raised_win, (int)last_win));

			switch (packet->type)
			{
			case MX_ENTER_WINDOW:
				focus_win = packet->body[0];
				if (focus_win != raised_win)
				{
					myfprintf((stderr,
						   "entered new window\n"));
					have_new_window = 1;
					raise_window_now = 0;
				}
				else if (focus_win == last_win)
				{
					have_new_window = 0;
				}
				else
				{
					myfprintf((stderr,
						   "entered other window\n"));
				}
				break;

			case MX_LEAVE_WINDOW:
				if (use_leave_mode)
				{
					if (focus_win == raised_win)
					{
						focus_win = 0;
					}
					myfprintf((stderr,
						   "left raised window\n"));
					have_new_window = 1;
					raise_window_now = 0;
				}
				break;

			case M_FOCUS_CHANGE:
				/* it's a focus package */
				focus_win = packet->body[0];
				if (focus_win != raised_win)
				{
					myfprintf((stderr,
						   "focus on new window\n"));
					have_new_window = 1;
					raise_window_now = 0;
				}
				else
				{
					myfprintf((stderr,
						   "focus on old window\n"));
				}
				break;

			case M_RAISE_WINDOW:
				myfprintf(
					(stderr, "raise packet 0x%x\n",
					 (int)packet->body[0]));
				raised_win = packet->body[0];
				if (have_new_window && focus_win == raised_win)
				{
					myfprintf(
						(stderr, "its the old window:"
						 " don't raise\n"));
					have_new_window = 0;
				}
				break;

			case M_LOWER_WINDOW:
				myfprintf(
					(stderr, "lower packet 0x%x\n",
					 (int)packet->body[0]));
				if (have_new_window &&
				    focus_win == packet->body[0])
				{
					myfprintf(
						(stderr,
						 "window was explicitly"
						 " lowered, don't raise it"
						 " again\n"));
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

			if (leave_fn &&
			    ((last_win && !use_leave_mode) ||
			     (raised_win && use_enter_mode)))
			{
				/* if focus_win isn't the root */
				if (do_pass_id)
				{
					sprintf(buf, "%s 0x%x\n", leave_fn,
						(int)last_win);
				}
				else
				{
					sprintf(buf, "%s\n", leave_fn);
				}
				SendInfo(fd, buf, last_win);
				if (use_enter_mode)
				{
					raised_win = 0;
				}
			}

			if (focus_win && enter_fn)
			{
				/* if focus_win isn't the root */
				if (do_pass_id)
				{
					sprintf(buf, "%s 0x%x\n", enter_fn,
						(int)focus_win);
				}
				else
				{
					sprintf(buf, "%s\n", enter_fn);
				}
				SendInfo(fd, buf, focus_win);
				raised_win = focus_win;
			}
			else if (focus_win && enter_fn == NULL)
			{
				raised_win = focus_win;
			}
			/* force fvwm to synchronise on slow X connections to
			 * avoid a race condition.  Still possible, but much
			 * less likely. */
			SendInfo(fd, "XSync", focus_win);

			/* switch to wait mode again */
			last_win = focus_win;
			have_new_window = 0;
		}
	} /* while */

	return 0;
}
