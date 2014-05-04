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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 * This module is based on FvwmModuleDebugger which has the following
 * copyright:
 *
 * This module, and the entire ModuleDebugger program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
/*
 * rplay includes:
 */
#include "libs/Fplay.h"

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

#define BUILTIN_STARTUP         0
#define BUILTIN_SHUTDOWN        1
#define BUILTIN_UNKNOWN         2
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
	union
	{
		char *action;
		FPLAY *rplay;
	} action;
} event_entry;

/* ---------------------------- forward declarations ------------------------ */

void execute_event(event_entry*, short, unsigned long*);
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

#define EVENT_ENTRY(name,action_arg) { name, action_arg, {NULL} }
static event_entry message_event_table[] =
{
	EVENT_ENTRY( "new_page", -1 ),
	EVENT_ENTRY( "new_desk", 0 | ARG_NO_WINID ),
	EVENT_ENTRY( "old_add_window", 0 ),
	EVENT_ENTRY( "raise_window", 0 ),
	EVENT_ENTRY( "lower_window", 0 ),
	EVENT_ENTRY( "old_configure_window", 0 ),
	EVENT_ENTRY( "focus_change", 0 ),
	EVENT_ENTRY( "destroy_window", 0 ),
	EVENT_ENTRY( "iconify", 0 ),
	EVENT_ENTRY( "deiconify", 0 ),
	EVENT_ENTRY( "window_name", 0 ),
	EVENT_ENTRY( "icon_name", 0 ),
	EVENT_ENTRY( "res_class", 0 ),
	EVENT_ENTRY( "res_name", 0 ),
	EVENT_ENTRY( "end_windowlist", -1 ),
	EVENT_ENTRY( "icon_location", 0 ),
	EVENT_ENTRY( "map", 0 ),
	EVENT_ENTRY( "error", -1 ),
	EVENT_ENTRY( "config_info", -1 ),
	EVENT_ENTRY( "end_config_info", -1 ),
	EVENT_ENTRY( "icon_file", 0 ),
	EVENT_ENTRY( "default_icon", -1 ),
	EVENT_ENTRY( "string", -1 ),
	EVENT_ENTRY( "mini_icon", 0 ),
	EVENT_ENTRY( "windowshade", 0 ),
	EVENT_ENTRY( "dewindowshade", 0 ),
	EVENT_ENTRY( "visible_name", 0 ),
	EVENT_ENTRY( "sendconfig", -1 ),
	EVENT_ENTRY( "restack", -1 ),
	EVENT_ENTRY( "add_window", 0 ),
	EVENT_ENTRY( "configure_window", 0 ),
	EVENT_ENTRY(NULL,0)
};
static event_entry extended_message_event_table[] =
{
	EVENT_ENTRY( "visible_icon_name", 0 ),
	EVENT_ENTRY( "enter_window", 0 ),
	EVENT_ENTRY( "leave_window", 0 ),
	EVENT_ENTRY( "property_change", 0),
	EVENT_ENTRY( "reply", 0), /* FvwmEvent will never receive MX_REPLY */
	EVENT_ENTRY(NULL,0)
};
static event_entry builtin_event_table[] =
{
	EVENT_ENTRY( "startup", -1 ),
	EVENT_ENTRY( "shutdown", -1 ),
	EVENT_ENTRY( "unknown", -1),
	EVENT_ENTRY(NULL,0)
};

#if 0
/* These are some events described in the man page, which does not exist in
   fvwm. */
static event_entry future_event_table[] =
{
#ifdef M_BELL
	EVENT_ENTRY( "beep", -1 ),
#endif
	EVENT_ENTRY(NULL,0)
}
#endif

static event_entry* event_tables[] =
{
	message_event_table,
	extended_message_event_table,
	builtin_event_table,
	NULL
};

#undef EVENT_ENTRY

static int rplay_fd = -1;
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

	cmd_line = xmalloc(1);
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
	MyName = xmalloc(MyNameLen+1);
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
	execute_event(builtin_event_table, BUILTIN_STARTUP, NULL);
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
		event_entry *event_table;

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
		if (
			event == -1 ||
			(event >= MAX_MESSAGES && !is_extended_msg) ||
			(event >= MAX_EXTENDED_MESSAGES && is_extended_msg))
		{
			event_table = builtin_event_table;
			event = BUILTIN_UNKNOWN;
		}
		else if (is_extended_msg)
		{
			event_table = extended_message_event_table;
		}
		else
		{
			event_table = message_event_table;
		}
		execute_event(event_table, event, body);
		unlock_event(header[1]);
	} /* while */
	execute_event(builtin_event_table, BUILTIN_SHUTDOWN, NULL);

	return 0;
}


/*
 *
 * Procedure:
 *
 *    execute_event - actually executes the actions from lookup table
 *
 */
void execute_event(event_entry *event_table, short event, unsigned long *body)
{
	/* this is the sign that rplay is used */
	if (rplay_fd != -1 && event_table[event].action.rplay != NULL)
	{
		if (Fplay(rplay_fd, event_table[event].action.rplay) >= 0)
		{
			last_time = now;
		}
		else
		{
			Fplay_perror("rplay");
		}
		return;
	}

	if (event_table[event].action.action != NULL)
	{
		char *buf = NULL;
		int len = 0;
		char *action;

		action = event_table[event].action.action;
		len = strlen(cmd_line) + strlen(action) + 32;
		if (audio_play_dir)
		{
			len += strlen(audio_play_dir);
		}
		buf = xmalloc(len);
		if (audio_compat)
		{
			/* Don't use audio_play_dir if it's NULL or if
			 * the sound file is an absolute pathname. */
			if (!audio_play_dir || audio_play_dir[0] == '\0' ||
			    action[0] == '/')
			{
				sprintf(buf,"%s %s", cmd_line, action);
			}
			else
			{
				sprintf(buf,"%s %s/%s &", cmd_line,
					audio_play_dir, action);
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
						action,	body[action_arg]);
				}
				else
				{
					sprintf(buf, "%s %s 0x%lx", cmd_line,
						action,	body[action_arg]);
				}
			}
			else
			{
				sprintf(buf,"%s %s", cmd_line, action);
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

static int volume   = FPLAY_DEFAULT_VOLUME;
static int priority = FPLAY_DEFAULT_PRIORITY;
void handle_config_line(char *buf, char **phost)
{
	char *event;
	char *action;
	char *p;
	char *token;
	char **e;
	int i;
	int found;

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
				cmd_line = xstrdup(token);
			}
			else
			{
				cmd_line = xstrdup("");
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
				audio_play_dir = xstrdup(token);
			}
			break;

		case 3:
			/* PassID */
			PassID = True;
			break;

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
				if (c1 != NULL)
				{
					*phost = xstrdup(c1);
					c2 = *phost;
					while (c1 && *c1 != ':')
					{
						*c2++ = *c1++;
					}
					*c2 = '\0';
				}
			}
			else if (token)
			{
				*phost = xstrdup(token);
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
		event_entry **event_table;

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
		event_table = event_tables;
		found = 0;
		while(found == 0 && *event_table != NULL)
		{
			event_entry *loop_event;

			for (loop_event = *event_table, i = 0;
			     found == 0 && loop_event->name != NULL;
			     loop_event++, i++)
			{
				if (MatchToken(event, loop_event->name))
				{
					if (loop_event->action.action != NULL)
					{
						free(loop_event->action.action);
					}
					loop_event->action.action = action;
					found = 1;
					if (*event_table == message_event_table)
					{
						m_selected |= (1 << i);
					}
					else if (
						*event_table ==
						extended_message_event_table)
					{
						mx_selected |= (1 << i);
					}
				}
			}
			event_table++;
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
	char *host = NULL;

	if (USE_FPLAY)
	{
		host = xstrdup(Fplay_default_host());
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

	/* Builtin rplay support is enabled when
	 * FvwmAudioPlayCmd == builtin-rplay. */
	if (USE_FPLAY && StrEquals(cmd_line, "builtin-rplay"))
	{
		event_entry **event_table;

		if ((rplay_fd = Fplay_open(host)) < 0)
		{
			Fplay_perror("rplay_open");
			exit(1);
		}
		/* change actions to rplay objects */
		event_table = event_tables;
		while(*event_table != NULL)
		{
			event_entry *loop_event;
			for (loop_event = *event_table;
			     loop_event->name != NULL; loop_event++)
			{
				if (loop_event->action.action != NULL)
				{
					char *tmp_action;
					tmp_action = loop_event->action.action;
					loop_event->action.rplay =
						Fplay_create(
							FPLAY_PLAY);
					Fplay_set(
						loop_event->action.rplay,
						FPLAY_APPEND,
						FPLAY_SOUND, tmp_action,
						FPLAY_PRIORITY, priority,
						FPLAY_VOLUME, volume, NULL);
					free(tmp_action);
				}
			}
			event_table++;
		}
	}

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
	execute_event(builtin_event_table, BUILTIN_SHUTDOWN, NULL);
	exit(flag);
	SIGNAL_RETURN;
}
