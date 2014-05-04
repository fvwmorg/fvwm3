/* -*-c-*- */
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

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "x.h"
#include "xmanager.h"

#include "libs/fvwmsignal.h"
#include "libs/Module.h"
#include "libs/FTips.h"
#include "libs/Parse.h"


char *MyName;
FlocaleWinString *FwinString;
int mods_unused = DEFAULT_MODS_UNUSED;

static RETSIGTYPE TerminateHandler(int);

char *copy_string(char **target, const char *src)
{
	int len;
	if (src == NULL)
	{
		len = 0;
		if (*target)
		{
			Free(*target);
		}
		return *target = NULL;
	}

	len = strlen(src);
	ConsoleDebug(CORE, "copy_string: 1: 0x%lx\n", (unsigned long)*target);

	if (*target)
	{
		Free(*target);
	}

	ConsoleDebug(CORE, "copy_string: 2\n");
	*target = xmalloc((len + 1) * sizeof(char));
	strcpy(*target, src);
	ConsoleDebug(CORE, "copy_string: 3\n");
	return *target;
}

#ifdef TRACE_MEMUSE

long MemUsed = 0;

void Free(void *p)
{
	struct malloc_header *head = (struct malloc_header *)p;

	if (p != NULL)
	{
		head--;
		if (head->magic != MALLOC_MAGIC)
		{
			fprintf(stderr, "Corrupted memory found in Free\n");
			abort();
			return;
		}
		if (head->len > MemUsed)
		{
			fprintf(stderr, "Free block too big\n");
			return;
		}
		MemUsed -= head->len;
		Free(head);
	}
}

void PrintMemuse(void)
{
	ConsoleDebug(CORE, "Memory used: %ld\n", MemUsed);
}

#else

void Free(void *p)
{
	if (p != NULL)
	{
		free(p);
	}
}

void PrintMemuse(void)
{
}

#endif


static RETSIGTYPE
TerminateHandler(int sig)
{
	fvwmSetTerminate(sig);
	SIGNAL_RETURN;
}


void
ShutMeDown(int flag)
{
	ConsoleDebug(CORE, "Bye Bye\n");
	exit(flag);
}

void
DeadPipe(int nothing)
{
	(void)nothing;
	ShutMeDown(0);
	SIGNAL_RETURN;
}

static void
main_loop(void)
{
	fd_set readset, saveset;
	fd_set_size_t fd_width = x_fd;
	unsigned long ms;
	struct timeval tv;

	/*
	 * Calculate which is descriptor is numerically higher:
	 * this determines how wide the descriptor set needs to be ...
	 */
	if (fd_width < fvwm_fd[1])
	{
		fd_width = fvwm_fd[1];
	}
	++fd_width;

	FD_ZERO(&saveset);
	FD_SET(fvwm_fd[1], &saveset);
	FD_SET(x_fd, &saveset);

	tv.tv_sec  = 60;
	tv.tv_usec = 0;

	while (!isTerminated)
	{
		/* Check the pipes for anything to read, and block if
		 * there is nothing there yet ...
		 */
		readset = saveset;

		if (fvwmSelect(fd_width, &readset, NULL, NULL, &tv) < 0)
		{
			ConsoleMessage(
				"Internal error with select: errno=%s\n",
				strerror(errno));
		}
		else
		{

			if (FD_ISSET(x_fd, &readset) || FPending(theDisplay))
			{
				xevent_loop();
			}
			if (FD_ISSET(fvwm_fd[1], &readset))
			{
				ReadFvwmPipe();
			}

		}
		if ((ms = FTipsCheck(theDisplay)))
		{
			tv.tv_sec  = ms / 1000;
			tv.tv_usec = (ms % 1000) * 1000;
		}
		else
		{
			tv.tv_sec  = 60;
			tv.tv_usec = 0;
		}
	} /* while */
}

int
main(int argc, char **argv)
{
	int i;

#ifdef HAVE_LIBEFENCE
	extern int EF_PROTECT_BELOW, EF_PROTECT_FREE;

	EF_PROTECT_BELOW = 1;
	EF_PROTECT_FREE = 1;
#endif

	FlocaleInit(LC_CTYPE, "", "", "FvwmIconMan");

	OpenConsole(OUTPUT_FILE);

	init_globals();
	init_winlists();

	MyName = GetFileNameFromPath(argv[0]);

	if (argc >= 7)
	{
		if (strcasecmp(argv[6], "-Transient") == 0)
		{
			globals.transient = 1;
			
			/* Optionally accept an alias to use as the transient
			 * FvwmIconMan instance.
			 */
			if (argv[7])
				MyName = argv[7];
		}
		else
		{
			MyName = argv[6];
		}
	}
	ModuleLen = strlen(MyName) + 1;
	Module = xmalloc(ModuleLen+1);
	*Module = '*';
	strcpy(Module+1, MyName);

	if (argc < 6)
	{
		fprintf(stderr,
			"%s version %s should only be executed by fvwm!\n",
			MyName, VERSION);
		ShutMeDown(1);
	}
	fvwm_fd[0] = atoi(argv[1]);
	fvwm_fd[1] = atoi(argv[2]);
	init_display();

#ifdef HAVE_SIGACTION
	{
		struct sigaction  sigact;

		sigemptyset(&sigact.sa_mask);
		sigaddset(&sigact.sa_mask, SIGPIPE);
		sigaddset(&sigact.sa_mask, SIGINT);
		sigaddset(&sigact.sa_mask, SIGHUP);
		sigaddset(&sigact.sa_mask, SIGTERM);
		sigaddset(&sigact.sa_mask, SIGQUIT);
# ifdef SA_RESTART
		sigact.sa_flags = SA_RESTART;
# else
		sigact.sa_flags = 0;
# endif
		sigact.sa_handler = TerminateHandler;

		sigaction(SIGPIPE, &sigact, NULL);
		sigaction(SIGINT,  &sigact, NULL);
		sigaction(SIGHUP,  &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);
		sigaction(SIGQUIT, &sigact, NULL);
	}
#else
	/* We don't have sigaction(), so fall back to less robust methods. */
#ifdef USE_BSD_SIGNALS
	fvwmSetSignalMask(
		sigmask(SIGPIPE) |
		sigmask(SIGINT) |
		sigmask(SIGHUP) |
		sigmask(SIGTERM) |
		sigmask(SIGQUIT));
#endif
	signal(SIGPIPE, TerminateHandler);
	signal(SIGINT,  TerminateHandler);
	signal(SIGHUP,  TerminateHandler);
	signal(SIGTERM, TerminateHandler);
	signal(SIGQUIT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGPIPE, 0);
	siginterrupt(SIGINT, 0);
	siginterrupt(SIGHUP, 0);
	siginterrupt(SIGTERM, 0);
	siginterrupt(SIGQUIT, 0);
#endif
#endif

	read_in_resources();
	FlocaleAllocateWinString(&FwinString);

	for (i = 0; i < globals.num_managers; i++)
	{
		X_init_manager(i);
	}

	assert(globals.managers);

	SetMessageMask(
		fvwm_fd, M_CONFIGURE_WINDOW | M_RES_CLASS | M_RES_NAME |
		M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICON_NAME |
		M_DEICONIFY | M_ICONIFY | M_ICON_LOCATION | M_END_WINDOWLIST |
		M_NEW_DESK | M_NEW_PAGE | M_FOCUS_CHANGE | M_WINDOW_NAME |
		M_CONFIG_INFO | M_SENDCONFIG | M_VISIBLE_NAME |
		M_MINI_ICON | M_STRING | M_WINDOWSHADE | M_DEWINDOWSHADE);
  /* extended messages */
  SetMessageMask(fvwm_fd, MX_VISIBLE_ICON_NAME | MX_PROPERTY_CHANGE);

  SendText(fvwm_fd, "Send_WindowList", 0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fvwm_fd);

  /* Lock on send only for iconify and deiconify (for NoIconAction) */
  SetSyncMask(fvwm_fd, M_DEICONIFY | M_ICONIFY);
  SetNoGrabMask(fvwm_fd, M_DEICONIFY | M_ICONIFY);

  main_loop();

  return 0;
}
