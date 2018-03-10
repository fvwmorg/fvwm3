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

#include <X11/Xlib.h>
#include "fvwmlib.h"
#include "Parse.h"
#include "Strings.h"
#include "gravity.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

struct _gravity_offset
{
	int x, y;
};

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

#define STRINGS_PER_DIR 7
static char *gravity_dir_optlist[] = {
	"-", "N",  "North",     "Top",         "t", "Up",         "u",
	"]", "E",  "East",      "Right",       "r", "Right",      "r",
	"_", "S",  "South",     "Bottom",      "b", "Down",       "d",
	"[", "W",  "West",      "Left",        "l", "Left",       "l",
	"^", "NE", "NorthEast", "TopRight",    "tr", "UpRight",   "ur",
	">", "SE", "SouthEast", "BottomRight", "br", "DownRight", "dr",
	"v", "SW", "SouthWest", "BottomLeft",  "bl", "DownLeft",  "dl",
	"<", "NW", "NorthWest", "TopLeft",     "tl", "UpLeft",    "ul",
	".", "C",  "Center",    "Centre",      NULL, NULL,        NULL,
	NULL
};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* map gravity to (x,y) offset signs for adding to x and y when window is
 * mapped to get proper placement. */
void gravity_get_offsets(int grav, int *xp,int *yp)
{
	static struct _gravity_offset gravity_offsets[11] = {
		{  0,  0 },     /* ForgetGravity */
		{ -1, -1 },     /* NorthWestGravity */
		{  0, -1 },     /* NorthGravity */
		{  1, -1 },     /* NorthEastGravity */
		{ -1,  0 },     /* WestGravity */
		{  0,  0 },     /* CenterGravity */
		{  1,  0 },     /* EastGravity */
		{ -1,  1 },     /* SouthWestGravity */
		{  0,  1 },     /* SouthGravity */
		{  1,  1 },     /* SouthEastGravity */
		{  0,  0 },     /* StaticGravity */
	};

	if (grav < ForgetGravity || grav > StaticGravity)
	{
		*xp = *yp = 0;
	}
	else
	{
		*xp = (int)gravity_offsets[grav].x;
		*yp = (int)gravity_offsets[grav].y;
	}

	return;
}

/* Move a rectangle while taking gravity into account. */
void gravity_move(int gravity, rectangle *rect, int xdiff, int ydiff)
{
	int xoff;
	int yoff;

	gravity_get_offsets(gravity, &xoff, &yoff);
	rect->x -= xoff * xdiff;
	rect->y -= yoff * ydiff;

	return;
}

/* Resize rectangle while taking gravity into account. */
void gravity_resize(int gravity, rectangle *rect, int wdiff, int hdiff)
{
	int xoff;
	int yoff;

	gravity_get_offsets(gravity, &xoff, &yoff);
	rect->x -= (wdiff * (xoff + 1)) / 2;
	rect->width += wdiff;
	rect->y -= (hdiff * (yoff + 1)) / 2;
	rect->height += hdiff;

	return;
}

/* Moves a child rectangle taking its gravity into accout as if the parent
 * rectangle was moved and resized. */
void gravity_move_resize_parent_child(
	int child_gravity, rectangle *parent_diff_r, rectangle *child_r)
{
	int xoff;
	int yoff;

	gravity_get_offsets(child_gravity, &xoff, &yoff);
	child_r->x -= xoff * parent_diff_r->x;
	child_r->y -= yoff * parent_diff_r->y;
	child_r->x += ((xoff + 1) * parent_diff_r->width) / 2;
	child_r->y += ((yoff + 1) * parent_diff_r->height) / 2;

	return;
}

direction_t gravity_grav_to_dir(
	int grav)
{
	switch (grav)
	{
	case NorthWestGravity:
		return DIR_NW;
	case NorthGravity:
		return DIR_N;
	case NorthEastGravity:
		return DIR_NE;
	case WestGravity:
		return DIR_W;
	case CenterGravity:
		return DIR_NONE;
	case EastGravity:
		return DIR_E;
	case SouthWestGravity:
		return DIR_SW;
	case SouthGravity:
		return DIR_S;
	case SouthEastGravity:
		return DIR_SE;
	case ForgetGravity:
	case StaticGravity:
	default:
		return DIR_NONE;
	}
}

int gravity_dir_to_grav(
	direction_t dir)
{
	switch (dir)
	{
	case DIR_N:
		return NorthGravity;
	case DIR_E:
		return EastGravity;
	case DIR_S:
		return SouthGravity;
	case DIR_W:
		return WestGravity;
	case DIR_NE:
		return NorthEastGravity;
	case DIR_SE:
		return SouthEastGravity;
	case DIR_SW:
		return SouthWestGravity;
	case DIR_NW:
		return NorthWestGravity;
	case DIR_NONE:
	default:
		return ForgetGravity;
	}
}

int gravity_combine_xy_grav(
	int grav_x, int grav_y)
{
	switch (grav_x)
	{
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
		grav_x = WestGravity;
		break;
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		grav_x = EastGravity;
		break;
	default:
		grav_x = CenterGravity;
		break;
	}
	switch (grav_y)
	{
	case NorthWestGravity:
	case NorthGravity:
	case NorthEastGravity:
		grav_y = NorthGravity;
		break;
	case SouthWestGravity:
	case SouthGravity:
	case SouthEastGravity:
		grav_y = SouthGravity;
		break;
	default:
		grav_y = CenterGravity;
		break;
	}
	if (grav_x == CenterGravity)
	{
		return grav_y;
	}
	switch (grav_y)
	{
	case NorthGravity:
		return (grav_x == WestGravity) ?
			NorthWestGravity : NorthEastGravity;
	case SouthGravity:
		return (grav_x == WestGravity) ?
			SouthWestGravity : SouthEastGravity;
	case CenterGravity:
	default:
		return grav_x;
	}

	return 0;
}

void gravity_split_xy_grav(
	int *ret_grav_x, int *ret_grav_y, int in_grav)
{
	switch (in_grav)
	{
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
		*ret_grav_x = WestGravity;
		break;
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		*ret_grav_x = EastGravity;
		break;
	case NorthGravity:
	case CenterGravity:
	case SouthGravity:
	case ForgetGravity:
	case StaticGravity:
	default:
		*ret_grav_x = CenterGravity;
		break;
	}
	switch (in_grav)
	{
	case NorthWestGravity:
	case NorthGravity:
	case NorthEastGravity:
		*ret_grav_y = NorthGravity;
		break;
	case SouthWestGravity:
	case SouthGravity:
	case SouthEastGravity:
		*ret_grav_y = SouthGravity;
		break;
	case WestGravity:
	case CenterGravity:
	case EastGravity:
	case ForgetGravity:
	case StaticGravity:
	default:
		*ret_grav_y = CenterGravity;
		break;
	}
}

int gravity_combine_xy_dir(
	int dir_x, int dir_y)
{
	switch (dir_x)
	{
	case DIR_W:
	case DIR_NW:
	case DIR_SW:
		dir_x = DIR_W;
		break;
	case DIR_E:
	case DIR_NE:
	case DIR_SE:
		dir_x = DIR_E;
		break;
	default:
		dir_x = DIR_NONE;
		break;
	}
	switch (dir_y)
	{
	case DIR_N:
	case DIR_NW:
	case DIR_NE:
		dir_y = DIR_N;
		break;
	case DIR_S:
	case DIR_SW:
	case DIR_SE:
		dir_y = DIR_S;
		break;
	default:
		dir_y = DIR_NONE;
		break;
	}
	if (dir_x == DIR_NONE)
	{
		return dir_y;
	}
	switch (dir_y)
	{
	case DIR_N:
		return (dir_x == DIR_W) ? DIR_NW : DIR_NE;
	case DIR_S:
		return (dir_x == DIR_W) ? DIR_SW : DIR_SE;
	case DIR_NONE:
	default:
		return dir_x;
	}
}

void gravity_split_xy_dir(
	int *ret_dir_x, int *ret_dir_y, int in_dir)
{
	switch (in_dir)
	{
	case DIR_W:
	case DIR_SW:
	case DIR_NW:
		*ret_dir_x = DIR_W;
		break;
	case DIR_E:
	case DIR_NE:
	case DIR_SE:
		*ret_dir_x = DIR_E;
		break;
	case DIR_N:
	case DIR_S:
	case DIR_NONE:
	default:
		*ret_dir_x = DIR_NONE;
		break;
	}
	switch (in_dir)
	{
	case DIR_N:
	case DIR_NW:
	case DIR_NE:
		*ret_dir_y = DIR_N;
		break;
	case DIR_S:
	case DIR_SW:
	case DIR_SE:
		*ret_dir_y = DIR_S;
		break;
	case DIR_W:
	case DIR_E:
	case DIR_NONE:
	default:
		*ret_dir_y = DIR_NONE;
		break;
	}
}

static inline int __gravity_override_one_axis(int dir_orig, int dir_mod)
{
	int ret_dir;

	if (dir_mod == DIR_NONE)
	{
		ret_dir = dir_orig;
	}
	else
	{
		ret_dir = dir_mod;
	}

	return ret_dir;
}

int gravity_override_dir(
	int dir_orig, int dir_mod)
{
	int ret_dir;
	int ret_x;
	int ret_y;
	int orig_x;
	int orig_y;
	int mod_x;
	int mod_y;

	gravity_split_xy_dir(&orig_x, &orig_y, dir_orig);
	gravity_split_xy_dir(&mod_x, &mod_y, dir_mod);
	ret_x = __gravity_override_one_axis(orig_x, mod_x);
	ret_y = __gravity_override_one_axis(orig_y, mod_y);
	ret_dir = gravity_combine_xy_dir(ret_x, ret_y);

	return ret_dir;
}

int gravity_dir_to_sign_one_axis(
	direction_t dir)
{
	switch (dir)
	{
	case DIR_N:
	case DIR_W:
		return -1;
	case DIR_S:
	case DIR_E:
		return 1;
	default:
		return 0;
	}
}

/* Parses the next token in action and returns
 *
 *   0 if it is N, North, Top or Up
 *   1 if it is E, East, Right or Right
 *   2 if it is S, South, Bottom or Down
 *   3 if it is E, West, Left or Left
 *   4 if it is NE, NorthEast, TopRight or UpRight
 *   5 if it is SE, SouthEast, BottomRight or DownRight
 *   6 if it is SW, SouthWest, BottomLeft or DownLeft
 *   7 if it is NW, NorthWest, TopLeft or UpLeft
 *   8 if it is C, Center or Centre
 *   default_ret if no string matches.
 *
 * A pointer to the first character in action behind the token is returned
 * through ret_action in this case. ret_action may be NULL. If the token
 * matches none of these strings the default_ret value is returned and the
 * action itself is passed back in ret_action. */
direction_t gravity_parse_dir_argument(
	char *action, char **ret_action, direction_t default_ret)
{
	int index;
	int rc;
	char *next;

	next = GetNextTokenIndex(action, gravity_dir_optlist, 0, &index);
	if (index == -1)
	{
		/* nothing selected, use default and don't modify action */
		rc = default_ret;
		next = action;
	}
	else
	{
		rc = index / STRINGS_PER_DIR;
	}
	if (ret_action)
	{
		*ret_action = next;
	}

	return (direction_t)rc;
}

char *gravity_dir_to_string(direction_t dir, char *default_str)
{
	char *str = NULL;
	int d = dir * STRINGS_PER_DIR;

	if (d >= sizeof(gravity_dir_optlist)/sizeof(gravity_dir_optlist[0]))
	{
		return default_str;
	}
	str = gravity_dir_optlist[d];

	if (str == NULL)
	{
		return default_str;
	}

	return str;
}

multi_direction_t gravity_parse_multi_dir_argument(
	char *action, char **ret_action)
{
	int rc = MULTI_DIR_NONE;
	char *token, *str;
	direction_t dir = gravity_parse_dir_argument(action, ret_action, -1);

	if (dir != -1)
	{
		rc = (1 << dir);
	}
	else
	{
		token = PeekToken(action, &str);
		if (StrEquals(token, "all"))
		{
			rc = MULTI_DIR_ALL;
			*ret_action = str;
		}
		else
		{
			rc = MULTI_DIR_NONE;
		}
	}

	return (multi_direction_t)rc;
}

void gravity_get_next_multi_dir(int dir_set, multi_direction_t *dir)
{
	if (*dir == MULTI_DIR_NONE)
	{
		*dir = MULTI_DIR_FIRST;
		if (dir_set & *dir)
		{
			return;
		}
	}
	while(*dir != MULTI_DIR_LAST)
	{
		*dir = (*dir << 1);
		if (dir_set & *dir)
		{
			return;
		}
	}
	*dir = MULTI_DIR_NONE;

	return;
}

direction_t gravity_multi_dir_to_dir(multi_direction_t mdir)
{
	direction_t dir = DIR_NONE;

	for ( ; mdir != 0; dir++)
	{
		mdir = (mdir >> 1);
	}
	if (dir > DIR_ALL_MASK)
	{
		dir = DIR_NONE;
	}

	return dir;
}

void gravity_rotate_xy(rotation_t rot, int x, int y, int *ret_x, int *ret_y)
{
	int tx;
	int ty;

	switch (rot)
	{
	case ROTATION_90:
		/* CW */
		tx = -y;
		ty = x;
		break;
	case ROTATION_180:
		tx = -x;
		ty = -y;
		break;
	case ROTATION_270:
		/* CCW */
		tx = y;
		ty = -x;
		break;
	default:
	case ROTATION_0:
		tx = x;
		ty = y;
		break;
	}
	*ret_x = tx;
	*ret_y = ty;

	return;
}

rotation_t gravity_add_rotations(rotation_t rot1, rotation_t rot2)
{
	rotation_t rot;

	rot = ((rot1 + rot2) & ROTATION_MASK);

	return rot;
}
