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
#include "libs/queue.h"
#include "fvwm.h"
#include "externs.h"
#include "colorset.h"
#include "bindings.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "screen.h"

typedef struct
{
	int id;
	Time time_to_execute;
	Window window;
	char *command;
	struct
	{
		unsigned is_scheduled_for_destruction : 1;
	} flags;
} sq_object_type;

static int last_schedule_id = 0;
static int next_schedule_id = -1;
static fqueue sq = FQUEUE_INIT;

static int cmp_times(Time t1, Time t2)
{
	unsigned long ul1 = (unsigned long)t1;
	unsigned long ul2 = (unsigned long)t2;
	unsigned long diff;
	signed long udiff;

	diff = ul1 - ul2;
	udiff = *(signed long *)&diff;
	if (udiff > 0)
	{
		return 1;
	}
	else if (udiff < 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

static int cmp_object_time(void *object1, void *object2, void *args)
{
	sq_object_type *so1 = (sq_object_type *)object1;
	sq_object_type *so2 = (sq_object_type *)object2;

	return cmp_times(so1->time_to_execute, so2->time_to_execute);
}

static int deschedule_obj_func(void *object, void *args)
{
	sq_object_type *obj = object;

	if (obj->id == *(int *)args &&
	    !obj->flags.is_scheduled_for_destruction)
	{
		if (obj->command != NULL)
		{
			free(obj->command);
		}
		free(obj);
		return 1;
	}

	return 0;
}

static void deschedule(int *pid)
{
	int id;

	if (FQUEUE_IS_EMPTY(&sq))
	{
		return;
	}
	/* get the job group id to deschedule */
	if (pid != NULL)
	{
		id = *pid;
	}
	else
	{
		id = last_schedule_id;
	}


	/* deschedule matching jobs */
	fqueue_remove_or_operate_all(&sq, deschedule_obj_func, (void *)&id);

	return;
}

static void schedule(
	Window window, char *command, Time time_to_execute, int *pid)
{
	sq_object_type *new_obj;

	if (command == NULL || *command == 0)
	{
		return;
	}
	/* create the new object */
	new_obj = (sq_object_type *)safemalloc(sizeof(sq_object_type));
	memset(new_obj, 0, sizeof(sq_object_type));
	new_obj->window = window;
	new_obj->command = safestrdup(command);
	new_obj->time_to_execute = time_to_execute;
	/* set the job group id */
	if (pid != NULL)
	{
		new_obj->id = *pid;
	}
	else
	{
		new_obj->id = next_schedule_id;
		next_schedule_id--;
		if (next_schedule_id >= 0)
		{
			/* wrapped around */
			next_schedule_id = -1;
		}
	}
	last_schedule_id = new_obj->id;
	/* insert into schedule queue */
	fqueue_add_inside(&sq, new_obj, cmp_object_time, NULL);

	return;
}

static int execute_obj_func(void *object, void *args)
{
	sq_object_type *obj = object;
	Time *ptime = (Time *)args;

	if (cmp_times(*ptime, obj->time_to_execute) >= 0)
	{
		if (obj->command != NULL)
		{
			/* execute the command */
			fvwm_cond_func_rc cond_rc;
			const exec_context_t *exc;
			exec_context_changes_t ecc;
			exec_context_change_mask_t mask = ECC_WCONTEXT;

			ecc.w.wcontext = C_ROOT;
			if (XFindContext(
				    dpy, obj->window, FvwmContext,
				    (caddr_t *)&ecc.w.fw) != XCNOENT)
			{
				ecc.w.wcontext = C_WINDOW;
				mask |= ECC_FW;
			}
			exc = exc_create_context(&ecc, mask);
			execute_function(&cond_rc, exc, obj->command, 0);
			free(obj->command);
			exc_destroy_context(exc);
		}
		free(obj);
		XFlush(dpy);
		return 1;
	}

	return 0;
}

/* executes all scheduled commands that are due for execution */
void squeue_execute(void)
{
	Time current_time;
	void *p[2];

	if (FQUEUE_IS_EMPTY(&sq))
	{
		return;
	}
	p[0] = &current_time;
	current_time = get_server_time();
	fqueue_remove_or_operate_all(&sq, execute_obj_func, p);

	return;
}

/* returns the time in milliseconds to wait before next queue command must be
 * executed or -1 if none is queued */
int squeue_get_next_ms(void)
{
	int ms;
	sq_object_type *obj;

	if (fqueue_get_first(&sq, (void **)&obj) == 0)
	{
		return -1;
	}
	if (cmp_times(fev_get_evtime(), obj->time_to_execute) >= 0)
	{
		/* jobs pending to be executed immediately */
		ms = 0;
	}
	else
	{
		/* execute jobs later */
		ms =  obj->time_to_execute - fev_get_evtime();
	}

	return ms;
}

int squeue_get_next_id(void)
{
	return next_schedule_id;
}


int squeue_get_last_id(void)
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
	FvwmWindow * const fw = exc->w.fw;

	/* get the time to execute */
	n = GetIntegerArguments(action, &action, &ms, 1);
	if (n <= 0)
	{
		fvwm_msg(ERR, "CMD_Schedule",
			 "Requires time to schedule as argument");
		return;
	}
	if (ms < 0)
	{
		ms = 0;
	}
#if 0
	/* eats up way too much cpu if schedule is used excessively */
	current_time = get_server_time();
#else
	/* with this version, scheduled commands may be executed later than
	 * intended. */
	current_time = fev_get_evtime();
#endif
	time = current_time + (Time)ms;
	/* get the job group id to schedule */
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
	/* get the window to operate on */
	if (fw != NULL)
	{
		xw = FW_W(fw);
	}
	else
	{
		xw = None;
	}
	/* schedule the job */
	schedule(xw, action, time, pid);

	return;
}

void CMD_Deschedule(F_CMD_ARGS)
{
	int id;
	int *pid;
	int n;

	/* get the job group id to deschedule */
	n = GetIntegerArguments(action, &action, &id, 1);
	if (n <= 0)
	{
		/* none, use default */
		pid = NULL;
	}
	else
	{
		pid = &id;
	}
	/* deschedule matching jobs */
	deschedule(pid);

	return;
}
