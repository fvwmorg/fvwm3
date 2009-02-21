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
#include <ctype.h>
#include <string.h>

#include "charmap.h"
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

	modmask = mask;
	allmods = safecalloc(1, sizeof(charmap_t));
	for (; table->key !=0; table++)
	{
		char *c;

		c = safecalloc(1, sizeof(table->key));
		strcpy(c, (char *)&table->key);
		*c = toupper(*c);
		if (modmask & table->value)
		{
			modmask |= table->value;
			strcat(allmods, c);
		}
		modmask &= ~table->value;
		free(c);
	}

	return allmods;
}
