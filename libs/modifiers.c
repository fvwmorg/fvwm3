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

#include "libs/charmap.h"
#include "libs/modifiers.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* The keys must be in lower case! */
charmap_t key_modifiers[] =
{
	{'s', ShiftMask},
	{'c', ControlMask},
	{'l', LockMask},
	{'m', Mod1Mask},
	{'1', Mod1Mask},
	{'2', Mod2Mask},
	{'3', Mod3Mask},
	{'4', Mod4Mask},
	{'5', Mod5Mask},
	{'a', AnyModifier},
	{'n', 0},
	{0, 0}
};

/* Table to translate a modifier map index to a modifier that we define that
 * generates that index.  This mapping can be chosen by each client, but the
 * settings below try to emulate the usual terminal behaviour. */
unsigned int modifier_mapindex_to_mask[8] =
{
	ShiftMask,
	Mod3Mask, /* Alt Gr */
	Mod3Mask | ShiftMask,
	/* Just guessing below here - LockMask is not used anywhere*/
	ControlMask,
	Mod1Mask, /* Alt/Meta on XFree86 */
	Mod2Mask, /* Num lock on XFree86 */
	Mod4Mask,
	Mod5Mask, /* Scroll lock on XFree86 */
};

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* Converts the input string into a mask with bits for the modifiers */
int modifiers_string_to_modmask(char *in_modifiers, int *out_modifier_mask)
{
	int error;

	error = charmap_string_to_mask(
		out_modifier_mask, in_modifiers, key_modifiers,
		"bad modifier");

	return error;
}

