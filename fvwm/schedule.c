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

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/queue.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/Parse.h"
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
	int period; /* in milliseconds */
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

static int check_deschedule_obj_func(void *object, void *args)
{
	sq_object_type *obj = object;

	return (obj->id == *(int *)args);
}

static void destroy_obj_func(void *object)
{
	sq_object_type *obj = object;

	if (obj->command != NULL)
	{
		free(obj->command);
	}
	free(obj);

	return;
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
	fqueue_remove_or_operate_all(
		&sq, check_deschedule_obj_func, NULL, destroy_obj_func,
		(void *)&id);

	return;
}

static void schedule(
	Window window, char *command, Time time_to_execute, int *pid,
	int period)
{
	sq_object_type *new_obj;

	if (command == NULL || *command == 0)
	{
		return;
	}
	/* create the new object */
	new_obj = xcalloc(1, sizeof(sq_object_type));
	new_obj->window = window;
	new_obj->command = xstrdup(command);
	new_obj->time_to_execute = time_to_execute;
	new_obj->period = period; /* 0 if this is not a periodic command */
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

static int check_execute_obj_func(void *object, void *args)
{
	sq_object_type *obj = object;
	Time *ptime = (Time *)args;

	return (cmp_times(*ptime, obj->time_to_execute) >= 0);
}

static void execute_obj_func(void *object, void *args)
{
	sq_object_type *obj = object;

	if (obj->command != NULL)
	{
		/* execute the command */
		const exec_context_t *exc;
		exec_context_changes_t ecc;
		exec_context_change_mask_t mask = ECC_TYPE | ECC_WCONTEXT;

		ecc.type = EXCT_SCHEDULE;
		ecc.w.wcontext = C_ROOT;
		if (XFindContext(
			    dpy, obj->window, FvwmContext,
			    (caddr_t *)&ecc.w.fw) != XCNOENT)
		{
			ecc.w.wcontext = C_WINDOW;
			mask |= ECC_FW;
		}
		exc = exc_create_context(&ecc, mask);
		execute_function(NULL, exc, obj->command, 0);
		exc_destroy_context(exc);
	}
	if (obj->period > 0)
	{
		/* This is a periodic function, so reschedule it. */
		sq_object_type *new_obj = xmalloc(sizeof(sq_object_type));
		memcpy(new_obj, obj, sizeof(sq_object_type));
		obj->command = NULL; /* new_obj->command now points to cmd. */
		new_obj->time_to_execute = fev_get_evtime() + new_obj->period;
		/* insert into schedule queue */
		fqueue_add_inside(&sq, new_obj, cmp_object_time, NULL);
	}
	XFlush(dpy);

	return;
}

/* executes all scheduled commands that are due for execution */
void squeue_execute(void)
{
	Time current_time;

	if (FQUEUE_IS_EMPTY(&sq))
	{
		return;
	}
	current_time = get_server_time();
	fqueue_remove_or_operate_all(
		&sq, check_execute_obj_func, execute_obj_func,
		destroy_obj_func, &current_time);

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
	char *token;
	char *next;
	int ms;
	int id;
	int *pid;
	int n;
	FvwmWindow * const fw = exc->w.fw;
	Bool is_periodic = False;

	token = PeekToken(action, &next);
	if (token && strcasecmp(token, "periodic") == 0)
	{
		is_periodic = True;
		action = next;
	}
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
	/* with this version, scheduled commands may be executed earlier than
	 * intended. */
	current_time = fev_get_evtime();
#endif
	time = current_time + (Time)ms;
	/* get the job group id to schedule */
	n = GetIntegerArgumentsAnyBase(action, &taction, &id, 1);
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
	schedule(xw, action, time, pid, (is_periodic == True ? ms : 0));

	return;
}

void CMD_Deschedule(F_CMD_ARGS)
{
	int id;
	int *pid;
	int n;

	/* get the job group id to deschedule */
	n = GetIntegerArgumentsAnyBase(action, &action, &id, 1);
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
