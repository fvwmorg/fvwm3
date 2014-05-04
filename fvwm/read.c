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

/* Changed 09/24/98 by Dan Espen:
 * - remove logic that processed and saved module configuration commands.
 * Its now in "modconf.c".
 */
#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/Parse.h"
#include "libs/Strings.h"
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
static int curr_read_depth = 0;
static char *prev_read_files[MAX_READ_DEPTH];

static int push_read_file(const char *file)
{
	if (curr_read_depth >= MAX_READ_DEPTH)
	{
		fvwm_msg(
			ERR, "Read", "Nested Read limit %d is reached",
			MAX_READ_DEPTH);
		return 0;
	}
	prev_read_files[curr_read_depth++] = curr_read_file;
	curr_read_file = xstrdup(file);
	if (curr_read_dir)
	{
		free(curr_read_dir);
	}
	curr_read_dir = NULL;

	return 1;
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
		curr_read_dir = xmalloc(dir_end - curr_read_file + 1);
		strncpy(curr_read_dir, curr_read_file,
			dir_end - curr_read_file);
		curr_read_dir[dir_end - curr_read_file] = '\0';
	}
	return curr_read_dir;
}


/*
 * Read and execute each line from stream.
 */
void run_command_stream(
	cond_rc_t *cond_rc, FILE *f, const exec_context_t *exc)
{
	char *tline;
	char line[1024];

	/* Set close-on-exec flag */
	fcntl(fileno(f), F_SETFD, 1);

	/* Update window decorations in case we were called from a menu that
	 * has now popped down. */
	handle_all_expose();

	tline = fgets(line, (sizeof line) - 1, f);
	while (tline)
	{
		int l;
		while (tline && (l = strlen(line)) < sizeof(line) && l >= 2 &&
		      line[l-2]=='\\' && line[l-1]=='\n')
		{
			tline = fgets(line+l-2,sizeof(line)-l+1,f);
		}
		tline=line;
		while (isspace((unsigned char)*tline))
		{
			tline++;
		}
		l = strlen(tline);
		if (l > 0 && tline[l - 1] == '\n')
		{
			tline[l - 1] = '\0';
		}
		execute_function(cond_rc, exc, tline, 0);
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
	if (option != NULL)
	{
		*quiet_flag = strncasecmp(option, "Quiet", 5) == 0;
		free(option);
	}

	return 1;
}


/**
 * Returns 0 if file not found
 **/
int run_command_file(
	char *filename, const exec_context_t *exc)
{
	char *full_filename;
	FILE* f = NULL;

	/* We attempt to open the filename by doing the following:
	 *
	 * - If the file does start with a "/" then it's treated as an
	 *   absolute path.
	 *
	 *  - Otherwise, it's assumed to be in FVWM_USERDIR OR FVWM_DATADIR,
	 *    whichever comes first.
	 *
	 * - If the file starts with "./" or "../" then try and
	 *   open the file exactly as specified which means
	 *   things like:
	 *
	 *   ../.././foo is catered for.  At this point, we just try and open
	 *   the specified file regardless.
	 *
	 *  - *Hidden* files in the CWD would have to be specified as:
	 *
	 *    ./.foo
	 */
	full_filename = filename;

	if (full_filename[0] == '/')
	{
		/* It's an absolute path */
		f = fopen(full_filename,"r");
	} else {
		/* It's a relative path.  Check in either FVWM_USERDIR or
		 * FVWM_DATADIR.
		 * */
		full_filename = CatString3(fvwm_userdir, "/", filename);

		if((f = fopen(full_filename, "r")) == NULL)
		{
			full_filename = CatString3(
					FVWM_DATADIR, "/", filename);
			f = fopen(full_filename, "r");
		}
	}

	if ((f == NULL) &&
		(f = fopen(filename, "r")) == NULL) {
		/* We really couldn't open the file. */
		return 0;
	}

	if (push_read_file(full_filename) == 0)
	{
		return 0;
	}
	run_command_stream(NULL, f, exc);
	fclose(f);
	pop_read_file();

	return 1;
}

/**
 * Busy Cursor Stuff for Read
 **/
static void cursor_control(Bool grab)
{
	static int read_depth = 0;
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

	if (cond_rc != NULL)
	{
		cond_rc->rc = COND_RC_OK;
	}
	if (!parse_filename("Read", action, &filename, &read_quietly))
	{
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_ERROR;
		}
		return;
	}
	cursor_control(True);
	if (!run_command_file(filename, exc))
	{
		if (!read_quietly)
		{
			if (filename[0] == '/')
			{
				fvwm_msg(
					ERR, "Read",
					"file '%s' not found", filename);
			}
			else
			{
				fvwm_msg(
					ERR, "Read",
					"file '%s' not found in %s or "
					FVWM_DATADIR, filename, fvwm_userdir);
			}
		}
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_ERROR;
		}
	}
	free(filename);
	cursor_control(False);

	return;
}

void CMD_PipeRead(F_CMD_ARGS)
{
	char* command;
	int read_quietly;
	FILE* f;

	DoingCommandLine = False;

	if (cond_rc != NULL)
	{
		cond_rc->rc = COND_RC_OK;
	}
	if (!parse_filename("PipeRead", action, &command, &read_quietly))
	{
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_ERROR;
		}
		return;
	}
	cursor_control(True);
	f = popen(command, "r");
	if (f == NULL)
	{
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_ERROR;
		}
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

	run_command_stream(cond_rc,f, exc);
	pclose(f);
	cursor_control(False);

	return;
}
