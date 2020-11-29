/* -*-c-*- */

#ifndef FVWMLIB_FLIST_H
#define FVWMLIB_FLIST_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct _flist
{
	void *object;
	struct _flist *next;
	struct _flist *prev;
} flist;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

flist *flist_append_obj(flist *list, void *object);
flist *flist_prepend_obj(flist *list, void *object);
flist *flist_insert_obj(flist *list, void *object, int position);
flist *flist_remove_obj(flist *list, void *object);
flist *flist_free_list(flist *list);

#endif /* FVWMLIB_FLIST_H */
