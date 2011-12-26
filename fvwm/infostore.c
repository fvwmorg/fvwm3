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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>
#include "fvwm.h"
#include "externs.h"
#include "libs/FGettext.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "infostore.h"
#include "misc.h"
#include "functions.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

static MetaInfo *mi_store;

/* ---------------------------- forward declarations ----------------------- */

static void delete_metainfo(const char *);

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

MetaInfo *new_metainfo(void)
{
	if (mi_store == NULL)
	{
		/* Initialise the main store. */
		mi_store = (MetaInfo *)safemalloc(sizeof(MetaInfo));
		memset(&mi_store, '\0', sizeof(MetaInfo));
	}

	MetaInfo *mi = (MetaInfo *)safemalloc(sizeof(MetaInfo));
	mi->key = NULL;
	mi->value = NULL;
	mi->next = NULL;

	return mi;
}

void insert_metainfo(char *key, char *value)
{
	MetaInfo *mi;
	MetaInfo *mi_new = new_metainfo();

	for (mi = mi_store; mi; mi = mi->next)
	{
		if (StrEquals(mi->key, key))
		{
			/* We already have an entry in the list with that key, so
			 * update the value of it only.
			 */
			CopyString(&mi->value, value);
			free (mi_new);

			return;
		}
	}

	/* It's a new item, add it to the list. */
	mi_new->key = key;
	mi_new->value = value;

	mi_new->next = mi_store;
	mi_store = mi_new;

	return;
}

static void delete_metainfo(const char *key)
{
	MetaInfo *mi_current, *mi_prev;
	mi_prev = NULL;

	for(mi_current = mi_store; mi_current != NULL;
		mi_prev = mi_current, mi_current = mi_current->next)
	{
		if (StrEquals(mi_current->key, key)) {
			if (mi_prev == NULL)
				mi_store = mi_current->next;
			else
				mi_prev->next = mi_current->next;

			free(mi_current->key);
			free(mi_current->value);
			free(mi_current);

			break;
		}
	}

	return;
}

inline char *get_metainfo_value(const char *key)
{
	MetaInfo *mi_current;

	for(mi_current = mi_store; mi_current; mi_current = mi_current->next)
	{
		if (StrEquals(mi_current->key, key))
			return mi_current->value;
	}

	return NULL;
}

/* ---------------------------- interface functions ------------------------ */

/* ---------------------------- builtin commands --------------------------- */
void CMD_InfoStore(F_CMD_ARGS)
{
	char *key, *value;
	char *token;

	/* Accessing the values of this struct happens with the standard FVWM
	 * expansion code (see expand.c); therefore, we have an optional flag,
	 * "-d" to delete the entry with the specified key.
	 */
	/* FIXME:  TA:  This would disallow using "-d" as a key.  Might need
	 *         to be a separate command after all.
	 */
	token = PeekToken(action, &action);
	key = value = NULL;

	if (StrEquals(token, "-d"))
	{
		/* Get the next item. */
		token = PeekToken(action, &action);

		if (token && (strcmp(token, "") != 0))
			key = strdup(token);

		if (!key)
		{
			fvwm_msg(ERR, "CMD_InfoStore",
				"No key given to -d to delete.");
			return;
		}

		delete_metainfo(key);
		free(key);

		return;
	}

	if (token)
		key = strdup(token);

	token = PeekToken(action, &action);

	if (token)
		value = strdup(token);

	if (!key && !value)
	{
		fvwm_msg(ERR, "CMD_InfoStore", "Bad arguments given.");
		return;
	}

	insert_metainfo(key, value);

	return;
}
