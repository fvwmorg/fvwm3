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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <err.h>
#include <ctype.h>

#ifdef HAVE_LIBBSD
#include <bsd/libutil.h>
#include <bsd/string.h>
#include <bsd/sys/queue.h>
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

struct str_parse_state {
	char		*buf;
	size_t 		 len, off;
	int 		 esc;
};
static struct str_parse_state sps;

static void str_append(char **, size_t *, const char *);
static char str_getc(void);
static char *str_tokenise(size_t);
static int str_to_argv(char *, size_t, int *, char ***);
static void argv_free(int, char **);

#define str_unappend(ch) if (sps.off > 0 && (ch) != EOF) { sps.off--; }

static void
str_append(char **buf, size_t *len, const char *add)
{
	size_t 	al = 1;

	if (al > SIZE_MAX - 1 || *len > SIZE_MAX - 1 - al) {
		fprintf(stderr, "buffer is too big");
		exit (1);
	}
	*buf = realloc(*buf, (*len) + 1 + al);
	memcpy((*buf) + *len, add, al);
	(*len) += al;
}

static char
str_getc(void)
{
	char 	ch;

	if (sps.esc != 0) {
		sps.esc--;
		return ('\\');
	}
	for (;;) {
		ch = EOF;
		if (sps.off < sps.len)
			ch = sps.buf[sps.off++];

		if (ch == '\\') {
			sps.esc++;
			continue;
		}
		if (ch == '\n' && (sps.esc % 2) == 1) {
			sps.esc--;
			continue;
		}

		if (sps.esc != 0) {
			str_unappend(ch);
			sps.esc--;
			return ('\\');
		}
		return (ch);
	}
}

static char *
str_tokenise(size_t lineno)
{
	char 			 ch;
	char			*buf;
	size_t			 len;
	enum { APPEND,
	       DOUBLE_QUOTES,
	       SINGLE_QUOTES,
	       BACKTICK_QUOTES,
	} 			 state = APPEND;

	len = 0;
	buf = malloc(1);

	for (;;) {
		ch = str_getc();
		/* EOF or \n are always the end of the token. */
		if (ch == EOF || (state == APPEND && ch == '\n'))
			break;

		/* Whitespace ends a token unless inside quotes.  But if we've
		 * also been given:
		 *
		 * + foo "" bar
		 *
		 * don't lsoe that.
		 */
		if (((ch == ' ' || ch == '\t') && state == APPEND) &&
		    buf[0] != '\0') {
			break;
		}

		if (ch == '\\' && state != SINGLE_QUOTES) {
			ch = str_getc();
			str_append(&buf, &len, &ch);
			continue;
		}
		switch (ch) {
		case '\'':
			if (state == APPEND) {
				state = SINGLE_QUOTES;
				continue;
			}
			if (state == SINGLE_QUOTES) {
				state = APPEND;
				continue;
			}
			break;
		case '`':
			if (state == APPEND) {
				state = BACKTICK_QUOTES;
				continue;
			}
			if (state == BACKTICK_QUOTES) {
				state = APPEND;
				continue;
			}
			break;
		case '"':
			if (state == APPEND) {
				state = DOUBLE_QUOTES;
				continue;
			}
			if (state == DOUBLE_QUOTES) {
				state = APPEND;
				continue;
			}
			break;
		default:
			/* Otherwise add the character to the buffer. */
			str_append(&buf, &len, &ch);
			break;

		}
	}
	str_unappend(ch);
	buf[len] = '\0';

	if (*buf == '\0' || state == SINGLE_QUOTES ||
	    state == DOUBLE_QUOTES || state == BACKTICK_QUOTES) {
		fprintf(stderr, "%ld: Unterminated string: <%s>, missing: %c\n",
			lineno, buf, state == SINGLE_QUOTES ? '\'' :
			state == DOUBLE_QUOTES ? '"' :
			state == BACKTICK_QUOTES ? '`' : ' ');
		goto error;
	}
	return (buf);

error:
	free(buf);
	return (NULL);
}

int str_to_argv(char *str, size_t lineno, int *ret_argc, char ***ret_argv)
{
	char			*token;
	char			 ch, next;
	char 			**argv = NULL;
	int 			 argc = 0;

	if (str == NULL) {
		fprintf(stderr, "str is NULL, skipping...\n");
		return -1;
	}
	free(sps.buf);
	sps.buf = strdup(str);
	sps.len = strlen(sps.buf);

	for (;;) {
		if (str[0] == '#') {
			/* Skip comment. */
			next = str_getc();
			while (((next = str_getc()) != EOF))
				; /* Nothing. */
		}

		ch = str_getc();

		if (ch == '\n' || ch == EOF)
			goto out;
		if (ch == ' ' || ch == '\t')
			continue;

		/* Tokenise the string according to quoting rules.  Note that
		 * the string is stepped-back by one character to make the
		 * tokenisation easier, and not to kick-off the state of the
		 * parsing from this point.
		 */
		str_unappend(ch);
		token = str_tokenise(lineno);
		if (token == NULL) {
			argv_free(argc, argv);
			return -1;
		}

		/* Add to argv. */
		argv = reallocarray(argv, argc + 1, sizeof *argv);
		argv[argc++] = strdup(token);
	}
out:
	*ret_argv = argv;
	*ret_argc = argc;

	return 0;
}

void
argv_free(int argc, char **argv)
{
	int 	i;

	if (argc == 0)
		return;

	for (i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
}

static int push_read_file(const char *file)
{
	if (curr_read_depth >= MAX_READ_DEPTH)
	{
		fvwm_debug(__func__, "Nested Read limit %d is reached",
			   MAX_READ_DEPTH);
		return 0;
	}
	prev_read_files[curr_read_depth++] = curr_read_file;
	curr_read_file = fxstrdup(file);
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
		curr_read_dir = fxmalloc(dir_end - curr_read_file + 1);
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
	const char	delims[3] = {'\\', '\\', '\0'};
	char 		*buf, *copy;
	char 		**argv;
	int 		 argc = 0;
	size_t 	 	 line = 0;
	int 		 ret;

	/* Set close-on-exec flag */
	fcntl(fileno(f), F_SETFD, 1);

	/* Update window decorations in case we were called from a menu that
	 * has now popped down. */
	handle_all_expose();

	while ((buf = fparseln(f, NULL, &line, delims, 0))) {
		memset(&sps, 0, sizeof sps);

		copy = buf;
		copy += strspn(copy, " \t\n");
		if (*copy == '\0') {
			free(buf);
			continue;
		}
		ret = str_to_argv(copy, line, &argc, &argv);

		if (ret != 0 || argv == NULL) {
			free(buf);
			continue;
		}
		execute_function(cond_rc, exc, buf, 0);
		free(buf);
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
		fvwm_debug(__func__, "missing filename parameter");
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
	FILE *f = NULL;

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
	full_filename = fxstrdup(filename);

	if (full_filename[0] == '/')
	{
		/* It's an absolute path */
		f = fopen(full_filename,"r");
	} else {
		/* It's a relative path.  Check in either FVWM_USERDIR or
		 * FVWM_DATADIR.
		 * */
		free(full_filename);
		xasprintf(&full_filename, "%s/%s", fvwm_userdir, filename);

		if((f = fopen(full_filename, "r")) == NULL)
		{
			free(full_filename);
			xasprintf(&full_filename, "%s/%s", FVWM_DATADIR, filename);
			f = fopen(full_filename, "r");
		}
	}

	if ((f == NULL) &&
		(f = fopen(filename, "r")) == NULL) {

		/* We really couldn't open the file. */
		free(full_filename);
		return 0;
	}

	if (push_read_file(full_filename) == 0)
	{
		free(full_filename);
		fclose(f);
		return 0;
	}
	run_command_stream(NULL, f, exc);
	fclose(f);
	pop_read_file();

	free(full_filename);

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
				fvwm_debug(__func__,
					   "file '%s' not found", filename);
			}
			else
			{
				fvwm_debug(__func__,
					   "file '%s' not found in %s or "
					   FVWM_DATADIR, filename,
					   fvwm_userdir);
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
			fvwm_debug(__func__, "command '%s' not run",
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
