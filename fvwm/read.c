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

/*
 * *************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 *
 * Changed 09/24/98 by Dan Espen:
 * - remove logic that processed and saved module configuration commands.
 * Its now in "modconf.c".
 * *************************************************************************
 */
#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif

#include "fvwm.h"
#include "read.h"
#include "misc.h"
#include "screen.h"
#include "Module.h"

extern Boolean debugging;

char *fvwm_file = NULL;


/**
 * Read and execute each line from stream.
 **/
void run_command_stream( FILE* f, XEvent *eventp, FvwmWindow *tmp_win,
			 unsigned long context, int Module )
{
    char *tline,line[1024];

    /* Set close-on-exec flag */
    fcntl(fileno(f), F_SETFD, 1);

    tline = fgets(line,(sizeof line)-1,f);
    while(tline)
    {
	int l;
	while(tline && (l = strlen(line)) < sizeof(line) && l >= 2 &&
	      line[l-2]=='\\' && line[l-1]=='\n')
        {
	    tline = fgets(line+l-2,sizeof(line)-l+1,f);
	}
	tline=line;
	while(isspace((unsigned char)*tline))
	    tline++;
	if (debugging)
	    fvwm_msg(DBG,"ReadSubFunc","about to exec: '%s'",tline);

	ExecuteFunction(tline,tmp_win,eventp,context,Module,EXPAND_COMMAND);
	tline = fgets(line,(sizeof line)-1,f);
    }
}


/**
 * Parse the action string.  We expect a filename, and optionally,
 * the keyword "Quiet".  The parameter `cmdname' is used for diagnostic
 * messages only.
 *
 * Returns true if the parse succeeded.
 * The filename and the presence of the quiet flag are returned
 * using the pointer arguments.
 **/
static int parse_filename( char* cmdname, char* action,
			   char** filename, int* quiet_flag )
{
    char* rest;
    char* option;

    /*  fvwm_msg(INFO,cmdname,"action == '%s'",action); */

    /* read file name arg */
    rest = GetNextToken(action,filename);
    if(*filename == NULL) {
	fvwm_msg(ERR, cmdname, "missing filename parameter");
	return 0;
    }

    /* optional "Quiet" argument -- flag defaults to `off' (noisy) */
    *quiet_flag = 0;
    rest = GetNextToken(rest,&option);
    if ( option != NULL) {
	*quiet_flag = strncasecmp(option,"Quiet",5) == 0;
	free(option);
    }

    return 1;
}


/**
 * Returns FALSE if file not found
 **/
int run_command_file( char* filename, XEvent *eventp, FvwmWindow *tmp_win,
		      unsigned long context, int Module )
{
    FILE* f;

    /* Save filename for passing as argument to modules */
    if (fvwm_file != NULL)
	free(fvwm_file);
    fvwm_file = NULL;

    if (filename[0] == '/') {             /* if absolute path */
	f = fopen(filename,"r");
	fvwm_file = strdup( filename );
    } else {                              /* else its a relative path */
	char* path = CatString3( user_home_dir, "/", filename );
	f = fopen( path, "r" );
	if ( f == NULL ) {
	    path = CatString3( FVWM_CONFIGDIR, "/", filename );
	    f = fopen( path, "r" );
	}
	fvwm_file = strdup( path );
    }

    if (f == NULL)
	return 0;

    run_command_stream( f, eventp, tmp_win, context, Module );
    fclose( f );

    free( fvwm_file );
    fvwm_file = NULL;
    return 1;
}


void ReadFile(F_CMD_ARGS)
{
    char* filename;
    int read_quietly;

    DoingCommandLine = False;
    
    if (fFvwmInStartup && *Module > -1)
    {
      Cursor cursor = XCreateFontCursor(dpy, XC_watch);
      XGrabPointer(dpy, Scr.Root, 0, 0, GrabModeSync, GrabModeSync,
		   None, cursor, CurrentTime);
    }

    if (debugging)
	fvwm_msg(DBG,"ReadFile","about to attempt '%s'",action);

    if ( !parse_filename( "Read", action, &filename, &read_quietly ) )
	return;

    if ( !run_command_file( filename, eventp, tmp_win, context, *Module ) &&
	 !read_quietly )
    {
	fvwm_msg( ERR, "Read",
		  "file '%s' not found in %s or "FVWM_CONFIGDIR,
		  filename, user_home_dir );
    }
    free( filename );
}



void PipeRead(F_CMD_ARGS)
{
    char* command;
    int read_quietly;
    FILE* f;

    DoingCommandLine = False;

    /* Save filename for passing as argument to modules */
    if (fvwm_file != NULL)
	free(fvwm_file);
    fvwm_file = NULL;

    if (debugging)
	fvwm_msg(DBG,"PipeRead","about to attempt '%s'",action);

    if ( !parse_filename( "PipeRead", action, &command, &read_quietly ) )
	return;

    f = popen( command, "r" );

    if (f == NULL) {
	if ( !read_quietly )
	    fvwm_msg( ERR, "PipeRead", "command '%s' not run", command );
	free( command );
	return;
    }
    free( command );

    run_command_stream( f, eventp, tmp_win, context, *Module );
    pclose( f );

}
