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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <math.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "functions.h"
#include "conditional.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "focus.h"
#include "geometry.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/**************************************************************************
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation
 * Direction = 0 ==> operation on current window (returns pass or fail)
 *
 **************************************************************************/
static FvwmWindow *Circulate(char *action, int Direction, char **restofline)
{
	int pass = 0;
	FvwmWindow *fw, *found = NULL;
	FvwmWindow *sf;
	WindowConditionMask mask;
	char *flags;

	/* Create window mask */
	flags = CreateFlagString(action, restofline);
	DefaultConditionMask(&mask);
	if (Direction == 0)
	{ /* override for Current [] */
		mask.my_flags.use_circulate_hit = 1;
		mask.my_flags.use_circulate_hit_icon = 1;
	}
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}
	sf = get_focus_window();
	if (sf)
	{
		if (Direction > 0)
		{
			fw = sf->prev;
		}
		else if (Direction < 0)
		{
			fw = sf->next;
		}
		else
		{
			fw = sf;
		}
	}
	else
	{
		fw = NULL;
		if (Direction == 0)
		{
			return NULL;
		}
	}

	for (pass = 0; pass < 3 && !found; pass++)
	{
		while (fw && !found && fw != &Scr.FvwmRoot)
		{
			/* Make CirculateUp and CirculateDown take args. by
			 * Y.NOMURA */
			if (MatchesConditionMask(fw, &mask))
			{
				found = fw;
			}
			else
			{
				if (Direction > 0)
				{
					fw = fw->prev;
				}
				else
				{
					fw = fw->next;
				}
			}
			if (Direction == 0)
			{
				FreeConditionMask(&mask);
				return found;
			}
		}
		if (!fw || fw == &Scr.FvwmRoot)
		{
			if (Direction > 0)
			{
				/* Go to end of list */
				for (fw = &Scr.FvwmRoot; fw && fw->next;
				     fw = fw->next)
				{
					/* nop */
				}
			}
			else
			{
				/* Go to top of list */
				fw = Scr.FvwmRoot.next;
			}
		}
	}
	FreeConditionMask(&mask);

	return found;
}

static void circulate_cmd(
	F_CMD_ARGS, int new_context, int circ_dir, Bool do_use_found)
{
	FvwmWindow *found;
	char *restofline;

	found = Circulate(action, circ_dir, &restofline);
	if (cond_rc != NULL)
	{
		*cond_rc = (found == NULL) ? COND_RC_NO_MATCH : COND_RC_OK;
	}
	if ((!found == !do_use_found) && restofline)
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;

		ecc.w.fw = found;
		ecc.w.w = (found != NULL) ? FW_W(found) : None;
		ecc.w.wcontext = new_context;
		exc2 = exc_clone_context(
			exc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
		execute_function(cond_rc, exc2, restofline, 0);
		exc_destroy_context(exc2);
	}

	return;
}

static void select_cmd(F_CMD_ARGS)
{
	char *restofline;
	char *flags;
	WindowConditionMask mask;
	FvwmWindow * const fw = exc->w.fw;

	if (!fw || IS_EWMH_DESKTOP(FW_W(fw)))
	{
		if (cond_rc != NULL)
		{
			*cond_rc = COND_RC_ERROR;
		}
		return;
	}
	flags = CreateFlagString(action, &restofline);
	DefaultConditionMask(&mask);
	mask.my_flags.use_circulate_hit = 1;
	mask.my_flags.use_circulate_hit_icon = 1;
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}
	if (MatchesConditionMask(fw, &mask) && restofline)
	{
		if (cond_rc != NULL)
		{
			*cond_rc = COND_RC_OK;
		}
		execute_function_override_wcontext(
			NULL, exc, restofline, 0, C_WINDOW);
	}
	else if (cond_rc != NULL)
	{
		*cond_rc = COND_RC_NO_MATCH;
	}
	FreeConditionMask(&mask);

	return;
}

/* ---------------------------- interface functions ------------------------- */

/**********************************************************************
 * Parses the flag string and returns the text between [ ] or ( )
 * characters.  The start of the rest of the line is put in restptr.
 * Note that the returned string is allocated here and it must be
 * freed when it is not needed anymore.
 * NOTE - exported via .h
 **********************************************************************/
char *CreateFlagString(char *string, char **restptr)
{
	char *retval;
	char *c;
	char *start;
	char closeopt;
	int length;

	c = string;
	while (isspace((unsigned char)*c) && (*c != 0))
	{
		c++;
	}

	if (*c == '[' || *c == '(')
	{
		/* Get the text between [ ] or ( ) */
		if (*c == '[')
		{
			closeopt = ']';
		}
		else
		{
			closeopt = ')';
		}
		c++;
		start = c;
		length = 0;
		while (*c != closeopt)
		{
			if (*c == 0)
			{
				fvwm_msg(ERR, "CreateFlagString",
					 "Conditionals require closing "
					 "parenthesis");
				*restptr = NULL;
				return NULL;
			}
			c++;
			length++;
		}

		/* We must allocate a new string because we null terminate the
		 * string between the [ ] or ( ) characters. */
		retval = safemalloc(length + 1);
		strncpy(retval, start, length);
		retval[length] = 0;

		*restptr = c + 1;
	}
	else
	{
		retval = NULL;
		*restptr = c;
	}

	return retval;
}

/**********************************************************************
 * The name field of the mask is allocated in CreateConditionMask.
 * It must be freed.
 * NOTE - exported via .h
 **********************************************************************/
void FreeConditionMask(WindowConditionMask *mask)
{
	if (mask->my_flags.needs_name)
	{
		free(mask->name);
	}
	else if (mask->my_flags.needs_not_name)
	{
		free(mask->name - 1);
	}

	return;
}

/* Assign the default values for the window mask
 * NOTE - exported via .h */
void DefaultConditionMask(WindowConditionMask *mask)
{
	memset(mask, 0, sizeof(WindowConditionMask));
	/* -2  means no layer condition, -1 means current */
	mask->layer = -2;

	return;
}

/**********************************************************************
 * Note that this function allocates the name field of the mask struct.
 * FreeConditionMask must be called for the mask when the mask is discarded.
 * NOTE - exported via .h
 **********************************************************************/
void CreateConditionMask(char *flags, WindowConditionMask *mask)
{
	char *condition;
	char *prev_condition = NULL;
	char *tmp;
	unsigned int state;

	if (flags == NULL)
	{
		return;
	}

	/* Next parse the flags in the string. */
	tmp = flags;
	tmp = GetNextSimpleOption(tmp, &condition);

	while (condition)
	{
		if (StrEquals(condition,"AcceptsFocus"))
		{
			mask->my_flags.do_accept_focus = 1;
			mask->my_flags.use_do_accept_focus = 1;
		}
		else if (StrEquals(condition,"!AcceptsFocus"))
		{
			mask->my_flags.do_accept_focus = 0;
			mask->my_flags.use_do_accept_focus = 1;
		}
		else if (StrEquals(condition,"Focused"))
		{
			mask->my_flags.needs_focus = NEEDS_TRUE;
		}
		else if (StrEquals(condition,"!Focused"))
		{
			mask->my_flags.needs_focus = NEEDS_FALSE;
		}
		else if (StrEquals(condition,"HasPointer"))
		{
			mask->my_flags.needs_pointer = NEEDS_TRUE;
		}
		else if (StrEquals(condition,"!HasPointer"))
		{
			mask->my_flags.needs_pointer = NEEDS_FALSE;
		}
		else if (StrEquals(condition,"Iconic"))
		{
			SET_ICONIFIED(mask, 1);
			SETM_ICONIFIED(mask, 1);
		}
		else if (StrEquals(condition,"!Iconic"))
		{
			SET_ICONIFIED(mask, 0);
			SETM_ICONIFIED(mask, 1);
		}
		else if (StrEquals(condition,"Visible"))
		{
			SET_PARTIALLY_VISIBLE(mask, 1);
			SETM_PARTIALLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(condition,"!Visible"))
		{
			SET_PARTIALLY_VISIBLE(mask, 0);
			SETM_PARTIALLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(condition,"PlacedByButton3"))
		{
			SET_PLACED_WB3(mask, 1);
			SETM_PLACED_WB3(mask, 1);
		}
		else if (StrEquals(condition,"!PlacedByButton3"))
		{
			SET_PLACED_WB3(mask, 0);
			SETM_PLACED_WB3(mask, 1);
		}
		else if (StrEquals(condition,"Raised"))
		{
			SET_FULLY_VISIBLE(mask, 1);
			SETM_FULLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(condition,"!Raised"))
		{
			SET_FULLY_VISIBLE(mask, 0);
			SETM_FULLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(condition,"Sticky"))
		{
			SET_STICKY(mask, 1);
			SETM_STICKY(mask, 1);
		}
		else if (StrEquals(condition,"!Sticky"))
		{
			SET_STICKY(mask, 0);
			SETM_STICKY(mask, 1);
		}
		else if (StrEquals(condition,"Maximized"))
		{
			SET_MAXIMIZED(mask, 1);
			SETM_MAXIMIZED(mask, 1);
		}
		else if (StrEquals(condition,"!Maximized"))
		{
			SET_MAXIMIZED(mask, 0);
			SETM_MAXIMIZED(mask, 1);
		}
		else if (StrEquals(condition,"Shaded"))
		{
			SET_SHADED(mask, 1);
			SETM_SHADED(mask, 1);
		}
		else if (StrEquals(condition,"!Shaded"))
		{
			SET_SHADED(mask, 0);
			SETM_SHADED(mask, 1);
		}
		else if (StrEquals(condition,"Transient"))
		{
			SET_TRANSIENT(mask, 1);
			SETM_TRANSIENT(mask, 1);
		}
		else if (StrEquals(condition,"!Transient"))
		{
			SET_TRANSIENT(mask, 0);
			SETM_TRANSIENT(mask, 1);
		}
		else if (StrEquals(condition,"PlacedByFvwm"))
		{
			SET_PLACED_BY_FVWM(mask, 1);
			SETM_PLACED_BY_FVWM(mask, 1);
		}
		else if (StrEquals(condition,"!PlacedByFvwm"))
		{
			SET_PLACED_BY_FVWM(mask, 0);
			SETM_PLACED_BY_FVWM(mask, 1);
		}
		else if (StrEquals(condition,"CurrentDesk"))
		{
			mask->my_flags.needs_current_desk = 1;
		}
		else if (StrEquals(condition,"CurrentPage"))
		{
			mask->my_flags.needs_current_desk = 1;
			mask->my_flags.needs_current_page = 1;
		}
		else if (StrEquals(condition,"CurrentGlobalPage"))
		{
			mask->my_flags.needs_current_desk = 1;
			mask->my_flags.needs_current_global_page = 1;
		}
		else if (StrEquals(condition,"CurrentPageAnyDesk") ||
			 StrEquals(condition,"CurrentScreen"))
		{
			mask->my_flags.needs_current_page = 1;
		}
		else if (StrEquals(condition,"CurrentGlobbalPageAnyDesk"))
		{
			mask->my_flags.needs_current_global_page = 1;
		}
		else if (StrEquals(condition,"CirculateHit"))
		{
			mask->my_flags.use_circulate_hit = 1;
		}
		else if (StrEquals(condition,"CirculateHitIcon"))
		{
			mask->my_flags.use_circulate_hit_icon = 1;
		}
		else if (StrEquals(condition,"CirculateHitShaded"))
		{
			mask->my_flags.use_circulate_hit_shaded = 1;
		}
		else if (StrEquals(condition,"State") ||
			 StrEquals(condition,"!State"))
		{
			if (sscanf(tmp, "%d", &state) &&
			    state >= 0 && state <= 31)
			{
				state = (1 << state);
				if (StrEquals(condition,"State"))
				{
					SET_USER_STATES(mask, state);
				}
				else
				{
					CLEAR_USER_STATES(mask, state);
				}
				SETM_USER_STATES(mask, state);
				tmp = GetNextToken(tmp, &condition);
			}
		}
		else if (StrEquals(condition, "Layer"))
		{
			if (sscanf(tmp,"%d",&mask->layer))
			{
				tmp = GetNextToken (tmp, &condition);
				if (mask->layer < 0)
				{
					/* silently ignore invalid layers */
					mask->layer = -2;
				}
			}
			else
			{
				/* needs current layer */
				mask->layer = -1;
			}
		}
		else if (!mask->my_flags.needs_name &&
			 !mask->my_flags.needs_not_name)
		{
			/* only 1st name to avoid mem leak */
			mask->name = condition;
			condition = NULL;
			if (mask->name[0] == '!')
			{
				mask->my_flags.needs_not_name = 1;
				mask->name++;
			}
			else
			{
				mask->my_flags.needs_name = 1;
			}
		}

		if (prev_condition)
		{
			free(prev_condition);
		}

		prev_condition = condition;
		tmp = GetNextSimpleOption(tmp, &condition);
	}
	if (prev_condition)
	{
		free(prev_condition);
	}

	return;
}

/**********************************************************************
 * Checks whether the given window matches the mask created with
 * CreateConditionMask.
 * NOTE - exported via .h
 **********************************************************************/
Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask)
{
	Bool fMatchesName;
	Bool fMatchesIconName;
	Bool fMatchesClass;
	Bool fMatchesResource;
	Bool fMatches;
	FvwmWindow *sf = get_focus_window();

	if (!blockcmpmask((char *)&(fw->flags), (char *)&(mask->flags),
			  (char *)&(mask->flag_mask), sizeof(fw->flags)))
	{
		return False;
	}
	if (!mask->my_flags.use_circulate_hit && DO_SKIP_CIRCULATE(fw))
	{
		return False;
	}
	if (!mask->my_flags.use_circulate_hit_icon && IS_ICONIFIED(fw) &&
	    DO_SKIP_ICON_CIRCULATE(fw))
	{
		return False;
	}
	if (!mask->my_flags.use_circulate_hit_shaded && IS_SHADED(fw) &&
	    DO_SKIP_SHADED_CIRCULATE(fw))
	{
		return False;
	}
	if (IS_ICONIFIED(fw) && IS_TRANSIENT(fw) && IS_ICONIFIED_BY_PARENT(fw))
	{
		return False;
	}
	if (mask->my_flags.needs_current_desk && fw->Desk != Scr.CurrentDesk)
	{
		return False;
	}
	if (mask->my_flags.needs_current_page)
	{
		if (FScreenIsEnabled())
		{
			if (!FScreenIsRectangleOnScreen(
				    NULL, FSCREEN_CURRENT, &(fw->frame_g)))
			{
				return False;
			}
		}
		else
		{
			if (!IsRectangleOnThisPage(
				    &(fw->frame_g), Scr.CurrentDesk))
			{
				return False;
			}
		}
	}
	else if (mask->my_flags.needs_current_global_page)
	{
		if (!IsRectangleOnThisPage(&(fw->frame_g), Scr.CurrentDesk))
		{
			return False;
		}
	}

	/* Yes, I know this could be shorter, but it's hard to understand then
	 */
	fMatchesName = matchWildcards(mask->name, fw->name.name);
	fMatchesIconName = matchWildcards(mask->name, fw->icon_name.name);
	fMatchesClass = (fw->class.res_class &&
			 matchWildcards(mask->name,fw->class.res_class));
	fMatchesResource = (fw->class.res_name &&
			    matchWildcards(mask->name, fw->class.res_name));
	fMatches = (fMatchesName || fMatchesIconName || fMatchesClass ||
		    fMatchesResource);

	if (mask->my_flags.needs_name && !fMatches)
	{
		return False;
	}
	if (mask->my_flags.needs_not_name && fMatches)
	{
		return False;
	}
	if ((mask->layer == -1) && (sf) && (fw->layer != sf->layer))
	{
		return False;
	}
	if ((mask->layer >= 0) && (fw->layer != mask->layer))
	{
		return False;
	}
	if (GET_USER_STATES(mask) !=
	    (mask->flag_mask.user_states & GET_USER_STATES(fw)))
	{
		return False;
	}
	if (mask->my_flags.use_do_accept_focus)
	{
		Bool f;

		f = focus_does_accept_input_focus(fw);
		if (fw && !FP_DO_FOCUS_BY_FUNCTION(FW_FOCUS_POLICY(fw)))
		{
			f = False;
		}
		else if (fw && FP_IS_LENIENT(FW_FOCUS_POLICY(fw)))
		{
			f = True;
		}
		if (!f != !mask->my_flags.do_accept_focus)
		{
			return False;
		}
	}
	if (mask->my_flags.needs_focus != NEEDS_ANY)
	{
		int is_focused;

		is_focused = (fw == get_focus_window());
		if (!is_focused && mask->my_flags.needs_focus == NEEDS_TRUE)
		{
			return False;
		}
		else if (is_focused &&
			 mask->my_flags.needs_focus == NEEDS_FALSE)
		{
			return False;
		}
	}
	if (mask->my_flags.needs_pointer != NEEDS_ANY)
	{
		int has_pointer;
		FvwmWindow *t;

		t = get_pointer_fvwm_window();
		if (t != NULL && t == fw)
		{
			has_pointer = 1;
		}
		else
		{
			has_pointer = 0;
		}
		if (!has_pointer && mask->my_flags.needs_pointer == NEEDS_TRUE)
		{
			return False;
		}
		else if (has_pointer &&
			 mask->my_flags.needs_pointer == NEEDS_FALSE)
		{
			return False;
		}
	}

	return True;
}

/* ---------------------------- builtin commands ---------------------------- */

void CMD_Prev(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, -1, True);

	return;
}

void CMD_Next(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, 1, True);

	return;
}

void CMD_None(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_ROOT, 1, False);

	return;
}

void CMD_Any(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, exc->w.wcontext, 1, False);

	return;
}

void CMD_Current(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, 0, True);

	return;
}

void CMD_PointerWindow(F_CMD_ARGS)
{
	exec_context_changes_t ecc;

	ecc.w.fw = get_pointer_fvwm_window();
	exc = exc_clone_context(exc, &ecc, ECC_FW);
	select_cmd(F_PASS_ARGS);
	exc_destroy_context(exc);

	return;
}

void CMD_ThisWindow(F_CMD_ARGS)
{
	select_cmd(F_PASS_ARGS);

	return;
}

void CMD_Pick(F_CMD_ARGS)
{
	select_cmd(F_PASS_ARGS);

	return;
}

void CMD_All(F_CMD_ARGS)
{
	FvwmWindow *t, **g;
	char *restofline;
	WindowConditionMask mask;
	char *flags;
	int num, i;
	Bool does_any_window_match = False;

	flags = CreateFlagString(action, &restofline);
	DefaultConditionMask(&mask);
	mask.my_flags.use_circulate_hit = 1;
	mask.my_flags.use_circulate_hit_icon = 1;
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}

	num = 0;
	for (t = Scr.FvwmRoot.next; t; t = t->next)
	{
		num++;
	}
	g = (FvwmWindow **) safemalloc (num * sizeof(FvwmWindow *));
	num = 0;
	for (t = Scr.FvwmRoot.next; t; t = t->next)
	{
		if (MatchesConditionMask(t, &mask))
		{
			g[num++] = t;
			does_any_window_match = True;
		}
	}
	for (i = 0; i < num; i++)
	{
		execute_function_override_window(
			NULL, exc, restofline, 0, g[i]);
	}
	if (cond_rc != NULL)
	{
		*cond_rc = (does_any_window_match == False) ?
			COND_RC_NO_MATCH : COND_RC_OK;
	}
	free(g);
	FreeConditionMask(&mask);

	return;
}

/**********************************************************************
 * Execute a function to the closest window in the given
 * direction.
 **********************************************************************/
void CMD_Direction(F_CMD_ARGS)
{
	/* The rectangles are inteded for a future enhancement and are not used
	 * yet. */
	rectangle my_g;
	rectangle his_g;
	int my_cx;
	int my_cy;
	int his_cx;
	int his_cy;
	int offset = 0;
	int distance = 0;
	int score;
	int best_score;
	FvwmWindow *window;
	FvwmWindow *best_window;
	int dir;
	char *flags;
	char *restofline;
	char *tmp;
	float tx;
	float ty;
	WindowConditionMask mask;
	Bool is_pointer_relative;
	FvwmWindow * const fw = exc->w.fw;

	/* Parse the direction. */
	tmp = PeekToken(action, &action);
	if (StrEquals(tmp, "FromPointer"))
	{
		is_pointer_relative = True;
		tmp = PeekToken(action, &action);
	}
	else
	{
		is_pointer_relative = False;
	}
	dir = gravity_parse_dir_argument(tmp, NULL, -1);
	if (dir == -1 || dir > DIR_ALL_MASK)
	{
		fvwm_msg(ERR, "Direction", "Invalid direction %s",
			 (tmp) ? tmp : "");
		if (cond_rc != NULL)
		{
			*cond_rc = COND_RC_ERROR;
		}
		return;
	}

	/* Create the mask for flags */
	flags = CreateFlagString(action, &restofline);
	if (!restofline)
	{
		if (flags)
		{
			free(flags);
		}
		if (cond_rc != NULL)
		{
			*cond_rc = COND_RC_NO_MATCH;
		}
		return;
	}
	DefaultConditionMask(&mask);
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}

	/* If there is a focused window, use that as a starting point.
	 * Otherwise we use the pointer as a starting point. */
	if (fw && is_pointer_relative == False)
	{
		get_visible_window_or_icon_geometry(fw, &my_g);
		my_cx = my_g.x + my_g.width / 2;
		my_cy = my_g.y + my_g.height / 2;
	}
	else
	{
		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &my_g.x,
			    &my_g.y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			my_g.x = 0;
			my_g.y = 0;
		}
		my_g.width = 1;
		my_g.height = 1;
		my_cx = my_g.x;
		my_cy = my_g.y;
	}

	/* Next we iterate through all windows and choose the closest one in
	 * the wanted direction. */
	best_window = NULL;
	best_score = -1;
	for (window = Scr.FvwmRoot.next; window; window = window->next)
	{
		/* Skip every window that does not match conditionals.  Also
		 * skip the currently focused window.  That would be too
		 * close. :) */
		if (window == fw || !MatchesConditionMask(window, &mask))
		{
			continue;
		}

		/* Calculate relative location of the window. */
		get_visible_window_or_icon_geometry(window, &his_g);
		his_g.x -= my_cx;
		his_g.y -= my_cy;
		his_cx = his_g.x + his_g.width / 2;
		his_cy = his_g.y + his_g.height / 2;

		if (dir > DIR_MAJOR_MASK && dir <= DIR_MINOR_MASK)
		{
			int tx;
			/* Rotate the diagonals 45 degrees counterclockwise. To
			 * do this, multiply the matrix /+h +h\ with the vector
			 * (x y).                       \-h +h/
			 * h = sqrt(0.5). We can set h := 1 since absolute
			 * distance doesn't * matter here. */
			tx = his_cx + his_cy;
			his_cy = -his_cx + his_cy;
			his_cx = tx;
		}
		/* Arrange so that distance and offset are positive in desired
		 * direction. */
		switch (dir)
		{
		case DIR_N:
		case DIR_S:
		case DIR_NE:
		case DIR_SW:
			offset = (his_cx < 0) ? -his_cx : his_cx;
			distance = (dir == 0 || dir == 4) ? -his_cy : his_cy;
			break;
		case DIR_E: /* E */
		case DIR_W: /* W */
		case DIR_SE: /* SE */
		case DIR_NW: /* NW */
			offset = (his_cy < 0) ? -his_cy : his_cy;
			distance = (dir == 3 || dir == 7) ? -his_cx : his_cx;
			break;
		case DIR_C:
			offset = 0;
			tx = (float)his_cx;
			ty = (float)his_cy;
			distance = (int)sqrt(tx * tx + ty * ty);
			break;
		}

		/* Target must be in given direction. */
		if (distance <= 0)
		{
			continue;
		}

		/* Calculate score for this window.  The smaller the better. */
		score = distance + offset;
		/* windows more than 45 degrees off the direction are heavily
		 * penalized and will only be chosen if nothing else within a
		 * million pixels */
		if (offset > distance)
		{
			score += 1000000;
		}
		if (best_score == -1 || score < best_score)
		{
			best_window = window;
			best_score = score;
		}
	} /* for */

	if (best_window)
	{
		if (cond_rc != NULL)
		{
			*cond_rc = COND_RC_OK;
		}
		execute_function_override_window(
			NULL, exc, restofline, 0, best_window);
	}
	else if (cond_rc != NULL)
	{
		*cond_rc = COND_RC_NO_MATCH;
	}
	FreeConditionMask(&mask);

	return;
}

void CMD_WindowId(F_CMD_ARGS)
{
	FvwmWindow *t;
	char *token;
	char *naction;
	unsigned long win;
	Bool use_condition = False;
	Bool use_screenroot = False;
	WindowConditionMask mask;
	char *flags, *restofline;

	/* Get window ID */
	action = GetNextToken(action, &token);

	if (token && StrEquals(token, "root"))
	{
		int screen = Scr.screen;

		free(token);
		token = PeekToken(action, &naction);
		if (!token || GetIntegerArguments(token, NULL, &screen, 1) != 1)
		{
			screen = Scr.screen;
		}
		else
		{
			action = naction;
		}
		use_screenroot = True;
		if (screen < 0 || screen >= Scr.NumberOfScreens)
		{
			screen = 0;
		}
		win = XRootWindow(dpy, screen);
		if (win == None)
		{
			if (cond_rc != NULL)
			{
				*cond_rc = COND_RC_ERROR;
			}
			return;
		}
	}
	else if (token)
	{
		/* SunOS doesn't have strtoul */
		win = (unsigned long)strtol(token, NULL, 0);
		free(token);
	}
	else
	{
		win = 0;
	}

	/* Look for condition - CreateFlagString returns NULL if no '(' or '['
	 */
	if (!use_screenroot)
	{
		flags = CreateFlagString(action, &restofline);
		if (flags)
		{
			/* Create window mask */
			use_condition = True;
			DefaultConditionMask(&mask);

			/* override for Current [] */
			mask.my_flags.use_circulate_hit = 1;
			mask.my_flags.use_circulate_hit_icon = 1;

			CreateConditionMask(flags, &mask);
			free(flags);

			/* Relocate action */
			action = restofline;
		}
	}

	/* Search windows */
	for (t = Scr.FvwmRoot.next; t; t = t->next)
	{
		if (FW_W(t) == win)
		{
			/* do it if no conditions or the conditions match */
			if (action && (!use_condition ||
				       MatchesConditionMask(t, &mask)))
			{
				if (cond_rc != NULL)
				{
					*cond_rc = COND_RC_OK;
				}
				execute_function_override_window(
					NULL, exc, action, 0, t);
			}
			else if (cond_rc != NULL)
			{
				*cond_rc = COND_RC_NO_MATCH;
			}
			break;
		}
	}
	if (!t)
	{
		/* The window is not managed by fvwm. Still some functions may
		 * work on it. */
		if (use_condition)
		{
			if (cond_rc != NULL)
			{
				*cond_rc = COND_RC_ERROR;
			}
		}
		else if (XGetGeometry(
				 dpy, win, &JunkRoot, &JunkX, &JunkY,
				 &JunkWidth, &JunkHeight, &JunkBW,
				 &JunkDepth) != 0)
		{
			if (cond_rc != NULL)
			{
				*cond_rc = COND_RC_OK;
			}
			if (action != NULL)
			{
				const exec_context_t *exc2;
				exec_context_changes_t ecc;

				ecc.w.fw = NULL;
				ecc.w.w = win;
				ecc.w.wcontext = C_UNMANAGED;
				exc2 = exc_clone_context(
					exc, &ecc,
					ECC_FW | ECC_W | ECC_WCONTEXT);
				execute_function(
					NULL, exc2, action, FUNC_IS_UNMANAGED);
				exc_destroy_context(exc2);
			}
		}
		else
		{
			/* window id does not exist */
			if (cond_rc != NULL)
			{
				*cond_rc = COND_RC_ERROR;
			}
		}
	}

	/* Tidy up */
	if (use_condition)
	{
		FreeConditionMask(&mask);
	}

	return;
}

void CMD_Cond(F_CMD_ARGS)
{
	fvwm_cond_func_rc match_rc;
	char *flags;
	char *restofline;

	if (cond_rc == NULL)
	{
		/* useless if no return code to compare to is given */
		return;
	}
	/* Create window mask */
	flags = CreateFlagString(action, &restofline);
	if (flags == NULL)
	{
		match_rc = COND_RC_NO_MATCH;
	}
	else if (StrEquals(flags, "1") || StrEquals(flags, "match"))
	{
		match_rc = COND_RC_OK;
	}
	else if (StrEquals(flags, "0") || StrEquals(flags, "nomatch"))
	{
		match_rc = COND_RC_NO_MATCH;
	}
	else if (StrEquals(flags, "-1") || StrEquals(flags, "error"))
	{
		match_rc = COND_RC_ERROR;
	}
	else
	{
		match_rc = COND_RC_NO_MATCH;
	}
	if (*cond_rc == match_rc && restofline != NULL)
	{
		/* execute the command in root window context; overwrite the
		 * return code with the return code of the command */
		execute_function(cond_rc, exc, restofline, 0);
	}
	if (flags != NULL)
	{
		free(flags);
	}

	return;
}

void CMD_CondCase(F_CMD_ARGS)
{
	fvwm_cond_func_rc tmp_rc;

	/* same as Cond, but does not modify the return code */
	if (cond_rc == NULL)
	{
		cond_rc = &tmp_rc;
	}
	CMD_Cond(F_PASS_ARGS);

	return;
}

void CMD_Break(F_CMD_ARGS)
{
	if (cond_rc != NULL)
	{
		*cond_rc = COND_RC_BREAK;
	}

	return;
}
