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

#include "libs/charmap.h"
#include "libs/wcontext.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* The keys must be in lower case! */
charmap_t win_contexts[] =
{
	{'d', C_EWMH_DESKTOP},
	{'f', C_FRAME},
	{'<', C_F_TOPLEFT},
	{'^', C_F_TOPRIGHT},
	{'>', C_F_BOTTOMRIGHT},
	{'v', C_F_BOTTOMLEFT},
	{'i', C_ICON},
	{'r', C_ROOT},
	{'s', C_SIDEBAR},
	{'[', C_SB_LEFT},
	{']', C_SB_RIGHT},
	{'-', C_SB_TOP},
	{'_', C_SB_BOTTOM},
	{'t', C_TITLE},
	{'w', C_WINDOW},
	{'1', C_L1},
	{'2', C_R1},
	{'3', C_L2},
	{'4', C_R2},
	{'5', C_L3},
	{'6', C_R3},
	{'7', C_L4},
	{'8', C_R4},
	{'9', C_L5},
	{'0', C_R5},
	{'a', C_ALL},
	{0, 0}
};

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* Converts the input string into a mask with bits for the contexts */
int wcontext_string_to_wcontext(char *in_context, int *out_context_mask)
{
	int error;

	error = charmap_string_to_mask(
		out_context_mask, in_context, win_contexts, "bad context");

	return error;
}

char wcontext_wcontext_to_char(win_context_t wcontext)
{
	char c;

	c = charmap_mask_to_char((int)wcontext, win_contexts);

	return c;
}

win_context_t wcontext_merge_border_wcontext(win_context_t wcontext)
{
	if (wcontext & C_FRAME)
	{
		wcontext |= C_FRAME;
	}
	if (wcontext & C_SIDEBAR)
	{
		wcontext |= C_SIDEBAR;
	}

	return wcontext;
}
