/*#define DEBUG
 *
 * Copyright (C) 1994 Mark Boyns (boyns@sdsu.edu) and
 *                    Mark Scott (mscott@mcd.mot.com)
 *		 1996-1998 Albrecht Kadlec (albrecht@auto.tuwien.ac.at)
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

/* ChangeLog

 * renamed & cleaned up so that FvwmEvent (TaDa !!) is a general event
   handler module.
               -- 21.04.98 Albrecht Kadlec (albrecht@auto.tuwien.ac.at)

 * changed parsing to use new library functions

 * merged bug fix from Christophe MARTIN <cmartin@ipnl.in2p3.fr>
     - FvwmAudio doesn't read messages it is not interrested in,
       thus desynchronize,
     - keep time stamp even if no sound is played
     - minimize malloc/free

   mark boyns ok-ed this patch, I reviewed it and put out those ugly
   #defines for RESYNC and READ_BODY by reordering the control structures
               -- Albrecht Kadlec (albrecht@auto.tuwien.ac.at)

 * made FvwmAudio insensitive to its name -> can be symlinked.
   corrected some error message deficiencies, code for quoted 'sound'
   parameters shamelessly stolen from FvwmButtons/parse.c (with minimal
   adjustments)
   FvwmAudio now supports rsynth's say command:
   *FvwmAudioPlayCmd say
   *FvwmAudio add_window	   "add window"
   *FvwmAudio raise_window	   'raise window'
               -- 08/07/96 Albrecht Kadlec (albrecht@auto.tuwien.ac.at)

 * Fixed FvwmAudio to reflect the changes made to the module protocol.

 * Szijarto Szabolcs <saby@sch.bme.hu> provided FvwmSound code that used
 $HOSTDISPLAY for the rplay host.  The code has been added to FvwmAudio.

 * Fixed bugs reported by beta testers, thanks!

 * Builtin rplay support has been added to FvwmAudio.  This support is
 enabled when FvwmAudio is compiled with HAVE_RPLAY defined and when
 FvwmAudioPlayCmd is set to builtin-rplay.  I guess it's safe to say
 that FvwmSound is now obsolete. -- Mark Boyns 5/7/94

 * FvwmAudio is based heavily on an Fvwm module "FvwmSound" by Mark Boyns
 and the real credit for this really goes to him for the concept.
 I just stripped out the "rplay" library dependencies to allow generic
 audio play command support.

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

#include "config.h"
#include "../../fvwm/module.h"
#include "fvwmlib.h"

/*
 * fvwm includes:
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

/*
 * rplay includes:
 */
#ifdef HAVE_RPLAY
#include <rplay.h>
#undef M_ERROR /* Solaris fix */
#endif

#include "Parse.h"

#ifdef DEBUG
#define INFO(x) fprintf(stderr,x)
#else
#define INFO(x)
#endif

#define BUILTIN_STARTUP		MAX_MESSAGES
#define BUILTIN_SHUTDOWN	MAX_MESSAGES+1
#define BUILTIN_UNKNOWN		MAX_MESSAGES+2
#define MAX_BUILTIN		3

#define BUFSIZE			512

/* globals */
char	*MyName;
int	MyNameLen;
int	fd[2];
char	cmd_line[BUFSIZE]="";
time_t	audio_delay = 0,		/* seconds */
        last_time = 0,
        now;

#ifdef HAVE_RPLAY
int	rplay_fd = -1;
#endif

/* prototypes */
int     execute_event(short);
void	config(void);
void	DeadPipe(int) __attribute__((__noreturn__));

static RETSIGTYPE TerminateHandler(int);

/* define the event table */
char	*events[MAX_MESSAGES+MAX_BUILTIN] =
{
	"new_page",
	"new_desk",
	"add_window",
	"raise_window",
	"lower_window",
	"configure_window",
	"focus_change",
	"destroy_window",
	"iconify",
	"deiconify",
	"window_name",
	"icon_name",
	"res_class",
	"res_name",
	"end_windowlist",
	"icon_location",
	"map",
	"error",
	"config_info",
	"end_config_info",
	"icon_file",
	"default_icon",
	"string",
#ifdef M_BELL
	"beep",
#endif
#ifdef M_TOGGLEPAGE
	"togglepage",
#endif
	"mini_icon",
	"windowshade",
	"dewindowshade",
/* add builtins here */
	"startup",
	"shutdown",
	"unknown"
};

/* define the action table  */
char	*action_table[MAX_MESSAGES+MAX_BUILTIN];

#ifdef HAVE_RPLAY
/* define the rplay table */
RPLAY	*rplay_table[MAX_MESSAGES+MAX_BUILTIN];
#endif

static volatile sig_atomic_t isTerminated = False;

int main(int argc, char **argv)
{
    char *s;
    unsigned long	header[HEADER_SIZE], body[MAX_BODY_SIZE];
    int			total, remaining, count, event;

INFO("--- started ----\n");

    /* Save our program  name - for error events */

    if ((s=strrchr(argv[0], '/')))	/* strip path */
	s++;
    else				/* no slash */
	s = argv[0];

    MyNameLen=strlen(s)+1;		/* account for '*' */
    MyName = safemalloc(MyNameLen+1);	/* account for \0 */
    *MyName='*';
    strcpy(MyName+1, s);		/* append name */

    if ((argc != 6)&&(argc != 7))	/* Now MyName is defined */
    {
	fprintf(stderr,"%s Version "VERSION" should only be executed by fvwm!\n",
		MyName+1);
	exit(1);
    }

  INFO("--- installing signal server\n");

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

    sigaction(SIGPIPE,&sigact,NULL);    /* Dead pipe == Fvwm died */
    sigaction(SIGTERM,&sigact,NULL);    /* "polite" termination signal */
  }
#else
    /* We don't have sigaction(), so fall back to less robust methods.  */
    signal(SIGPIPE, TerminateHandler);
    signal(SIGTERM, TerminateHandler);
#endif

    fd[0] = atoi(argv[1]);
    fd[1] = atoi(argv[2]);

INFO("--- configuring\n");

    config();				/* configure events */
    execute_event(BUILTIN_STARTUP);	/* Startup event */

    SendText(fd,"Nop",0);		/* what for ? */


    /* main loop */

INFO("--- waiting\n");
    while ( !isTerminated )
    {
	if ((count = read(fd[1],header,
			  HEADER_SIZE*sizeof(unsigned long))) <= 0)
	    exit(0);
/* if this read is interrrupted  EINTR, the wrong event is triggered !!! */

	if( header[0] != START_FLAG )
	    continue;	/* should find something better for resyncing */

	/* Ignore events that occur during the delay period. */
	now = time(0);

	/* junk the event body */
	total=0;
	remaining = (header[2] - HEADER_SIZE) * sizeof(unsigned long);
	while (remaining)
	{
	    if((count=read(fd[1],&body[total],remaining)) < 0)
		exit(0);
	    remaining -= count;
	    total +=count;
	}

	if (now < last_time + audio_delay)
	    continue;			/* quash event */

	/*
	 * event will equal the number of shifts in the
	 * base-2 header[1] number.  Could use log here
	 * but this should be fast enough.
	 */
	event = -1;
	while (header[1])
	{
	    event++;
	    header[1] >>= 1;
	}
	if (event < 0 || event >= MAX_MESSAGES)
	    event=BUILTIN_UNKNOWN;

	execute_event(event);		/* execute action */
    } /* while */

    execute_event(BUILTIN_SHUTDOWN);
    return 0;
}


/***********************************************************************
 *
 * Procedure:
 *
 *    execute_event - actually executes the actions from lookup table
 *
 **********************************************************************/
int execute_event(short event)
{
    static char buf[BUFSIZE];

#ifdef HAVE_RPLAY
    if (rplay_fd != -1)		/* this is the sign that rplay is used */
    {
	if (rplay_table[event])
	    if (rplay(rplay_fd, rplay_table[event]) >= 0)
		last_time = now;
	    else
		rplay_perror("rplay");
	return 0;
    } else	/* avoid invalid second execute */
#endif
    if (action_table[event])
    {
        sprintf(buf,"%s %s", cmd_line, action_table[event]);
#if 1
INFO(buf);
INFO("\n");

	SendText(fd,buf,0);		/* let fvwm2 execute the function */
        last_time = now;
#else
	if( ! ( ret = system(buf)))	/* directly execute external program */
	    last_time = now;
	return ret;
#endif
    }
    return 1;
}


/***********************************************************************
 *
 *  Procedure:
 *	config - read the configuration file.
 *
 ***********************************************************************/

char *table[]=
{
    "Cmd",
    "Delay"
#ifdef HAVE_RPLAY
	,
    "RplayHost",
    "RplayPriority",
    "RplayVolume"
#endif
};	/* define entries here, if this list becomes unsorted, use LFindToken */


void	config(void)
{
    char *buf, *event, *action, *p, *q, **e;
    int  i, found;
#ifdef HAVE_RPLAY
    int	 volume   = RPLAY_DEFAULT_VOLUME,
	 priority = RPLAY_DEFAULT_PRIORITY;
    char host[128];

    strcpy(host, rplay_default_host());
#endif

    /* Intialize all the actions */

    for (i = 0; i < MAX_MESSAGES+MAX_BUILTIN; i++)
    {
	action_table[i] = NULL;
#ifdef HAVE_RPLAY
	rplay_table[i] = NULL;
#endif
    }

    while (GetConfigLine(fd,&buf), buf != NULL)
    {
        if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
	  buf[strlen(buf)-1] = '\0';	/* strip off \n */
        }

	/* Search for MyName (normally *FvwmAudio) */
	if (strncasecmp(buf, MyName, MyNameLen) == 0)
	{
	    p = buf+MyNameLen;
INFO(buf);
INFO("\n");
	    if ((e= FindToken(p,table,char *))) /* config option ? */
	    {
                p+=strlen(*e);		/* skip matched token */
		q=GetArgument(&p);
		if (!q)
		{
		  fprintf(stderr,"%s: %s%s needs a parameter\n",
			  MyName+1, MyName+1,*e);
		  continue;
		}

		switch (e - (char**)table)
		{
		case 0: if (q) strcpy(cmd_line, q);	       /* Cmd */
		        else *cmd_line='\0';
INFO("cmd_line = ->");
INFO(cmd_line);
INFO("<-\n");
                                                        break;
		case 1: audio_delay = atoi(q);		break; /* Delay */
#ifdef HAVE_RPLAY
		case 2: if (*q == '$')		       /* RPlayHost */
			{			 /* Check for $HOSTDISPLAY */
			  char *c1= (char *)getenv(q+1), *c2= host;
			  while (c1 && *c1 != ':')
			    *c2++ = *c1++;
			  *c2 = '\0';
			}
			else
			  strcpy(host, q);
		                                        break;
		case 3: priority = atoi(q);		break; /* RplayPriority */
		case 4: volume = atoi(q);		break; /* RplayVolume */
#endif
		}
	    }
	    else  /* test for isspace(*p) ??? */
	    {
		event = GetArgument(&p);
		action = GetArgument(&p);
INFO(event);
INFO("  ");
INFO(action);
INFO("\n");
		if (!event || !*event || !action || !*action)
		{
		    fprintf(stderr,"%s: incomplete event definition %s\n",
			    MyName+1, buf);
		    continue;
		}
		for (found = 0,i = 0; !found && i < MAX_MESSAGES+MAX_BUILTIN;
		     i++)
	       {
INFO(events[i]);
INFO("\n");

		    if (MatchToken(event, events[i]))
		    {
#ifdef HAVE_RPLAY
			rplay_table[i] = rplay_create(RPLAY_PLAY);
			rplay_set(rplay_table[i], RPLAY_APPEND,
				  RPLAY_SOUND,		action,
				  RPLAY_PRIORITY,	priority,
				  RPLAY_VOLUME,		volume,
				  NULL);
#endif
			action_table[i]=action;
			found=1;
INFO("found\n");
		    }
	        }
		if (!found) fprintf(stderr,"%s: unknown event type: %s\n",
				    MyName+1, event);
	    }
	}
/*else INFO("NO CONFIG\n");*/
    }

#ifdef HAVE_RPLAY
    /*
     * Builtin rplay support is enabled when FvwmAudioPlayCmd == builtin-rplay.
     */
    if (strcasecmp(cmd_line, "builtin-rplay") == 0)
	if ((rplay_fd = rplay_open(host)) < 0)
	{
	    rplay_perror("rplay_open");
	    exit(1);
	}
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
static RETSIGTYPE
TerminateHandler(int nonsense)
{
  isTerminated = True;
}

/***********************************************************************
 *
 *  Procedure:
 *	Externally callable procedure to quit
 *
 ***********************************************************************/
void DeadPipe(int flag)
{
  execute_event(BUILTIN_SHUTDOWN);
  exit(flag);
}
