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

/* code for parsing the fvwm style command */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>

#include "safemalloc.h"
#include "flist.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

flist *flist_append_obj(flist *list, void *object)
{
	flist *new = xmalloc(sizeof(flist));
	flist *tl = list;

	new->object = object;
	new->next = NULL;
	new->prev = NULL;

	if (list == NULL)
	{
		return new;
	}
	while(tl->next)
	{
		tl = tl->next;
	}
	tl->next = new;
	new->prev = tl;

	return list;
}

flist *flist_prepend_obj(flist *list, void *object)
{
	flist *new = xmalloc(sizeof(flist));

	new->object = object;
	new->next = NULL;
	new->prev = NULL;

	if (list == NULL)
	{
		return new;
	}

	if (list->prev)
	{
		list->prev->next = new;
		new->prev = list->prev;
	}
	list->prev = new;
	new->next = list;

	return new;
}

flist *flist_insert_obj(flist *list, void *object, int position)
{
	flist *new;
	flist *tl;

	if (position < 0)
	{
		return flist_append_obj(list, object);
	}
	if (position == 0)
	{
		return flist_prepend_obj(list, object);
	}
	tl = list;
	while(tl && position-- > 0)
	{
		tl = tl->next;
	}

	if (!tl)
	{
		return flist_append_obj(list, object);
	}

	new = xmalloc(sizeof(flist));;
	new->object = object;
	new->prev = NULL;

	if (tl->prev)
	{
		tl->prev->next = new;
		new->prev = tl->prev;
	}
	new->next = tl;
	tl->prev = new;

	if (tl == list)
	{
		return new;
	}

	return list;
}

flist *flist_remove_obj(flist *list, void *object)
{
	flist *tl = list;

	while (tl && tl->object != object)
	{
		tl = tl->next;
	}

	if (tl == NULL)
	{
		return NULL;
	}

	if (tl->prev)
	{
		tl->prev->next = tl->next;
	}
	if (tl->next)
	{
		tl->next->prev = tl->prev;
	}
	if (list == tl)
	{
		list = list->next;
	}
	free(tl);

	return list;
}

flist *flist_free_list(flist *list)
{
	flist *tl;

	while (list)
	{
		tl = list;
		list = list->next;
		free(tl);
	}
	return NULL;
}
