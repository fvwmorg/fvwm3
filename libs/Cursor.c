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

/*  Modification History */

/*  Created on 10/05/01 by Dan Espen (dane):
    Extracted from fvwm/cursor.c.
    Contains common routine to verify and convert a cursor name
    into a cursor number from X11/cursorfont.h.
    Used by fvwm CursorStyle command and FvwmForm.
*/
#include "config.h"
#include <stdio.h>
#include "fvwmlib.h"
#include <X11/cursorfont.h>
#include "Cursor.h"

/*
 *  fvwmCursorNameToIndex: return the number of a X11 cursor from its
 *  name, if not found return -1.
 */
int fvwmCursorNameToIndex (char *cursor_name)
{
	static const struct CursorNameIndex {
		const char  *name;
		unsigned int number;
	} cursor_table[] = {
		{"arrow", XC_arrow},
		{"based_arrow_down", XC_based_arrow_down},
		{"based_arrow_up", XC_based_arrow_up},
		{"boat", XC_boat},
		{"bogosity", XC_bogosity},
		{"bottom_left_corner", XC_bottom_left_corner},
		{"bottom_right_corner", XC_bottom_right_corner},
		{"bottom_side", XC_bottom_side},
		{"bottom_tee", XC_bottom_tee},
		{"box_spiral", XC_box_spiral},
		{"center_ptr", XC_center_ptr},
		{"circle", XC_circle},
		{"clock", XC_clock},
		{"coffee_mug", XC_coffee_mug},
		{"cross", XC_cross},
		{"cross_reverse", XC_cross_reverse},
		{"crosshair", XC_crosshair},
		{"diamond_cross", XC_diamond_cross},
		{"dot", XC_dot},
		{"dotbox", XC_dotbox},
		{"double_arrow", XC_double_arrow},
		{"draft_large", XC_draft_large},
		{"draft_small", XC_draft_small},
		{"draped_box", XC_draped_box},
		{"exchange", XC_exchange},
		{"fleur", XC_fleur},
		{"gobbler", XC_gobbler},
		{"gumby", XC_gumby},
		{"hand1", XC_hand1},
		{"hand2", XC_hand2},
		{"heart", XC_heart},
		{"icon", XC_icon},
		{"iron_cross", XC_iron_cross},
		{"left_ptr", XC_left_ptr},
		{"left_side", XC_left_side},
		{"left_tee", XC_left_tee},
		{"leftbutton", XC_leftbutton},
		{"ll_angle", XC_ll_angle},
		{"lr_angle", XC_lr_angle},
		{"man", XC_man},
		{"middlebutton", XC_middlebutton},
		{"mouse", XC_mouse},
		{"pencil", XC_pencil},
		{"pirate", XC_pirate},
		{"plus", XC_plus},
		{"question_arrow", XC_question_arrow},
		{"right_ptr", XC_right_ptr},
		{"right_side", XC_right_side},
		{"right_tee", XC_right_tee},
		{"rightbutton", XC_rightbutton},
		{"rtl_logo", XC_rtl_logo},
		{"sailboat", XC_sailboat},
		{"sb_down_arrow", XC_sb_down_arrow},
		{"sb_h_double_arrow", XC_sb_h_double_arrow},
		{"sb_left_arrow", XC_sb_left_arrow},
		{"sb_right_arrow", XC_sb_right_arrow},
		{"sb_up_arrow", XC_sb_up_arrow},
		{"sb_v_double_arrow", XC_sb_v_double_arrow},
		{"sizing", XC_sizing},
		{"spider", XC_spider},
		{"spraycan", XC_spraycan},
		{"star", XC_star},
		{"target", XC_target},
		{"tcross", XC_tcross},
		{"top_left_arrow", XC_top_left_arrow},
		{"top_left_corner", XC_top_left_corner},
		{"top_right_corner", XC_top_right_corner},
		{"top_side", XC_top_side},
		{"top_tee", XC_top_tee},
		{"trek", XC_trek},
		{"ul_angle", XC_ul_angle},
		{"umbrella", XC_umbrella},
		{"ur_angle", XC_ur_angle},
		{"watch", XC_watch},
		{"x_cursor", XC_X_cursor},
		{"xterm", XC_xterm},
	};

	int cond;
	int down = 0;
	int up = (sizeof cursor_table / sizeof cursor_table[0]) - 1;
	int middle;
	char temp[20];
	char *s;

	if (!cursor_name || cursor_name[0] == 0 ||
	    strlen(cursor_name) >= sizeof temp)
		return -1;
	strcpy(temp, cursor_name);

	for (s = temp; *s != 0; s++)
		if (isupper(*s))
			*s = tolower(*s);

	while (down <= up)
	{
		middle= (down + up) / 2;
		if ((cond = strcmp(temp, cursor_table[middle].name)) < 0)
			up = middle - 1;
		else if (cond > 0)
			down =  middle + 1;
		else
			return cursor_table[middle].number;
	}
	return -1;
}
