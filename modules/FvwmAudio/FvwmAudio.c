/* 
 * Copyright (C) 1994 Mark Boyns (boyns@sdsu.edu) and
 *                    Mark Scott (mscott@mcd.mot.com)
 *
 * FvwmAudio version 1.1
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


 * merged bug fix from Christophe MARTIN <cmartin@ipnl.in2p3.fr>
     - FvwmAudio doesn't read messages it is not interrested in,
       thus desynchronize,
     - keep time stamp even if no sound is played
     - minimize malloc/free

   mark boyns ok-ed this patch, I reviewed it and put out those ugly 
   #defines for RESYNC and READ_BODY by reordering the control structures

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
#include <stdlib.h>

#ifdef HAVE_RPLAY
#include <rplay.h>
#undef M_ERROR /* Solaris fix */
#endif

#define BUILTIN_STARTUP		MAX_MESSAGES
#define BUILTIN_SHUTDOWN	MAX_MESSAGES+1
#define BUILTIN_UNKNOWN		MAX_MESSAGES+2
#define MAX_BUILTIN		3

#define BUFSIZE			512

/* globals */
char	*MyName;
int	MyNameLen;
int	fd_width;
int	fd[2];
char	audio_play_cmd_line[BUFSIZE], audio_play_dir[BUFSIZE];
time_t	audio_delay = 0,	/* seconds */
        last_time = 0,
        now;

#ifdef HAVE_RPLAY
int	rplay_fd = -1;
#endif

/* prototypes */
void	Loop(int *);
void	process_message(unsigned long,unsigned long *);
void	DeadPipe(int);
void	config(void);
void	done(int);
int     audio_play(short); 

/* define the message table */
char	*messages[MAX_MESSAGES+MAX_BUILTIN] =
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
        "mini_icon",
        "windowshade",
        "dewindowshade",
/* add builtins here */
	"startup",
	"shutdown",
	"unknown"
};

/* define the sound table  */
char	*sound_table[MAX_MESSAGES+MAX_BUILTIN];

#ifdef HAVE_RPLAY
/* define the rplay table */
RPLAY	*rplay_table[MAX_MESSAGES+MAX_BUILTIN];
#endif

int main(int argc, char **argv)
{
	char *temp, *s;

	/* Save our program  name - for error messages */
	temp = argv[0];
	s=strrchr(argv[0], '/');
	if (s != NULL)
		temp = s + 1;

	MyNameLen=strlen(temp)+1;	/* acoount for '*' */
	MyName = safemalloc(MyNameLen+1); /* account for \0 */
	strcpy(MyName,"*");
	strcat(MyName, temp);

	if ((argc != 6)&&(argc != 7))	/* Now Myname is defined */
	{
          fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",
                  MyName+1, VERSION);
          exit(1);
	}

	/* Dead pipe == Fvwm died */
	signal (SIGPIPE, DeadPipe);  

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);

	audio_play_dir[0] = '\0';
	audio_play_cmd_line[0] = '\0';

	/*
	 * Read the sound configuration.
	 */
	config();

	/*
	 * Play the startup sound.
	 */
	audio_play(BUILTIN_STARTUP);
	SendText(fd,"Nop",0);
	Loop(fd);
	return 0;
}

/***********************************************************************
 *
 *  Procedure:
 *	config - read the sound configuration file.
 *
 ***********************************************************************/
void	config(void)
{
  char	*buf;
  char	*message;
  char	*sound;
  char	*p;
  int	i, found;
#ifdef HAVE_RPLAY
  int	volume = RPLAY_DEFAULT_VOLUME;
  int	priority = RPLAY_DEFAULT_PRIORITY;
  char	host[128];

  strcpy(host, rplay_default_host());
#endif

  /*
   * Intialize all the sounds.
   */
  for (i = 0; i < MAX_MESSAGES+MAX_BUILTIN; i++) {
    sound_table[i] = NULL;
#ifdef HAVE_RPLAY
    rplay_table[i] = NULL;
#endif
  }


  GetConfigLine(fd,&buf);
  while (buf != NULL)
    {
      if (buf[0] == '*')		/* this check is obsolete */
					/* - included in the next if */
	{
	  buf[strlen(buf)-1] = '\0';	/* strip off \n */
	  /*
	   * Search for MyName (normally *FvwmAudio)
	   */
	  if (strncasecmp(buf, MyName, MyNameLen) == 0)
	    {
	      p = strtok(buf+MyNameLen, " \t");	
	
	      if (strcasecmp(p, "PlayCmd") == 0) {
		p = strtok(NULL, "\n"); /* allow parameters */
		if (p && *p) {
		  strcpy(audio_play_cmd_line, p);
		}
	      }
	      else if (strcasecmp(p, "Dir") == 0) {
		p = strtok(NULL, " \t");
		if (p && *p) {
		  strcpy(audio_play_dir, p);
		}
	      }
	      else if (strcasecmp(p, "Delay") == 0) {
		p = strtok(NULL, " \t");
		if (p && *p) {
		  audio_delay = atoi(p);
		}
	      }
	
#ifdef HAVE_RPLAY
	      /*
	       * Check for rplay configuration options.
	       */
	      else if (strcasecmp(p, "RplayHost") == 0)
		{
		  p = strtok(NULL, " \t");
		  if (p && *p)
		    {
		      /*
		       * Check for environment variables like $HOSTDISPLAY.
		       */
		      if (*p == '$')
			{
			  char	*c1, *c2;
			  c2 = host;
			  for (c1 = (char *)getenv(p+1); *c1 && (*c1 != ':'); c1++)
			    {
			      *c2++ = *c1;
			    }
			  *c2 = '\0';
			}
		      else
			{
			  strcpy(host, p);
			}
		    }
		}
	      else if (strcasecmp(p, "RplayVolume") == 0)
		{
		  p = strtok(NULL, " \t");
		  if (p && *p) {
		    volume = atoi(p);
		  }
		}
	      else if (strcasecmp(p, "RplayPriority") == 0)
		{
		  p = strtok(NULL, " \t");
		  if (p && *p) {
		    priority = atoi(p);
		  }
		}
#endif
	      /*
               * *FvwmAudio <message_type> <audio_file>
	       * for builtin rplay
	       *
	       *  or
	       *
	       * *FvwmAudio <message_type> <"any quoted parameters">
	       * can be (mis)used to call FvwmAudioPlayCmd
	       * with arbitrary parameters e.g: rsynth/say "window raised"
	       *
	       * can even be used to call arbitrary commands,
	       * if the following wrapper is used as FvwmAudioPlayCmd
	       * 
	       *	#!/bin/tcsh
	       *	${argv[1-]}
	       */
	      else  {
		message = p;
		p=strtok(NULL, "");	/* get rest of line */
		sound = PeekToken(p);	/* extract next parameter */
	
		if (!message || !*message || !sound || !*sound)
		  {
		    fprintf(stderr,"%s: incomplete event definition %s\n",
			    MyName+1, buf);
		    GetConfigLine(fd,&buf);
		    continue;
		  }
	
		found = 0;

	    	for (i = 0; !found && i < MAX_MESSAGES+MAX_BUILTIN; i++)
		  {
		    if (strcasecmp(message, messages[i]) == 0) {
#ifdef HAVE_RPLAY
		      rplay_table[i] = rplay_create(RPLAY_PLAY);
		      rplay_set(rplay_table[i], RPLAY_APPEND,
				RPLAY_SOUND,	sound,
				RPLAY_PRIORITY,	priority,
				RPLAY_VOLUME,	volume,
				NULL);
#endif
		      sound_table[i]=sound;
		      found++;
		    }
		  }
		if (!found) fprintf(stderr,"%s: unknown message type: %s\n",
				    MyName+1, message);
	      }
	   }
	}
      GetConfigLine(fd,&buf);
    }

#ifdef HAVE_RPLAY
  /*
   * Builtin rplay support is enabled when FvwmAudioPlayCmd == builtin-rplay.
   */
  if (strcasecmp(audio_play_cmd_line, "builtin-rplay") == 0)
    {
      rplay_fd = rplay_open(host);
      if (rplay_fd < 0)
	{
	  rplay_perror("rplay_open");
	  done(1);
	}
    }
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(int *fd)
{
	unsigned long	header[HEADER_SIZE], body[ MAX_BODY_SIZE ];
	int		body_length,count,count2=0, total;
	long		code;
	
	while (1)
	{
	    if ((count = read(fd[1],header,
			      HEADER_SIZE*sizeof(unsigned long))) <= 0)
		done(0);
		
	    /* Ignore messages that occur during the delay period. */
	    now = time(0);
		
	    body_length = header[2]-HEADER_SIZE;
	
	    if( header[ 0 ] != START_FLAG )
		continue;	/* should find something better for resyncing */


	    /* read the message so we are still synchronized */

	    total = 0;
	    while(total < body_length*sizeof(unsigned long))
	    {
		if((count2=read(fd[1],&body[total],
				body_length*sizeof(unsigned long)-total)) >0)
		    total += count2;
		else 
		    if(count2 < 0)
			done(0);
	    }
		
	    if (now < (last_time + audio_delay))
		continue;		/* quash message */

	    /*
	     * code will equal the number of shifts in the
	     * base-2 header[1] number.  Could use log here
	     * but this should be fast enough.
	     */
	    code = -1;
	    while (header[1])
	    {
		code++;
		header[1] >>= 1;
	    }
	
	    /*
	     * Play the sound.
	     */
	    if (code >= 0 && code < MAX_MESSAGES)
		audio_play(code);
	    else
		audio_play(BUILTIN_UNKNOWN);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
	done(1);
}

/***********************************************************************
 *
 *  Procedure:
 *	done - common exit point for FvwmAudio.
 *
 ***********************************************************************/
void	done(int n)
{
  if (n == 0)
    audio_play(BUILTIN_SHUTDOWN); /* only play if no error... */
  exit(n);
}

/***********************************************************************
 *
 * Procedure:
 *
 *    audio_play - actually plays sound from lookup table
 *
 **********************************************************************/
int audio_play(short sound) 
{
	static char buf[BUFSIZE];
	int ret;

#ifdef HAVE_RPLAY
	if (rplay_fd != -1)
	{
		if (rplay_table[sound])
		{
			if (rplay(rplay_fd, rplay_table[sound]) >= 0)
				last_time = now;
			else
				rplay_perror("rplay");
		}
		return 0;
	}
#endif
	if (sound_table[sound])
	{
		memset(buf,0,BUFSIZE);

		/*
		 * Don't use audio_play_dir if it's NULL or if the sound file
		 * is an absolute pathname.
		 */
		if (audio_play_dir[0] == '\0' || sound_table[sound][0] == '/')
			sprintf(buf,"%s %s", audio_play_cmd_line, sound_table[sound]);
		else
			sprintf(buf,"%s %s/%s &", audio_play_cmd_line, audio_play_dir,
				sound_table[sound]);
		
		if( ! ( ret = system(buf)))
		        last_time = now;
		return ret;
	}  
	return 1;
}

