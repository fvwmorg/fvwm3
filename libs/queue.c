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

#include "safemalloc.h"
#include "queue.h"

typedef struct fqueue_record
{
	struct fqueue_record *next;
	void *object;
} fqueue_record;

/*
 * Basic queue management
 */

void fqueue_init(fqueue *fq)
{
	fq->first = NULL;
	fq->last = NULL;

	return;
}

unsigned int fqueue_get_length(fqueue *fq)
{
	unsigned int len;
	fqueue_record *t;

	for (t = fq->first, len = 0; t != NULL; len++, t = t->next)
	{
		/* nothing to do here */
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

	rec = (fqueue_record *)safemalloc(sizeof(fqueue_record));
	rec->object = object;
	rec->next = fq->first;
	fq->first = rec;

	return;
}

void fqueue_add_at_end(
	fqueue *fq, void *object)
{
	fqueue_record *rec;

	rec = (fqueue_record *)safemalloc(sizeof(fqueue_record));
	rec->object = object;
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

	rec = (fqueue_record *)safemalloc(sizeof(fqueue_record));
	rec->object = object;

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
	if (fq->first == NULL)
	{
		return 0;
	}
	*ret_object = fq->first->object;

	return 1;
}

/*
 * Operate on queue objects and possibly remove them from the queue
 */

/* Runs the operate_func on the first record in the queue.  If that function
 * is NULL or returns 1, the record is removed from the queue.  The object of
 * the queue record must have been freed in operate_func. */
void fqueue_remove_or_operate_from_front(
	fqueue *fq, check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func, void *operate_args)
{
	fqueue_record *t;

	if (fq->first == NULL)
	{
		return;
	}
	/* remove object from queue */
	if (check_func == NULL ||
	    check_func(fq->first->object, operate_args) == 1)
	{
		/* must free the record */
		t = fq->first;
		fq->first = fq->first->next;
		t->next = NULL;
		if (fq->first == NULL)
		{
			fq->last = NULL;
		}
		else if (fq->first->next == NULL)
		{
			fq->last = fq->first;
		}
		if (operate_func != NULL)
		{
			operate_func(t->object, operate_args);
		}
		free(t);
	}

	return;
}

/* Same as above but operates on last record in queue */
void fqueue_remove_or_operate_from_end(
	fqueue *fq, check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func, void *operate_args)
{
	fqueue_record *l;
	fqueue_record *t;

	if (fq->first == NULL)
	{
		return;
	}
	if (check_func == NULL ||
	    check_func(fq->last->object, operate_args) == 1)
	{
		/* must free this record */
		l = fq->last;
		/* seek next to last item in queue */
		for (t = fq->first; t->next != NULL; t = t->next)
		{
			/* nothing to do here */
		}
		if (t->next == NULL)
		{
			/* only one item in queue */
			fq->first = NULL;
			fq->last = NULL;
		}
		else
		{
			t->next = NULL;
			fq->last = t;
		}
		if (operate_func != NULL)
		{
			operate_func(l->object, operate_args);
		}
		free(l);
	}

	return;
}

/* Same as above but operates on all records in the queue. */
void fqueue_remove_or_operate_all(
	fqueue *fq, check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func, void *operate_args)
{
	fqueue_record *t;
	fqueue_record *n;
	fqueue_record *l;

	if (fq->first == NULL)
	{
		return;
	}

	/* search record(s) to remove */
	for (n = NULL, l = NULL, t = fq->first; t != NULL; t = n)
	{
		if (check_func == NULL ||
		    check_func(t->object, operate_args) == 1)
		{
			/* must free record */
			if (l == NULL)
			{
				fq->first = t->next;
			}
			else
			{
				l->next = t->next;
			}
			n = t->next;
			t->next = NULL;
			if (operate_func != NULL)
			{
				operate_func(t->object, operate_args);
			}
			free(t);
		}
		else
		{
			/* keep it */
			l = t;
			n = t->next;
		}
	}
	fq->last = l;

	return;
}
