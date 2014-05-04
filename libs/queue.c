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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "safemalloc.h"
#include "queue.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

typedef struct fqueue_record
{
	struct fqueue_record *next;
	void *object;
	struct
	{
		unsigned is_scheduled_for_deletion;
		unsigned is_just_created;
	} flags;
} fqueue_record;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* Make newly added items permanent and destroy items scheduled for deletion. */
static void fqueue_cleanup_queue(
	fqueue *fq, destroy_fqueue_object_t destroy_func)
{
	fqueue_record *head;
	fqueue_record *tail;
	fqueue_record *prev;
	fqueue_record *next;
	fqueue_record *rec;

	for (head = NULL, tail = NULL, prev = NULL, rec = fq->first;
	     rec != NULL; rec = next)
	{
		if (rec->flags.is_scheduled_for_deletion)
		{
			/* destroy and skip it */
			next = rec->next;
			if (rec->object != NULL && destroy_func != NULL)
			{
				destroy_func(rec->object);
			}
			if (prev != NULL)
			{
				prev->next = next;
			}
			free(rec);
		}
		else
		{
			rec->flags.is_just_created = 0;
			if (head == NULL)
			{
				head = rec;
			}
			tail = rec;
			prev = rec;
			next = rec->next;
		}
	}
	fq->first = head;
	fq->last = tail;

	return;
}

/* Recursively lock the queue.  While locked, objects are not deleted from the
 * queue but marked for later deletion.  New objects are marked as such and are
 * skipped by the queue functions. */
static void fqueue_lock_queue(fqueue *fq)
{
	fq->lock_level++;

	return;
}

/* Remove one lock level */
static void fqueue_unlock_queue(
	fqueue *fq, destroy_fqueue_object_t destroy_func)
{
	switch (fq->lock_level)
	{
	case 0:
		/* bug */
		break;
	case 1:
		if (fq->flags.is_dirty)
		{
			fqueue_cleanup_queue(fq, destroy_func);
		}
		/* fall through */
	default:
		fq->lock_level--;
		return;
	}

	return;
}

/* Chack and possibly execute the action associated with a queue object.
 * Schedule the object for deletion if it was executed. */
static void fqueue_operate(
	fqueue *fq, fqueue_record *rec,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	void *operate_args)
{
	if (rec == NULL || rec->flags.is_scheduled_for_deletion)
	{
		return;
	}
	if (check_func == NULL || check_func(rec->object, operate_args) == 1)
	{
		if (operate_func != NULL)
		{
			operate_func(rec->object, operate_args);
		}
		rec->flags.is_scheduled_for_deletion = 1;
		fq->flags.is_dirty = 1;
	}

	return;
}

/* ---------------------------- builtin commands --------------------------- */

/*
 * Basic queue management
 */

void fqueue_init(fqueue *fq)
{
	memset(fq, 0, sizeof(*fq));

	return;
}

unsigned int fqueue_get_length(fqueue *fq)
{
	unsigned int len;
	fqueue_record *t;

	for (t = fq->first, len = 0; t != NULL; t = t->next)
	{
		if (!t->flags.is_scheduled_for_deletion)
		{
			len++;
		}
	}

	return len;
}

/*
 * Add record to queue
 */

void fqueue_add_at_front(
	fqueue *fq, void *object)
{
	fqueue_record *rec;

	rec = xcalloc(1, sizeof *rec);
	rec->object = object;
	rec->next = fq->first;
	if (fq->lock_level > 0)
	{
		rec->flags.is_just_created = 1;
		fq->flags.is_dirty = 1;
	}
	fq->first = rec;

	return;
}

void fqueue_add_at_end(
	fqueue *fq, void *object)
{
	fqueue_record *rec;

	rec = xcalloc(1, sizeof *rec);
	rec->object = object;
	if (fq->lock_level > 0)
	{
		rec->flags.is_just_created = 1;
		fq->flags.is_dirty = 1;
	}
	if (fq->first == NULL)
	{
		fq->first = rec;
	}
	else
	{
		fq->last->next = rec;
	}
	fq->last = rec;
	rec->next = NULL;

	return;
}

void fqueue_add_inside(
	fqueue *fq, void *object, cmp_objects_t cmp_objects, void *cmp_args)
{
	fqueue_record *rec;
	fqueue_record *p;
	fqueue_record *t;

	rec = xcalloc(1, sizeof *rec);
	rec->object = object;
	if (fq->lock_level > 0)
	{
		rec->flags.is_just_created = 1;
		fq->flags.is_dirty = 1;
	}

	/* search place to insert record */
	for (p = NULL, t = fq->first;
	     t != NULL && cmp_objects(object, t->object, cmp_args) >= 0;
	     p = t, t = t->next)
	{
		/* nothing to do here */
	}
	/* insert record */
	if (p == NULL)
	{
		/* insert at start */
		rec->next = fq->first;
		fq->first = rec;
	}
	else
	{
		/* insert after p */
		rec->next = p->next;
		p->next = rec;
	}
	if (t == NULL)
	{
		fq->last = rec;
	}

	return;
}

/*
 * Fetch queue objects
 */

/* Returns the object of the first queue record throuch *ret_object.  Returns
 * 0 if the queue is empty and 1 otherwise. */
int fqueue_get_first(
	fqueue *fq, void **ret_object)
{
	fqueue_record *rec;

	for (rec = fq->first;
	     rec != NULL && rec->flags.is_scheduled_for_deletion;
	     rec = rec->next)
	{
		/* nothing */
	}
	if (rec == NULL)
	{
		return 0;
	}
	*ret_object = rec->object;

	return 1;
}

/*
 * Operate on queue objects and possibly remove them from the queue
 */

/* Runs the operate_func on the first record in the queue.  If that function
 * is NULL or returns 1, the record is removed from the queue.  The object of
 * the queue record must have been freed in operate_func. */
void fqueue_remove_or_operate_from_front(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args)
{
	fqueue_lock_queue(fq);
	fqueue_operate(fq, fq->first, check_func, operate_func, operate_args);
	fqueue_unlock_queue(fq, destroy_func);

	return;
}

/* Same as above but operates on last record in queue */
void fqueue_remove_or_operate_from_end(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args)
{
	fqueue_lock_queue(fq);
	fqueue_operate( fq, fq->last, check_func, operate_func, operate_args);
	fqueue_unlock_queue(fq, destroy_func);

	return;
}

/* Same as above but operates on all records in the queue. */
void fqueue_remove_or_operate_all(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args)
{
	fqueue_record *t;

	if (fq->first == NULL)
	{
		return;
	}
	fqueue_lock_queue(fq);
	/* search record(s) to remove */
	for (t = fq->first; t != NULL; t = t->next)
	{
		if (t->flags.is_just_created ||
		    t->flags.is_scheduled_for_deletion)
		{
			/* skip */
			continue;
		}
		fqueue_operate(fq, t, check_func, operate_func, operate_args);
	}
	fqueue_unlock_queue(fq, destroy_func);

	return;
}
