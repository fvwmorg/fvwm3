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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "events.h"
#include "misc.h"
#include "screen.h"

#define MAX_READ_DEPTH 40
static char *curr_read_file = NULL;
static char *curr_read_dir = NULL;
static unsigned int curr_read_depth = 0;
static char *prev_read_files[MAX_READ_DEPTH];

static int push_read_file(const char *file)
{
	if (curr_read_depth >= MAX_READ_DEPTH)
	{
		fvwm_msg(
			ERR, "Read", "Nested Read limit %d is reached",
			MAX_READ_DEPTH);
		return FALSE;
	}
	prev_read_files[curr_read_depth++] = curr_read_file;
	curr_read_file = safestrdup(file);
	if (curr_read_dir)
	{
		free(curr_read_dir);
	}
	curr_read_dir = NULL;

	return TRUE;
}

static void pop_read_file(void)
{
	if (curr_read_depth == 0)
	{
		return;
	}
	if (curr_read_file)
	{
		free(curr_read_file);
	}
	curr_read_file = prev_read_files[--curr_read_depth];
	if (curr_read_dir)
	{
		free(curr_read_dir);
	}
	curr_read_dir = NULL;

	return;
}

const char *get_current_read_file(void)
{
	return curr_read_file;
}

const char *get_current_read_dir(void)
{
	if (!curr_read_dir)
	{
		char *dir_end;
		if (!curr_read_file)
		{
			return ".";
		}
		/* it should be a library function parse_file_dir() */
		dir_end = strrchr(curr_read_file, '/');
		if (!dir_end)
		{
			dir_end = curr_read_file;
		}
		curr_read_dir = safemalloc(dir_end - curr_read_file + 1);
		strncpy(curr_read_dir, curr_read_file,
			dir_end - curr_read_file);
		curr_read_dir[dir_end - curr_read_file] = '\0';
	}
	return curr_read_dir;
}


/**
 * Read and execute each line from stream.
 **/
void run_command_stream(
	FILE* f, const XEvent *eventp, FvwmWindow *fw,
	unsigned long context, int Module)
{
	char *tline;
	char line[1024];

	/* Set close-on-exec flag */
	fcntl(fileno(f), F_SETFD, 1);

	/* Update window decorations in case we were called from a menu that
	 * has now popped down. */
	handle_all_expose();

	tline = fgets(line, (sizeof line) - 1, f);
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
		l = strlen(tline);
		if (l > 0 && tline[l - 1] == '\n')
		{
			tline[l - 1] = '\0';
		}
		if (debugging)
		{
			fvwm_msg(
				DBG, "ReadSubFunc",
				"Module switch %d, about to exec: '%s'",
			Module, tline);
		}
		old_execute_function(
			NULL, tline, fw, eventp, context, Module, 0, NULL);
		tline = fgets(line, (sizeof line) - 1, f);
	}

	return;
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
static int parse_filename(
	char *cmdname, char *action, char **filename, int *quiet_flag)
{
	char *rest;
	char *option;

	/*  fvwm_msg(INFO,cmdname,"action == '%s'",action); */

	/* read file name arg */
	rest = GetNextToken(action,filename);
	if (*filename == NULL)
	{
		fvwm_msg(ERR, cmdname, "missing filename parameter");
		return 0;
	}
	/* optional "Quiet" argument -- flag defaults to `off' (noisy) */
	*quiet_flag = 0;
	rest = GetNextToken(rest,&option);
	if ( option != NULL)
	{
		*quiet_flag = strncasecmp(option, "Quiet", 5) == 0;
		free(option);
	}

	return 1;
}


/**
 * Returns FALSE if file not found
 **/
int run_command_file(
	char* filename, const XEvent *eventp, FvwmWindow *fw,
	unsigned long context, int Module)
{
	char *full_filename;
	FILE* f;

	if (filename[0] == '/')
	{             /* if absolute path */
		f = fopen(filename,"r");
		full_filename = filename;
	}
	else
	{             /* else its a relative path */
		full_filename = CatString3( fvwm_userdir, "/", filename);
		f = fopen( full_filename, "r");
		if ( f == NULL)
		{
			full_filename = CatString3(
				FVWM_DATADIR, "/", filename);
			f = fopen( full_filename, "r");
		}
	}
	if (f == NULL)
	{
		return FALSE;
	}
	if (push_read_file(full_filename) == FALSE)
	{
		return FALSE;
	}
	run_command_stream( f, eventp, fw, context, Module);
	fclose( f);
	pop_read_file();

	return TRUE;
}

/**
 * Busy Cursor Stuff for Read
 **/
static void cursor_control(Bool grab)
{
	static unsigned int read_depth = 0;
	static Bool need_ungrab = False;

	if (!(Scr.BusyCursor & BUSY_READ) && !need_ungrab)
	{
		return;
	}
	if (grab)
	{
		if (!read_depth && GrabEm(CRS_WAIT, GRAB_BUSY))
		{
			need_ungrab = True;
		}
		if (need_ungrab)
		{
			read_depth++;
		}
	}
	else if (need_ungrab)
	{
		read_depth--;
		if (!read_depth || !(Scr.BusyCursor & BUSY_READ))
		{
			UngrabEm(GRAB_BUSY);
			need_ungrab = False;
			read_depth = 0;
		}
	}

	return;
}

void CMD_Read(F_CMD_ARGS)
{
	char* filename;
	int read_quietly;

	DoingCommandLine = False;

	if (debugging)
	{
		fvwm_msg(DBG, "ReadFile", "Module flag %d, about to attempt %s",
			 *Module, action);
	}

	if ( !parse_filename( "Read", action, &filename, &read_quietly))
	{
		return;
	}
	cursor_control(True);
	if ( !run_command_file( filename, eventp, fw, context, *Module) &&
	     !read_quietly)
	{
		if (filename[0] == '/')
		{
			fvwm_msg( ERR, "Read",
				  "file '%s' not found",filename);
		}
		else
		{
			fvwm_msg( ERR, "Read",
				  "file '%s' not found in %s or "FVWM_DATADIR,
				  filename, fvwm_userdir);
		}
	}
	free( filename);
	cursor_control(False);

	return;
}

void CMD_PipeRead(F_CMD_ARGS)
{
	char* command;
	int read_quietly;
	FILE* f;

	DoingCommandLine = False;

	if (debugging)
	{
		fvwm_msg(DBG,"PipeRead","about to attempt '%s'", action);
	}
	if (!parse_filename("PipeRead", action, &command, &read_quietly))
	{
		return;
	}
	cursor_control(True);
	f = popen(command, "r");
	if (f == NULL)
	{
		if (!read_quietly)
		{
			fvwm_msg(
				ERR, "PipeRead", "command '%s' not run",
				command);
		}
		free(command);
		cursor_control(False);
		return;
	}
	free(command);

	run_command_stream(f, eventp, fw, context, *Module);
	pclose(f);
	cursor_control(False);

	return;
}
