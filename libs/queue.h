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

#ifndef QUEUE_H
#define QUEUE_H

/*
 * type definitions
 */

typedef struct
{
	struct fqueue_record *first;
	struct fqueue_record *last;
} fqueue;
#define FQUEUE_INIT { NULL, NULL }



typedef int (*operate_fqueue_object_type)(void *object, void *operate_args);
typedef int (*cmp_objects_type)(void *object1, void *object2, void *args);

/*
 * Basic queue management
 */

void fqueue_init(fqueue *fq);
unsigned int fqueue_get_length(fqueue *fq);
#define FQUEUE_IS_EMPTY(fq) ((fq)->first == NULL)

/*
 * Add record to queue
 */

void fqueue_add_at_front(fqueue *fq, void *object);
void fqueue_add_at_end(fqueue *fq, void *object);
void fqueue_add_inside(
	fqueue *fq, void *object, cmp_objects_type cmp_objects, void *cmp_args);

/*
 * Fetch queue objects
 */

int fqueue_get_first(fqueue *fq, void **ret_object);

/*
 * Operate on queue objects and possibly remove them from the queue
 */

void fqueue_remove_or_operate_from_front(
	fqueue *fq, operate_fqueue_object_type operate_func,
	void *operate_args);
void fqueue_remove_or_operate_from_end(
	fqueue *fq, operate_fqueue_object_type operate_func,
	void *operate_args);
void fqueue_remove_or_operate_all(
	fqueue *fq, operate_fqueue_object_type operate_func,
	void *operate_args);

#endif /* QUEUE_H */
