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

#include "FvwmConsole.h"

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#define fvwm_rl_bind_key(x, y) rl_bind_key(x, y)
#define fvwm_readline(x) readline(x)
#define fvwm_add_history(cmd) add_history(cmd)
#define fvwm_stifle_history(x) stifle_history(x)
#define fvwm_read_history(x) read_history(x)
#define fvwm_write_history(x) write_history(x)
#ifdef HAVE_GNU_READLINE
#define fvwm_append_history(x, y) append_history(x, y)
#define fvwm_history_truncate_file(x, y) history_truncate_file(x, y)
#define fvwm_read_history_range(x, y, z) read_history_range(x, y, z)
#else
#define fvwm_append_history(x, y) -1
#define fvwm_history_truncate_file(x, y) -1
#define fvwm_read_history_range(x, y, z) -1
#endif
#define USE_READLINE 1
#else
#define fvwm_rl_bind_key(x, y) -1
#define fvwm_readline(x) NULL
#define fvwm_add_history(cmd)
#define fvwm_append_history(x, y) -1
#define fvwm_history_truncate_file(x, y) -1
#define fvwm_read_history_range(x, y, z) -1
#define fvwm_stifle_history(x)
#define fvwm_read_history(x) -1
#define fvwm_write_history(x) -1
#define USE_READLINE 0
#endif

static char cmd[MAX_COMMAND_SIZE];
static char *line = (char *)NULL;
static int done_init = 0;
static char *h_file;

char *get_line(void)
{
	char *prompt;
	int len;
	char *home;
	int fdh;

	if (USE_READLINE == 0)
	{
		if (fgets(cmd, MAX_COMMAND_SIZE, stdin) == NULL)
		{
			return NULL;
		}

		return cmd;
	}
	/* If initialization hasn't been done, do it now:
	 *  - We don't want TAB completion
	 */
	if (!done_init)
	{
		(void)fvwm_rl_bind_key('\t', rl_insert);
		/* get history from file */
		home = getenv("FVWM_USERDIR");
		h_file = xmalloc(strlen(home) + sizeof(HISTFILE) + 1);
		strcpy(h_file, home);
		strcat(h_file, HISTFILE);
		fvwm_stifle_history(HISTSIZE);
		if (access(h_file, F_OK)  < 0)
		{
			/* if it doesn't exist create it */
			fdh = creat(h_file, S_IRUSR | S_IWUSR);
			if (fdh != -1)
			{
				close(fdh);
			}
		}
		else
		{
			(void)fvwm_read_history(h_file);
		}
		done_init = 1;
	}
	/* Empty out the previous info */
	len = 0;
	*cmd = '\0';
	prompt = PS1;
	while (1)
	{
		int linelen = 0;

		/* If the buffer has already been allocated, free the memory.
		 */
		if (line != (char *)NULL)
		{
			free(line);
		}

		/* Get a line from the user. */
		SUPPRESS_UNUSED_VAR_WARNING(prompt);
		line  = fvwm_readline(prompt);
		if (line == NULL)
		{
			return NULL;
		}

		/* Make sure we have enough space for the new line */
		linelen = strlen(line);
		if (len + linelen > MAX_COMMAND_SIZE-2)
		{
			fprintf(
				stderr, "line too long %d chars max %d \a\n",
				len + linelen, MAX_COMMAND_SIZE - 2);
			strncat(cmd, line, MAX_COMMAND_SIZE - len - 2);
			fvwm_add_history(cmd);
			break;
		}

		/* Copy the new line onto the end of the current line */
		strcat(cmd, line);
		/* If the current line doesn't end with a backslash, we're done
		 */
		len = strlen(cmd);
		if (cmd[len-1] != '\\')
		{
			break;
		}
		/* Otherwise, remove it and wait for more (add a space if
		 * needed) */
		prompt = PS2;
		cmd[len-1] =
			(cmd[len-2] == ' ' || cmd[len-2] == '\t') ? '\0' : ' ';
	}
	/* If the command has any text in it, save it on the history. */
	if (*cmd != '\0')
	{
		fvwm_add_history(cmd);
		if (fvwm_append_history(1, h_file) == 0)
		{
			(void)fvwm_history_truncate_file(h_file, HISTSIZE);
		}
		else
		{
			(void)fvwm_write_history(h_file);
		}
	}
	cmd[len]   = '\n';
	cmd[len+1] = '\0';

	return cmd;
}
