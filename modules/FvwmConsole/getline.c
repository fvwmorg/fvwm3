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

#include "FvwmConsole.h"

#ifndef HAVE_READLINE
static   char cmd[MAX_COMMAND_SIZE];

/* no readline - starts here */
char *getline() {
  if( fgets(cmd,MAX_COMMAND_SIZE,stdin) == NULL  ) {
	return(NULL);
  }
  return(cmd);
}

#else
/* readline - starts here */
#include <readline/readline.h>
#include <readline/history.h>

extern int rl_bind_key();


static char cmd[MAX_COMMAND_SIZE];
static char *line = (char *)NULL;
static int done_init = 0;
static char *h_file;

char *getline()
{
    char *prompt;
    int len;
	char *home;
	int  fdh;

    /* If initialization hasn't been done, do it now:
     *  - We don't want TAB completion
     */
    if (!done_init) {
        rl_bind_key('\t', rl_insert);

		/* get history from file */
		home = getenv("FVWM_USERDIR");
		h_file = safemalloc(strlen(home) + sizeof(HISTFILE) + 1);
		strcpy(h_file, home);
		strcat(h_file, HISTFILE);
		if( access( h_file, F_OK)  < 0) {
		  /* if it doesn't exist create it */
		  fdh = creat( h_file, S_IRUSR | S_IWUSR );
		  if( fdh != -1 ) {
			close( fdh );
		  }
		} else {
		  read_history_range( h_file, 0, HISTSIZE );
		}
		done_init = 1;
  }

    /* Empty out the previous info */
    len = 0;
    *cmd = '\0';
    prompt = PS1;

    while (1) {
        int linelen = 0;

        /* If the buffer has already been allocated, free the memory. */
        if (line != (char *)NULL)
            free(line);

  /* Get a line from the user. */
        line  = readline(prompt);
        if (line == NULL)
            return (NULL);

        /* Make sure we have enough space for the new line */
        linelen = strlen(line);
        if (len + linelen > MAX_COMMAND_SIZE-2 ) {
		  fprintf( stderr, "line too long %d chars max %d \a\n",
				   len+linelen, MAX_COMMAND_SIZE-2 );
		  strncat(cmd, line, MAX_COMMAND_SIZE-len-2);
		  add_history(cmd);
		  break;
        }

        /* Copy the new line onto the end of the current line */
        strcat(cmd, line);

        /* If the current line doesn't end with a backslash, we're done */
        len = strlen(cmd);
        if (cmd[len-1] != '\\')
            break;

        /* Otherwise, remove it and wait for more (add a space if needed) */
        prompt = PS2;
        cmd[len-1] = (cmd[len-2]==' ' || cmd[len-2]=='\t') ? '\0' : ' ';
  }

    /* If the command has any text in it, save it on the history. */
    if (*cmd != '\0') {
	  add_history(cmd);
	  append_history( 1,h_file );
	  history_truncate_file( h_file, HISTSIZE );
	}

    cmd[len]   = '\n';
    cmd[len+1] = '\0';

    return (cmd);
}
/* readline - end here */
#endif


