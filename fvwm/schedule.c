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

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "libs/Colorset.h"
#include "bindings.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "screen.h"

typedef struct schedule_queue_type
{
  struct schedule_queue_type *next;
  Window window;
  int id;
  char *command;
  Time time_to_execute;
} schedule_queue_type;

static int last_schedule_id = 0;
static int next_schedule_id = -1;

static schedule_queue_type *sq_start = NULL;

static int is_not_earlier(Time t1, Time t2)
{
	unsigned long ul1 = (unsigned long)t1;
	unsigned long ul2 = (unsigned long)t2;
	unsigned long diff;
	signed long udiff;

	diff = ul2 - ul1;
	udiff = *(signed long *)&diff;

	return (udiff <= 0);
}

static void remove_ids_from_schedule_queue(
	int *pid)
{
	schedule_queue_type *temp;
	schedule_queue_type *next;
	schedule_queue_type *prev;
	int id;

	if (sq_start == NULL)
	{
		return;
	}
	if (pid != NULL)
	{
		id = *pid;
	}
	else
	{
		id = last_schedule_id;
	}


	for (temp = sq_start, prev = NULL; temp != NULL; temp = next)
	{
		if (temp->id != id)
		{
			prev = temp;
			next = temp->next;
		}
		else
		{
			if (temp->command)
			{
				free(temp->command);
			}
			if (prev == NULL)
			{
				sq_start = temp->next;
			}
			else
			{
				prev->next = temp->next;
			}
			next = temp->next;
			free(temp);
		}
	}

	return;






}


static void add_to_schedule_queue(
	Window window, char *command, Time time_to_execute, int *pid)
{
	schedule_queue_type *new_item;
	schedule_queue_type *prev;
	schedule_queue_type *temp;

	if (command == NULL || *command == 0)
	{
		return;
	}

	new_item = (schedule_queue_type *)safemalloc(
		sizeof(schedule_queue_type));
	new_item->window = window;
	new_item->command = safestrdup(command);
	new_item->time_to_execute = time_to_execute;
	new_item->next = NULL;
	if (pid != NULL)
	{
		new_item->id = *pid;
	}
	else
	{
		new_item->id = next_schedule_id;
		next_schedule_id--;
		if (next_schedule_id >= 0)
		{
			/* wrapped around */
			next_schedule_id = -1;
		}
	}
	last_schedule_id = new_item->id;
	if (sq_start == NULL)
	{
		sq_start = new_item;
		return;
	}
	for (temp = sq_start, prev = NULL; temp != NULL;
	     prev = temp, temp = temp->next)
	{
		if (is_not_earlier(time_to_execute, temp->time_to_execute))
		{
			break;
		}
	}
	if (prev == NULL)
	{
		/* insert at start */
		new_item->next = sq_start;
		sq_start = new_item;
	}
	else
	{
		/* insert in the middle of the queue or at the end */
		new_item->next = prev->next;
		prev->next = new_item;
	}

	return;
}

/* executes all scheduled commands that are due for execution */
void execute_schedule_queue(void)
{
	schedule_queue_type *temp;
	schedule_queue_type *last;
	Time current_time;

	if (sq_start == NULL)
	{
		return;
	}
	current_time = get_server_time();
	for (temp = sq_start, last = NULL; temp != NULL; )
	{
		int do_free_temp = 0;
		int do_execute_command = 0;

		if (temp->time_to_execute == CurrentTime ||
		    (is_not_earlier(current_time, temp->time_to_execute)))
		{
			do_execute_command = (temp->command != NULL);
			do_free_temp = 1;
		}
		if (do_execute_command)
		{
			exec_func_args_type efa;
			XEvent ev;
			FvwmWindow *fw;

			memset(&efa, 0, sizeof(efa));
			memset(&ev, 0, sizeof(ev));
			efa.eventp = &ev;
			efa.tmp_win = NULL;
			efa.action = temp->command;
			efa.args = NULL;
			if (XFindContext(dpy, temp->window, FvwmContext,
					 (caddr_t *)&fw) == XCNOENT)
			{
				fw = NULL;
				efa.context = C_ROOT;
			}
			else
			{
				efa.context = C_WINDOW;
			}
			efa.module = -1;
			efa.flags.exec = 0;
			execute_function(&efa);
			free(temp->command);
		}
		if (do_free_temp)
		{
			schedule_queue_type *prev;

			if (last == NULL)
			{
				sq_start = temp->next;
			}
			else
			{
				last->next = temp->next;
			}
			prev = temp;
			temp = temp->next;
			free(prev);
		}
		else
		{
			last = temp;
			temp = temp->next;
		}
	}

	return;
}

/* returns the time in milliseconds to wait before next queue command must be
 * executed or 0 if none is queued */
int get_next_schedule_queue_ms(void)
{
	int ms;

	if (sq_start == NULL)
	{
		return 0;
	}
	ms = sq_start->time_to_execute - lastTimestamp;
	if (ms <= 0)
	{
		ms = 1;
	}

	return ms;
}

int get_next_schedule_id(void)
{
	return next_schedule_id;
}

int get_last_schedule_id(void)
{
	return last_schedule_id;
}

void CMD_Schedule(F_CMD_ARGS)
{
	Window xw;
	Time time;
	Time current_time;
	char *taction;
	int ms;
	int id;
	int *pid;
	int n;

	n = GetIntegerArguments(action, &action, &ms, 1);
	if (n <= 0)
	{
		fvwm_msg(ERR, "CMD_Schedule",
			 "Requires time to schedule as argument");
		return;
	}
	n = GetIntegerArguments(action, &taction, &id, 1);
	if (n >= 1)
	{
		pid = &id;
		action = taction;
	}
	else
	{
		pid = NULL;
	}
	/* get the time to execute */
	if (ms > 0)
	{
		current_time = get_server_time();
		time = current_time + (Time)ms;
	}
	else
	{
		time = CurrentTime;
	}
	/* get the window to operate on */
	if (tmp_win != NULL)
	{
		xw = tmp_win->w;
	}
	else
	{
		xw = None;
	}
	add_to_schedule_queue(xw, action, time, pid);

	return;
}

void CMD_Deschedule(F_CMD_ARGS)
{
	int id;
	int *pid;
	int n;

	n = GetIntegerArguments(action, &action, &id, 1);
	if (n <= 0)
	{
		pid = NULL;
	}
	else
	{
		pid = &id;
	}
	remove_ids_from_schedule_queue(pid);

	return;

}
