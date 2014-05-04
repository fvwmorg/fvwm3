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

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/charmap.h"
#include "libs/modifiers.h"
#include "libs/ColorUtils.h"
#include "libs/Parse.h"
#include "libs/wild.h"
#include "fvwm.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "colorset.h"
#include "ewmh.h"
#include "placement.h"

/* ---------------------------- local definitions -------------------------- */

#define SAFEFREE( p )  {if (p) {free(p);(p)=NULL;}}

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* list of window names with attributes */
static window_style *all_styles = NULL;
static window_style *last_style_in_list = NULL;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static Bool __validate_titleformat_string(const char *formatstr)
{
	const char *fmt;

	/* Setting this to "True" here ensures we don't erroneously report on
	 * an invalid TitleFormat string which contains no placeholders
	 * whatsoever.
	 */
	Bool ret_condition = True;

	for (fmt = formatstr; *fmt; fmt++)
	{
		if (*fmt != '%')
			continue;

		switch (*++fmt)
		{
			case 'c':
			case 'n':
			case 'r':
			case 't':
			case 'i':
			case 'I':
				ret_condition = True;
				break;
			default:
				ret_condition = False;
				break;
		}
	}
	return ret_condition;
}

static int blockor(char *dest, char *blk1, char *blk2, int length)
{
	int i;
	char result = 0;

	for (i = 0; i < length; i++)
	{
		dest[i] = (blk1[i] | blk2[i]);
		result |= dest[i];
	}

	return (result) ? 1 : 0;
}

static int blockand(char *dest, char *blk1, char *blk2, int length)
{
	int i;
	char result = 0;

	for (i = 0; i < length; i++)
	{
		dest[i] = (blk1[i] & blk2[i]);
		result |= dest[i];
	}

	return (result) ? 1 : 0;
}

static int blockunmask(char *dest, char *blk1, char *blk2, int length)
{
	int i;
	char result = (char)0xff;

	for (i = 0; i < length; i++)
	{
		dest[i] = (blk1[i] & ~blk2[i]);
		result |= dest[i];
	}

	return (result) ? 1 : 0;
}

static int blockissubset(char *sub, char *super, int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		if ((sub[i] & super[i]) != sub[i])
		{
			return 0;
		}
	}

	return 1;
}

static int blocksintersect(char *blk1, char *blk2, int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		if (blk1[i] & blk2[i])
		{
			return 1;
		}
	}

	return 0;
}

static int style_ids_are_equal(style_id_t *a, style_id_t *b)
{
	if (
		SID_GET_HAS_NAME(*a) && SID_GET_HAS_NAME(*b) &&
		!strcmp(SID_GET_NAME(*a), SID_GET_NAME(*b)))
	{
		return 1;
	}
	if (
		SID_GET_HAS_WINDOW_ID(*a) && SID_GET_HAS_WINDOW_ID(*b) &&
		SID_GET_WINDOW_ID(*a) == SID_GET_WINDOW_ID(*b))
	{
		return 1;
	}

	return 0;
}

static int style_id_equals_id(window_style *s, style_id_t* id)
{
	return style_ids_are_equal(&SGET_ID(*s), id);
}

static int styles_have_same_id(window_style* s, window_style* t)
{
	return style_ids_are_equal(&SGET_ID(*s), &SGET_ID(*t));
}

static int fw_match_style_id(FvwmWindow *fw, style_id_t s_id)
{
	if (SID_GET_HAS_NAME(s_id))
	{
		if (matchWildcards(SID_GET_NAME(s_id), fw->class.res_class) ==
		    1)
		{
			return 1;
		}
		if (matchWildcards(SID_GET_NAME(s_id), fw->class.res_name) ==
		    1)
		{
			return 1;
		}
		if (matchWildcards(SID_GET_NAME(s_id), fw->visible_name) == 1)
		{
			return 1;
		}
		if (matchWildcards(SID_GET_NAME(s_id), fw->name.name) == 1)
		{
			return 1;
		}
		if (fw->style_name != NULL &&
		    matchWildcards(SID_GET_NAME(s_id), fw->style_name) == 1)
		{
			return 1;
		}
	}
	if (SID_GET_HAS_WINDOW_ID(s_id) &&
	    SID_GET_WINDOW_ID(s_id) == (XID)FW_W(fw))
	{
		return 1;
	}

	return 0;
}

static int one_fw_can_match_both_ids(window_style *s, window_style *t)
{
	if (SGET_ID_HAS_WINDOW_ID(*s) && SGET_ID_HAS_WINDOW_ID(*t) &&
	    SGET_WINDOW_ID(*s) != SGET_WINDOW_ID(*t))
	{
		return 0;
	}

	return 1;
}

static void remove_icon_boxes_from_style(window_style *pstyle)
{
	if (SHAS_ICON_BOXES(&pstyle->flags))
	{
		free_icon_boxes(SGET_ICON_BOXES(*pstyle));
		pstyle->flags.has_icon_boxes = 0;
		SSET_ICON_BOXES(*pstyle, NULL);
	}

	return;
}

static void copy_icon_boxes(icon_boxes **pdest, icon_boxes *src)
{
	icon_boxes *last = NULL;
	icon_boxes *temp;

	*pdest = NULL;
	/* copy the icon boxes */
	for ( ; src != NULL; src = src->next)
	{
		temp = xmalloc(sizeof(icon_boxes));
		memcpy(temp, src, sizeof(icon_boxes));
		temp->next = NULL;
		if (last != NULL)
			last->next = temp;
		else
			*pdest = temp;
		last = temp;
	}
}

/* Check word after IconFill to see if its "Top,Bottom,Left,Right" */
static int Get_TBLR(char *token, unsigned char *IconFill)
{
	/* init */
	if (StrEquals(token, "B") ||
	    StrEquals(token, "BOT")||
	    StrEquals(token, "BOTTOM"))
	{
		/* turn on bottom and verical */
		*IconFill = ICONFILLBOT | ICONFILLHRZ;
	}
	else if (StrEquals(token, "T") ||
		 StrEquals(token, "TOP"))
	{
		/* turn on vertical */
		*IconFill = ICONFILLHRZ;
	}
	else if (StrEquals(token, "R") ||
		 StrEquals(token, "RGT") ||
		 StrEquals(token, "RIGHT"))
	{
		/* turn on right bit */
		*IconFill = ICONFILLRGT;
	}
	else if (StrEquals(token, "L") ||
		 StrEquals(token, "LFT") ||
		 StrEquals(token, "LEFT"))
	{
		*IconFill = 0;
	}
	else
	{
		/* anything else is bad */
		return 0;
	}

	/* return OK */
	return 1;
}

static void cleanup_style_defaults(window_style *style)
{
	int i;
	char *dflt;
	char *mask;

	mask = (char *)&(style->flag_mask);
	dflt = (char *)&(style->flag_default);
	for (i = 0; i < sizeof(style_flags); i++)
	{
		dflt[i] &= ~mask[i];
	}

	return;
}

/* merge_styles - For a matching style, merge window_style to window_style
 *
 *  Returned Value:
 *	merged matching styles in callers window_style.
 *
 *  Inputs:
 *	merged_style - style resulting from the merge
 *	add_style    - the style to be added into the merge_style
 *	do_free_src_and_alloc_copy
 *		     - free allocated parts of merge_style that are replaced
 *		       from add_style.	Create a copy of of the replaced
 *		       styles in allocated memory.
 *
 *  Note:
 *	The only trick here is that on and off flags/buttons are
 *	combined into the on flag/button. */
static void merge_styles(
	window_style *merged_style, window_style *add_style,
	Bool do_free_src_and_alloc_copy)
{
	int i;
	char *merge_flags;
	char *add_flags;
	char *merge_mask;
	char *add_mask;
	char *merge_dflt;
	char *add_dflt;
	char *merge_change_mask;
	char *add_change_mask;

	if (add_style->flag_mask.has_icon)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_ICON_NAME(*merged_style));
			SSET_ICON_NAME(
				*merged_style, (SGET_ICON_NAME(*add_style)) ?
				xstrdup(SGET_ICON_NAME(*add_style)) : NULL);
		}
		else
		{
			SSET_ICON_NAME(
				*merged_style, SGET_ICON_NAME(*add_style));
		}
	}
	if (FMiniIconsSupported && add_style->flag_mask.has_mini_icon)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_MINI_ICON_NAME(*merged_style));
			SSET_MINI_ICON_NAME(
				*merged_style,
				(SGET_MINI_ICON_NAME(*add_style)) ?
				xstrdup(SGET_MINI_ICON_NAME(*add_style)) :
				NULL);
		}
		else
		{
			SSET_MINI_ICON_NAME(
				*merged_style,
				SGET_MINI_ICON_NAME(*add_style));
		}
	}
	if (add_style->flag_mask.has_decor)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_DECOR_NAME(*merged_style));
			SSET_DECOR_NAME(
				*merged_style, (SGET_DECOR_NAME(*add_style)) ?
				xstrdup(SGET_DECOR_NAME(*add_style)) :
				NULL);
		}
		else
		{
			SSET_DECOR_NAME(
				*merged_style, SGET_DECOR_NAME(*add_style));
		}
	}
	if (S_HAS_ICON_FONT(SCF(*add_style)))
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_ICON_FONT(*merged_style));
			SSET_ICON_FONT(
				*merged_style, (SGET_ICON_FONT(*add_style)) ?
				xstrdup(SGET_ICON_FONT(*add_style)) : NULL);
		}
		else
		{
			SSET_ICON_FONT(
				*merged_style, SGET_ICON_FONT(*add_style));
		}
	}
	if (S_HAS_WINDOW_FONT(SCF(*add_style)))
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_WINDOW_FONT(*merged_style));
			SSET_WINDOW_FONT(
				*merged_style, (SGET_WINDOW_FONT(*add_style)) ?
				xstrdup(SGET_WINDOW_FONT(*add_style)) :
				NULL);
		}
		else
		{
			SSET_WINDOW_FONT(
				*merged_style, SGET_WINDOW_FONT(*add_style));
		}
	}
	if (add_style->flags.use_start_on_desk)
	{
		SSET_START_DESK(*merged_style, SGET_START_DESK(*add_style));
		SSET_START_PAGE_X(
			*merged_style, SGET_START_PAGE_X(*add_style));
		SSET_START_PAGE_Y(
			*merged_style, SGET_START_PAGE_Y(*add_style));
	}
	if (add_style->flags.use_start_on_screen)
	{
		SSET_START_SCREEN
			(*merged_style, SGET_START_SCREEN(*add_style));
	}
	if (add_style->flag_mask.has_color_fore)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_FORE_COLOR_NAME(*merged_style));
			SSET_FORE_COLOR_NAME(
				*merged_style,
				(SGET_FORE_COLOR_NAME(*add_style)) ?
				xstrdup(SGET_FORE_COLOR_NAME(*add_style)) :
				NULL);
		}
		else
		{
			SSET_FORE_COLOR_NAME(
				*merged_style,
				SGET_FORE_COLOR_NAME(*add_style));
		}
	}
	if (add_style->flag_mask.has_color_back)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_BACK_COLOR_NAME(*merged_style));
			SSET_BACK_COLOR_NAME(
				*merged_style,
				(SGET_BACK_COLOR_NAME(*add_style)) ?
				xstrdup(SGET_BACK_COLOR_NAME(*add_style)) :
				NULL);
		}
		else
		{
			SSET_BACK_COLOR_NAME(
				*merged_style,
				SGET_BACK_COLOR_NAME(*add_style));
		}
	}
	if (add_style->flag_mask.has_color_fore_hi)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_FORE_COLOR_NAME_HI(*merged_style));
			SSET_FORE_COLOR_NAME_HI(
				*merged_style,
				(SGET_FORE_COLOR_NAME_HI(*add_style)) ?
				xstrdup(SGET_FORE_COLOR_NAME_HI(*add_style)) :
				NULL);
		}
		else
		{
			SSET_FORE_COLOR_NAME_HI(
				*merged_style,
				SGET_FORE_COLOR_NAME_HI(*add_style));
		}
	}
	if (add_style->flag_mask.has_color_back_hi)
	{
		if (do_free_src_and_alloc_copy)
		{
			SAFEFREE(SGET_BACK_COLOR_NAME_HI(*merged_style));
			SSET_BACK_COLOR_NAME_HI(
				*merged_style,
				(SGET_BACK_COLOR_NAME_HI(*add_style)) ?
				xstrdup(SGET_BACK_COLOR_NAME_HI(*add_style)) :
				NULL);
		}
		else
		{
			SSET_BACK_COLOR_NAME_HI(
				*merged_style,
				SGET_BACK_COLOR_NAME_HI(*add_style));
		}
	}
	if (add_style->flags.has_border_width)
	{
		SSET_BORDER_WIDTH(
			*merged_style, SGET_BORDER_WIDTH(*add_style));
	}
	if (add_style->flags.has_handle_width)
	{
		SSET_HANDLE_WIDTH(
			*merged_style, SGET_HANDLE_WIDTH(*add_style));
	}
	if (add_style->flags.has_icon_size_limits)
	{
		SSET_MIN_ICON_WIDTH(
			*merged_style, SGET_MIN_ICON_WIDTH(*add_style));
		SSET_MIN_ICON_HEIGHT(
			*merged_style, SGET_MIN_ICON_HEIGHT(*add_style));
		SSET_MAX_ICON_WIDTH(
			*merged_style, SGET_MAX_ICON_WIDTH(*add_style));
		SSET_MAX_ICON_HEIGHT(
			*merged_style, SGET_MAX_ICON_HEIGHT(*add_style));
		SSET_ICON_RESIZE_TYPE(
			*merged_style, SGET_ICON_RESIZE_TYPE(*add_style));
	}
	if (add_style->flags.has_min_window_size)
	{
		SSET_MIN_WINDOW_WIDTH(
			*merged_style, SGET_MIN_WINDOW_WIDTH(*add_style));
		SSET_MIN_WINDOW_HEIGHT(
			*merged_style, SGET_MIN_WINDOW_HEIGHT(*add_style));
	}
	if (add_style->flags.has_max_window_size)
	{
		SSET_MAX_WINDOW_WIDTH(
			*merged_style, SGET_MAX_WINDOW_WIDTH(*add_style));
		SSET_MAX_WINDOW_HEIGHT(
			*merged_style, SGET_MAX_WINDOW_HEIGHT(*add_style));
	}
	if (add_style->flags.has_icon_background_relief)
	{
		SSET_ICON_BACKGROUND_RELIEF(
			*merged_style,
			SGET_ICON_BACKGROUND_RELIEF(*add_style));
	}
	if (add_style->flags.has_icon_background_padding)
	{
		SSET_ICON_BACKGROUND_PADDING(
			*merged_style,
			SGET_ICON_BACKGROUND_PADDING(*add_style));
	}
	if (add_style->flags.has_icon_title_relief)
	{
		SSET_ICON_TITLE_RELIEF(
			*merged_style, SGET_ICON_TITLE_RELIEF(*add_style));
	}
	if (add_style->flags.has_window_shade_steps)
	{
		SSET_WINDOW_SHADE_STEPS(
			*merged_style, SGET_WINDOW_SHADE_STEPS(*add_style));
	}
	if (add_style->flags.has_snap_attraction)
	{
		SSET_SNAP_PROXIMITY(
			*merged_style, SGET_SNAP_PROXIMITY(*add_style));
		SSET_SNAP_MODE(
			*merged_style, SGET_SNAP_MODE(*add_style));
	}
	if (add_style->flags.has_snap_grid)
	{
		SSET_SNAP_GRID_X(
			*merged_style, SGET_SNAP_GRID_X(*add_style));
		SSET_SNAP_GRID_Y(
			*merged_style, SGET_SNAP_GRID_Y(*add_style));
	}
	if (add_style->flags.has_edge_delay_ms_move)
	{
		SSET_EDGE_DELAY_MS_MOVE(
			*merged_style, SGET_EDGE_DELAY_MS_MOVE(*add_style));
	}
	if (add_style->flags.has_edge_delay_ms_resize)
	{
		SSET_EDGE_DELAY_MS_RESIZE(
			*merged_style, SGET_EDGE_DELAY_MS_RESIZE(*add_style));
	}
	if (add_style->flags.has_edge_resistance_move)
	{
		SSET_EDGE_RESISTANCE_MOVE(
			*merged_style, SGET_EDGE_RESISTANCE_MOVE(*add_style));
	}
	if (add_style->flags.has_edge_resistance_xinerama_move)
	{
		SSET_EDGE_RESISTANCE_XINERAMA_MOVE(
			*merged_style,
			SGET_EDGE_RESISTANCE_XINERAMA_MOVE(*add_style));
	}

	/* Note:  Only one style cmd can define a window's iconboxes, the last
	 * one encountered. */
	if (SHAS_ICON_BOXES(&add_style->flag_mask))
	{
		/* If style has iconboxes */
		/* copy it */
		if (do_free_src_and_alloc_copy)
		{
			remove_icon_boxes_from_style(merged_style);
			copy_icon_boxes(
				&SGET_ICON_BOXES(*merged_style),
				SGET_ICON_BOXES(*add_style));
		}
		else
		{
			SSET_ICON_BOXES(
				*merged_style, SGET_ICON_BOXES(*add_style));
		}
	}
	if (add_style->flags.use_layer)
	{
		SSET_LAYER(*merged_style, SGET_LAYER(*add_style));
	}
	if (add_style->flags.do_start_shaded)
	{
		SSET_STARTS_SHADED_DIR(
			*merged_style, SGET_STARTS_SHADED_DIR(*add_style));
	}
	if (add_style->flags.use_colorset)
	{
		SSET_COLORSET(*merged_style, SGET_COLORSET(*add_style));
	}
	if (add_style->flags.use_colorset_hi)
	{
		SSET_COLORSET_HI(*merged_style, SGET_COLORSET_HI(*add_style));
	}
	if (add_style->flags.use_border_colorset)
	{
		SSET_BORDER_COLORSET(
			*merged_style, SGET_BORDER_COLORSET(*add_style));
	}
	if (add_style->flags.use_border_colorset_hi)
	{
		SSET_BORDER_COLORSET_HI(
			*merged_style,SGET_BORDER_COLORSET_HI(*add_style));
	}
	if (add_style->flags.use_icon_title_colorset)
	{
		SSET_ICON_TITLE_COLORSET(
			*merged_style,SGET_ICON_TITLE_COLORSET(*add_style));
	}
	if (add_style->flags.use_icon_title_colorset_hi)
	{
		SSET_ICON_TITLE_COLORSET_HI(
			*merged_style,SGET_ICON_TITLE_COLORSET_HI(*add_style));
	}
	if (add_style->flags.use_icon_background_colorset)
	{
		SSET_ICON_BACKGROUND_COLORSET(
			*merged_style,SGET_ICON_BACKGROUND_COLORSET(
				*add_style));
	}
	if (add_style->flags.has_placement_penalty)
	{
		SSET_NORMAL_PLACEMENT_PENALTY(
			*merged_style,
			SGET_NORMAL_PLACEMENT_PENALTY(*add_style));
		SSET_ONTOP_PLACEMENT_PENALTY(
			*merged_style,
			SGET_ONTOP_PLACEMENT_PENALTY(*add_style));
		SSET_ICON_PLACEMENT_PENALTY(
			*merged_style, SGET_ICON_PLACEMENT_PENALTY(
				*add_style));
		SSET_STICKY_PLACEMENT_PENALTY(
			*merged_style,
			SGET_STICKY_PLACEMENT_PENALTY(*add_style));
		SSET_BELOW_PLACEMENT_PENALTY(
			*merged_style,
			SGET_BELOW_PLACEMENT_PENALTY(*add_style));
		SSET_EWMH_STRUT_PLACEMENT_PENALTY(
			*merged_style,
			SGET_EWMH_STRUT_PLACEMENT_PENALTY(*add_style));
	}
	if (add_style->flags.has_placement_percentage_penalty)
	{
		SSET_99_PLACEMENT_PERCENTAGE_PENALTY(
			*merged_style,
			SGET_99_PLACEMENT_PERCENTAGE_PENALTY(*add_style));
		SSET_95_PLACEMENT_PERCENTAGE_PENALTY(
			*merged_style,
			SGET_95_PLACEMENT_PERCENTAGE_PENALTY(*add_style));
		SSET_85_PLACEMENT_PERCENTAGE_PENALTY(
			*merged_style,
			SGET_85_PLACEMENT_PERCENTAGE_PENALTY(*add_style));
		SSET_75_PLACEMENT_PERCENTAGE_PENALTY(
			*merged_style,
			SGET_75_PLACEMENT_PERCENTAGE_PENALTY(*add_style));
	}
	if (add_style->flags.has_placement_position_string)
	{
		SAFEFREE(SGET_PLACEMENT_POSITION_STRING(*merged_style));
		SSET_PLACEMENT_POSITION_STRING(
			*merged_style,
			strdup(SGET_PLACEMENT_POSITION_STRING(*add_style)));
	}
	if (add_style->flags.has_initial_map_command_string)
	{
		SAFEFREE(SGET_INITIAL_MAP_COMMAND_STRING(*merged_style));
		SSET_INITIAL_MAP_COMMAND_STRING(
			*merged_style,
			strdup(SGET_INITIAL_MAP_COMMAND_STRING(*add_style)));
	}

	if (add_style->flags.has_title_format_string)
	{
		SAFEFREE(SGET_TITLE_FORMAT_STRING(*merged_style));
		SSET_TITLE_FORMAT_STRING(*merged_style,
				strdup(SGET_TITLE_FORMAT_STRING(*add_style)));
	}

	if (add_style->flags.has_icon_title_format_string)
	{
		SAFEFREE(SGET_ICON_TITLE_FORMAT_STRING(*merged_style));
		SSET_ICON_TITLE_FORMAT_STRING(*merged_style,
				strdup(SGET_ICON_TITLE_FORMAT_STRING(*add_style)));
	}
	/* merge the style flags */

	/*** ATTENTION:
	 ***	 This must be the last thing that is done in this function! */
	merge_flags = (char *)&(merged_style->flags);
	add_flags = (char *)&(add_style->flags);
	merge_mask = (char *)&(merged_style->flag_mask);
	add_mask = (char *)&(add_style->flag_mask);
	merge_dflt = (char *)&(merged_style->flag_default);
	add_dflt = (char *)&(add_style->flag_default);
	merge_change_mask = (char *)&(merged_style->change_mask);
	add_change_mask = (char *)&(add_style->change_mask);
	for (i = 0; i < sizeof(style_flags); i++)
	{
		char m;

		/* overwrite set styles */
		merge_flags[i] |= (add_flags[i] & add_mask[i]);
		merge_flags[i] &= (add_flags[i] | ~add_mask[i]);
		/* overwrite default values */
		m = add_dflt[i] & ~add_mask[i] & ~merge_mask[i];
		merge_flags[i] |= (add_flags[i] & m);
		merge_flags[i] &= (add_flags[i] | ~m);
		/* overwrite even weaker default values */
		m = ~add_dflt[i] & ~add_mask[i] & ~merge_dflt[i] &
			~merge_mask[i];
		merge_flags[i] |= (add_flags[i] & m);
		merge_flags[i] &= (add_flags[i] | ~m);
		/* other flags */
		merge_change_mask[i] &= ~(add_mask[i]);
		merge_change_mask[i] |= add_change_mask[i];
		merge_mask[i] |= add_mask[i];
		merge_dflt[i] |= add_dflt[i];
		merge_dflt[i] &= ~merge_mask[i];
	}
	merged_style->has_style_changed |= add_style->has_style_changed;

	return;
}

static void free_style(window_style *style)
{
	/* Free contents of style */
	SAFEFREE(SGET_NAME(*style));
	SAFEFREE(SGET_BACK_COLOR_NAME(*style));
	SAFEFREE(SGET_FORE_COLOR_NAME(*style));
	SAFEFREE(SGET_BACK_COLOR_NAME_HI(*style));
	SAFEFREE(SGET_FORE_COLOR_NAME_HI(*style));
	SAFEFREE(SGET_DECOR_NAME(*style));
	SAFEFREE(SGET_ICON_FONT(*style));
	SAFEFREE(SGET_WINDOW_FONT(*style));
	SAFEFREE(SGET_ICON_NAME(*style));
	SAFEFREE(SGET_MINI_ICON_NAME(*style));
	remove_icon_boxes_from_style(style);
	SAFEFREE(SGET_PLACEMENT_POSITION_STRING(*style));
	SAFEFREE(SGET_INITIAL_MAP_COMMAND_STRING(*style));
	SAFEFREE(SGET_TITLE_FORMAT_STRING(*style));
	SAFEFREE(SGET_ICON_TITLE_FORMAT_STRING(*style));

	return;
}

/* Frees only selected members of a style; adjusts the flag_mask and
 * change_mask appropriately. */
static void free_style_mask(window_style *style, style_flags *mask)
{
	style_flags local_mask;
	style_flags *pmask;

	/* mask out all bits that are not set in the target style */
	pmask =&local_mask;
	blockand((char *)pmask, (char *)&style->flag_mask, (char *)mask,
		 sizeof(style_flags));

	/* Free contents of style */
	if (pmask->has_color_back)
	{
		SAFEFREE(SGET_BACK_COLOR_NAME(*style));
	}
	if (pmask->has_color_fore)
	{
		SAFEFREE(SGET_FORE_COLOR_NAME(*style));
	}
	if (pmask->has_color_back_hi)
	{
		SAFEFREE(SGET_BACK_COLOR_NAME_HI(*style));
	}
	if (pmask->has_color_fore_hi)
	{
		SAFEFREE(SGET_FORE_COLOR_NAME_HI(*style));
	}
	if (pmask->has_decor)
	{
		SAFEFREE(SGET_DECOR_NAME(*style));
	}
	if (pmask->common.has_icon_font)
	{
		SAFEFREE(SGET_ICON_FONT(*style));
	}
	if (pmask->common.has_window_font)
	{
		SAFEFREE(SGET_WINDOW_FONT(*style));
	}
	if (pmask->has_icon)
	{
		SAFEFREE(SGET_ICON_NAME(*style));
	}
	if (pmask->has_mini_icon)
	{
		SAFEFREE(SGET_MINI_ICON_NAME(*style));
	}
	if (pmask->has_icon_boxes)
	{
		remove_icon_boxes_from_style(style);
	}
	/* remove styles from definitiion */
	blockunmask((char *)&style->flag_mask, (char *)&style->flag_mask,
		    (char *)pmask, sizeof(style_flags));
	blockunmask((char *)&style->flag_default, (char *)&style->flag_default,
		    (char *)pmask, sizeof(style_flags));
	blockunmask((char *)&style->change_mask, (char *)&style->change_mask,
		    (char *)pmask, sizeof(style_flags));

	return;
}

static void add_style_to_list(window_style *new_style)
{
	/* This used to contain logic that returned if the style didn't contain
	 * anything.    I don't see why we should bother. dje.
	 *
	 * used to merge duplicate entries, but that is no longer
	 * appropriate since conflicting styles are possible, and the
	 * last match should win! */

	if (last_style_in_list != NULL)
	{
		/* not first entry in list chain this entry to the list */
		SSET_NEXT_STYLE(*last_style_in_list, new_style);
	}
	else
	{
		/* first entry in list set the list root pointer. */
		all_styles = new_style;
	}
	SSET_PREV_STYLE(*new_style, last_style_in_list);
	SSET_NEXT_STYLE(*new_style, NULL);
	last_style_in_list = new_style;
	Scr.flags.do_need_style_list_update = 1;

	return;
} /* end function */

static void remove_style_from_list(window_style *style, Bool do_free_style)
{
	window_style *prev;
	window_style *next;

	prev = SGET_PREV_STYLE(*style);
	next = SGET_NEXT_STYLE(*style);
	if (!prev)
	{
		/* first style in list */
		all_styles = next;
	}
	else
	{
		/* not first style in list */
		SSET_NEXT_STYLE(*prev, next);
	}
	if (!next)
	{
		/* last style in list */
		last_style_in_list = prev;
	}
	else
	{
		SSET_PREV_STYLE(*next, prev);
	}
	if (do_free_style)
	{
		free_style(style);
		free(style);
	}
}

static int remove_all_of_style_from_list(style_id_t style_id)
{
	window_style *nptr = all_styles;
	window_style *next;
	int is_changed = 0;

	/* loop though styles */
	while (nptr)
	{
		next = SGET_NEXT_STYLE(*nptr);
		/* Check if it's to be wiped */
		if (style_id_equals_id(nptr, &style_id))
		{
			remove_style_from_list(nptr, True);
			is_changed = 1;
		}
		/* move on */
		nptr = next;
	}

	return is_changed;
}

static int __simplify_style_list(void)
{
	window_style *cur;
	int has_modified;

	/* Step 1:
	 *   Remove styles that are completely overridden by later
	 *   style definitions.  At the same time...
	 * Step 2:
	 *   Merge styles with the same name if there are no
	 *   conflicting styles with other names set in between. */
	for (
		cur = last_style_in_list, has_modified = 0; cur;
		cur = SGET_PREV_STYLE(*cur))
	{
		style_flags dummyflags;
		/* incremental flags set in styles with the same name */
		style_flags sumflags;
		style_flags sumdflags;
		/* incremental flags set in styles with other names */
		style_flags interflags;
		window_style *cmp;

		memset(&interflags, 0, sizeof(style_flags));
		memcpy(&sumflags, &cur->flag_mask, sizeof(style_flags));
		memcpy(&sumdflags, &cur->flag_default, sizeof(style_flags));
		cmp = SGET_PREV_STYLE(*cur);
		while (cmp)
		{
			if (!styles_have_same_id(cur, cmp))
			{
				if (one_fw_can_match_both_ids(cur, cmp))
				{
					blockor((char *)&interflags,
						(char *)&interflags,
						(char *)&cmp->flag_mask,
						sizeof(style_flags));
					blockor((char *)&interflags,
						(char *)&interflags,
						(char *)&cmp->flag_default,
						sizeof(style_flags));
				}
				cmp = SGET_PREV_STYLE(*cmp);
				continue;
			}
			if (blockissubset(
				    (char *)&cmp->flag_mask,
				    (char *)&sumflags,
				    sizeof(style_flags)) &&
			    blockissubset(
				    (char *)&cmp->flag_default,
				    (char *)&sumdflags,
				    sizeof(style_flags)))
			{
				/* The style is a subset of later style
				 * definitions; nuke it */
				window_style *tmp = SGET_PREV_STYLE(*cmp);
				remove_style_from_list(cmp, True);
				cmp = tmp;
				has_modified = 1;
				continue;
			}
			/* remove all styles that are overridden later from the
			 * style */
			blockor((char *)&dummyflags,
				(char *)&sumdflags,
				(char *)&sumflags,
				sizeof(style_flags));
			free_style_mask(cmp, &dummyflags);
			if (
				!blocksintersect(
					(char *)&cmp->flag_mask,
					(char *)&interflags,
					sizeof(style_flags)) &&
				!blocksintersect(
					(char *)&cmp->flag_default,
					(char *)&interflags,
					sizeof(style_flags)))
			{
				/* merge old style into new style */
				window_style *tmp = SGET_PREV_STYLE(*cmp);
				window_style *prev = SGET_PREV_STYLE(*cur);
				window_style *next = SGET_NEXT_STYLE(*cur);

				/* Add the style to the set */
				blockor((char *)&sumflags,
					(char *)&sumflags,
					(char *)&cmp->flag_mask,
					sizeof(style_flags));
				blockor((char *)&sumdflags,
					(char *)&sumflags,
					(char *)&cmp->flag_default,
					sizeof(style_flags));
				/* merge cmp into cur and delete it
				 * afterwards */
				merge_styles(cmp, cur, True);
				free_style(cur);
				memcpy(cur, cmp, sizeof(window_style));
				/* restore fields overwritten by memcpy */
				SSET_PREV_STYLE(*cur, prev);
				SSET_NEXT_STYLE(*cur, next);
				/* remove the style without freeing the
				 * memory */
				remove_style_from_list(cmp, False);
				/* release the style structure */
				free(cmp);
				cmp = tmp;
				has_modified = 1;
			}
			else if	(
				!blocksintersect(
					(char *)&cur->flag_mask,
					(char *)&interflags,
					sizeof(style_flags)) &&
				!blocksintersect(
					(char *)&cur->flag_default,
					(char *)&interflags,
					sizeof(style_flags)))
			{
				/* merge new style into old style */
				window_style *tmp = SGET_PREV_STYLE(*cmp);

				/* Add the style to the set */
				blockor((char *)&sumflags,
					(char *)&sumflags,
					(char *)&cur->flag_mask,
					sizeof(style_flags));
				blockor((char *)&sumdflags,
					(char *)&sumflags,
					(char *)&cur->flag_default,
					sizeof(style_flags));
				/* merge cur into cmp and delete it
				 * afterwards */
				merge_styles(cmp, cur, True);
				remove_style_from_list(cur, True);
				cur = cmp;
				cmp = tmp;
				has_modified = 1;
				memset(&interflags, 0, sizeof(style_flags));
				continue;
			}
			else
			{
				/* Add it to the set of interfering styles. */
				blockor((char *)&interflags,
					(char *)&interflags,
					(char *)&cmp->flag_mask,
					sizeof(style_flags));
				blockor((char *)&interflags,
					(char *)&interflags,
					(char *)&cmp->flag_default,
					sizeof(style_flags));
				cmp = SGET_PREV_STYLE(*cmp);
			}
		}
	}

	return has_modified;
}

static void style_set_old_focus_policy(window_style *ps, int policy)
{
	focus_policy_t fp;

	switch (policy)
	{
	case 0:
		/* ClickToFocus */
		FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_BY_PROGRAM(fp, 1);
		FPS_GRAB_FOCUS(fp, 1);
		FPS_RELEASE_FOCUS(fp, 1);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(fp, 1);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(fp, 1);
		FPS_RAISE_FOCUSED_DECOR_CLICK(fp, 1);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(fp, 1);
		FPS_PASS_FOCUS_CLICK(fp, 1);
		FPS_PASS_RAISE_CLICK(fp, 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(fp, 1);
		FPS_ALLOW_FUNC_RAISE_CLICK(fp, 1);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(fp, 0);
		break;
	case 1:
		/* MouseFocus */
		FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_BY_PROGRAM(fp, 1);
		FPS_GRAB_FOCUS(fp, 0);
		FPS_RELEASE_FOCUS(fp, 0);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_FOCUSED_DECOR_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(fp, 0);
		FPS_PASS_FOCUS_CLICK(fp, 1);
		FPS_PASS_RAISE_CLICK(fp, 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(fp, 0);
		FPS_ALLOW_FUNC_RAISE_CLICK(fp, 0);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(fp, 1);
		break;
	case 2:
		/* SloppyFocus */
		FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCF(*ps)), 1);
		FPS_FOCUS_BY_PROGRAM(fp, 1);
		FPS_GRAB_FOCUS(fp, 0);
		FPS_RELEASE_FOCUS(fp, 1);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_FOCUSED_DECOR_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(fp, 0);
		FPS_PASS_FOCUS_CLICK(fp, 1);
		FPS_PASS_RAISE_CLICK(fp, 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(fp, 0);
		FPS_ALLOW_FUNC_RAISE_CLICK(fp, 0);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(fp, 1);
		break;
	case 3:
		/* NeverFocus */
		FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCF(*ps)), 0);
		FPS_FOCUS_BY_PROGRAM(fp, 1);
		FPS_GRAB_FOCUS(fp, 0);
		FPS_RELEASE_FOCUS(fp, 0);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(fp, 0);
		FPS_RAISE_FOCUSED_DECOR_CLICK(fp, 0);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(fp, 0);
		FPS_PASS_FOCUS_CLICK(fp, 1);
		FPS_PASS_RAISE_CLICK(fp, 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(fp, 1);
		FPS_ALLOW_FUNC_RAISE_CLICK(fp, 1);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(fp, 0);
		break;
	}
	FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_FOCUS_ENTER(S_FOCUS_POLICY(SCC(*ps)), 1);
	FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_UNFOCUS_LEAVE(S_FOCUS_POLICY(SCC(*ps)), 1);
	FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_FOCUS_CLICK_CLIENT(S_FOCUS_POLICY(SCC(*ps)), 1);
	FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_FOCUS_CLICK_DECOR(S_FOCUS_POLICY(SCC(*ps)), 1);
	FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_FOCUS_CLICK_ICON(S_FOCUS_POLICY(SCC(*ps)), 1);
	FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCM(*ps)), 1);
	FPS_FOCUS_BY_FUNCTION(S_FOCUS_POLICY(SCC(*ps)), 1);

	if (!FP_DO_FOCUS_BY_PROGRAM(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_FOCUS_BY_PROGRAM(
			S_FOCUS_POLICY(SCF(*ps)), FP_DO_FOCUS_BY_PROGRAM(fp));
		FPS_FOCUS_BY_PROGRAM(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_FOCUS_BY_PROGRAM(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_GRAB_FOCUS(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCF(*ps)), FP_DO_GRAB_FOCUS(fp));
		FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_RELEASE_FOCUS(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_RELEASE_FOCUS(
			S_FOCUS_POLICY(SCF(*ps)), FP_DO_RELEASE_FOCUS(fp));
		FPS_RELEASE_FOCUS(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_RELEASE_FOCUS(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_RAISE_FOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_RAISE_FOCUSED_CLIENT_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_RAISE_FOCUSED_CLIENT_CLICK(fp));
		FPS_RAISE_FOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(fp));
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_RAISE_FOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_RAISE_FOCUSED_DECOR_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_RAISE_FOCUSED_DECOR_CLICK(fp));
		FPS_RAISE_FOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_RAISE_FOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_RAISE_UNFOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_RAISE_UNFOCUSED_DECOR_CLICK(fp));
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_PASS_FOCUS_CLICK(
			S_FOCUS_POLICY(SCF(*ps)), FP_DO_PASS_FOCUS_CLICK(fp));
		FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_PASS_RAISE_CLICK(
			S_FOCUS_POLICY(SCF(*ps)), FP_DO_PASS_RAISE_CLICK(fp));
		FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_ALLOW_FUNC_FOCUS_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_ALLOW_FUNC_FOCUS_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_ALLOW_FUNC_FOCUS_CLICK(fp));
		FPS_ALLOW_FUNC_FOCUS_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_ALLOW_FUNC_RAISE_CLICK(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_ALLOW_FUNC_RAISE_CLICK(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_ALLOW_FUNC_RAISE_CLICK(fp));
		FPS_ALLOW_FUNC_RAISE_CLICK(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_ALLOW_FUNC_RAISE_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_WARP_POINTER_ON_FOCUS_FUNC(
		    S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_WARP_POINTER_ON_FOCUS_FUNC(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_WARP_POINTER_ON_FOCUS_FUNC(fp));
		FPS_WARP_POINTER_ON_FOCUS_FUNC(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(S_FOCUS_POLICY(SCC(*ps)), 1);
	}
	if (!FP_DO_SORT_WINDOWLIST_BY(S_FOCUS_POLICY(SCM(*ps))))
	{
		FPS_SORT_WINDOWLIST_BY(
			S_FOCUS_POLICY(SCF(*ps)),
			FP_DO_SORT_WINDOWLIST_BY(fp));
		FPS_SORT_WINDOWLIST_BY(S_FOCUS_POLICY(SCD(*ps)), 1);
		FPS_SORT_WINDOWLIST_BY(S_FOCUS_POLICY(SCC(*ps)), 1);
	}

	return;
}

static char *style_parse_button_style(
	window_style *ps, char *button_string, int on)
{
	int button;
	char *rest;

	button = -1;
	GetIntegerArguments(button_string, &rest, &button, 1);
	button = BUTTON_INDEX(button);
	if (button < 0 || button >= NUMBER_OF_TITLE_BUTTONS)
	{
		fvwm_msg(
			ERR, "CMD_Style",
			"Button and NoButton styles require an argument");
	}
	else
	{
		if (on)
		{
			ps->flags.is_button_disabled &= ~(1 << button);
			ps->flag_mask.is_button_disabled |= (1 << button);
			ps->change_mask.is_button_disabled |= (1 << button);
		}
		else
		{
			ps->flags.is_button_disabled |= (1 << button);
			ps->flag_mask.is_button_disabled |= (1 << button);
			ps->change_mask.is_button_disabled |= (1 << button);
		}
	}

	return rest;
}

static Bool style_parse_focus_policy_style(
	char *option, char *rest, char **ret_rest, Bool is_reversed,
	focus_policy_t *f, focus_policy_t *m, focus_policy_t *c)
{
	char *optlist[] = {
		"SortWindowlistByFocus",
		"FocusClickButtons",
		"FocusClickModifiers",
		"ClickRaisesFocused",
		"ClickDecorRaisesFocused",
		"ClickIconRaisesFocused",
		"ClickRaisesUnfocused",
		"ClickDecorRaisesUnfocused",
		"ClickIconRaisesUnfocused",
		"ClickToFocus",
		"ClickDecorToFocus",
		"ClickIconToFocus",
		"EnterToFocus",
		"LeaveToUnfocus",
		"FocusByProgram",
		"FocusByFunction",
		"FocusByFunctionWarpPointer",
		"Lenient",
		"PassFocusClick",
		"PassRaiseClick",
		"IgnoreFocusClickMotion",
		"IgnoreRaiseClickMotion",
		"AllowFocusClickFunction",
		"AllowRaiseClickFunction",
		"GrabFocus",
		"GrabFocusTransient",
		"OverrideGrabFocus",
		"ReleaseFocus",
		"ReleaseFocusTransient",
		"OverrideReleaseFocus",
		NULL
	};
	Bool found;
	int val;
	int index;
	char *token;

	if (ret_rest)
	{
		*ret_rest = rest;
	}

	found = True;
	val = !is_reversed;
	GetNextTokenIndex(option, optlist, 0, &index);
	switch (index)
	{
	case 0:
		/* SortWindowlistByFocus */
		FPS_SORT_WINDOWLIST_BY(
			*f, (val) ?
			FPOL_SORT_WL_BY_FOCUS : FPOL_SORT_WL_BY_OPEN);
		FPS_SORT_WINDOWLIST_BY(*m, 1);
		FPS_SORT_WINDOWLIST_BY(*c, 1);
		break;
	case 1:
		/* FocusClickButtons */
		if (is_reversed)
		{
			found = False;
			break;
		}
		token = PeekToken(rest, ret_rest);
		val = 0;
		for ( ; token != NULL && isdigit(*token); token++)
		{
			int button;
			char s[2];

			s[0] = *token;
			s[1] = 0;
			button = atoi(s);
			if (button == 0)
			{
				val = ~0;
			}
			else if (button > NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				break;
			}
			else
			{
				val |= (1 << (button - 1));
			}
		}
		if (token != NULL && *token != 0)
		{
			fvwm_msg(
				ERR, "style_parse_focus_policy_style",
				"illegal mouse button '%c'", *token);
			val = DEF_FP_MOUSE_BUTTONS;
		}
		if (token == NULL)
		{
			val = DEF_FP_MOUSE_BUTTONS;
		}
		FPS_MOUSE_BUTTONS(*f, val);
		FPS_MOUSE_BUTTONS(*m, 0x1FF);
		FPS_MOUSE_BUTTONS(*c, 0x1FF);
		break;
	case 2:
		/* FocusClickModifiers */
		if (is_reversed)
		{
			found = False;
			break;
		}
		token = PeekToken(rest, ret_rest);
		if (token == NULL ||
		    modifiers_string_to_modmask(token, &val) == 1)
		{
			val = DEF_FP_MODIFIERS;
		}
		if (val & AnyModifier)
		{
			val = FPOL_ANY_MODIFIER;
		}
		FPS_MODIFIERS(*f, val);
		FPS_MODIFIERS(*m, 0xFF);
		FPS_MODIFIERS(*c, 0xFF);
		break;
	case 3:
		/* ClickRaisesFocused */
		FPS_RAISE_FOCUSED_CLIENT_CLICK(*f, val);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(*m, 1);
		FPS_RAISE_FOCUSED_CLIENT_CLICK(*c, 1);
		break;
	case 4:
		/* ClickDecorRaisesFocused */
		FPS_RAISE_FOCUSED_DECOR_CLICK(*f, val);
		FPS_RAISE_FOCUSED_DECOR_CLICK(*m, 1);
		FPS_RAISE_FOCUSED_DECOR_CLICK(*c, 1);
		break;
	case 5:
		/* ClickIconRaisesFocused */
		FPS_RAISE_FOCUSED_ICON_CLICK(*f, val);
		FPS_RAISE_FOCUSED_ICON_CLICK(*m, 1);
		FPS_RAISE_FOCUSED_ICON_CLICK(*c, 1);
		break;
	case 6:
		/* ClickRaisesUnfocused */
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(*f, val);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(*m, 1);
		FPS_RAISE_UNFOCUSED_CLIENT_CLICK(*c, 1);
		break;
	case 7:
		/* ClickDecorRaisesUnfocused */
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(*f, val);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(*m, 1);
		FPS_RAISE_UNFOCUSED_DECOR_CLICK(*c, 1);
		break;
	case 8:
		/* ClickIconRaisesUnfocused */
		FPS_RAISE_UNFOCUSED_ICON_CLICK(*f, val);
		FPS_RAISE_UNFOCUSED_ICON_CLICK(*m, 1);
		FPS_RAISE_UNFOCUSED_ICON_CLICK(*c, 1);
		break;
	case 9:
		/* ClickToFocus */
		FPS_FOCUS_CLICK_CLIENT(*f, val);
		FPS_FOCUS_CLICK_CLIENT(*m, 1);
		FPS_FOCUS_CLICK_CLIENT(*c, 1);
		break;
	case 10:
		/* ClickDecorToFocus */
		FPS_FOCUS_CLICK_DECOR(*f, val);
		FPS_FOCUS_CLICK_DECOR(*m, 1);
		FPS_FOCUS_CLICK_DECOR(*c, 1);
		break;
	case 11:
		/* ClickIconToFocus */
		FPS_FOCUS_CLICK_ICON(*f, val);
		FPS_FOCUS_CLICK_ICON(*m, 1);
		FPS_FOCUS_CLICK_ICON(*c, 1);
		break;
	case 12:
		/* EnterToFocus */
		FPS_FOCUS_ENTER(*f, val);
		FPS_FOCUS_ENTER(*m, 1);
		FPS_FOCUS_ENTER(*c, 1);
		break;
	case 13:
		/* LeaveToUnfocus */
		FPS_UNFOCUS_LEAVE(*f, val);
		FPS_UNFOCUS_LEAVE(*m, 1);
		FPS_UNFOCUS_LEAVE(*c, 1);
		break;
	case 14:
		/* FocusByProgram */
		FPS_FOCUS_BY_PROGRAM(*f, val);
		FPS_FOCUS_BY_PROGRAM(*m, 1);
		FPS_FOCUS_BY_PROGRAM(*c, 1);
		break;
	case 15:
		/* FocusByFunction */
		FPS_FOCUS_BY_FUNCTION(*f, val);
		FPS_FOCUS_BY_FUNCTION(*m, 1);
		FPS_FOCUS_BY_FUNCTION(*c, 1);
		break;
	case 16:
		/* FocusByFunctionWarpPointer */
		FPS_WARP_POINTER_ON_FOCUS_FUNC(*f, val);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(*m, 1);
		FPS_WARP_POINTER_ON_FOCUS_FUNC(*c, 1);
		break;
	case 17:
		/* Lenient */
		FPS_LENIENT(*f, val);
		FPS_LENIENT(*m, 1);
		FPS_LENIENT(*c, 1);
		break;
	case 18:
		/* PassFocusClick */
		FPS_PASS_FOCUS_CLICK(*f, val);
		FPS_PASS_FOCUS_CLICK(*m, 1);
		FPS_PASS_FOCUS_CLICK(*c, 1);
		break;
	case 19:
		/* PassRaiseClick */
		FPS_PASS_RAISE_CLICK(*f, val);
		FPS_PASS_RAISE_CLICK(*m, 1);
		FPS_PASS_RAISE_CLICK(*c, 1);
		break;
	case 20:
		/* IgnoreFocusClickMotion */
		FPS_IGNORE_FOCUS_CLICK_MOTION(*f, val);
		FPS_IGNORE_FOCUS_CLICK_MOTION(*m, 1);
		FPS_IGNORE_FOCUS_CLICK_MOTION(*c, 1);
		break;
	case 21:
		/* IgnoreRaiseClickMotion */
		FPS_IGNORE_RAISE_CLICK_MOTION(*f, val);
		FPS_IGNORE_RAISE_CLICK_MOTION(*m, 1);
		FPS_IGNORE_RAISE_CLICK_MOTION(*c, 1);
		break;
	case 22:
		/* AllowFocusClickFunction */
		FPS_ALLOW_FUNC_FOCUS_CLICK(*f, val);
		FPS_ALLOW_FUNC_FOCUS_CLICK(*m, 1);
		FPS_ALLOW_FUNC_FOCUS_CLICK(*c, 1);
		break;
	case 23:
		/* AllowRaiseClickFunction */
		FPS_ALLOW_FUNC_RAISE_CLICK(*f, val);
		FPS_ALLOW_FUNC_RAISE_CLICK(*m, 1);
		FPS_ALLOW_FUNC_RAISE_CLICK(*c, 1);
		break;
	case 24:
		/* GrabFocus */
		FPS_GRAB_FOCUS(*f, val);
		FPS_GRAB_FOCUS(*m, 1);
		FPS_GRAB_FOCUS(*c, 1);
		break;
	case 25:
		/* GrabFocusTransient */
		FPS_GRAB_FOCUS_TRANSIENT(*f, val);
		FPS_GRAB_FOCUS_TRANSIENT(*m, 1);
		FPS_GRAB_FOCUS_TRANSIENT(*c, 1);
		break;
	case 26:
		/* OverrideGrabFocus */
		FPS_OVERRIDE_GRAB_FOCUS(*f, val);
		FPS_OVERRIDE_GRAB_FOCUS(*m, 1);
		FPS_OVERRIDE_GRAB_FOCUS(*c, 1);
		break;
	case 27:
		/* ReleaseFocus */
		FPS_RELEASE_FOCUS(*f, val);
		FPS_RELEASE_FOCUS(*m, 1);
		FPS_RELEASE_FOCUS(*c, 1);
		break;
	case 28:
		/* ReleaseFocusTransient */
		FPS_RELEASE_FOCUS_TRANSIENT(*f, val);
		FPS_RELEASE_FOCUS_TRANSIENT(*m, 1);
		FPS_RELEASE_FOCUS_TRANSIENT(*c, 1);
		break;
	case 29:
		/* OverrideReleaseFocus */
		FPS_OVERRIDE_RELEASE_FOCUS(*f, val);
		FPS_OVERRIDE_RELEASE_FOCUS(*m, 1);
		FPS_OVERRIDE_RELEASE_FOCUS(*c, 1);
		break;
	default:
		found = False;
		break;
	}

	return found;
}

static char *style_parse_icon_size_style(
	char *option, char *rest, window_style *ps)
{
	int vals[4];
	int i;

	option = PeekToken(rest, NULL);
	if (StrEquals(option, "Stretched"))
	{
		SSET_ICON_RESIZE_TYPE(*ps, ICON_RESIZE_TYPE_STRETCHED);
		option = PeekToken(rest, &rest);
	}
	else if (StrEquals(option, "Adjusted"))
	{
		SSET_ICON_RESIZE_TYPE(*ps, ICON_RESIZE_TYPE_ADJUSTED);
		option = PeekToken(rest, &rest);
	}
	else if (StrEquals(option, "Shrunk"))
	{
		SSET_ICON_RESIZE_TYPE(*ps, ICON_RESIZE_TYPE_SHRUNK);
		option = PeekToken(rest, &rest);
	}
	else
	{
		SSET_ICON_RESIZE_TYPE(*ps, ICON_RESIZE_TYPE_NONE);
	}

	switch (GetIntegerArguments(rest, &rest, vals, 4))
	{
	case 0:
		/* No arguments results in default values */
		vals[0] = vals[1] = UNSPECIFIED_ICON_DIMENSION;
		/* fall through */
	case 2:
		/* Max and min values are the same */
		vals[2] = vals[0];
		vals[3] = vals[1];
		/* fall through */
	case 4:
		/* Validate values */
		for (i = 0; i < 4; i++)
		{
			int use_default = 0;

			if (vals[i] != UNSPECIFIED_ICON_DIMENSION &&
			    (vals[i] < MIN_ALLOWABLE_ICON_DIMENSION ||
			     vals[i] > MAX_ALLOWABLE_ICON_DIMENSION))
			{
				fvwm_msg(
					ERR, "CMD_Style",
					"IconSize dimension (%d) not in valid"
					" range (%d-%d)",
					vals[i], MIN_ALLOWABLE_ICON_DIMENSION,
					MAX_ALLOWABLE_ICON_DIMENSION);
				use_default = 1;
			}

			/* User requests default value for this dimension */
			else if (vals[i] == UNSPECIFIED_ICON_DIMENSION)
			{
				use_default = 1;
			}

			if (use_default)
			{
				/* Set default value for this dimension.  The
				 * first two indexes refer to min values, the
				 * latter two to max values. */
				vals[i] = i < 2 ? MIN_ALLOWABLE_ICON_DIMENSION :
					MAX_ALLOWABLE_ICON_DIMENSION;
			}
		}
		SSET_MIN_ICON_WIDTH(*ps, vals[0]);
		SSET_MIN_ICON_HEIGHT(*ps, vals[1]);
		SSET_MAX_ICON_WIDTH(*ps, vals[2]);
		SSET_MAX_ICON_HEIGHT(*ps, vals[3]);

		ps->flags.has_icon_size_limits = 1;
		ps->flag_mask.has_icon_size_limits = 1;
		ps->change_mask.has_icon_size_limits = 1;
		break;
	default:
		fvwm_msg(
			ERR, "CMD_Style",
			"IconSize requires exactly 0, 2 or 4"
			" numerical arguments");
		break;
	}

	return rest;
}

static char *style_parse_icon_box_style(
	icon_boxes **ret_ib, char *option, char *rest, window_style *ps)
{
	icon_boxes *IconBoxes = NULL;
	Bool is_screen_given = False;
	int val[4];
	int num;
	int i;

	option = PeekToken(rest, NULL);
	if (!option || StrEquals(option, "none"))
	{
		option = PeekToken(rest, &rest);
		/* delete icon boxes from style */
		if (SGET_ICON_BOXES(*ps))
		{
			remove_icon_boxes_from_style(ps);
		}
		(*ret_ib) = NULL;
		if (option)
		{
			/* disable default icon box */
			S_SET_DO_IGNORE_ICON_BOXES(SCF(*ps), 1);
		}
		else
		{
			/* use default icon box */
			S_SET_DO_IGNORE_ICON_BOXES(SCF(*ps), 0);
		}
		S_SET_DO_IGNORE_ICON_BOXES(SCM(*ps), 1);
		S_SET_DO_IGNORE_ICON_BOXES(SCC(*ps), 1);
		ps->flags.has_icon_boxes = 0;
		ps->flag_mask.has_icon_boxes = 1;
		ps->change_mask.has_icon_boxes = 1;
		return rest;
	}

	/* otherwise try to parse the icon box */
	IconBoxes = xcalloc(1, sizeof(icon_boxes));
	IconBoxes->IconScreen = FSCREEN_GLOBAL;
	/* init grid x */
	IconBoxes->IconGrid[0] = 3;
	/* init grid y */
	IconBoxes->IconGrid[1] = 3;

	/* check for screen (for 4 numbers) */
	if (StrEquals(option, "screen"))
	{
		is_screen_given = True;
		option = PeekToken(rest, &rest); /* skip screen */
		option = PeekToken(rest, &rest); /* get the screen spec */
		IconBoxes->IconScreen =
			FScreenGetScreenArgument(option, FSCREEN_SPEC_PRIMARY);
	}

	/* try for 4 numbers x y x y */
	num = GetIntegerArguments(rest, NULL, val, 4);

	/* if 4 numbers */
	if (num == 4)
	{
		for (i = 0; i < 4; i++)
		{
			/* make sure the value fits into a short */
			if (val[i] < -32768)
			{
				val[i] = -32768;
			}
			if (val[i] > 32767)
			{
				val[i] = 32767;
			}
			IconBoxes->IconBox[i] = val[i];
			/* If leading minus sign */
			option = PeekToken(rest, &rest);
			if (option[0] == '-')
			{
				IconBoxes->IconSign[i]='-';
			} /* end leading minus sign */
		}
		/* Note: here there is no test for valid co-ords, use geom */
	}
	else if  (is_screen_given)
	{
		/* screen-spec is given but not 4 numbers */
		fvwm_msg(
			ERR,"CMD_Style",
			"IconBox requires 4 numbers if screen is given!"
			" Invalid: <%s>.", option);
		/* Drop the box */
		free(IconBoxes);
		/* forget about it */
		IconBoxes = 0;
	}
	else
	{
		/* Not 4 numeric args dje */
		/* bigger than =32767x32767+32767+32767 */
		int geom_flags;
		int l;
		int width;
		int height;

		/* read in 1 word w/o advancing */
		option = PeekToken(rest, NULL);
		if (!option)
		{
			return rest;
		}
		l = strlen(option);
		if (l > 0 && l < 24)
		{
			/* advance */
			option = PeekToken(rest, &rest);
			/* if word found, not too long */
			geom_flags = FScreenParseGeometryWithScreen(
				option, &IconBoxes->IconBox[0],
				&IconBoxes->IconBox[1],	(unsigned int*)&width,
				(unsigned int*)&height,
				&IconBoxes->IconScreen);
			if (width == 0 || !(geom_flags & WidthValue))
			{
				/* zero width is invalid */
				fvwm_msg(
					ERR,"CMD_Style",
					"IconBox requires 4 numbers or"
					" geometry! Invalid string <%s>.",
					option);
				/* Drop the box */
				free(IconBoxes);
				/* forget about it */
				IconBoxes = 0;
			}
			else
			{
				/* got valid iconbox geom */
				if (geom_flags & XNegative)
				{
					IconBoxes->IconBox[0] =
						/* neg x coord */
						IconBoxes->IconBox[0] -
						width - 2;
					/* save for later */
					IconBoxes->IconSign[0]='-';
					IconBoxes->IconSign[2]='-';
				}
				if (geom_flags & YNegative)
				{
					IconBoxes->IconBox[1] =
						/* neg y coord */
						IconBoxes->IconBox[1]
						- height -2;
					/* save for later */
					IconBoxes->IconSign[1]='-';
					IconBoxes->IconSign[3]='-';
				}
				/* x + wid = right x */
				IconBoxes->IconBox[2] =
					width + IconBoxes->IconBox[0];
				/* y + height = bottom y */
				IconBoxes->IconBox[3] =
					height + IconBoxes->IconBox[1];
			} /* end icon geom worked */
		}
		else
		{
			/* no word or too long; drop the box */
			free(IconBoxes);
			/* forget about it */
			IconBoxes = 0;
		} /* end word found, not too long */
	} /* end not 4 args */
	/* If we created an IconBox, put it in the chain. */
	if (IconBoxes != 0)
	{
		/* no error */
		if (SGET_ICON_BOXES(*ps) == 0)
		{
			/* first one, chain to root */
			SSET_ICON_BOXES(*ps, IconBoxes);
		}
		else
		{
			/* else not first one, add to end of chain */
			(*ret_ib)->next = IconBoxes;
		} /* end not first one */
		/* new current box. save for grid */
		(*ret_ib) = IconBoxes;
	} /* end no error */
	S_SET_DO_IGNORE_ICON_BOXES(SCF(*ps), 0);
	S_SET_DO_IGNORE_ICON_BOXES(SCM(*ps), 1);
	S_SET_DO_IGNORE_ICON_BOXES(SCC(*ps), 1);
	ps->flags.has_icon_boxes = !!(SGET_ICON_BOXES(*ps));
	ps->flag_mask.has_icon_boxes = 1;
	ps->change_mask.has_icon_boxes = 1;

	return rest;
}

static char *style_parse_icon_grid_style(
	char *option, char *rest, window_style *ps, icon_boxes *ib)
{
	int val[4];
	int num;
	int i;

	/* The grid always affects the prior iconbox */
	if (ib == 0)
	{
		/* If no current box */
		fvwm_msg(
			ERR,"CMD_Style",
			"IconGrid must follow an IconBox in same Style"
			" command");
		return rest;
	}
	/* have a place to grid */
	/* 2 ints */
	num = GetIntegerArguments(rest, &rest, val, 2);
	if (num != 2 || val[0] < 1 || val[1] < 1)
	{
		fvwm_msg(
			ERR,"CMD_Style",
			"IconGrid needs 2 numbers > 0. Got %d numbers."
			" x=%d y=%d!", num, val[0], val[1]);
		/* reset grid */
		ib->IconGrid[0] = 3;
		ib->IconGrid[1] = 3;
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			ib->IconGrid[i] = val[i];
		}
	} /* end bad grid */

	return rest;
}

static char *style_parse_icon_fill_style(
	char *option, char *rest, window_style *ps, icon_boxes *ib)
{
	/* first  type direction parsed */
	unsigned char IconFill_1;
	/* second type direction parsed */
	unsigned char IconFill_2;

	/* direction to fill iconbox */
	/* The fill always affects the prior iconbox */
	if (ib == 0)
	{
		/* If no current box */
		fvwm_msg(
			ERR,"CMD_Style",
			"IconFill must follow an IconBox in same Style"
			" command");
		return rest;
	}
	/* have a place to fill */
	option = PeekToken(rest, &rest);
	/* top/bot/lft/rgt */
	if (!option || Get_TBLR(option, &IconFill_1) == 0)
	{
		/* its wrong */
		if (!option)
		{
			option = "(none)";
		}
		fvwm_msg(
			ERR,"CMD_Style",
			"IconFill must be followed by T|B|R|L, found"
			" %s.", option);
		return rest;
	}
	/* first word valid */

	/* read in second word */
	option = PeekToken(rest, &rest);
	/* top/bot/lft/rgt */
	if (!option || Get_TBLR(option, &IconFill_2) == 0)
	{
		/* its wrong */
		if (!option)
		{
			option = "(none)";
		}
		fvwm_msg(
			ERR,"CMD_Style",
			"IconFill must be followed by T|B|R|L,"
			" found %s.", option);
		return rest;
	}
	if ((IconFill_1 & ICONFILLHRZ) == (IconFill_2 & ICONFILLHRZ))
	{
		fvwm_msg(
			ERR, "CMD_Style",
			"IconFill must specify a horizontal"
			" and vertical direction.");
		return rest;
	}
	/* Its valid! */
	/* merge in flags */
	ib->IconFlags |= IconFill_1;
	/* ignore horiz in 2nd arg */
	IconFill_2 &= ~ICONFILLHRZ;
	/* merge in flags */
	ib->IconFlags |= IconFill_2;

	return rest;
}

static Bool style_parse_one_style_option(
	char *token, char *rest, char **ret_rest, char *prefix,
	window_style *ps, icon_boxes **cur_ib)
{
	window_style *add_style;
	/* work area for button number */
	int num;
	int i;
	int tmpno[3] = { -1, -1, -1 };
	int val[4];
	int spargs = 0;
	Bool found;
	int on;
	char *token_l = NULL;

	found = True;
	on = 1;
	while (token[0] == '!')
	{
		on ^= 1;
		token++;
	}
	if (prefix != NULL && *prefix != 0)
	{
		int l;

		l = strlen(prefix);
		if (strncasecmp(token, prefix, l) != 0)
		{
			/* add missing prefix */
			/* TA:  xasprintf() */
			token_l = xmalloc(l + strlen(token) + 1);
			strcpy(token_l, prefix);
			strcat(token_l, token);
			token = token_l;
		}
	}
	switch (tolower(token[0]))
	{
	case 'a':
		if (StrEquals(token, "ACTIVEPLACEMENT"))
		{
			ps->flags.placement_mode &= (~PLACE_RANDOM);
			ps->flag_mask.placement_mode |= PLACE_RANDOM;
			ps->change_mask.placement_mode |= PLACE_RANDOM;
		}
		else if (StrEquals(token, "ACTIVEPLACEMENTHONORSSTARTSONPAGE"))
		{
			ps->flags.manual_placement_honors_starts_on_page = on;
			ps->flag_mask.manual_placement_honors_starts_on_page =
				1;
			ps->change_mask.manual_placement_honors_starts_on_page
				= 1;
		}
		else if (StrEquals(
				 token, "ACTIVEPLACEMENTIGNORESSTARTSONPAGE"))
		{
			ps->flags.manual_placement_honors_starts_on_page = !on;
			ps->flag_mask.manual_placement_honors_starts_on_page =
				1;
			ps->change_mask.manual_placement_honors_starts_on_page
				= 1;
		}
		else if (StrEquals(token, "AllowRestack"))
		{
			S_SET_DO_IGNORE_RESTACK(SCF(*ps), !on);
			S_SET_DO_IGNORE_RESTACK(SCM(*ps), 1);
			S_SET_DO_IGNORE_RESTACK(SCC(*ps), 1);
		}
		else if (StrEquals(token, "AllowMaximizeFixedSize"))
		{
			S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCF(*ps), !on);
			S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCM(*ps), 1);
			S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'b':
		if (StrEquals(token, "BackColor"))
		{
			rest = GetNextToken(rest, &token);
			if (token)
			{
				SAFEFREE(SGET_BACK_COLOR_NAME(*ps));
				SSET_BACK_COLOR_NAME(*ps, token);
				ps->flags.has_color_back = 1;
				ps->flag_mask.has_color_back = 1;
				ps->change_mask.has_color_back = 1;
				ps->flags.use_colorset = 0;
				ps->flag_mask.use_colorset = 1;
				ps->change_mask.use_colorset = 1;
			}
			else
			{
				fvwm_msg(
					ERR, "style_parse_on_estyle_option",
					"Style BackColor requires color"
					" argument");
			}
		}
		else if (StrEquals(token, "Button"))
		{
			rest = style_parse_button_style(ps, rest, on);
		}
		else if (StrEquals(token, "BorderWidth"))
		{
			if (GetIntegerArguments(rest, &rest, val, 1))
			{
				SSET_BORDER_WIDTH(*ps, (short)*val);
				ps->flags.has_border_width = 1;
				ps->flag_mask.has_border_width = 1;
				ps->change_mask.has_border_width = 1;
			}
			else
			{
				ps->flags.has_border_width = 0;
				ps->flag_mask.has_border_width = 1;
				ps->change_mask.has_border_width = 1;
			}
		}
		else if (StrEquals(token, "BackingStore"))
		{
			ps->flags.use_backing_store = BACKINGSTORE_ON;
			ps->flag_mask.use_backing_store = BACKINGSTORE_MASK;
			ps->change_mask.use_backing_store = BACKINGSTORE_MASK;
		}
		else if (StrEquals(token, "BackingStoreOff"))
		{
			ps->flags.use_backing_store = BACKINGSTORE_OFF;
			ps->flag_mask.use_backing_store = BACKINGSTORE_MASK;
			ps->change_mask.use_backing_store = BACKINGSTORE_MASK;
		}
		else if (StrEquals(token, "BackingStoreWindowDefault"))
		{
			ps->flags.use_backing_store = BACKINGSTORE_DEFAULT;
			ps->flag_mask.use_backing_store = BACKINGSTORE_MASK;
			ps->change_mask.use_backing_store = BACKINGSTORE_MASK;
		}
		else if (StrEquals(token, "BorderColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			SSET_BORDER_COLORSET(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_border_colorset = (*val >= 0);
			ps->flag_mask.use_border_colorset = 1;
			ps->change_mask.use_border_colorset = 1;
		}
		else if (StrEquals(token, "BottomTitleRotated"))
		{
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCF(*ps), on);
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCM(*ps), 1);
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "BottomTitleNotRotated"))
		{
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCF(*ps), !on);
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCM(*ps), 1);
			S_SET_IS_BOTTOM_TITLE_ROTATED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Borders"))
		{
			S_SET_HAS_NO_BORDER(SCF(*ps), !on);
			S_SET_HAS_NO_BORDER(SCM(*ps), 1);
			S_SET_HAS_NO_BORDER(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'c':
		if (StrEquals(token, "CascadePlacement"))
		{
			ps->flags.placement_mode = PLACE_CASCADE;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "CLEVERPLACEMENT"))
		{
			ps->flags.placement_mode |= PLACE_CLEVER;
			ps->flag_mask.placement_mode |= PLACE_CLEVER;
			ps->change_mask.placement_mode |= PLACE_CLEVER;
		}
		else if (StrEquals(token, "CleverPlacementOff"))
		{
			ps->flags.placement_mode &= (~PLACE_CLEVER);
			ps->flag_mask.placement_mode |= PLACE_CLEVER;
			ps->change_mask.placement_mode |= PLACE_CLEVER;
		}
		else if (StrEquals(token, "CaptureHonorsStartsOnPage"))
		{
			ps->flags.capture_honors_starts_on_page = on;
			ps->flag_mask.capture_honors_starts_on_page = 1;
			ps->change_mask.capture_honors_starts_on_page = 1;
		}
		else if (StrEquals(token, "CaptureIgnoresStartsonPage"))
		{
			ps->flags.capture_honors_starts_on_page = !on;
			ps->flag_mask.capture_honors_starts_on_page = 1;
			ps->change_mask.capture_honors_starts_on_page = 1;
		}
		else if (StrEquals(token, "ColorSet"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			if (*val < 0)
			{
				*val = -1;
			}
			SSET_COLORSET(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_colorset = (*val >= 0);
			ps->flag_mask.use_colorset = 1;
			ps->change_mask.use_colorset = 1;
		}
		else if (StrEquals(token, "Color"))
		{
			char c = 0;
			char *next;

			next = GetNextToken(rest, &token);
			if (token == NULL)
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"Color Style requires a color"
					" argument");
				break;
			}
			if (strncasecmp(token, "rgb:", 4) == 0)
			{
				char *s;
				int i;

				/* spool to third '/' */
				for (i = 0, s = token + 4; *s && i < 3; s++)
				{
					if (*s == '/')
					{
						i++;
					}
				}
				s--;
				if (i == 3)
				{
					*s = 0;
					/* spool to third '/' in original
					 * string too */
					for (i = 0, s = rest; *s && i < 3; s++)
					{
						if (*s == '/')
						{
							i++;
						}
					}
					next = s - 1;
				}
			}
			else
			{
				free(token);
				next = DoGetNextToken(
					rest, &token, NULL, ",/", &c);
			}
			rest = next;
			SAFEFREE(SGET_FORE_COLOR_NAME(*ps));
			SSET_FORE_COLOR_NAME(*ps, token);
			ps->flags.has_color_fore = 1;
			ps->flag_mask.has_color_fore = 1;
			ps->change_mask.has_color_fore = 1;
			ps->flags.use_colorset = 0;
			ps->flag_mask.use_colorset = 1;
			ps->change_mask.use_colorset = 1;

			/* skip over '/' */
			if (c != '/')
			{
				while (rest && *rest &&
				       isspace((unsigned char)*rest) &&
				       *rest != ',' && *rest != '/')
				{
					rest++;
				}
				if (*rest == '/')
				{
					rest++;
				}
			}

			rest=GetNextToken(rest, &token);
			if (!token)
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"Color Style called with incomplete"
					" color argument.");
				break;
			}
			SAFEFREE(SGET_BACK_COLOR_NAME(*ps));
			SSET_BACK_COLOR_NAME(*ps, token);
			ps->flags.has_color_back = 1;
			ps->flag_mask.has_color_back = 1;
			ps->change_mask.has_color_back = 1;
			break;
		}
		else if (StrEquals(token, "CirculateSkipIcon"))
		{
			S_SET_DO_CIRCULATE_SKIP_ICON(SCF(*ps), on);
			S_SET_DO_CIRCULATE_SKIP_ICON(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP_ICON(SCC(*ps), 1);
		}
		else if (StrEquals(token, "CirculateSkipShaded"))
		{
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCF(*ps), on);
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "CirculateHitShaded"))
		{
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCF(*ps), !on);
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP_SHADED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "CirculateHitIcon"))
		{
			S_SET_DO_CIRCULATE_SKIP_ICON(SCF(*ps), !on);
			S_SET_DO_CIRCULATE_SKIP_ICON(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP_ICON(SCC(*ps), 1);
		}
		else if (StrEquals(token, "ClickToFocus"))
		{
			style_set_old_focus_policy(ps, 0);
		}
		else if (StrEquals(token, "ClickToFocusPassesClick"))
		{
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "ClickToFocusPassesClickOff"))
		{
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_PASS_FOCUS_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_PASS_RAISE_CLICK(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "ClickToFocusRaises"))
		{
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "ClickToFocusRaisesOff"))
		{
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "CirculateSkip"))
		{
			S_SET_DO_CIRCULATE_SKIP(SCF(*ps), on);
			S_SET_DO_CIRCULATE_SKIP(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "CirculateHit"))
		{
			S_SET_DO_CIRCULATE_SKIP(SCF(*ps), !on);
			S_SET_DO_CIRCULATE_SKIP(SCM(*ps), 1);
			S_SET_DO_CIRCULATE_SKIP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Closable"))
		{
			S_SET_IS_UNCLOSABLE(SCF(*ps), !on);
			S_SET_IS_UNCLOSABLE(SCM(*ps), 1);
			S_SET_IS_UNCLOSABLE(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'd':
		if (StrEquals(token, "DepressableBorder"))
		{
			S_SET_HAS_DEPRESSABLE_BORDER(SCF(*ps), on);
			S_SET_HAS_DEPRESSABLE_BORDER(SCM(*ps), 1);
			S_SET_HAS_DEPRESSABLE_BORDER(SCC(*ps), 1);
		}
		else if (StrEquals(token, "DecorateTransient"))
		{
			ps->flags.do_decorate_transient = on;
			ps->flag_mask.do_decorate_transient = 1;
			ps->change_mask.do_decorate_transient = 1;
		}
		else if (StrEquals(token, "DumbPlacement"))
		{
			ps->flags.placement_mode &= (~PLACE_SMART);
			ps->flag_mask.placement_mode |= PLACE_SMART;
			ps->change_mask.placement_mode |= PLACE_SMART;
		}
		else if (StrEquals(token, "DONTRAISETRANSIENT"))
		{
			S_SET_DO_RAISE_TRANSIENT(SCF(*ps), !on);
			S_SET_DO_RAISE_TRANSIENT(SCM(*ps), 1);
			S_SET_DO_RAISE_TRANSIENT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "DONTLOWERTRANSIENT"))
		{
			S_SET_DO_LOWER_TRANSIENT(SCF(*ps), !on);
			S_SET_DO_LOWER_TRANSIENT(SCM(*ps), 1);
			S_SET_DO_LOWER_TRANSIENT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "DontStackTransientParent"))
		{
			S_SET_DO_STACK_TRANSIENT_PARENT(SCF(*ps), !on);
			S_SET_DO_STACK_TRANSIENT_PARENT(SCM(*ps), 1);
			S_SET_DO_STACK_TRANSIENT_PARENT(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'e':
		if (StrEquals(token, "ExactWindowName"))
		{
			char *format;
			/* TA:  This is being deprecated in favour of the more
			 * generic:
			 *
			 * TitleFormat %n
			 */
			fvwm_msg(WARN, "style_parse_one_style_option",
				"ExactWindowName is deprecated -- using"
				" TitleFormat %%n");
			format = strdup(DEFAULT_TITLE_FORMAT);
			SSET_TITLE_FORMAT_STRING(*ps, format);
			ps->flags.has_title_format_string = 1;
			ps->flag_mask.has_title_format_string = 1;
			ps->change_mask.has_title_format_string = 1;
		}
		else if (StrEquals(token, "ExactIconName"))
		{
			char *format;
			/* TA:  This is being deprecated in favour of the more
			 * generic:
			 *
			 * IconTitleFormat %n
			 */
			fvwm_msg(WARN, "style_parse_one_style_option",
				"ExactIconName is deprecated -- using"
				" IconTitleFormat %%n");
			format = strdup(DEFAULT_TITLE_FORMAT);
			SSET_ICON_TITLE_FORMAT_STRING(*ps, format);
			ps->flags.has_icon_title_format_string = 1;
			ps->flag_mask.has_icon_title_format_string = 1;
			ps->change_mask.has_icon_title_format_string = 1;
		}
		else if (StrEquals(token, "EdgeMoveResistance"))
		{
			int num;
			unsigned has_move;
			unsigned has_xinerama_move;

			num = GetIntegerArguments(rest, &rest, val, 2);
			if (num == 1)
			{
				has_move = 1;
				has_xinerama_move = 0;
			}
			else if (num == 2)
			{
				has_move = 1;
				has_xinerama_move = 1;
			}
			else
			{
				val[0] = 0;
				val[1] = 0;
				has_move = 0;
				has_xinerama_move = 0;
			}
			if (val[0] < 0)
			{
				val[0] = 0;
			}
			if (val[1] < 0)
			{
				val[1] = 0;
			}
			ps->flags.has_edge_resistance_move = has_move;
			ps->flag_mask.has_edge_resistance_move = 1;
			ps->change_mask.has_edge_resistance_move = 1;
			SSET_EDGE_RESISTANCE_MOVE(*ps, val[0]);
			ps->flags.has_edge_resistance_xinerama_move =
				has_xinerama_move;
			ps->flag_mask.has_edge_resistance_xinerama_move = 1;
			ps->change_mask.has_edge_resistance_xinerama_move = 1;
			SSET_EDGE_RESISTANCE_XINERAMA_MOVE(*ps, val[1]);
		}
		else if (StrEquals(token, "EdgeMoveDelay"))
		{
			if (GetIntegerArguments(rest, &rest, val, 1))
			{
				SSET_EDGE_DELAY_MS_MOVE(*ps, (short)*val);
				ps->flags.has_edge_delay_ms_move = 1;
				ps->flag_mask.has_edge_delay_ms_move = 1;
				ps->change_mask.has_edge_delay_ms_move = 1;
			}
			else
			{
				ps->flags.has_edge_delay_ms_move = 0;
				ps->flag_mask.has_edge_delay_ms_move = 1;
				ps->change_mask.has_edge_delay_ms_move = 1;
			}
		}
		else if (StrEquals(token, "EdgeResizeDelay"))
		{
			if (GetIntegerArguments(rest, &rest, val, 1))
			{
				SSET_EDGE_DELAY_MS_RESIZE(*ps, (short)*val);
				ps->flags.has_edge_delay_ms_resize = 1;
				ps->flag_mask.has_edge_delay_ms_resize = 1;
				ps->change_mask.has_edge_delay_ms_resize = 1;
			}
			else
			{
				ps->flags.has_edge_delay_ms_resize = 0;
				ps->flag_mask.has_edge_delay_ms_resize = 1;
				ps->change_mask.has_edge_delay_ms_resize = 1;
			}
		}
		else
		{
			found = EWMH_CMD_Style(token, ps, on);
		}
		break;

	case 'f':
		if (strncasecmp(token, "fp", 2) == 0)
		{
			/* parse focus policy options */
			found = style_parse_focus_policy_style(
				token + 2, rest, &rest, (on) ? False : True,
				&S_FOCUS_POLICY(SCF(*ps)),
				&S_FOCUS_POLICY(SCM(*ps)),
				&S_FOCUS_POLICY(SCC(*ps)));
		}
		else if (StrEquals(token, "Font"))
		{
			SAFEFREE(SGET_WINDOW_FONT(*ps));
			rest = GetNextToken(rest, &token);
			SSET_WINDOW_FONT(*ps, token);
			S_SET_HAS_WINDOW_FONT(SCF(*ps), (token != NULL));
			S_SET_HAS_WINDOW_FONT(SCM(*ps), 1);
			S_SET_HAS_WINDOW_FONT(SCC(*ps), 1);

		}
		else if (StrEquals(token, "ForeColor"))
		{
			rest = GetNextToken(rest, &token);
			if (token)
			{
				SAFEFREE(SGET_FORE_COLOR_NAME(*ps));
				SSET_FORE_COLOR_NAME(*ps, token);
				ps->flags.has_color_fore = 1;
				ps->flag_mask.has_color_fore = 1;
				ps->change_mask.has_color_fore = 1;
				ps->flags.use_colorset = 0;
				ps->flag_mask.use_colorset = 1;
				ps->change_mask.use_colorset = 1;
			}
			else
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"ForeColor Style needs color argument"
					);
			}
		}
		else if (StrEquals(token, "FVWMBUTTONS"))
		{
			S_SET_HAS_MWM_BUTTONS(SCF(*ps), !on);
			S_SET_HAS_MWM_BUTTONS(SCM(*ps), 1);
			S_SET_HAS_MWM_BUTTONS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FVWMBORDER"))
		{
			S_SET_HAS_MWM_BORDER(SCF(*ps), !on);
			S_SET_HAS_MWM_BORDER(SCM(*ps), 1);
			S_SET_HAS_MWM_BORDER(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FocusFollowsMouse"))
		{
			style_set_old_focus_policy(ps, 1);
		}
		else if (StrEquals(token, "FirmBorder"))
		{
			S_SET_HAS_DEPRESSABLE_BORDER(SCF(*ps), !on);
			S_SET_HAS_DEPRESSABLE_BORDER(SCM(*ps), 1);
			S_SET_HAS_DEPRESSABLE_BORDER(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FixedPosition") ||
			 StrEquals(token, "FixedUSPosition"))
		{
			S_SET_IS_FIXED(SCF(*ps), on);
			S_SET_IS_FIXED(SCM(*ps), 1);
			S_SET_IS_FIXED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FixedPPosition"))
		{
			S_SET_IS_FIXED_PPOS(SCF(*ps), on);
			S_SET_IS_FIXED_PPOS(SCM(*ps), 1);
			S_SET_IS_FIXED_PPOS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FixedSize") ||
			 StrEquals(token, "FixedUSSize"))
		{
			S_SET_IS_SIZE_FIXED(SCF(*ps), on);
			S_SET_IS_SIZE_FIXED(SCM(*ps), 1);
			S_SET_IS_SIZE_FIXED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "FixedPSize"))
		{
			S_SET_IS_PSIZE_FIXED(SCF(*ps), on);
			S_SET_IS_PSIZE_FIXED(SCM(*ps), 1);
			S_SET_IS_PSIZE_FIXED(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'g':
		if (StrEquals(token, "GrabFocusOff"))
		{
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "GrabFocus"))
		{
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_GRAB_FOCUS(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "GrabFocusTransientOff"))
		{
			FPS_GRAB_FOCUS_TRANSIENT(
				S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_GRAB_FOCUS_TRANSIENT(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_GRAB_FOCUS_TRANSIENT(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "GrabFocusTransient"))
		{
			FPS_GRAB_FOCUS_TRANSIENT(S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_GRAB_FOCUS_TRANSIENT(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_GRAB_FOCUS_TRANSIENT(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'h':
		if (StrEquals(token, "HintOverride"))
		{
			S_SET_HAS_MWM_OVERRIDE(SCF(*ps), on);
			S_SET_HAS_MWM_OVERRIDE(SCM(*ps), 1);
			S_SET_HAS_MWM_OVERRIDE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Handles"))
		{
			ps->flags.has_no_handles = !on;
			ps->flag_mask.has_no_handles = 1;
			ps->change_mask.has_no_handles = 1;
		}
		else if (StrEquals(token, "HandleWidth"))
		{
			if (GetIntegerArguments(rest, &rest, val, 1))
			{
				SSET_HANDLE_WIDTH(*ps, (short)*val);
				ps->flags.has_handle_width = 1;
				ps->flag_mask.has_handle_width = 1;
				ps->change_mask.has_handle_width = 1;
			}
			else
			{
				ps->flags.has_handle_width = 0;
				ps->flag_mask.has_handle_width = 1;
				ps->change_mask.has_handle_width = 1;
			}
		}
		else if (StrEquals(token, "HilightFore"))
		{
			rest = GetNextToken(rest, &token);
			if (token)
			{
				SAFEFREE(SGET_FORE_COLOR_NAME_HI(*ps));
				SSET_FORE_COLOR_NAME_HI(*ps, token);
				ps->flags.has_color_fore_hi = 1;
				ps->flag_mask.has_color_fore_hi = 1;
				ps->change_mask.has_color_fore_hi = 1;
				ps->flags.use_colorset_hi = 0;
				ps->flag_mask.use_colorset_hi = 1;
				ps->change_mask.use_colorset_hi = 1;
			}
			else
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"HilightFore Style needs color"
					" argument");
			}
		}
		else if (StrEquals(token, "HilightBack"))
		{
			rest = GetNextToken(rest, &token);
			if (token)
			{
				SAFEFREE(SGET_BACK_COLOR_NAME_HI(*ps));
				SSET_BACK_COLOR_NAME_HI(*ps, token);
				ps->flags.has_color_back_hi = 1;
				ps->flag_mask.has_color_back_hi = 1;
				ps->change_mask.has_color_back_hi = 1;
				ps->flags.use_colorset_hi = 0;
				ps->flag_mask.use_colorset_hi = 1;
				ps->change_mask.use_colorset_hi = 1;
			}
			else
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"HilightBack Style needs color"
					" argument");
			}
		}
		else if (StrEquals(token, "HilightColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			SSET_COLORSET_HI(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_colorset_hi = (*val >= 0);
			ps->flag_mask.use_colorset_hi = 1;
			ps->change_mask.use_colorset_hi = 1;
		}
		else if (StrEquals(token, "HilightBorderColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			SSET_BORDER_COLORSET_HI(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_border_colorset_hi = (*val >= 0);
			ps->flag_mask.use_border_colorset_hi = 1;
			ps->change_mask.use_border_colorset_hi = 1;
		}
		else if (StrEquals(token, "HilightIconTitleColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			SSET_ICON_TITLE_COLORSET_HI(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_icon_title_colorset_hi = (*val >= 0);
			ps->flag_mask.use_icon_title_colorset_hi = 1;
			ps->change_mask.use_icon_title_colorset_hi = 1;
		}
		else
		{
			found = False;
		}
		break;

	case 'i':
		if (StrEquals(token, "Icon"))
		{
			if (on == 1)
			{
				rest = GetNextToken(rest, &token);

				SAFEFREE(SGET_ICON_NAME(*ps));
				SSET_ICON_NAME(*ps,token);
				ps->flags.has_icon = (token != NULL);
				ps->flag_mask.has_icon = 1;
				ps->change_mask.has_icon = 1;

				S_SET_IS_ICON_SUPPRESSED(SCF(*ps), 0);
				S_SET_IS_ICON_SUPPRESSED(SCM(*ps), 1);
				S_SET_IS_ICON_SUPPRESSED(SCC(*ps), 1);
			}
			else
			{
				S_SET_IS_ICON_SUPPRESSED(SCF(*ps), 1);
				S_SET_IS_ICON_SUPPRESSED(SCM(*ps), 1);
				S_SET_IS_ICON_SUPPRESSED(SCC(*ps), 1);
			}
		}
		else if (StrEquals(token, "IconBackgroundColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest, &rest, val, 1);
			SSET_ICON_BACKGROUND_COLORSET(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_icon_background_colorset = (*val >= 0);
			ps->flag_mask.use_icon_background_colorset = 1;
			ps->change_mask.use_icon_background_colorset = 1;
		}
		else if (StrEquals(token, "IconBackgroundPadding"))
		{
			*val = ICON_BACKGROUND_PADDING;
			GetIntegerArguments(rest, &rest, val, 1);
			if (*val < 0)
			{
				*val = 0;
			}
			else if (*val > 50)
			{
				*val = 50;
			}
			SSET_ICON_BACKGROUND_PADDING(*ps, (unsigned char)*val);
			ps->flags.has_icon_background_padding = 1;
			ps->flag_mask.has_icon_background_padding = 1;
			ps->change_mask.has_icon_background_padding = 1;
		}
		else if (StrEquals(token, "IconBackgroundRelief"))
		{
			*val = ICON_RELIEF_WIDTH;
			GetIntegerArguments(rest, &rest, val, 1);
			if (*val < -50)
			{
				*val = -50;
			}
			else if (*val > 50)
			{
				*val = 50;
			}
			SSET_ICON_BACKGROUND_RELIEF(*ps, (signed char)*val);
			ps->flags.has_icon_background_relief = 1;
			ps->flag_mask.has_icon_background_relief = 1;
			ps->change_mask.has_icon_background_relief = 1;
		}
		else if (StrEquals(token, "IconFont"))
		{
			SAFEFREE(SGET_ICON_FONT(*ps));
			rest = GetNextToken(rest, &token);
			SSET_ICON_FONT(*ps, token);
			S_SET_HAS_ICON_FONT(SCF(*ps), (token != NULL));
			S_SET_HAS_ICON_FONT(SCM(*ps), 1);
			S_SET_HAS_ICON_FONT(SCC(*ps), 1);

		}
		else if (StrEquals(token, "IconOverride"))
		{
			S_SET_ICON_OVERRIDE(SCF(*ps), ICON_OVERRIDE);
			S_SET_ICON_OVERRIDE(SCM(*ps), ICON_OVERRIDE_MASK);
			S_SET_ICON_OVERRIDE(SCC(*ps), ICON_OVERRIDE_MASK);
		}
		else if (StrEquals(token, "IgnoreRestack"))
		{
			S_SET_DO_IGNORE_RESTACK(SCF(*ps), on);
			S_SET_DO_IGNORE_RESTACK(SCM(*ps), 1);
			S_SET_DO_IGNORE_RESTACK(SCC(*ps), 1);
		}
		else if (StrEquals(token, "IconTitle"))
		{
			S_SET_HAS_NO_ICON_TITLE(SCF(*ps), !on);
			S_SET_HAS_NO_ICON_TITLE(SCM(*ps), 1);
			S_SET_HAS_NO_ICON_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "IconTitleFormat"))
		{
			char *fmt_string = NULL;
			(rest != NULL) ? fmt_string = strdup(rest) : NULL;
			rest = NULL; /* Consume the string. */

			if (fmt_string == NULL)
			{
				fmt_string = DEFAULT_TITLE_FORMAT;
			}

			if (!__validate_titleformat_string(fmt_string))
			{
				fvwm_msg(ERR, "style_parse_one_style_option",
					"TitleFormat string invalid:  %s",
					fmt_string);
			}

			SSET_ICON_TITLE_FORMAT_STRING(*ps, fmt_string);
			ps->flags.has_icon_title_format_string = 1;
			ps->flag_mask.has_icon_title_format_string = 1;
			ps->change_mask.has_icon_title_format_string = 1;

		}
		else if (StrEquals(token, "IconTitleColorset"))
		{
			*val = -1;
			GetIntegerArguments(rest,&rest, val, 1);
			SSET_ICON_TITLE_COLORSET(*ps, *val);
			alloc_colorset(*val);
			ps->flags.use_icon_title_colorset = (*val >= 0);
			ps->flag_mask.use_icon_title_colorset = 1;
			ps->change_mask.use_icon_title_colorset = 1;
		}
		else if (StrEquals(token, "IconTitleRelief"))
		{
			*val = ICON_RELIEF_WIDTH;
			GetIntegerArguments(rest, &rest, val, 1);
			if (*val < -50)
			{
				*val = -50;
			}
			else if (*val > 50)
			{
				*val = 50;
			}
			SSET_ICON_TITLE_RELIEF(*ps, (signed char)*val);
			ps->flags.has_icon_title_relief = 1;
			ps->flag_mask.has_icon_title_relief = 1;
			ps->change_mask.has_icon_title_relief = 1;
		}
		else if (StrEquals(token, "IconSize"))
		{
			rest = style_parse_icon_size_style(token, rest, ps);
		}
		else if (StrEquals(token, "IconBox"))
		{
			rest = style_parse_icon_box_style(cur_ib, token, rest,
							  ps);
		} /* end iconbox parameter */
		else if (StrEquals(token, "ICONGRID"))
		{
			rest = style_parse_icon_grid_style(token, rest, ps,
							   *cur_ib);
		}
		else if (StrEquals(token, "ICONFILL"))
		{
			rest = style_parse_icon_fill_style(token, rest, ps,
							   *cur_ib);
		} /* end iconfill */
		else if (StrEquals(token, "IconifyWindowGroups"))
		{
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCF(*ps), on);
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCM(*ps), 1);
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "IconifyWindowGroupsOff"))
		{
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCF(*ps), !on);
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCM(*ps), 1);
			S_SET_DO_ICONIFY_WINDOW_GROUPS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Iconifiable"))
		{
			S_SET_IS_UNICONIFIABLE(SCF(*ps), !on);
			S_SET_IS_UNICONIFIABLE(SCM(*ps), 1);
			S_SET_IS_UNICONIFIABLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "IndexedWindowName"))
		{
			char *format;
			/* TA:  This is being deprecated in favour of the more
			 * generic:
			 *
			 * TitleFormat %n
			 */
			fvwm_msg(WARN, "style_parse_one_style_option",
				"IndexedWindowName is deprecated.  "
				"Converting to use:  TitleFormat %%n (%%t)");
			format = strdup( "%n (%t)" );

			SSET_TITLE_FORMAT_STRING(*ps, format);
			ps->flags.has_title_format_string = 1;
			ps->flag_mask.has_title_format_string = 1;
			ps->change_mask.has_title_format_string = 1;
		}
		else if (StrEquals(token, "IndexedIconName"))
		{
			char *format;
			/* TA:  This is being deprecated in favour of the more
			 * generic:
			 *
			 * TitleFormat %n
			 */
			fvwm_msg(WARN, "style_parse_one_style_option",
				"IndexedIconName is deprecated.  "
				"Converting to use:  IconTitleFormat %%n (%%t)");
			format = strdup( "%n (%t)" );

			SSET_ICON_TITLE_FORMAT_STRING(*ps, format);
			ps->flags.has_icon_title_format_string = 1;
			ps->flag_mask.has_icon_title_format_string = 1;
			ps->change_mask.has_icon_title_format_string = 1;
		}
		else if (StrEquals(token, "InitialMapCommand"))
		{
			char *s;

			s = (rest != NULL) ? strdup(rest) : NULL;
			SSET_INITIAL_MAP_COMMAND_STRING(*ps, s);
			ps->flags.has_initial_map_command_string = on;
			ps->flag_mask.has_initial_map_command_string = on;
			ps->change_mask.has_initial_map_command_string = 1;
			rest = NULL; /* consume the entire string */
		}
		else
		{
			found = False;
		}
		break;

	case 'j':
		if (0)
		{
		}
		else
		{
			found = False;
		}
		break;

	case 'k':
		if (StrEquals(token, "KeepWindowGroupsOnDesk"))
		{
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCF(*ps), on);
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCM(*ps), 1);
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'l':
		if (StrEquals(token, "LeftTitleRotatedCW"))
		{
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCF(*ps), on);
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCM(*ps), 1);
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCC(*ps), 1);
		}
		else if (StrEquals(token, "LeftTitleRotatedCCW"))
		{
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCF(*ps), !on);
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCM(*ps), 1);
			S_SET_IS_LEFT_TITLE_ROTATED_CW(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Lenience"))
		{
			FPS_LENIENT(S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_LENIENT(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_LENIENT(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "Layer"))
		{
			*val = -1;
			if (GetIntegerArguments(rest, &rest, val, 1) &&
			    *val < 0)
			{
				fvwm_msg(ERR, "style_parse_one_style_option",
					 "Layer must be positive or zero.");
			}
			if (*val < 0)
			{
				SSET_LAYER(*ps, -9);
				/* mark layer unset */
				ps->flags.use_layer = 0;
				ps->flag_mask.use_layer = 1;
				ps->change_mask.use_layer = 1;
			}
			else
			{
				SSET_LAYER(*ps, *val);
				ps->flags.use_layer = 1;
				ps->flag_mask.use_layer = 1;
				ps->change_mask.use_layer = 1;
			}
		}
		else if (StrEquals(token, "LOWERTRANSIENT"))
		{
			S_SET_DO_LOWER_TRANSIENT(SCF(*ps), on);
			S_SET_DO_LOWER_TRANSIENT(SCM(*ps), 1);
			S_SET_DO_LOWER_TRANSIENT(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'm':
		if (StrEquals(token, "ManualPlacement"))
		{
			ps->flags.placement_mode = PLACE_MANUAL;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "ManualPlacementHonorsStartsOnPage"))
		{
			ps->flags.manual_placement_honors_starts_on_page = on;
			ps->flag_mask.manual_placement_honors_starts_on_page =
				1;
			ps->change_mask.manual_placement_honors_starts_on_page
				= 1;
		}
		else if (StrEquals(
				 token, "ManualPlacementIgnoresStartsOnPage"))
		{
			ps->flags.manual_placement_honors_starts_on_page = !on;
			ps->flag_mask.manual_placement_honors_starts_on_page =
				1;
			ps->change_mask.manual_placement_honors_starts_on_page
				= 1;
		}
		else if (StrEquals(token, "Maximizable"))
		{
			S_SET_IS_UNMAXIMIZABLE(SCF(*ps), !on);
			S_SET_IS_UNMAXIMIZABLE(SCM(*ps), 1);
			S_SET_IS_UNMAXIMIZABLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "MinOverlapPlacement"))
		{
			ps->flags.placement_mode = PLACE_MINOVERLAP;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "MinOverlapPercentPlacement"))
		{
			ps->flags.placement_mode = PLACE_MINOVERLAPPERCENT;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "MinOverlapPlacementPenalties"))
		{
			float f[6] = {-1, -1, -1, -1, -1, -1};
			Bool bad = False;

			num = 0;
			if (on != 0 && rest != NULL)
			{
				num = sscanf(
					rest, "%f %f %f %f %f %f", &f[0],
					&f[1], &f[2], &f[3], &f[4], &f[5]);
				for (i=0; i < num; i++)
				{
					PeekToken(rest,&rest);
					if (f[i] < 0)
					{
						bad = True;
					}
				}
			}
			if (bad)
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"Bad argument to MinOverlap"
					"PlacementPenalties: %s", rest);
				break;
			}
			{
				pl_penalty_struct *p;

				p = SGET_PLACEMENT_PENALTY_PTR(*ps);
				*p = default_pl_penalty;
			}
			if (num > 0)
			{
				(*ps).pl_penalty.normal = f[0];
			}
			if (num > 1)
			{
				(*ps).pl_penalty.ontop = f[1];
			}
			if (num > 2)
			{
				(*ps).pl_penalty.icon = f[2];
			}
			if (num > 3)
			{
				(*ps).pl_penalty.sticky = f[3];
			}
			if (num > 4)
			{
				(*ps).pl_penalty.below = f[4];
			}
			if (num > 5)
			{
				(*ps).pl_penalty.strut = f[5];
			}
			ps->flags.has_placement_penalty = 1;
			ps->flag_mask.has_placement_penalty = 1;
			ps->change_mask.has_placement_penalty = 1;
		}
		else if (StrEquals(
				 token, "MinOverlapPercentPlacementPenalties"))
		{
			Bool bad = False;

			num = 0;
			if (on != 0)
			{
				num = GetIntegerArguments(rest, &rest, val, 4);
				for (i=0; i < num; i++)
				{
					if (val[i] < 0)
						bad = True;
				}
			}
			if (bad)
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"Bad argument to MinOverlapPercent"
					"PlacementPenalties: %s", rest);
				break;
			}
			{
				pl_percent_penalty_struct *p;

				p = SGET_PLACEMENT_PERCENTAGE_PENALTY_PTR(*ps);
				*p = default_pl_percent_penalty;
			}
			if (num > 0)
			{
				(*ps).pl_percent_penalty.p99 = val[0];
			}
			if (num > 1)
			{
				(*ps).pl_percent_penalty.p95 = val[1];
			}
			if (num > 2)
			{
				(*ps).pl_percent_penalty.p85 = val[2];
			}
			if (num > 3)
			{
				(*ps).pl_percent_penalty.p75 = val[3];
			}
			ps->flags.has_placement_percentage_penalty = 1;
			ps->flag_mask.has_placement_percentage_penalty = 1;
			ps->change_mask.has_placement_percentage_penalty = 1;
		}
		else if (StrEquals(token, "MwmButtons"))
		{
			S_SET_HAS_MWM_BUTTONS(SCF(*ps), on);
			S_SET_HAS_MWM_BUTTONS(SCM(*ps), 1);
			S_SET_HAS_MWM_BUTTONS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "MiniIcon"))
		{
			if (!FMiniIconsSupported)
			{
				break;
			}
			rest = GetNextToken(rest, &token);
			if (token)
			{
				SAFEFREE(SGET_MINI_ICON_NAME(*ps));
				SSET_MINI_ICON_NAME(*ps, token);
				ps->flags.has_mini_icon = 1;
				ps->flag_mask.has_mini_icon = 1;
				ps->change_mask.has_mini_icon = 1;
			}
			else
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"MiniIcon Style requires an Argument");
			}
		}
		else if (StrEquals(token, "MwmBorder"))
		{
			S_SET_HAS_MWM_BORDER(SCF(*ps), on);
			S_SET_HAS_MWM_BORDER(SCM(*ps), 1);
			S_SET_HAS_MWM_BORDER(SCC(*ps), 1);
		}
		else if (StrEquals(token, "MwmDecor"))
		{
			ps->flags.has_mwm_decor = on;
			ps->flag_mask.has_mwm_decor = 1;
			ps->change_mask.has_mwm_decor = 1;
		}
		else if (StrEquals(token, "MwmFunctions"))
		{
			ps->flags.has_mwm_functions = on;
			ps->flag_mask.has_mwm_functions = 1;
			ps->change_mask.has_mwm_functions = 1;
		}
		else if (StrEquals(token, "MouseFocus"))
		{
			style_set_old_focus_policy(ps, 1);
		}
		else if (StrEquals(token, "MouseFocusClickRaises"))
		{
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), on);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "MouseFocusClickRaisesOff"))
		{
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_FOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
				S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "MinWindowSize"))
		{
			int val1;
			int val2;
			int val1_unit;
			int val2_unit;

			num = GetTwoArguments(
				rest, &val1, &val2, &val1_unit, &val2_unit);
			rest = SkipNTokens(rest, num);
			if (num != 2)
			{
				val1 = 0;
				val2 = 0;
			}
			else
			{
				val1 = val1 * val1_unit / 100;
				val2 = val2 * val2_unit / 100;
			}
			if (val1 < 0)
			{
				val1 = 0;
			}
			if (val2 < 0)
			{
				val2 = 0;
			}
			SSET_MIN_WINDOW_WIDTH(*ps, val1);
			SSET_MIN_WINDOW_HEIGHT(*ps, val2);
			ps->flags.has_min_window_size = 1;
			ps->flag_mask.has_min_window_size = 1;
			ps->change_mask.has_min_window_size = 1;
		}
		else if (StrEquals(token, "MaxWindowSize"))
		{
			int val1;
			int val2;
			int val1_unit;
			int val2_unit;

			num = GetTwoArguments(
				rest, &val1, &val2, &val1_unit, &val2_unit);
			rest = SkipNTokens(rest, num);
			if (num != 2)
			{
				val1 = DEFAULT_MAX_MAX_WINDOW_WIDTH;
				val2 = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			}
			else
			{
				val1 = val1 * val1_unit / 100;
				val2 = val2 * val2_unit / 100;
			}
			if (val1 < DEFAULT_MIN_MAX_WINDOW_WIDTH)
			{
				val1 = DEFAULT_MIN_MAX_WINDOW_WIDTH;
			}
			if (val1 > DEFAULT_MAX_MAX_WINDOW_WIDTH || val1 <= 0)
			{
				val1 = DEFAULT_MAX_MAX_WINDOW_WIDTH;
			}
			if (val2 < DEFAULT_MIN_MAX_WINDOW_HEIGHT)
			{
				val2 = DEFAULT_MIN_MAX_WINDOW_HEIGHT;
			}
			if (val2 > DEFAULT_MAX_MAX_WINDOW_HEIGHT || val2 <= 0)
			{
				val2 = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			}
			SSET_MAX_WINDOW_WIDTH(*ps, val1);
			SSET_MAX_WINDOW_HEIGHT(*ps, val2);
			ps->flags.has_max_window_size = 1;
			ps->flag_mask.has_max_window_size = 1;
			ps->change_mask.has_max_window_size = 1;
		}
		else if (StrEquals(token, "MoveByProgramMethod"))
		{
			int i;
			char *methodlist[] = {
				"AutoDetect",
				"UseGravity",
				"IgnoreGravity",
				NULL
			};

			i = GetTokenIndex(rest, methodlist, 0, &rest);
			if (i == -1)
			{
				i = WS_CR_MOTION_METHOD_AUTO;
			}
			SCR_MOTION_METHOD(&ps->flags) = i;
			SCR_MOTION_METHOD(&ps->flag_mask) =
				WS_CR_MOTION_METHOD_MASK;
			SCR_MOTION_METHOD(&ps->change_mask) =
				WS_CR_MOTION_METHOD_MASK;
		}
		else
		{
			found = False;
		}
		break;

	case 'n':
		if (StrEquals(token, "NoActiveIconOverride"))
		{
			S_SET_ICON_OVERRIDE(SCF(*ps), NO_ACTIVE_ICON_OVERRIDE);
			S_SET_ICON_OVERRIDE(SCM(*ps), ICON_OVERRIDE_MASK);
			S_SET_ICON_OVERRIDE(SCC(*ps), ICON_OVERRIDE_MASK);
		}
		else if (StrEquals(token, "NoIconOverride"))
		{
			S_SET_ICON_OVERRIDE(SCF(*ps), NO_ICON_OVERRIDE);
			S_SET_ICON_OVERRIDE(SCM(*ps), ICON_OVERRIDE_MASK);
			S_SET_ICON_OVERRIDE(SCC(*ps), ICON_OVERRIDE_MASK);
		}
		else if (StrEquals(token, "NoIconTitle"))
		{
			S_SET_HAS_NO_ICON_TITLE(SCF(*ps), on);
			S_SET_HAS_NO_ICON_TITLE(SCM(*ps), 1);
			S_SET_HAS_NO_ICON_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "NOICON"))
		{
			S_SET_IS_ICON_SUPPRESSED(SCF(*ps), on);
			S_SET_IS_ICON_SUPPRESSED(SCM(*ps), 1);
			S_SET_IS_ICON_SUPPRESSED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "NOTITLE"))
		{
			ps->flags.has_no_title = on;
			ps->flag_mask.has_no_title = 1;
			ps->change_mask.has_no_title = 1;
		}
		else if (StrEquals(token, "NoPPosition"))
		{
			ps->flags.use_no_pposition = on;
			ps->flag_mask.use_no_pposition = 1;
			ps->change_mask.use_no_pposition = 1;
		}
		else if (StrEquals(token, "NoUSPosition"))
		{
			ps->flags.use_no_usposition = on;
			ps->flag_mask.use_no_usposition = 1;
			ps->change_mask.use_no_usposition = 1;
		}
		else if (StrEquals(token, "NoTransientPPosition"))
		{
			ps->flags.use_no_transient_pposition = on;
			ps->flag_mask.use_no_transient_pposition = 1;
			ps->change_mask.use_no_transient_pposition = 1;
		}
		else if (StrEquals(token, "NoTransientUSPosition"))
		{
			ps->flags.use_no_transient_usposition = on;
			ps->flag_mask.use_no_transient_usposition = 1;
			ps->change_mask.use_no_transient_usposition = 1;
		}
		else if (StrEquals(token, "NoIconPosition"))
		{
			S_SET_USE_ICON_POSITION_HINT(SCF(*ps), !on);
			S_SET_USE_ICON_POSITION_HINT(SCM(*ps), 1);
			S_SET_USE_ICON_POSITION_HINT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "NakedTransient"))
		{
			ps->flags.do_decorate_transient = !on;
			ps->flag_mask.do_decorate_transient = 1;
			ps->change_mask.do_decorate_transient = 1;
		}
		else if (StrEquals(token, "NODECORHINT"))
		{
			ps->flags.has_mwm_decor = !on;
			ps->flag_mask.has_mwm_decor = 1;
			ps->change_mask.has_mwm_decor = 1;
		}
		else if (StrEquals(token, "NOFUNCHINT"))
		{
			ps->flags.has_mwm_functions = !on;
			ps->flag_mask.has_mwm_functions = 1;
			ps->change_mask.has_mwm_functions = 1;
		}
		else if (StrEquals(token, "NOOVERRIDE"))
		{
			S_SET_HAS_MWM_OVERRIDE(SCF(*ps), !on);
			S_SET_HAS_MWM_OVERRIDE(SCM(*ps), 1);
			S_SET_HAS_MWM_OVERRIDE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "NORESIZEOVERRIDE") ||
			 StrEquals(token, "NORESIZEHINTOVERRIDE"))
		{
			S_SET_HAS_OVERRIDE_SIZE(SCF(*ps), !on);
			S_SET_HAS_OVERRIDE_SIZE(SCM(*ps), 1);
			S_SET_HAS_OVERRIDE_SIZE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "NOHANDLES"))
		{
			ps->flags.has_no_handles = on;
			ps->flag_mask.has_no_handles = 1;
			ps->change_mask.has_no_handles = 1;
		}
		else if (StrEquals(token, "NOLENIENCE"))
		{
			FPS_LENIENT(S_FOCUS_POLICY(SCF(*ps)), !on);
			FPS_LENIENT(S_FOCUS_POLICY(SCM(*ps)), 1);
			FPS_LENIENT(S_FOCUS_POLICY(SCC(*ps)), 1);
		}
		else if (StrEquals(token, "NoButton"))
		{
			rest = style_parse_button_style(ps, rest, !on);
		}
		else if (StrEquals(token, "NOOLDECOR"))
		{
			ps->flags.has_ol_decor = !on;
			ps->flag_mask.has_ol_decor = 1;
			ps->change_mask.has_ol_decor = 1;
		}
		else if (StrEquals(token, "NeverFocus"))
		{
			style_set_old_focus_policy(ps, 3);
		}
		else
		{
			found = False;
		}
		break;

	case 'o':
		if (StrEquals(token, "OLDECOR"))
		{
			ps->flags.has_ol_decor = on;
			ps->flag_mask.has_ol_decor = 1;
			ps->change_mask.has_ol_decor = 1;
		}
		else if (StrEquals(token, "Opacity"))
		{
			ps->flags.use_parent_relative = !on;
			ps->flag_mask.use_parent_relative = 1;
			ps->change_mask.use_parent_relative = 1;
		}
		else
		{
			found = False;
		}
		break;

	case 'p':
		if (StrEquals(token, "PositionPlacement"))
		{
			char *s;

			ps->flags.placement_mode = PLACE_POSITION;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
			s = (rest != NULL) ? strdup(rest) : NULL;
			rest = NULL; /* consume the entire string */
			SSET_PLACEMENT_POSITION_STRING(*ps, s);
			ps->flags.has_placement_position_string = 1;
			ps->flag_mask.has_placement_position_string = 1;
			ps->change_mask.has_placement_position_string = 1;
		}
		else if (StrEquals(token, "ParentalRelativity"))
		{
			ps->flags.use_parent_relative = on;
			ps->flag_mask.use_parent_relative = 1;
			ps->change_mask.use_parent_relative = 1;
		}
		else
		{
			found = False;
		}
		break;

	case 'q':
		if (0)
		{
		}
		else
		{
			found = False;
		}
		break;

	case 'r':
		if (StrEquals(token, "RAISETRANSIENT"))
		{
			S_SET_DO_RAISE_TRANSIENT(SCF(*ps), on);
			S_SET_DO_RAISE_TRANSIENT(SCM(*ps), 1);
			S_SET_DO_RAISE_TRANSIENT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "RANDOMPLACEMENT"))
		{
			ps->flags.placement_mode |= PLACE_RANDOM;
			ps->flag_mask.placement_mode |= PLACE_RANDOM;
			ps->change_mask.placement_mode |= PLACE_RANDOM;
		}
		else if (StrEquals(token, "RECAPTUREHONORSSTARTSONPAGE"))
		{
			ps->flags.recapture_honors_starts_on_page = on;
			ps->flag_mask.recapture_honors_starts_on_page = 1;
			ps->change_mask.recapture_honors_starts_on_page = 1;
		}
		else if (StrEquals(token, "RECAPTUREIGNORESSTARTSONPAGE"))
		{
			ps->flags.recapture_honors_starts_on_page = !on;
			ps->flag_mask.recapture_honors_starts_on_page = 1;
			ps->change_mask.recapture_honors_starts_on_page = 1;
		}
		else if (StrEquals(token, "RESIZEHINTOVERRIDE"))
		{
			S_SET_HAS_OVERRIDE_SIZE(SCF(*ps), on);
			S_SET_HAS_OVERRIDE_SIZE(SCM(*ps), 1);
			S_SET_HAS_OVERRIDE_SIZE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "ResizeOpaque"))
		{
			S_SET_DO_RESIZE_OPAQUE(SCF(*ps), on);
			S_SET_DO_RESIZE_OPAQUE(SCM(*ps), 1);
			S_SET_DO_RESIZE_OPAQUE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "ResizeOutline"))
		{
			S_SET_DO_RESIZE_OPAQUE(SCF(*ps), !on);
			S_SET_DO_RESIZE_OPAQUE(SCM(*ps), 1);
			S_SET_DO_RESIZE_OPAQUE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "RightTitleRotatedCW"))
		{
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCF(*ps), on);
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCM(*ps), 1);
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCC(*ps), 1);
		}
		else if (StrEquals(token, "RightTitleRotatedCCW"))
		{
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCF(*ps), !on);
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCM(*ps), 1);
			S_SET_IS_RIGHT_TITLE_ROTATED_CW(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 's':
		if (StrEquals(token, "SMARTPLACEMENT"))
		{
			ps->flags.placement_mode |= PLACE_SMART;
			ps->flag_mask.placement_mode |= PLACE_SMART;
			ps->change_mask.placement_mode |= PLACE_SMART;
		}
		else if (StrEquals(token, "SkipMapping"))
		{
			S_SET_DO_NOT_SHOW_ON_MAP(SCF(*ps), on);
			S_SET_DO_NOT_SHOW_ON_MAP(SCM(*ps), 1);
			S_SET_DO_NOT_SHOW_ON_MAP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "ShowMapping"))
		{
			S_SET_DO_NOT_SHOW_ON_MAP(SCF(*ps), !on);
			S_SET_DO_NOT_SHOW_ON_MAP(SCM(*ps), 1);
			S_SET_DO_NOT_SHOW_ON_MAP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StackTransientParent"))
		{
			S_SET_DO_STACK_TRANSIENT_PARENT(SCF(*ps), on);
			S_SET_DO_STACK_TRANSIENT_PARENT(SCM(*ps), 1);
			S_SET_DO_STACK_TRANSIENT_PARENT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyIcon"))
		{
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCF(*ps), on);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCC(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCF(*ps), on);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyAcrossPagesIcon"))
		{
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCF(*ps), on);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyAcrossDesksIcon"))
		{
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCF(*ps), on);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "SlipperyIcon"))
		{
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCF(*ps), !on);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_PAGES(SCC(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCF(*ps), !on);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_ICON_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "SloppyFocus"))
		{
			style_set_old_focus_policy(ps, 2);
		}
		else if (StrEquals(token, "StartIconic"))
		{
			ps->flags.do_start_iconic = on;
			ps->flag_mask.do_start_iconic = 1;
			ps->change_mask.do_start_iconic = 1;
		}
		else if (StrEquals(token, "StartNormal"))
		{
			ps->flags.do_start_iconic = !on;
			ps->flag_mask.do_start_iconic = 1;
			ps->change_mask.do_start_iconic = 1;
		}
		else if (StrEquals(token, "StaysOnBottom"))
		{
			SSET_LAYER(
				*ps, (on) ? Scr.BottomLayer : Scr.DefaultLayer);
			ps->flags.use_layer = 1;
			ps->flag_mask.use_layer = 1;
			ps->change_mask.use_layer = 1;
		}
		else if (StrEquals(token, "StaysOnTop"))
		{
			SSET_LAYER(
				*ps, (on) ? Scr.BottomLayer : Scr.DefaultLayer);
			SSET_LAYER(*ps, Scr.TopLayer);
			ps->flags.use_layer = 1;
			ps->flag_mask.use_layer = 1;
			ps->change_mask.use_layer = 1;
		}
		else if (StrEquals(token, "StaysPut"))
		{
			SSET_LAYER(*ps, Scr.DefaultLayer);
			ps->flags.use_layer = 1;
			ps->flag_mask.use_layer = 1;
			ps->change_mask.use_layer = 1;
		}
		else if (StrEquals(token, "Sticky"))
		{
			S_SET_IS_STICKY_ACROSS_PAGES(SCF(*ps), on);
			S_SET_IS_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCC(*ps), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCF(*ps), on);
			S_SET_IS_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyAcrossPages"))
		{
			S_SET_IS_STICKY_ACROSS_PAGES(SCF(*ps), on);
			S_SET_IS_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyAcrossDesks"))
		{
			S_SET_IS_STICKY_ACROSS_DESKS(SCF(*ps), on);
			S_SET_IS_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyStippledTitle"))
		{
			S_SET_HAS_NO_STICKY_STIPPLED_TITLE(SCF(*ps), !on);
			S_SET_HAS_NO_STICKY_STIPPLED_TITLE(SCM(*ps), 1);
			S_SET_HAS_NO_STICKY_STIPPLED_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StickyStippledIconTitle"))
		{
			S_SET_HAS_NO_STICKY_STIPPLED_ICON_TITLE(SCF(*ps), !on);
			S_SET_HAS_NO_STICKY_STIPPLED_ICON_TITLE(SCM(*ps), 1);
			S_SET_HAS_NO_STICKY_STIPPLED_ICON_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "Slippery"))
		{
			S_SET_IS_STICKY_ACROSS_PAGES(SCF(*ps), !on);
			S_SET_IS_STICKY_ACROSS_PAGES(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCC(*ps), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCF(*ps), !on);
			S_SET_IS_STICKY_ACROSS_DESKS(SCM(*ps), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "STARTSONDESK"))
		{
			spargs = GetIntegerArguments(rest, NULL, tmpno, 1);
			if (spargs == 1)
			{
				PeekToken(rest,&rest);
				ps->flags.use_start_on_desk = 1;
				ps->flag_mask.use_start_on_desk = 1;
				ps->change_mask.use_start_on_desk = 1;
				/*  RBW - 11/20/1998 - allow for the special
				 * case of -1  */
				SSET_START_DESK(
					*ps, (tmpno[0] > -1) ?
					tmpno[0] + 1 : tmpno[0]);
			}
			else
			{
				fvwm_msg(ERR,"style_parse_one_style_option",
					 "bad StartsOnDesk arg: %s", rest);
			}
		}
		/* StartsOnPage is like StartsOnDesk-Plus */
		else if (StrEquals(token, "STARTSONPAGE"))
		{
			char *ret_rest;
			spargs = GetIntegerArguments(rest, &ret_rest,
						     tmpno, 3);
			if (spargs == 1 || spargs == 3)
			{
				/* We have a desk no., with or without page. */
				/* RBW - 11/20/1998 - allow for the special
				 * case of -1 */
				/* Desk is now actual + 1 */
				SSET_START_DESK(
					*ps, (tmpno[0] > -1) ?
					tmpno[0] + 1 : tmpno[0]);
			}
			if (spargs == 2 || spargs == 3)
			{
				if (spargs == 3)
				{
					/*  RBW - 11/20/1998 - allow for the
					 * special case of -1  */
					SSET_START_PAGE_X(
						*ps, (tmpno[1] > -1) ?
						tmpno[1] + 1 : tmpno[1]);
					SSET_START_PAGE_Y(
						*ps, (tmpno[2] > -1) ?
						tmpno[2] + 1 : tmpno[2]);
				}
				else
				{
					SSET_START_PAGE_X(
						*ps, (tmpno[0] > -1) ?
						tmpno[0] + 1 : tmpno[0]);
					SSET_START_PAGE_Y(
						*ps, (tmpno[1] > -1) ?
						tmpno[1] + 1 : tmpno[1]);
				}
			}
			if (spargs < 1 || spargs > 3)
			{
				fvwm_msg(ERR, "style_parse_one_style_option",
					 "bad StartsOnPage args: %s", rest);
			}
			else
			{
				ps->flags.use_start_on_desk = 1;
				ps->flag_mask.use_start_on_desk = 1;
				ps->change_mask.use_start_on_desk = 1;
			}
			rest = ret_rest;
		}
		else if (StrEquals(token, "STARTSONPAGEINCLUDESTRANSIENTS"))
		{
			ps->flags.use_start_on_page_for_transient = on;
			ps->flag_mask.use_start_on_page_for_transient = 1;
			ps->change_mask.use_start_on_page_for_transient = 1;
		}
		else if (StrEquals(token, "STARTSONPAGEIGNORESTRANSIENTS"))
		{
			ps->flags.use_start_on_page_for_transient = !on;
			ps->flag_mask.use_start_on_page_for_transient = 1;
			ps->change_mask.use_start_on_page_for_transient = 1;
		}
		else if (StrEquals(token, "StartsOnScreen"))
		{
			if (rest)
			{
				tmpno[0] = FScreenGetScreenArgument(rest, 'c');
				PeekToken(rest,&rest);
				ps->flags.use_start_on_screen = 1;
				ps->flag_mask.use_start_on_screen = 1;
				ps->change_mask.use_start_on_screen = 1;
				SSET_START_SCREEN(*ps, tmpno[0]);
			}
			else
			{
				ps->flags.use_start_on_screen = 0;
				ps->flag_mask.use_start_on_screen = 1;
				ps->change_mask.use_start_on_screen = 1;
			}
		}
		else if (StrEquals(token, "STARTSANYWHERE"))
		{
			ps->flags.use_start_on_desk = 0;
			ps->flag_mask.use_start_on_desk = 1;
			ps->change_mask.use_start_on_desk = 1;
		}
		else if (StrEquals(token, "STARTSLOWERED"))
		{
			ps->flags.do_start_lowered = on;
			ps->flag_mask.do_start_lowered = 1;
			ps->change_mask.do_start_lowered = 1;
		}
		else if (StrEquals(token, "STARTSRAISED"))
		{
			ps->flags.do_start_lowered = !on;
			ps->flag_mask.do_start_lowered = 1;
			ps->change_mask.do_start_lowered = 1;
		}
		else if (StrEquals(token, "StartShaded"))
		{
			token = PeekToken(rest, &rest);

			if (token)
			{
				direction_t direction;

				direction = gravity_parse_dir_argument(
					token, &token, DIR_NONE);
				if (direction >= 0 && direction <= DIR_MASK)
				{
					SSET_STARTS_SHADED_DIR(*ps, direction);
				}
				else
				{
					fvwm_msg(
						ERR,
						"style_parse_one_style_option",
						"Option: %s is not valid with"
						" StartShaded", token);
				}
			}
			else
			{
				SSET_STARTS_SHADED_DIR(*ps, DIR_N);
			}
			ps->flags.do_start_shaded = on;
			ps->flag_mask.do_start_shaded = 1;
			ps->change_mask.do_start_shaded = 1;
		}
		else if (StrEquals(token, "SaveUnder"))
		{
			ps->flags.do_save_under = on;
			ps->flag_mask.do_save_under = 1;
			ps->change_mask.do_save_under = 1;
		}
		else if (StrEquals(token, "SaveUnderOff"))
		{
			ps->flags.do_save_under = !on;
			ps->flag_mask.do_save_under = 1;
			ps->change_mask.do_save_under = 1;
		}
		else if (StrEquals(token, "StippledTitle"))
		{
			S_SET_HAS_STIPPLED_TITLE(SCF(*ps), on);
			S_SET_HAS_STIPPLED_TITLE(SCM(*ps), 1);
			S_SET_HAS_STIPPLED_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StippledTitleOff"))
		{
			S_SET_HAS_STIPPLED_TITLE(SCF(*ps), !on);
			S_SET_HAS_STIPPLED_TITLE(SCM(*ps), 1);
			S_SET_HAS_STIPPLED_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "StippledIconTitle"))
		{
			S_SET_HAS_STIPPLED_ICON_TITLE(SCF(*ps), on);
			S_SET_HAS_STIPPLED_ICON_TITLE(SCM(*ps), 1);
			S_SET_HAS_STIPPLED_ICON_TITLE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "ScatterWindowGroups"))
		{
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCF(*ps), !on);
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCM(*ps), 1);
			S_SET_DO_USE_WINDOW_GROUP_HINT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "State"))
		{
			unsigned int mask;
			unsigned int states;

			spargs = GetIntegerArguments(rest, NULL, tmpno, 1);
			if (spargs == 1 && tmpno[0] >= 0 && tmpno[0] <= 31)
			{
				PeekToken(rest,&rest);
				states = S_USER_STATES(SCF(*ps));
				mask = (1 << tmpno[0]);
				if (on)
				{
					states |= mask;
				}
				else
				{
					states &= ~mask;
				}
				S_SET_USER_STATES(SCF(*ps), states);
				S_ADD_USER_STATES(SCM(*ps), mask);
				S_ADD_USER_STATES(SCC(*ps), mask);
			}
			else
			{
				fvwm_msg(
					ERR,"style_parse_one_style_option",
					"bad State arg: %s", rest);
			}
		}
		else if (StrEquals(token, "SnapAttraction"))
		{
			int val;
			char *token;
			int snap_proximity;
			int snap_mode;

			do
			{
				snap_proximity = DEFAULT_SNAP_ATTRACTION;
				snap_mode = DEFAULT_SNAP_ATTRACTION_MODE;
				if (
					GetIntegerArguments(
						rest, &rest, &val, 1) != 1)
				{
					break;
				}
				if (val >= 0)
				{
					snap_proximity = val;
				}
				if (val == 0)
				{
					break;
				}
				token = PeekToken(rest, &rest);
				if (token == NULL)
				{
					break;
				}
				if (StrEquals(token, "All"))
				{
					snap_mode = SNAP_ICONS | SNAP_WINDOWS;
					token = PeekToken(rest, &rest);
				}
				else if (StrEquals(token, "None"))
				{
					snap_mode = SNAP_NONE;
					token = PeekToken(rest, &rest);
				}
				else if (StrEquals(token, "SameType"))
				{
					snap_mode = SNAP_SAME;
					token = PeekToken(rest, &rest);
				}
				else if (StrEquals(token, "Icons"))
				{
					snap_mode = SNAP_ICONS;
					token = PeekToken(rest, &rest);
				}
				else if (StrEquals(token, "Windows"))
				{
					snap_mode = SNAP_WINDOWS;
					token = PeekToken(rest, &rest);
				}
				if (token == NULL)
				{
					break;
				}
				if (StrEquals(token, "Screen"))
				{
					snap_mode |= SNAP_SCREEN;
				}
				else if (StrEquals(token, "ScreenWindows"))
				{
					snap_mode |= SNAP_SCREEN_WINDOWS;
				}
				else if (StrEquals(token, "ScreenIcons"))
				{
					snap_mode |= SNAP_SCREEN_ICONS;
				}
				else if (StrEquals(token, "ScreenAll"))
				{
					snap_mode |= SNAP_SCREEN_ALL;
				}
			} while (0);
			ps->flags.has_snap_attraction = 1;
			ps->flag_mask.has_snap_attraction = 1;
			ps->change_mask.has_snap_attraction = 1;
			SSET_SNAP_PROXIMITY(*ps, snap_proximity);
			SSET_SNAP_MODE(*ps, snap_mode);
		}
		else if (StrEquals(token, "SnapGrid"))
		{
			int num;

			num = GetIntegerArguments(rest, &rest, val, 2);
			if (num != 2)
			{
				val[0] = DEFAULT_SNAP_GRID_X;
				val[1] = DEFAULT_SNAP_GRID_Y;
			}
			if (val[0] < 0)
			{
				val[0] = DEFAULT_SNAP_GRID_X;
			}
			if (val[1] < 0)
			{
				val[1] = DEFAULT_SNAP_GRID_Y;
			}
			ps->flags.has_snap_grid = 1;
			ps->flag_mask.has_snap_grid = 1;
			ps->change_mask.has_snap_grid = 1;
			SSET_SNAP_GRID_X(*ps, val[0]);
			SSET_SNAP_GRID_Y(*ps, val[1]);
		}
		else
		{
			found = False;
		}
		break;

	case 't':
		if (StrEquals(token, "TileCascadePlacement"))
		{
			ps->flags.placement_mode = PLACE_TILECASCADE;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "TileManualPlacement"))
		{
			ps->flags.placement_mode = PLACE_TILEMANUAL;
			ps->flag_mask.placement_mode = PLACE_MASK;
			ps->change_mask.placement_mode = PLACE_MASK;
		}
		else if (StrEquals(token, "Title"))
		{
			ps->flags.has_no_title = !on;
			ps->flag_mask.has_no_title = 1;
			ps->change_mask.has_no_title = 1;
		}
		else if (StrEquals(token, "TitleAtBottom"))
		{
			S_SET_TITLE_DIR(SCF(*ps), DIR_S);
			S_SET_TITLE_DIR(SCM(*ps), DIR_MAJOR_MASK);
			S_SET_TITLE_DIR(SCC(*ps), DIR_MAJOR_MASK);
		}
		else if (StrEquals(token, "TitleAtTop"))
		{
			S_SET_TITLE_DIR(SCF(*ps), DIR_N);
			S_SET_TITLE_DIR(SCM(*ps), DIR_MAJOR_MASK);
			S_SET_TITLE_DIR(SCC(*ps), DIR_MAJOR_MASK);
		}
		else if (StrEquals(token, "TitleAtLeft"))
		{
			S_SET_TITLE_DIR(SCF(*ps), DIR_W);
			S_SET_TITLE_DIR(SCM(*ps), DIR_MAJOR_MASK);
			S_SET_TITLE_DIR(SCC(*ps), DIR_MAJOR_MASK);
		}
		else if (StrEquals(token, "TitleAtRight"))
		{
			S_SET_TITLE_DIR(SCF(*ps), DIR_E);
			S_SET_TITLE_DIR(SCM(*ps), DIR_MAJOR_MASK);
			S_SET_TITLE_DIR(SCC(*ps), DIR_MAJOR_MASK);
		}
		else if (StrEquals(token, "TitleFormat"))
		{
			char *fmt_string = NULL;
			(rest != NULL) ? fmt_string = strdup(rest) : NULL;
			rest = NULL; /* Consume the string. */

			if (fmt_string == NULL)
			{
				fmt_string = DEFAULT_TITLE_FORMAT;
			}

			if (!__validate_titleformat_string(fmt_string))
			{
				fvwm_msg(ERR, "style_parse_one_style_option",
					"TitleFormat string invalid:  %s",
					fmt_string);
			}

			SSET_TITLE_FORMAT_STRING(*ps, fmt_string);
			ps->flags.has_title_format_string = 1;
			ps->flag_mask.has_title_format_string = 1;
			ps->change_mask.has_title_format_string = 1;

		}
		else if (StrEquals(token, "TopTitleRotated"))
		{
			S_SET_IS_TOP_TITLE_ROTATED(SCF(*ps), on);
			S_SET_IS_TOP_TITLE_ROTATED(SCM(*ps), 1);
			S_SET_IS_TOP_TITLE_ROTATED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "TopTitleNotRotated"))
		{
			S_SET_IS_TOP_TITLE_ROTATED(SCF(*ps), !on);
			S_SET_IS_TOP_TITLE_ROTATED(SCM(*ps), 1);
			S_SET_IS_TOP_TITLE_ROTATED(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'u':
		if (StrEquals(token, "UsePPosition"))
		{
			ps->flags.use_no_pposition = !on;
			ps->flag_mask.use_no_pposition = 1;
			ps->change_mask.use_no_pposition = 1;
		}
		else if (StrEquals(token, "UseUSPosition"))
		{
			ps->flags.use_no_usposition = !on;
			ps->flag_mask.use_no_usposition = 1;
			ps->change_mask.use_no_usposition = 1;
		}
		else if (StrEquals(token, "UseTransientPPosition"))
		{
			ps->flags.use_no_transient_pposition = !on;
			ps->flag_mask.use_no_transient_pposition = 1;
			ps->change_mask.use_no_transient_pposition = 1;
		}
		else if (StrEquals(token, "UseTransientUSPosition"))
		{
			ps->flags.use_no_transient_usposition = !on;
			ps->flag_mask.use_no_transient_usposition = 1;
			ps->change_mask.use_no_transient_usposition = 1;
		}
		else if (StrEquals(token, "UseIconPosition"))
		{
			S_SET_USE_ICON_POSITION_HINT(SCF(*ps), on);
			S_SET_USE_ICON_POSITION_HINT(SCM(*ps), 1);
			S_SET_USE_ICON_POSITION_HINT(SCC(*ps), 1);
		}
		else if (StrEquals(token, "UseTitleDecorRotation"))
		{
			S_SET_USE_TITLE_DECOR_ROTATION(SCF(*ps), on);
			S_SET_USE_TITLE_DECOR_ROTATION(SCM(*ps), 1);
			S_SET_USE_TITLE_DECOR_ROTATION(SCC(*ps), 1);
		}
		else if (StrEquals(token, "UseDecor"))
		{
			SAFEFREE(SGET_DECOR_NAME(*ps));
			rest = GetNextToken(rest, &token);
			SSET_DECOR_NAME(*ps, token);
			ps->flags.has_decor = (token != NULL);
			ps->flag_mask.has_decor = 1;
			ps->change_mask.has_decor = 1;
		}
		else if (StrEquals(token, "UseStyle"))
		{
			int hit;

			token = PeekToken(rest, &rest);
			if (!token)
			{
				fvwm_msg(ERR, "style_parse_one_style_option",
					 "UseStyle needs an argument");
				break;
			}
			hit = 0;
			/* changed to accum multiple Style definitions
			 * (veliaa@rpi.edu) */
			for (add_style = all_styles; add_style;
			     add_style = SGET_NEXT_STYLE(*add_style))
			{
				if (SGET_ID_HAS_NAME(*add_style) &&
				    StrEquals(token, SGET_NAME(*add_style)))
				{
					/* match style */
					hit = 1;
					merge_styles(ps, add_style, True);
				} /* end found matching style */
			} /* end looking at all styles */
			/* move forward one word */
			if (!hit)
			{
				fvwm_msg(
					ERR, "style_parse_one_style_option",
					"UseStyle: %s style not found", token);
			}
		}
		else if (StrEquals(token, "Unmanaged"))
		{
			ps->flags.is_unmanaged = on;
			ps->flag_mask.is_unmanaged = 1;
			ps->change_mask.is_unmanaged = 1;
		}
		else
		{
			found = False;
		}
		break;

	case 'v':
		if (StrEquals(token, "VariablePosition") ||
		    StrEquals(token, "VariableUSPosition"))
		{
			S_SET_IS_FIXED(SCF(*ps), !on);
			S_SET_IS_FIXED(SCM(*ps), 1);
			S_SET_IS_FIXED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "VariablePPosition"))
		{
			S_SET_IS_FIXED_PPOS(SCF(*ps), !on);
			S_SET_IS_FIXED_PPOS(SCM(*ps), 1);
			S_SET_IS_FIXED_PPOS(SCC(*ps), 1);
		}
		else if (StrEquals(token, "VariableSize") ||
			 StrEquals(token, "VariableUSSize"))
		{
			S_SET_IS_SIZE_FIXED(SCF(*ps), !on);
			S_SET_IS_SIZE_FIXED(SCM(*ps), 1);
			S_SET_IS_SIZE_FIXED(SCC(*ps), 1);
		}
		else if (StrEquals(token, "VariablePSize"))
		{
			S_SET_IS_PSIZE_FIXED(SCF(*ps), !on);
			S_SET_IS_PSIZE_FIXED(SCM(*ps), 1);
			S_SET_IS_PSIZE_FIXED(SCC(*ps), 1);
		}
		else
		{
			found = False;
		}
		break;

	case 'w':
		if (StrEquals(token, "WindowListSkip"))
		{
			S_SET_DO_WINDOW_LIST_SKIP(SCF(*ps), on);
			S_SET_DO_WINDOW_LIST_SKIP(SCM(*ps), 1);
			S_SET_DO_WINDOW_LIST_SKIP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "WindowListHit"))
		{
			S_SET_DO_WINDOW_LIST_SKIP(SCF(*ps), !on);
			S_SET_DO_WINDOW_LIST_SKIP(SCM(*ps), 1);
			S_SET_DO_WINDOW_LIST_SKIP(SCC(*ps), 1);
		}
		else if (StrEquals(token, "WindowShadeSteps"))
		{
			int n = 0;
			int val = 0;
			int unit = 0;

			n = GetOnePercentArgument(rest, &val, &unit);
			if (n != 1)
			{
				val = 0;
			}
			else
			{
				PeekToken(rest,&rest);
			}
			/* we have a 'pixel' suffix if unit != 0; negative
			 * values mean pixels */
			val = (unit != 0) ? -val : val;
			ps->flags.has_window_shade_steps = 1;
			ps->flag_mask.has_window_shade_steps = 1;
			ps->change_mask.has_window_shade_steps = 1;
			SSET_WINDOW_SHADE_STEPS(*ps, val);
		}
		else if (StrEquals(token, "WindowShadeScrolls"))
		{
			S_SET_DO_SHRINK_WINDOWSHADE(SCF(*ps), !on);
			S_SET_DO_SHRINK_WINDOWSHADE(SCM(*ps), 1);
			S_SET_DO_SHRINK_WINDOWSHADE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "WindowShadeShrinks"))
		{
			S_SET_DO_SHRINK_WINDOWSHADE(SCF(*ps), on);
			S_SET_DO_SHRINK_WINDOWSHADE(SCM(*ps), 1);
			S_SET_DO_SHRINK_WINDOWSHADE(SCC(*ps), 1);
		}
		else if (StrEquals(token, "WindowShadeLazy"))
		{
			S_SET_WINDOWSHADE_LAZINESS(SCF(*ps), WINDOWSHADE_LAZY);
			S_SET_WINDOWSHADE_LAZINESS(
				SCM(*ps), WINDOWSHADE_LAZY_MASK);
			S_SET_WINDOWSHADE_LAZINESS(
				SCC(*ps), WINDOWSHADE_LAZY_MASK);
		}
		else if (StrEquals(token, "WindowShadeAlwaysLazy"))
		{
			S_SET_WINDOWSHADE_LAZINESS(
				SCF(*ps), WINDOWSHADE_ALWAYS_LAZY);
			S_SET_WINDOWSHADE_LAZINESS(
				SCM(*ps), WINDOWSHADE_LAZY_MASK);
			S_SET_WINDOWSHADE_LAZINESS(
				SCC(*ps), WINDOWSHADE_LAZY_MASK);
		}
		else if (StrEquals(token, "WindowShadeBusy"))
		{
			S_SET_WINDOWSHADE_LAZINESS(SCF(*ps), WINDOWSHADE_BUSY);
			S_SET_WINDOWSHADE_LAZINESS(
				SCM(*ps), WINDOWSHADE_LAZY_MASK);
			S_SET_WINDOWSHADE_LAZINESS(
				SCC(*ps), WINDOWSHADE_LAZY_MASK);
		}
		else
		{
			found = False;
		}
		break;

	case 'x':
	case 'y':
	case 'z':
		if (0)
		{
		}
		else
		{
			found = False;
		}
		break;

	default:
		found = False;
		break;
	}
	if (ret_rest)
	{
		*ret_rest = rest;
	}
	if (token_l != NULL)
	{
		free(token_l);
	}

	return found;
}

static
void parse_and_set_window_style(char *action, char *prefix, window_style *ps)
{
	char *option;
	char *token;
	char *rest;
	Bool found;
	/* which current boxes to chain to */
	icon_boxes *cur_ib = NULL;

	while (isspace((unsigned char)*action))
	{
		action++;
	}

	while (action && *action && *action != '\n')
	{
		action = GetNextFullOption(action, &option);
		if (!option)
		{
			break;
		}
		token = PeekToken(option, &rest);
		if (!token)
		{
			free(option);
			break;
		}

		/* It might make more sense to capture the whole word, fix its
		 * case, and use strcmp, but there aren't many caseless compares
		 * because of this "switch" on the first letter. */
		found = style_parse_one_style_option(
			token, rest, &rest, prefix, ps, &cur_ib);

		if (found == False)
		{
			fvwm_msg(
				ERR, "style_parse_and_set_window_style",
				"Bad style option: %s", option);
			/* Can't return here since all malloced memory will be
			 * lost. Ignore rest of line instead. */
			/* No, I think we /can/ return here. In fact, /not/
			 * bombing out leaves a half-done style in the list!
			 * N.Bird 07-Sep-1999 */
			/* domivogt (01-Oct-1999): Which is exactly what we
			 * want! Why should all the styles be thrown away if a
			 * single one is mis-spelled? Let's just continue
			 * parsing styles. */
		}
		else if (rest != NULL)
		{
			rest = SkipSpaces(rest,NULL,0);
			if (*rest)
			{
				fvwm_msg(WARN,
					 "style_parse_and_set_window_style",
					 "Unconsumed argument in %s: %s",
					 option, rest);
			}
		}
		free(option);
	} /* end while still stuff on command */

	return;
}

/* Process a style command.  First built up in a temp area.
 * If valid, added to the list in a malloced area.
 *
 *		      *** Important note ***
 *
 * Remember that *all* styles need a flag, flag_mask and change_mask.
 * It is not enough to add the code for new styles in this function.
 * There *must* be corresponding code in handle_new_window_style()
 * and merge_styles() too.  And don't forget that allocated memory
 * must be freed in ProcessDestroyStyle().
 */

static void __style_command(F_CMD_ARGS, char *prefix, Bool is_window_style)
{
	/* temp area to build name list */
	window_style *ps;

	ps = xcalloc(1, sizeof(window_style));
	/* init default focus policy */
	fpol_init_default_fp(&S_FOCUS_POLICY(SCF(*ps)));
	/* mark style as changed */
	ps->has_style_changed = 1;
	/* set global flag */
	Scr.flags.do_need_window_update = 1;
	/* default StartsOnPage behavior for initial capture */
	ps->flags.capture_honors_starts_on_page = 1;

	if (!is_window_style)
	{
		/* parse style name */
		action = GetNextToken(action, &SGET_NAME(*ps));
		/* in case there was no argument! */
		if (SGET_NAME(*ps) == NULL)
		{
			free(ps);
			return;
		}
		SSET_ID_HAS_NAME(*ps, True);
	}
	else
	{
		SSET_WINDOW_ID(*ps, (XID)FW_W(exc->w.fw));
		SSET_ID_HAS_WINDOW_ID(*ps, True);
		CopyString(&SGET_NAME(*ps), ""); /* safe */
	}

	if (action == NULL)
	{
		free(SGET_NAME(*ps));
		free(ps);
		return;
	}

	parse_and_set_window_style(action, prefix, ps);

	/* capture default icons */
	if (SGET_ID_HAS_NAME(*ps) && StrEquals(SGET_NAME(*ps), "*"))
	{
		if (ps->flags.has_icon == 1)
		{
			if (Scr.DefaultIcon)
			{
				free(Scr.DefaultIcon);
			}
			Scr.DefaultIcon = SGET_ICON_NAME(*ps);
			ps->flags.has_icon = 0;
			ps->flag_mask.has_icon = 0;
			ps->change_mask.has_icon = 1;
			SSET_ICON_NAME(*ps, NULL);
		}
	}
	if (last_style_in_list && styles_have_same_id(ps, last_style_in_list))
	{
		/* merge with previous style */
		merge_styles(last_style_in_list, ps, True);
		free_style(ps);
		free(ps);
	}
	else
	{
		/* add temp name list to list */
		add_style_to_list(ps);
		cleanup_style_defaults(ps);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* Compare two flag structures passed as byte arrays. Only compare bits set in
 * the mask.
 *
 *  Returned Value:
 *	zero if the flags are the same
 *	non-zero otherwise
 *
 *  Inputs:
 *	flags1 - first byte array of flags to compare
 *	flags2 - second byte array of flags to compare
 *	mask   - byte array of flags to be considered for the comparison
 *	len    - number of bytes to compare */
Bool blockcmpmask(char *blk1, char *blk2, char *mask, int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		if ((blk1[i] & mask[i]) != (blk2[i] & mask[i]))
		{
			/* flags are not the same, return 1 */
			return False;
		}
	}
	return True;
}

void free_icon_boxes(icon_boxes *ib)
{
	icon_boxes *temp;

	for ( ; ib != NULL; ib = temp)
	{
		temp = ib->next;
		if (ib->use_count == 0)
		{
			free(ib);
		}
		else
		{
			/* we can't delete the icon box yet, it is still in use
			 */
			ib->is_orphan = True;
		}
	}

	return;
}

void simplify_style_list(void)
{
	/* one pass through the style list, then process other events first */
	Scr.flags.do_need_style_list_update = __simplify_style_list();

	return;
}

/* lookup_style - look through a list for a window name, or class
 *
 *  Returned Value:
 *	merged matching styles in callers window_style.
 *
 *  Inputs:
 *	fw     - FvwmWindow structure to match against
 *	styles - callers return area
 */
void lookup_style(FvwmWindow *fw, window_style *styles)
{
	window_style *nptr;

	/* clear callers return area */
	memset(styles, 0, sizeof(window_style));

	/* look thru all styles in order defined. */
	for (nptr = all_styles; nptr != NULL; nptr = SGET_NEXT_STYLE(*nptr))
	{
		if (fw_match_style_id(fw, SGET_ID(*nptr)))
		{
			merge_styles(styles, nptr, False);
		}
	}
	EWMH_GetStyle(fw, styles);

	return;
}

/* This function sets the style update flags as necessary */
void check_window_style_change(
	FvwmWindow *t, update_win *flags, window_style *ret_style)
{
	int i;
	char *wf;
	char *sf;
	char *sc;

	lookup_style(t, ret_style);
	if (!ret_style->has_style_changed && !IS_STYLE_DELETED(t))
	{
		/* nothing to do */
		return;
	}

	/*** common style flags ***/
	wf = (char *)(&FW_COMMON_STATIC_FLAGS(t));
	sf = (char *)(&SCFS(*ret_style));
	if (IS_STYLE_DELETED(t))
	{
		/* update all styles */
		memset(flags, 0xff, sizeof(*flags));
		SET_STYLE_DELETED(t, 0);
		/* copy the static common window flags */
		for (i = 0; i < sizeof(SCFS(*ret_style)); i++)
		{
			wf[i] = sf[i];
		}

		return;
	}
	/* All static common styles can simply be copied. For some there is
	 * additional work to be done below. */
	sc = (char *)(&SCCS(*ret_style));
	for (i = 0; i < sizeof(SCFS(*ret_style)); i++)
	{
		wf[i] = (wf[i] & ~sc[i]) | (sf[i] & sc[i]);
		sf[i] = wf[i];
	}

	/* is_sticky
	 * is_icon_sticky */
	if (S_IS_STICKY_ACROSS_PAGES(SCC(*ret_style)) ||
	    S_IS_STICKY_ACROSS_DESKS(SCC(*ret_style)))
	{
		flags->do_update_stick = 1;
	}
	else if (S_IS_ICON_STICKY_ACROSS_PAGES(SCC(*ret_style)) &&
		 IS_ICONIFIED(t) && !IS_STICKY_ACROSS_PAGES(t))
	{
		flags->do_update_stick_icon = 1;
	}
	else if (S_IS_ICON_STICKY_ACROSS_DESKS(SCC(*ret_style)) &&
		 IS_ICONIFIED(t) && !IS_STICKY_ACROSS_DESKS(t))
	{
		flags->do_update_stick_icon = 1;
	}

	/* focus policy */
	if (fpol_is_policy_changed(&S_FOCUS_POLICY(SCC(*ret_style))))
	{
		flags->do_setup_focus_policy = 1;
	}

	/* is_left_title_rotated_cw
	 * is_right_title_rotated_cw
	 * is_top_title_rotated
	 * is_bottom_title_rotated */
	if (S_IS_LEFT_TITLE_ROTATED_CW(SCC(*ret_style)) ||
	    S_IS_RIGHT_TITLE_ROTATED_CW(SCC(*ret_style)) ||
	    S_IS_TOP_TITLE_ROTATED(SCC(*ret_style)) ||
	    S_IS_BOTTOM_TITLE_ROTATED(SCC(*ret_style)))
	{
		flags->do_update_title_text_dir = 1;
	}

	/* title_dir */
	if (S_TITLE_DIR(SCC(*ret_style)))
	{
		flags->do_update_title_dir = 1;
	}

	/* use_title_decor_rotation */
	if (S_USE_TITLE_DECOR_ROTATION(SCC(*ret_style)))
	{
		flags->do_update_rotated_title = 1;
	}

	/* has_mwm_border
	 * has_mwm_buttons */
	if (S_HAS_MWM_BORDER(SCC(*ret_style)) ||
	    S_HAS_MWM_BUTTONS(SCC(*ret_style)))
	{
		flags->do_redecorate = 1;
	}

	/* has_icon_font */
	if (S_HAS_ICON_FONT(SCC(*ret_style)))
	{
		flags->do_update_icon_font = 1;
	}

	/* has_window_font */
	if (S_HAS_WINDOW_FONT(SCC(*ret_style)))
	{
		flags->do_update_window_font = 1;
	}

	/* has_stippled_title */
	if (S_HAS_STIPPLED_TITLE(SCC(*ret_style)) ||
	    S_HAS_NO_STICKY_STIPPLED_TITLE(SCC(*ret_style)) ||
	    S_HAS_STIPPLED_ICON_TITLE(SCC(*ret_style)) ||
	    S_HAS_NO_STICKY_STIPPLED_ICON_TITLE(SCC(*ret_style)))
	{
		flags->do_redraw_decoration = 1;
	}

	/* has_no_icon_title
	 * is_icon_suppressed
	 *
	 * handled below */

	/*** private style flags ***/

	/* nothing to do for these flags (only used when mapping new windows):
	 *
	 *   do_place_random
	 *   do_place_smart
	 *   do_start_lowered
	 *   use_no_pposition
	 *   use_no_usposition
	 *   use_no_transient_pposition
	 *   use_no_transient_usposition
	 *   use_start_on_desk
	 *   use_start_on_page_for_transient
	 *   use_start_on_screen
	 *   manual_placement_honors_starts_on_page
	 *   capture_honors_starts_on_page
	 *   recapture_honors_starts_on_page
	 *   ewmh_placement_mode
	 */

	/* not implemented yet:
	 *
	 *   handling the 'usestyle' style
	 */

	/* do_window_list_skip */
	if (S_DO_WINDOW_LIST_SKIP(SCC(*ret_style)))
	{
		flags->do_update_modules_flags = 1;
		flags->do_update_ewmh_state_hints = 1;
	}

	/* has_icon
	 * icon_override */
	if (ret_style->change_mask.has_icon ||
	    S_ICON_OVERRIDE(SCC(*ret_style)))
	{
		flags->do_update_icon_font = 1;
		flags->do_update_icon = 1;
	}

	/* has_icon_background_padding
	 * has_icon_background_relief
	 * has_icon_title_relief */
	if (ret_style->change_mask.has_icon_background_padding ||
	    ret_style->change_mask.has_icon_background_relief ||
	    ret_style->change_mask.has_icon_title_relief)
	{
		flags->do_update_icon = 1;
	}

	/* has_no_icon_title
	 * is_icon_suppressed */
	if (S_HAS_NO_ICON_TITLE(SCC(*ret_style)) ||
	    S_IS_ICON_SUPPRESSED(SCC(*ret_style)))
	{
		flags->do_update_icon_font = 1;
		flags->do_update_icon_title = 1;
		flags->do_update_icon = 1;
		flags->do_update_modules_flags = 1;
	}

	/* has_icon_size_limits */
	if (ret_style->change_mask.has_icon_size_limits)
	{
		flags->do_update_icon_size_limits = 1;
		flags->do_update_icon = 1;
	}

	/*   has_icon_boxes */
	if (ret_style->change_mask.has_icon_boxes)
	{
		flags->do_update_icon_boxes = 1;
		flags->do_update_icon = 1;
	}

	/* do_ewmh_donate_icon */
	if (S_DO_EWMH_DONATE_ICON(SCC(*ret_style)))
	{
		flags->do_update_ewmh_icon = 1;
	}

	/* has_mini_icon
	 * do_ewmh_mini_icon_override */
	if (
		FMiniIconsSupported && (
			ret_style->change_mask.has_mini_icon ||
			S_DO_EWMH_MINI_ICON_OVERRIDE(SCC(*ret_style))))
	{
		flags->do_update_mini_icon = 1;
		flags->do_update_ewmh_mini_icon = 1;
		flags->do_redecorate = 1;
	}

	/* do_ewmh_donate_mini_icon */
	if (FMiniIconsSupported && S_DO_EWMH_DONATE_MINI_ICON(SCC(*ret_style)))
	{
		flags->do_update_ewmh_mini_icon = 1;
	}

	/* has_min_window_size */
	/* has_max_window_size */
	if (ret_style->change_mask.has_min_window_size)
	{
		flags->do_resize_window = 1;
		flags->do_update_ewmh_allowed_actions = 1;
		flags->do_update_modules_flags = 1;
	}
	if (ret_style->change_mask.has_max_window_size)
	{
		flags->do_resize_window = 1;
		flags->do_update_ewmh_allowed_actions = 1;
		flags->do_update_modules_flags = 1;
	}

	/* has_color_back
	 * has_color_fore
	 * use_colorset
	 * use_border_colorset */
	if (ret_style->change_mask.has_color_fore ||
	    ret_style->change_mask.has_color_back ||
	    ret_style->change_mask.use_colorset ||
	    ret_style->change_mask.use_border_colorset)
	{
		flags->do_update_window_color = 1;
	}
	/* has_color_back_hi
	 * has_color_fore_hi
	 * use_colorset_hi
	 * use_border_colorset_hi */
	if (ret_style->change_mask.has_color_fore_hi ||
	    ret_style->change_mask.has_color_back_hi ||
	    ret_style->change_mask.use_colorset_hi ||
	    ret_style->change_mask.use_border_colorset_hi)
	{
		flags->do_update_window_color_hi = 1;
	}

	/* use_icon_title_colorset */
	if (ret_style->change_mask.use_icon_title_colorset)
	{
		flags->do_update_icon_title_cs = 1;
	}

	/* use_icon_title_colorset_hi */
	if (ret_style->change_mask.use_icon_title_colorset_hi)
	{
		flags->do_update_icon_title_cs_hi = 1;
	}

	/* use_icon_title_colorset */
	if (ret_style->change_mask.use_icon_title_colorset)
	{
		flags->do_update_icon_title_cs = 1;
	}

	/* use_icon_background_colorset */
	if (ret_style->change_mask.use_icon_background_colorset)
	{
		flags->do_update_icon_background_cs = 1;
	}

	/* has_decor */
	if (ret_style->change_mask.has_decor)
	{
		flags->do_redecorate = 1;
		flags->do_update_window_font_height = 1;
	}

	/* has_no_title */
	if (ret_style->change_mask.has_no_title)
	{
		flags->do_redecorate = 1;
		flags->do_update_window_font = 1;
	}

	/* do_decorate_transient */
	if (ret_style->change_mask.do_decorate_transient)
	{
		flags->do_redecorate_transient = 1;
	}

	/* has_ol_decor */
	if (ret_style->change_mask.has_ol_decor)
	{
		/* old decor overrides 'has_no_icon_title'! */
		flags->do_update_icon_font = 1;
		flags->do_update_icon_title = 1;
		flags->do_update_icon = 1;
		flags->do_redecorate = 1;
	}

	/* Changing layer. */
	if (ret_style->change_mask.use_layer)
	{
		flags->do_update_layer = 1;
	}

	/* has_no_border
	 * has_border_width
	 * has_handle_width
	 * has_mwm_decor
	 * has_mwm_functions
	 * has_no_handles
	 * is_button_disabled */
	if (S_HAS_NO_BORDER(SCC(*ret_style)) ||
	    ret_style->change_mask.has_border_width ||
	    ret_style->change_mask.has_handle_width ||
	    ret_style->change_mask.has_mwm_decor ||
	    ret_style->change_mask.has_mwm_functions ||
	    ret_style->change_mask.has_no_handles ||
	    ret_style->change_mask.is_button_disabled)
	{
		flags->do_redecorate = 1;
		flags->do_update_ewmh_allowed_actions = 1;
		flags->do_update_modules_flags = 1;
	}

	if (ret_style->change_mask.do_save_under ||
	    ret_style->change_mask.use_backing_store ||
	    ret_style->change_mask.use_parent_relative)
	{
		flags->do_update_frame_attributes = 1;
	}

	if (ret_style->change_mask.use_parent_relative &&
	    ret_style->flags.use_parent_relative)
	{
		/* needed only for Opacity -> ParentalRelativity */
		flags->do_refresh = 1;
	}

	/* has_placement_penalty
	 * has_placement_percentage_penalty */
	if (ret_style->change_mask.has_placement_penalty ||
	    ret_style->change_mask.has_placement_percentage_penalty)
	{
		flags->do_update_placement_penalty = 1;
	}

	/* do_ewmh_ignore_strut_hints */
	if (S_DO_EWMH_IGNORE_STRUT_HINTS(SCC(*ret_style)))
	{
		flags->do_update_working_area = 1;
	}

	/* do_ewmh_ignore_state_hints */
	if (S_DO_EWMH_IGNORE_STATE_HINTS(SCC(*ret_style)))
	{
		flags->do_update_ewmh_state_hints = 1;
		flags->do_update_modules_flags = 1;
	}

	/* do_ewmh_use_staking_hints */
	if (S_DO_EWMH_USE_STACKING_HINTS(SCC(*ret_style)))
	{
		flags->do_update_ewmh_stacking_hints = 1;
	}

	/* has_title_format_string */
	if (ret_style->change_mask.has_title_format_string)
	{
		flags->do_update_visible_window_name = 1;
		flags->do_redecorate = 1;
	}

	/* has_icon_title_format_string */
	if (ret_style->change_mask.has_icon_title_format_string)
	{
		flags->do_update_visible_icon_name = 1;
		flags->do_update_icon_title = 1;
	}

	/*  is_fixed */
	if (S_IS_FIXED(SCC(*ret_style)) ||
	    S_IS_FIXED_PPOS(SCC(*ret_style)) ||
	    S_IS_SIZE_FIXED(SCC(*ret_style)) ||
	    S_IS_PSIZE_FIXED(SCC(*ret_style)) ||
	    S_HAS_OVERRIDE_SIZE(SCC(*ret_style)))
	{
		flags->do_update_ewmh_allowed_actions = 1;
		flags->do_update_modules_flags = 1;
	}

	/* cr_motion_method */
	if (SCR_MOTION_METHOD(&ret_style->change_mask))
	{
		flags->do_update_cr_motion_method = 1;
	}

	return;
}

/* Mark all styles as unchanged. */
void reset_style_changes(void)
{
	window_style *temp;

	for (temp = all_styles; temp != NULL; temp = SGET_NEXT_STYLE(*temp))
	{
		temp->has_style_changed = 0;
		memset(&SCCS(*temp), 0, sizeof(SCCS(*temp)));
		memset(&(temp->change_mask), 0, sizeof(temp->change_mask));
	}

	return;
}

/* Mark styles as updated if their colorset changed. */
void update_style_colorset(int colorset)
{
	window_style *temp;

	for (temp = all_styles; temp != NULL; temp = SGET_NEXT_STYLE(*temp))
	{
		if (SUSE_COLORSET(&temp->flags) &&
		    SGET_COLORSET(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_colorset = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_COLORSET_HI(&temp->flags) &&
		    SGET_COLORSET_HI(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_colorset_hi = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_BORDER_COLORSET(&temp->flags) &&
		    SGET_BORDER_COLORSET(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_border_colorset = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_BORDER_COLORSET_HI(&temp->flags) &&
		    SGET_BORDER_COLORSET_HI(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_border_colorset_hi = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_ICON_TITLE_COLORSET(&temp->flags) &&
		    SGET_ICON_TITLE_COLORSET(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_icon_title_colorset = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_ICON_TITLE_COLORSET_HI(&temp->flags) &&
		    SGET_ICON_TITLE_COLORSET_HI(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_icon_title_colorset_hi = 1;
			Scr.flags.do_need_window_update = 1;
		}
		if (SUSE_ICON_BACKGROUND_COLORSET(&temp->flags) &&
		    SGET_ICON_BACKGROUND_COLORSET(*temp) == colorset)
		{
			temp->has_style_changed = 1;
			temp->change_mask.use_icon_background_colorset = 1;
			Scr.flags.do_need_window_update = 1;
		}
	}

	return;
}

/* Update fore and back colours for a specific window */
void update_window_color_style(FvwmWindow *fw, window_style *pstyle)
{
	int cs = Scr.DefaultColorset;

	if (SUSE_COLORSET(&pstyle->flags))
	{
		cs = SGET_COLORSET(*pstyle);
		fw->cs = cs;
	}
	else
	{
		fw->cs = -1;
	}
	if (SGET_FORE_COLOR_NAME(*pstyle) != NULL &&
	    !SUSE_COLORSET(&pstyle->flags))
	{
		fw->colors.fore = GetColor(SGET_FORE_COLOR_NAME(*pstyle));
	}
	else
	{
		fw->colors.fore = Colorset[cs].fg;
	}
	if (SGET_BACK_COLOR_NAME(*pstyle) != NULL &&
	    !SUSE_COLORSET(&pstyle->flags))
	{
		fw->colors.back = GetColor(SGET_BACK_COLOR_NAME(*pstyle));
		fw->colors.shadow = GetShadow(fw->colors.back);
		fw->colors.hilight = GetHilite(fw->colors.back);
	}
	else
	{
		fw->colors.hilight = Colorset[cs].hilite;
		fw->colors.shadow = Colorset[cs].shadow;
		fw->colors.back = Colorset[cs].bg;
	}
	if (SUSE_BORDER_COLORSET(&pstyle->flags))
	{
		cs = SGET_BORDER_COLORSET(*pstyle);
		fw->border_cs = cs;
		fw->border_colors.hilight = Colorset[cs].hilite;
		fw->border_colors.shadow = Colorset[cs].shadow;
		fw->border_colors.back = Colorset[cs].bg;
	}
	else
	{
		fw->border_cs = -1;
		fw->border_colors.hilight = fw->colors.hilight;
		fw->border_colors.shadow = fw->colors.shadow;
		fw->border_colors.back = fw->colors.back;
	}
}

void update_window_color_hi_style(FvwmWindow *fw, window_style *pstyle)
{
	int cs = Scr.DefaultColorset;

	if (SUSE_COLORSET_HI(&pstyle->flags))
	{
		cs = SGET_COLORSET_HI(*pstyle);
		fw->cs_hi = cs;
	}
	else
	{
		fw->cs_hi = -1;
	}
	if (
		SGET_FORE_COLOR_NAME_HI(*pstyle) != NULL &&
		!SUSE_COLORSET_HI(&pstyle->flags))
	{
		fw->hicolors.fore = GetColor(SGET_FORE_COLOR_NAME_HI(*pstyle));
	}
	else
	{
		fw->hicolors.fore = Colorset[cs].fg;
	}
	if (
		SGET_BACK_COLOR_NAME_HI(*pstyle) != NULL &&
		!SUSE_COLORSET_HI(&pstyle->flags))
	{
		fw->hicolors.back = GetColor(SGET_BACK_COLOR_NAME_HI(*pstyle));
		fw->hicolors.shadow = GetShadow(fw->hicolors.back);
		fw->hicolors.hilight = GetHilite(fw->hicolors.back);
	}
	else
	{
		fw->hicolors.hilight = Colorset[cs].hilite;
		fw->hicolors.shadow = Colorset[cs].shadow;
		fw->hicolors.back = Colorset[cs].bg;
	}
	if (SUSE_BORDER_COLORSET_HI(&pstyle->flags))
	{
		cs = SGET_BORDER_COLORSET_HI(*pstyle);
		fw->border_cs_hi = cs;
		fw->border_hicolors.hilight = Colorset[cs].hilite;
		fw->border_hicolors.shadow = Colorset[cs].shadow;
		fw->border_hicolors.back = Colorset[cs].bg;
	}
	else
	{
		fw->border_cs_hi = -1;
		fw->border_hicolors.hilight = fw->hicolors.hilight;
		fw->border_hicolors.shadow = fw->hicolors.shadow;
		fw->border_hicolors.back = fw->hicolors.back;
	}
}

void update_icon_title_cs_style(FvwmWindow *fw, window_style *pstyle)
{
	if (SUSE_ICON_TITLE_COLORSET(&pstyle->flags))
	{
		fw->icon_title_cs = SGET_ICON_TITLE_COLORSET(*pstyle);
	}
	else
	{
		fw->icon_title_cs = -1;
	}
}

void update_icon_title_cs_hi_style(FvwmWindow *fw, window_style *pstyle)
{
	if (SUSE_ICON_TITLE_COLORSET_HI(&pstyle->flags))
	{
		fw->icon_title_cs_hi = SGET_ICON_TITLE_COLORSET_HI(*pstyle);
	}
	else
	{
		fw->icon_title_cs_hi = -1;
	}
}

void update_icon_background_cs_style(FvwmWindow *fw, window_style *pstyle)
{
	if (SUSE_ICON_BACKGROUND_COLORSET(&pstyle->flags))
	{
		fw->icon_background_cs =
			SGET_ICON_BACKGROUND_COLORSET(*pstyle);
	}
	else
	{
		fw->icon_background_cs = -1;
	}
}

void style_destroy_style(style_id_t s_id)
{
	FvwmWindow *t;

	if (remove_all_of_style_from_list(s_id))
	{
		/* compact the current list of styles */
		Scr.flags.do_need_style_list_update = 1;
	}
	else
	{
		return;
	}
	/* mark windows for update */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (fw_match_style_id(t, s_id))
		{
			SET_STYLE_DELETED(t, 1);
			Scr.flags.do_need_window_update = 1;
		}
	}

	return;
}

void print_styles(int verbose)
{
	window_style *nptr;
	int count = 0;
	int mem = 0;

	fprintf(stderr,"Info on fvwm Styles:\n");
	if (verbose)
	{
		fprintf(stderr,"  List of Styles Names:\n");
	}
	for (nptr = all_styles; nptr != NULL; nptr = SGET_NEXT_STYLE(*nptr))
	{
		count++;
		if (SGET_ID_HAS_NAME(*nptr))
		{
			mem += strlen(SGET_NAME(*nptr));
			if (verbose)
			{
				fprintf(stderr,"    * %s\n", SGET_NAME(*nptr));
			}
		}
		else
		{
			mem++;
			if (verbose)
			{
				fprintf(stderr,"    * 0x%lx\n",
					(unsigned long)SGET_WINDOW_ID(*nptr));
			}
		}
		if (SGET_BACK_COLOR_NAME(*nptr))
		{
			mem += strlen(SGET_BACK_COLOR_NAME(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Back Color: %s\n",
					SGET_BACK_COLOR_NAME(*nptr));
			}
		}
		if (SGET_FORE_COLOR_NAME(*nptr))
		{
			mem += strlen(SGET_FORE_COLOR_NAME(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Fore Color: %s\n",
					SGET_FORE_COLOR_NAME(*nptr));
			}
		}
		if (SGET_BACK_COLOR_NAME_HI(*nptr))
		{
			mem += strlen(SGET_BACK_COLOR_NAME_HI(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Back Color hi: %s\n",
					SGET_BACK_COLOR_NAME_HI(*nptr));
			}
		}
		if (SGET_FORE_COLOR_NAME_HI(*nptr))
		{
			mem += strlen(SGET_FORE_COLOR_NAME_HI(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Fore Color hi: %s\n",
					SGET_FORE_COLOR_NAME_HI(*nptr));
			}
		}
		if (SGET_DECOR_NAME(*nptr))
		{
			mem += strlen(SGET_DECOR_NAME(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Decor: %s\n",
					SGET_DECOR_NAME(*nptr));
			}
		}
		if (SGET_WINDOW_FONT(*nptr))
		{
			mem += strlen(SGET_WINDOW_FONT(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Window Font: %s\n",
					SGET_WINDOW_FONT(*nptr));
			}
		}
		if (SGET_ICON_FONT(*nptr))
		{
			mem += strlen(SGET_ICON_FONT(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Icon Font: %s\n",
					SGET_ICON_FONT(*nptr));
			}
		}
		if (SGET_ICON_NAME(*nptr))
		{
			mem += strlen(SGET_ICON_NAME(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        Icon Name: %s\n",
					SGET_ICON_NAME(*nptr));
			}
		}
		if (SGET_MINI_ICON_NAME(*nptr))
		{
			mem += strlen(SGET_MINI_ICON_NAME(*nptr));
			if (verbose > 1)
			{
				fprintf(
					stderr,"        MiniIcon Name: %s\n",
					SGET_MINI_ICON_NAME(*nptr));
			}
		}
		if (SGET_ICON_BOXES(*nptr))
		{
			mem += sizeof(icon_boxes);
		}
	}
	fprintf(stderr,"  Number of styles: %d, Memory Used: %d bits\n",
		count, (int)(count*sizeof(window_style) + mem));

	return;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_Style(F_CMD_ARGS)
{
	__style_command(F_PASS_ARGS, NULL, False);

	return;
}

void CMD_WindowStyle(F_CMD_ARGS)
{
	__style_command(F_PASS_ARGS, NULL, True);

	return;
}

void CMD_FocusStyle(F_CMD_ARGS)
{
	__style_command(F_PASS_ARGS, "FP", False);

	return;
}

void CMD_DestroyStyle(F_CMD_ARGS)
{
	char *name;
	style_id_t s_id;

	/* parse style name */
	name = PeekToken(action, &action);

	/* in case there was no argument! */
	if (name == NULL)
		return;

	memset(&s_id, 0, sizeof(style_id_t));
	SID_SET_NAME(s_id, name);
	SID_SET_HAS_NAME(s_id, True);

	/* Do it */
	style_destroy_style(s_id);
	return;
}

void CMD_DestroyWindowStyle(F_CMD_ARGS)
{
	style_id_t s_id;

	memset(&s_id, 0, sizeof(style_id_t));
	SID_SET_WINDOW_ID(s_id, (XID)FW_W(exc->w.fw));
	SID_SET_HAS_WINDOW_ID(s_id, True);

	/* Do it */
	style_destroy_style(s_id);
	return;
}
