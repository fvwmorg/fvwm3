/* -*-c-*- */
/*
 * Copyright (C) 1994 Mark Boyns (boyns@sdsu.edu) and
 *                    Mark Scott (mscott@mcd.mot.com)
 *               1996-1998 Albrecht Kadlec (albrecht@auto.tuwien.ac.at)
 *
 * FvwmEvent version 1.0
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This module is based on FvwmModuleDebugger which has the following
 * copyright:
 *
 * This module, and the entire ModuleDebugger program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
/*
 * rplay includes:
 */
#ifdef HAVE_RPLAY
#include <rplay.h>
#undef M_ERROR /* Solaris fix */
#endif

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/Strings.h"

/*
 * fvwm includes:
 */
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include "libs/ftime.h"
#include <ctype.h>

/* ---------------------------- local definitions --------------------------- */

#define BUILTIN_STARTUP         (MAX_TOTAL_MESSAGES)
#define BUILTIN_SHUTDOWN        (MAX_TOTAL_MESSAGES + 1)
#define BUILTIN_UNKNOWN         (MAX_TOTAL_MESSAGES + 2)
#define MAX_BUILTIN             3
#define MAX_RPLAY_HOSTNAME_LEN  MAX_MODULE_INPUT_TEXT_LEN
#define SYNC_MASK_M             (M_DESTROY_WINDOW | M_LOWER_WINDOW | \
	M_RESTACK | M_CONFIGURE_WINDOW)
#define SYNC_MASK_MX            (M_EXTENDED_MSG)

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef struct
{
	char *name;
	int action_arg;
} event_entry;

/* ---------------------------- forward declarations ------------------------ */

void execute_event(short, unsigned long*);
void config(void);
RETSIGTYPE DeadPipe(int);
static RETSIGTYPE TerminateHandler(int);

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

static char *MyName;
static int MyNameLen;
static int fd[2];
static char *cmd_line = NULL;
static time_t audio_delay = 0; /* seconds */
static time_t last_time = 0;
static time_t now;
static time_t start_audio_delay = 0;
/* don't tag on the windowID by default */
static Bool PassID = False;
static Bool audio_compat = False;
static char *audio_play_dir = NULL;

#define ARG_NO_WINID 1024  /* just a large number */

static event_entry event_table[MAX_TOTAL_MESSAGES+MAX_BUILTIN] =
{
	{ "new_page", -1 },
	{ "new_desk", 0 | ARG_NO_WINID },
	{ "old_add_window", 0 },
	{ "raise_window", 0 },
	{ "lower_window", 0 },
	{ "old_configure_window", 0 },
	{ "focus_change", 0 },
	{ "destroy_window", 0 },
	{ "iconify", 0 },
	{ "deiconify", 0 },
	{ "window_name", 0 },
	{ "icon_name", 0 },
	{ "res_class", 0 },
	{ "res_name", 0 },
	{ "end_windowlist", -1 },
	{ "icon_location", 0 },
	{ "map", 0 },
	{ "error", -1 },
	{ "config_info", -1 },
	{ "end_config_info", -1 },
	{ "icon_file", 0 },
	{ "default_icon", -1 },
	{ "string", -1 },
	{ "mini_icon", 0 },
	{ "windowshade", 0 },
	{ "dewindowshade", 0 },
	{ "visible_name", 0 },
	{ "sendconfig", -1 },
	{ "restack", -1 },
	{ "add_window", 0 },
	{ "configure_window", 0 },
	/* begin of extended message area */
	{ "visible_icon_name", 0 },
	{ "enter_window", 0 },
	{ "leave_window", 0 },
	{ "property_change", 0},
	/* built in events */
#ifdef M_BELL
	{ "beep", -1 },
#endif
#ifdef M_TOGGLEPAGE
	{ "toggle_paging", -1 },
#endif
	/* add builtins here */
	{ "startup", -1 },
	{ "shutdown", -1 },
	{ "unknown", -1}
};

/* define the action table  */
static char *action_table[MAX_TOTAL_MESSAGES+MAX_BUILTIN];

#ifdef HAVE_RPLAY
static int rplay_fd = -1;
static RPLAY *rplay_table[MAX_TOTAL_MESSAGES+MAX_BUILTIN];
#endif

static unsigned int m_selected = 0;
static unsigned int mx_selected = 0;
static unsigned int m_sync = 0;
static unsigned int mx_sync = 0;


static volatile sig_atomic_t isTerminated = False;

/* ---------------------------- local functions ----------------------------- */

void unlock_event(unsigned long evtype)
{
	unsigned int mask;

	if (evtype & M_EXTENDED_MSG)
	{
		mask = mx_sync;
	}
	else
	{
		mask = m_sync;
	}
	if (evtype & mask)
	{
		SendUnlockNotification(fd);
	}

	return;
}

int main(int argc, char **argv)
{
	char *s;
	unsigned long header[FvwmPacketHeaderSize];
	unsigned long body[FvwmPacketBodyMaxSize];
	int total, remaining, count, event;
	int is_extended_msg;

	cmd_line = (char *)safemalloc(1);
	*cmd_line = 0;
	/* Save our program  name - for error events */
	if ((s=strrchr(argv[0], '/')))
	{
		/* strip path */
		s++;
	}
	else
	{
		/* no slash */
		s = argv[0];
	}
	if (argc == 7)
	{
		if (strcmp(argv[6], "-audio") == 0)
		{
			audio_compat = True;
		}
		else
		{
			/* use an alias */
			s = argv[6];
		}
	}

	/* account for '*' */
	MyNameLen=strlen(s)+1;
	/* account for \0 */
	MyName = safemalloc(MyNameLen+1);
	*MyName='*';
	/* append name */
	strcpy(MyName+1, s);
	if (StrEquals("FvwmAudio", s))
	{
                /* catch the FvwmAudio alias */
		audio_compat = True;
	}

	/* Now MyName is defined */
	if ((argc != 6)&&(argc != 7))
	{
		fprintf(stderr, "%s Version "VERSION" should only be "
			"executed by fvwm!\n", MyName+1);
		exit(1);
	}

#ifdef HAVE_SIGACTION
	{
		struct sigaction    sigact;

		sigemptyset(&sigact.sa_mask);
# ifdef SA_INTERRUPT
		sigact.sa_flags = SA_INTERRUPT;
# else
		sigact.sa_flags = 0;
# endif
		sigact.sa_handler = TerminateHandler;

		/* Dead pipe == Fvwm died */
		sigaction(SIGPIPE,&sigact,NULL);
		/* "polite" termination signal */
		sigaction(SIGTERM,&sigact,NULL);
	}
#else
	/* We don't have sigaction(), so fall back to less robust methods. */
	signal(SIGPIPE, TerminateHandler);
	signal(SIGTERM, TerminateHandler);
#endif

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);

	/* configure events */
	config();
	/* Startup event */
	execute_event(BUILTIN_STARTUP, NULL);
	if (start_audio_delay)
	{
		last_time = time(0);
	}
	/* tell fvwm we're running */
	SetMessageMask(fd, m_selected);
	SetMessageMask(fd, mx_selected | M_EXTENDED_MSG);
	m_sync = (m_selected & SYNC_MASK_M);
	mx_sync = (mx_selected & SYNC_MASK_M) | M_EXTENDED_MSG;
	/* migo (19-Aug-2000): synchronize on M_DESTROY_WINDOW
	 * dv (6-Jul-2002: synchronize on a number of events that can hide or
	 * destroy a window */
	if (m_sync)
	{
		SetSyncMask(fd, m_sync);
	}
	if (mx_sync != M_EXTENDED_MSG)
	{
		SetSyncMask(fd, mx_sync);
	}
	mx_sync &= ~M_EXTENDED_MSG;
	SendFinishedStartupNotification(fd);

	/* main loop */
	while (!isTerminated)
	{
		unsigned long msg_bit;

		if ((count = read(fd[1], header, FvwmPacketHeaderSize_byte)) <=
		    0)
		{
			isTerminated = 1;
			continue;
		}
		/* if this read is interrrupted EINTR, the wrong event is
		 * triggered! */

		if (header[0] != START_FLAG)
		{
			/* should find something better for resyncing */
			unlock_event(header[1]);
			continue;
		}

		/* Ignore events that occur during the delay period. */
		now = time(0);

		/* junk the event body */
		total=0;
		remaining = (header[2] - FvwmPacketHeaderSize) *
			sizeof(unsigned long);
		while (remaining)
		{
			if ((count=read(fd[1],&body[total],remaining)) < 0)
			{
				isTerminated = 1;
				unlock_event(header[1]);
				continue;
			}
			remaining -= count;
			total +=count;
		}

		if (now < last_time + audio_delay + start_audio_delay)
		{
			/* quash event */
			unlock_event(header[1]);
			continue;
		}
		else
		{
			start_audio_delay = 0;
		}

		/* event will equal the number of shifts in the base-2
		 * header[1] number.  Could use log here but this should be
		 * fast enough. */
		event = -1;
		msg_bit = header[1];
		is_extended_msg = (msg_bit & M_EXTENDED_MSG);
		msg_bit &= ~M_EXTENDED_MSG;
		while (msg_bit)
		{
			event++;
			msg_bit >>= 1;
		}
		if (event == -1)
		{
			event = BUILTIN_UNKNOWN;
		}
		else if (event >= MAX_MESSAGES && !is_extended_msg)
		{
			event = BUILTIN_UNKNOWN;
		}
		else if (event >= MAX_EXTENDED_MESSAGES && is_extended_msg)
		{
			event = BUILTIN_UNKNOWN;
		}
		else if (is_extended_msg)
		{
			event += MAX_MESSAGES;
		}
		execute_event(event, body);
		unlock_event(header[1]);
	} /* while */
	execute_event(BUILTIN_SHUTDOWN, NULL);

	return 0;
}


/*
 *
 * Procedure:
 *
 *    execute_event - actually executes the actions from lookup table
 *
 */
void execute_event(short event, unsigned long *body)
{
#ifdef HAVE_RPLAY
	/* this is the sign that rplay is used */
	if (rplay_fd != -1 && rplay_table[event])
	{
		if (rplay(rplay_fd, rplay_table[event]) >= 0)
		{
			last_time = now;
		}
		else
		{
			rplay_perror("rplay");
		}
		return;
	}
#endif

	if (action_table[event])
	{
		char *buf = NULL;
		int len = 0;

		len = strlen(cmd_line) + strlen(action_table[event]) + 32;
		if (audio_play_dir)
		{
			len += strlen(audio_play_dir);
		}
		buf = (char *)safemalloc(len);
		if (audio_compat)
		{
			/* Don't use audio_play_dir if it's NULL or if
			 * the sound file is an absolute pathname. */
			if (!audio_play_dir || audio_play_dir[0] == '\0' ||
			    action_table[event][0] == '/')
			{
				sprintf(buf,"%s %s", cmd_line,
					action_table[event]);
			}
			else
			{
				sprintf(buf,"%s %s/%s &", cmd_line,
					audio_play_dir, action_table[event]);
			}
			if (!system(buf))
			{
				last_time = now;
			}
		}
		else
		{
			int action_arg = event_table[event].action_arg;
			int fw = 0;

			if (action_arg != -1 && !(action_arg & ARG_NO_WINID))
			{
				fw = body[action_arg];
			}

			if (PassID && action_arg != -1)
			{
				if (action_arg & ARG_NO_WINID)
				{
					action_arg &= ~ARG_NO_WINID;
					sprintf(buf, "%s %s %ld", cmd_line,
						action_table[event],
						body[action_arg]);
				}
				else
				{
					sprintf(buf, "%s %s 0x%lx", cmd_line,
						action_table[event],
						body[action_arg]);
				}
			}
			else
			{
				sprintf(buf,"%s %s", cmd_line,
					action_table[event]);
			}
			/* let fvwm execute the function */
			SendText(fd, buf, fw);
			last_time = now;
		}
		free(buf);

		return;
	}

	return;
}


/*
 *
 *  Procedure:
 *      config - read the configuration file.
 *
 */

char *table[]=
{
	"Cmd",
	"Delay",
	"Dir",
	"PassID",
	"PlayCmd",
	"RplayHost",
	"RplayPriority",
	"RplayVolume",
	"StartDelay"
}; /* define entries here, if this list becomes unsorted, use FindToken */

void handle_config_line(char *buf, char **phost)
{
	char *event;
	char *action;
	char *p;
	char *token;
	char **e;
	int i;
	int found;
#ifdef HAVE_RPLAY
	int volume   = RPLAY_DEFAULT_VOLUME;
	int priority = RPLAY_DEFAULT_PRIORITY;
#endif

	p = buf + MyNameLen;
	/* config option ? */
	if ((e = FindToken(p, table, char *)))
	{
		/* skip matched token */
		p += strlen(*e);
		token = PeekToken(p, &p);
		switch (e - (char**)table)
		{
		case 4:
			/* PlayCmd */
			if (!audio_compat)
			{
				/* PlayCmd */
				fprintf(stderr,
					"%s: PlayCmd supported only when"
					" invoked as FvwmAudio\n", MyName+1);
				break;
			}
			/* fall through */
		case 0:
			/* Cmd */
			if (cmd_line)
			{
				free(cmd_line);
			}
			if (token)
			{
				cmd_line = safestrdup(token);
			}
			else
			{
				cmd_line = safestrdup("");
			}
			break;

		case 1:
			/* Delay */
			if (token)
			{
				audio_delay = atoi(token);
			}
			break;

		case 2:
			/* Dir */
			if (!audio_compat)
			{
				fprintf(stderr,
					"%s: Dir supported only when invoked as"
					" FvwmAudio\n", MyName+1);
				break;
			}
			if (token)
			{
				if (audio_play_dir)
				{
					free(audio_play_dir);
				}
				audio_play_dir = safestrdup(token);
			}
			break;

		case 3:
			/* PassID */
			PassID = True;
			break;

#ifdef HAVE_RPLAY
		case 5:
			/* RPlayHost */
			if (*phost)
			{
				free(*phost);
				*phost = NULL;
			}
			if (token && (*token == '$'))
			{
				/* Check for $HOSTDISPLAY */
				char *c1 = (char *)getenv(token + 1);
				char *c2;

				*phost = safestrdup(c1);
				c2 = *phost;
				while (c1 && *c1 != ':')
				{
					*c2++ = *c1++;
				}
				*c2 = '\0';
			}
			else
			{
				*phost = safestrdup(token);
			}
			break;

		case 6:
			/* RplayPriority */
			if (token)
			{
				priority = atoi(token);
			}
			break;

		case 7:
			/* RplayVolume */
			if (token)
			{
				volume = atoi(token);
			}
			break;
#endif

		case 8:
			/* StartDelay */
			if (token)
			{
				start_audio_delay = atoi(token);
			}
			break;

		}
	}
	else
	{
		/* test for isspace(*p) ??? */
		p = GetNextSimpleOption(p, &event);
		p = GetNextSimpleOption(p, &action);
		if (!event || !*event || !action || !*action)
		{
			if (event)
			{
				free(event);
			}
			if (action)
			{
				free(action);
			}
			fprintf(stderr, "%s: incomplete event definition %s\n",
				MyName + 1, buf);
			return;
		}
		for (found = 0, i = 0;
		     !found && i < MAX_TOTAL_MESSAGES + MAX_BUILTIN; i++)
		{
			if (MatchToken(event, event_table[i].name))
			{
#ifdef HAVE_RPLAY
				rplay_table[i] = rplay_create(RPLAY_PLAY);
				rplay_set(
					rplay_table[i], RPLAY_APPEND,
					RPLAY_SOUND, action, RPLAY_PRIORITY,
					priority, RPLAY_VOLUME, volume, NULL);
#endif
				if (action_table[i])
				{
					free(action_table[i]);
				}
				action_table[i] = action;
				found = 1;
				if (i < MAX_MESSAGES)
				{
					m_selected |= (1 << i);
				}
				else if (i < MAX_TOTAL_MESSAGES)
				{
					mx_selected |=
						(1 << (i - MAX_MESSAGES));
				}
			}
		}
		if (!found)
		{
			fprintf(stderr, "%s: unknown event type: %s\n",
				MyName+1, event);
			if (action)
			{
				free(action);
			}
		}
		if (event)
		{
			free(event);
		}
	}

	return;
}

void config(void)
{
	char *buf;
	int i;
	char *host = NULL;

#ifdef HAVE_RPLAY
	host = safestrdup(rplay_default_host());
#endif

	/* Intialize all the actions */
	for (i = 0; i < MAX_TOTAL_MESSAGES+MAX_BUILTIN; i++)
	{
		action_table[i] = NULL;
#ifdef HAVE_RPLAY
		rplay_table[i] = NULL;
#endif
	}

	/* get config lines with my name */
	InitGetConfigLine(fd,MyName);
	while (GetConfigLine(fd,&buf), buf != NULL && *buf != 0)
	{
		if (buf[strlen(buf)-1] == '\n')
		{
			/* if line ends with newline, strip it off */
			buf[strlen(buf)-1] = '\0';
		}

		/* Search for MyName (normally *FvwmEvent) */
		if (strncasecmp(buf, MyName, MyNameLen) == 0)
		{
			handle_config_line(buf, &host);
		}
	}

#ifdef HAVE_RPLAY
	/* Builtin rplay support is enabled when
	 * FvwmAudioPlayCmd == builtin-rplay. */
	if (StrEquals(cmd_line, "builtin-rplay"))
	{
		if ((rplay_fd = rplay_open(host)) < 0)
		{
			rplay_perror("rplay_open");
			exit(1);
		}
	}
#endif

	return;
}

/*
 *
 *  Procedure:
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 */
static RETSIGTYPE
TerminateHandler(int nonsense)
{
	isTerminated = True;

	SIGNAL_RETURN;
}

/*
 *
 *  Procedure:
 *      Externally callable procedure to quit
 *
 */
RETSIGTYPE DeadPipe(int flag)
{
	execute_event(BUILTIN_SHUTDOWN, NULL);
	exit(flag);
	SIGNAL_RETURN;
}
