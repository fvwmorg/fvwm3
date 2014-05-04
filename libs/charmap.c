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
#include <ctype.h>
#include <string.h>

#include "charmap.h"
#include "wcontext.h"
#include "safemalloc.h"

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

/* Turns a  string context of context or modifier values into an array of
 * true/false values (bits). */
int charmap_string_to_mask(
	int *ret, const char *string, charmap_t *table, char *errstring)
{
	int len = strlen(string);
	int error = 0;
	int i;

	*ret = 0;
	for (i = 0; i < len; ++i)
	{
		int found_match;
		int j;
		char c;

		c = tolower(string[i]);
		for (j = 0, found_match = 0; table[j].key != 0; j++)
		{
			if (table[j].key == c)
			{
				*ret |= table[j].value;
				found_match = 1;
				break;
			}
		}
		if (!found_match)
		{
			fputs("charmap_string_to_mask: ", stderr);
			if (errstring != NULL)
			{
				fputs(errstring, stderr);
			}
			fputc(' ', stderr);
			fputc(c, stderr);
			fputc('\n', stderr);
			error = 1;
		}
	}

	return error;
}

/* Reverse function of above.  Returns zero if no matching mask is found in the
 * table. */
char charmap_mask_to_char(int mask, charmap_t *table)
{
	char c;

	for (c = 0; table->key != 0; table++)
	{
		if (mask == table->value)
		{
			c = table->key;
			break;
		}
	}

	return c;
}

/* Used from "PrintInfo Bindings". */
char *charmap_table_to_string(int mask, charmap_t *table)
{
	char *allmods;
	int modmask;
	char c[2];

	c[1] = 0;
	modmask = mask;
	allmods = xmalloc(sizeof(table->value) * 8 + 1);
	*allmods = 0;
	for (; table->key !=0; table++)
	{
		c[0] = toupper(table->key);

		/* Don't explicitly match "A" for any context as doing so
		 * means we never see the individual bindings.  Incremental
		 * matching here for AnyContext is disasterous.*/
		if ((modmask & table->value) &&
			(table->value != C_ALL))
		{
			/* incremental match */
			strcat(allmods, c);
			modmask &= ~table->value;
		}
		else if (mask == table->value)
		{
			/* exact match */
			strcpy(allmods, c);
			break;
		}
	}

	return allmods;
}
