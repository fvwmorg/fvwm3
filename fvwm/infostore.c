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
#include "fvwm.h"
#include "externs.h"
#include "libs/log.h"
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

/* ---------------------------- forward declarations ----------------------- */

static void delete_metainfo(const char *);

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

struct meta_infos meta_info_q;

/* ---------------------------- local functions ---------------------------- */

void insert_metainfo(char *key, char *value)
{
	MetaInfo *mi;
	MetaInfo *mi_new;

	if (TAILQ_EMPTY(&meta_info_q))
		TAILQ_INIT(&meta_info_q);

	TAILQ_FOREACH(mi, &meta_info_q, entry)
	{
		if (StrEquals(mi->key, key))
		{
			/* We already have an entry in the list with that key, so
			 * update the value of it only.
			 */
			free (mi->value);
			CopyString(&mi->value, value);

			return;
		}
	}

	/* It's a new item, add it to the list. */
	mi_new = fxcalloc(1, sizeof *mi_new);
	mi_new->key = fxstrdup(key);
	CopyString(&mi_new->value, value);

	TAILQ_INSERT_TAIL(&meta_info_q, mi_new, entry);

	return;
}

static void delete_metainfo(const char *key)
{
	MetaInfo *mi = NULL, *mi1;

	TAILQ_FOREACH_SAFE(mi, &meta_info_q, entry, mi1)
	{
		if (StrEquals(mi->key, key)) {
			TAILQ_REMOVE(&meta_info_q, mi, entry);

			free(mi->key);
			free(mi->value);
			free(mi);

			return;
		}
	}
}

inline char *get_metainfo_value(const char *key)
{
	MetaInfo *mi;

	TAILQ_FOREACH(mi, &meta_info_q, entry)
	{
		if (StrEquals(mi->key, key))
			return mi->value;
	}

	return NULL;
}

void print_infostore(void)
{
	MetaInfo *mi;

	if (TAILQ_EMPTY(&meta_info_q)) {
		fvwm_debug(__func__,
			   "No items are currently stored in the infostore.\n");
		return;
	}

	fvwm_debug(__func__, "Current items in infostore (key, value):\n\n");
	TAILQ_FOREACH(mi, &meta_info_q, entry)
		fvwm_debug(__func__, "%s\t%s\n", mi->key, mi->value);

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* ---------------------------- builtin commands --------------------------- */
void CMD_InfoStoreAdd(F_CMD_ARGS)
{
	char *key = NULL, *value = NULL;
	char *token;

	if ((token = PeekToken(action, &action)) == NULL)
		goto error;
	key = fxstrdup(token);

	if ((token = PeekToken(action, &action)) == NULL)
		goto error;
	value = fxstrdup(token);

error:
	if (key == NULL || value == NULL) {
		fvwm_debug(__func__, "Bad arguments given.");
		goto out;
	}
	insert_metainfo(key, value);
out:
	free(key);
	free(value);

	return;
}

void CMD_InfoStoreRemove(F_CMD_ARGS)
{
	char *token;

	token = PeekToken(action, &action);

	if (token == NULL)
	{
		fvwm_debug(__func__, "No key given to remove item.");
		return;
	}

	delete_metainfo(token);

	return;
}

void CMD_InfoStoreClear(F_CMD_ARGS)
{
	MetaInfo	*mi, *mi2;

	TAILQ_FOREACH_SAFE(mi, &meta_info_q, entry, mi2)
	{
		TAILQ_REMOVE(&meta_info_q, mi, entry);
		free(mi->key);
		free(mi->value);
		free(mi);
	}
}
