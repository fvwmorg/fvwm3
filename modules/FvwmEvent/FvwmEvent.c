/* #define DEBUG */
/*
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

 * added *FvwmEventPassID config option

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

/*
 * rplay includes:
 */
#ifdef HAVE_RPLAY
#include <rplay.h>
#undef M_ERROR /* Solaris fix */
#endif

#include <libs/Module.h>
#include <libs/fvwmlib.h>

/*
 * fvwm includes:
 */
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>
#include <values.h>

#ifdef DEBUG
#define INFO(x) fprintf(stderr,x)
#else
#define INFO(x)
#endif

#define BUILTIN_STARTUP		MAX_MESSAGES
#define BUILTIN_SHUTDOWN	MAX_MESSAGES+1
#define BUILTIN_UNKNOWN		MAX_MESSAGES+2
#define MAX_BUILTIN		3

/* globals */
char   *MyName;
int	MyNameLen;
int	fd[2];
char   *cmd_line = NULL;
time_t	audio_delay = 0,		/* seconds */
        last_time = 0,
        now,
        start_audio_delay = 0;
Bool	PassID = False;	/* don't tag on the windowID by default */
Bool	audio_compat = False;
char   *audio_play_dir = NULL;

#ifdef HAVE_RPLAY
int	rplay_fd = -1;
#endif

/* prototypes */
void    execute_event(short, unsigned long*);
void	config(void);
void	DeadPipe(int) __attribute__((__noreturn__));

static RETSIGTYPE TerminateHandler(int);

typedef struct
{
  char *name;
  int action_arg;
} event_entry;

event_entry event_table[MAX_MESSAGES+MAX_BUILTIN] =
{
  { "new_page",	-1 },
  { "new_desk", 0 },
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
  { "icon_file", -1 },
  { "default_icon", -1 },
  { "string", -1 },
  { "mini_icon", -1 },
  { "windowshade", -1 },
  { "dewindowshade", -1 },
  { "lockonsend", -1 },
  { "sendconfig", -1 },
  { "restack", -1 },
  { "add_window", 0 },
  { "configure_window", 0 },
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
char	*action_table[MAX_MESSAGES+MAX_BUILTIN];

#ifdef HAVE_RPLAY
/* define the rplay table */
RPLAY	*rplay_table[MAX_MESSAGES+MAX_BUILTIN];
#endif

static volatile sig_atomic_t isTerminated = False;

int main(int argc, char **argv)
{
  char *s;
  unsigned long	header[FvwmPacketHeaderSize], body[FvwmPacketBodyMaxSize];
  int			total, remaining, count, event;

  INFO("--- started ----\n");

  cmd_line = (char *)safemalloc(1);
  *cmd_line = 0;
  /* Save our program  name - for error events */

  if ((s=strrchr(argv[0], '/')))	/* strip path */
    s++;
  else				/* no slash */
    s = argv[0];
  if ( argc==7 )
    {
      if (strcmp(argv[6], "-audio") == 0)
	audio_compat = True;
      else
	s = argv[6];			/* use an alias */
    }

  MyNameLen=strlen(s)+1;		/* account for '*' */
  MyName = safemalloc(MyNameLen+1);	/* account for \0 */
  *MyName='*';
  strcpy(MyName+1, s);		/* append name */
  if (StrEquals("FvwmAudio", s))
    audio_compat = True;		/* catch the FvwmAudio alias */

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
    execute_event(BUILTIN_STARTUP, NULL);	/* Startup event */
    if (start_audio_delay) last_time = time(0);
    /* tell fvwm we're running */

    /* migo (19-Aug-2000): synchronize on M_DESTROY_WINDOW */
    /* SetMessageMask(fd, MAX_MASK); */
    SetSyncMask(fd, M_DESTROY_WINDOW);

    SendFinishedStartupNotification(fd);

    /* main loop */

    INFO("--- waiting\n");
    while ( !isTerminated )
    {
      unsigned long msg_bit;
      if ((count = read(fd[1],header, FvwmPacketHeaderSize_byte)) <= 0)
	exit(0);
      /* if this read is interrrupted  EINTR, the wrong event is triggered !!! */

      if( header[0] != START_FLAG )
	goto CONTINUE;  /* should find something better for resyncing */

      /* Ignore events that occur during the delay period. */
      now = time(0);

      /* junk the event body */
      total=0;
      remaining = (header[2] - FvwmPacketHeaderSize) * sizeof(unsigned long);
      while (remaining)
      {
	if((count=read(fd[1],&body[total],remaining)) < 0)
	  exit(0);
	remaining -= count;
	total +=count;
      }

      if (now < last_time + audio_delay + start_audio_delay)
	goto CONTINUE;  /* quash event */
      else
	start_audio_delay = 0;

      /*
       * event will equal the number of shifts in the
       * base-2 header[1] number.  Could use log here
       * but this should be fast enough.
       */
      event = -1;
      msg_bit = header[1];
      while (msg_bit)
      {
	event++;
	msg_bit >>= 1;
      }
      if (event < 0 || event >= MAX_MESSAGES)
	event=BUILTIN_UNKNOWN;

      execute_event(event, body);		/* execute action */

CONTINUE:
      if (header[1] == M_DESTROY_WINDOW)
        SendUnlockNotification(fd);
    } /* while */

    execute_event(BUILTIN_SHUTDOWN, NULL);
    return 0;
}


/***********************************************************************
 *
 * Procedure:
 *
 *    execute_event - actually executes the actions from lookup table
 *
 **********************************************************************/
void execute_event(short event, unsigned long *body)
{
#ifdef HAVE_RPLAY

  if (rplay_fd != -1)		/* this is the sign that rplay is used */
    {
      if (rplay_table[event])
	{
	  if (rplay(rplay_fd, rplay_table[event]) >= 0)
	    last_time = now;
	  else
	    rplay_perror("rplay");
	}
    }
  else 	/* avoid invalid second execute */
#endif
    if (action_table[event])
      {
	char *buf = NULL;
	int len = 0;

	len = strlen(cmd_line) + strlen(action_table[event]) + 32;
	if (audio_play_dir)
	  len += strlen(audio_play_dir);
	buf = (char *)safemalloc(len);
	if (audio_compat)
	  {
	    /*
	       * Don't use audio_play_dir if it's NULL or if the sound file
	       * is an absolute pathname.
	       */
	    if (!audio_play_dir || audio_play_dir[0] == '\0' ||
		action_table[event][0] == '/')
	      sprintf(buf,"%s %s", cmd_line, action_table[event]);
	    else
	      sprintf(buf,"%s %s/%s &", cmd_line, audio_play_dir,
		      action_table[event]);
	      if(!system(buf))
		last_time = now;
	  }
	else
	  {
	    if(PassID && (event_table[event].action_arg != -1))
	      sprintf(buf,"%s %s %ld", cmd_line, action_table[event],
		      body[event_table[event].action_arg]);
	    else
	      sprintf(buf,"%s %s", cmd_line, action_table[event]);
	    INFO(buf);
	    INFO("\n");
	    SendText(fd,buf,0);		/* let fvwm execute the function */
	    last_time = now;
	  }
	free(buf);
      }
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
  "Delay",
  "Dir",
  "PassID",
  "PlayCmd",
  "RplayHost",
  "RplayPriority",
  "RplayVolume",
  "StartDelay"
}; /* define entries here, if this list becomes unsorted, use FindToken */


void config(void)
{
  char *buf, *event, *action, *p, *token, **e;
  int i;
  int found;
#ifdef HAVE_RPLAY
  int volume   = RPLAY_DEFAULT_VOLUME;
  int priority = RPLAY_DEFAULT_PRIORITY;
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

  InitGetConfigLine(fd,MyName);         /* get config lines with my name */
  while (GetConfigLine(fd,&buf), buf != NULL)
  {
    if (buf[strlen(buf)-1] == '\n')
    {
      /* if line ends with newline, strip it off */
      buf[strlen(buf)-1] = '\0';
    }

    /* Search for MyName (normally *FvwmEvent) */
    if (strncasecmp(buf, MyName, MyNameLen) == 0)
    {
      p = buf+MyNameLen;
      INFO(buf);
      INFO("\n");
      if ((e = FindToken(p,table,char *))) /* config option ? */
      {
	p += strlen(*e);		/* skip matched token */
	p = GetNextToken(p, &token);

	switch (e - (char**)table)
	{
	case 0:	       /* Cmd */
	case 4:
	  if (! audio_compat && e - (char**)table == 4) /* PlayCmd */
	  {
	    fprintf(stderr,
		    "%s: PlayCmd supported only when invoked as FvwmAudio\n",
		    MyName+1);
	    break;
	  }
	  if (cmd_line)
	    free(cmd_line);
	  if (token)
	  {
	    if (e - (char**)table == 4)
	    {
	      cmd_line = (char *)safemalloc(strlen(token)+11);
	      strcpy(cmd_line, "Exec exec ");
	      strcat(cmd_line, token);
	    }
	    else
	    {
	      cmd_line = (char *)safemalloc(strlen(token)+1);
	      strcpy(cmd_line, token);
	    }
	  }
	  else
	  {
	    cmd_line = (char *)safemalloc(1);
	    *cmd_line = 0;
	  }
	  INFO("cmd_line = ->");
	  INFO(cmd_line);
	  INFO("<-\n");
	  break;

	case 1:
	  if (token)
	    audio_delay = atoi(token);
	  break; /* Delay */

	case 2:
	  if (! audio_compat)		       /* Dir */
	    fprintf(stderr,
		    "%s: Dir supported only when invoked as FvwmAudio\n",
		    MyName+1);
	  else
	    if (token)
	    {
	      if (audio_play_dir)
		free(audio_play_dir);
	      if (token)
	      {
		audio_play_dir = (char *)safemalloc(strlen(token)+1);
		strcpy(audio_play_dir, token);
	      }
	    }

	  break;

	case 3:
	  PassID = True;
	  break; /* PassID */

#ifdef HAVE_RPLAY
	case 5:
	  if (token && (*token == '$'))		       /* RPlayHost */
	  {			 /* Check for $HOSTDISPLAY */
	    char *c1= (char *)getenv(token+1), *c2= host;
	    while (c1 && *c1 != ':')
	      *c2++ = *c1++;
	    *c2 = '\0';
	  }
	  else
	    strcpy(host, token);
	  break;

	case 6:
	  if (token)
	    priority = atoi(token);
	  break; /* RplayPriority */

	case 7:
	  if (token)
	    volume = atoi(token);
	  break; /* RplayVolume */
#endif

	case 8:
	  if (token)
	    start_audio_delay = atoi(token);
	  break; /* StartDelay */

	}
	if (token)
	  free(token);
      }
      else  /* test for isspace(*p) ??? */
      {
	p = GetNextSimpleOption( p, &event );
	p = GetNextSimpleOption( p, &action );

	INFO(event);
	INFO("  ");
	INFO(action);
	INFO("\n");
	if (!event || !*event || !action || !*action)
	{
	  if (event)
	    free(event);
	  if (action)
	    free(action);
	  fprintf(stderr,"%s: incomplete event definition %s\n",
		  MyName+1, buf);
	  continue;
	}

	for (found = 0,i = 0; !found && i < MAX_MESSAGES+MAX_BUILTIN;
	     i++)
	{
	  INFO(event_table[i].name);
	  INFO("\n");

	  if (MatchToken(event, event_table[i].name))
	  {
#ifdef HAVE_RPLAY
	    rplay_table[i] = rplay_create(RPLAY_PLAY);
	    rplay_set(rplay_table[i], RPLAY_APPEND,
		      RPLAY_SOUND, action,
		      RPLAY_PRIORITY, priority,
		      RPLAY_VOLUME, volume,
		      NULL);
#endif
	    if (action_table[i])
	      free(action_table[i]);
	    action_table[i]=action;
	    found=1;
	    INFO("found\n");
	  }
	}
	if (!found)
	{
	  fprintf(stderr,"%s: unknown event type: %s\n", MyName+1, event);
	  if (action)
	    free(action);
	}
	if (event)
	  free(event);
      }
    }
    /*else INFO("NO CONFIG\n");*/
  }

#ifdef HAVE_RPLAY
  /*
   * Builtin rplay support is enabled when FvwmAudioPlayCmd == builtin-rplay.
   */
  if (StrEquals(cmd_line, "builtin-rplay"))
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
  execute_event(BUILTIN_SHUTDOWN, NULL);
  exit(flag);
}
