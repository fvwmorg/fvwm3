/* -*-c-*- */

#ifndef QUEUE_H
#define QUEUE_H

/*
 * type definitions
 */

typedef struct
{
	/* do not read or write any members of this structure outside queue.c!
	 */
	struct fqueue_record *first;
	struct fqueue_record *last;
	unsigned int lock_level;
	struct
	{
		unsigned is_dirty : 1;
	} flags;
} fqueue;
#define FQUEUE_INIT { NULL, NULL, 0 }



typedef int (*check_fqueue_object_t)(void *object, void *operate_args);
typedef void (*operate_fqueue_object_t)(void *object, void *operate_args);
typedef int (*cmp_objects_t)(void *object1, void *object2, void *args);
typedef void (*destroy_fqueue_object_t)(void *object);

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
	fqueue *fq, void *object, cmp_objects_t cmp_objects, void *cmp_args);

/*
 * Fetch queue objects
 */

int fqueue_get_first(fqueue *fq, void **ret_object);

/*
 * Operate on queue objects and possibly remove them from the queue
 */

void fqueue_remove_or_operate_from_front(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args);
void fqueue_remove_or_operate_from_end(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args);
void fqueue_remove_or_operate_all(
	fqueue *fq,
	check_fqueue_object_t check_func,
	operate_fqueue_object_t operate_func,
	destroy_fqueue_object_t destroy_func,
	void *operate_args);

#endif /* QUEUE_H */
