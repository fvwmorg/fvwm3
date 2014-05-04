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
#include <math.h>

#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/wild.h"
#include "libs/FScreen.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
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
#include "stack.h"
#include "commands.h"
#include "decorations.h"
#include "virtual.h"
#include "infostore.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation
 * Direction = 0 ==> operation on current window (returns pass or fail)
 *
 */
static FvwmWindow *Circulate(
	FvwmWindow *sf, char *action, int Direction, char **restofline)
{
	int pass = 0;
	FvwmWindow *fw, *found = NULL;
	WindowConditionMask mask;
	char *flags;

	/* Create window mask */
	flags = CreateFlagString(action, restofline);
	DefaultConditionMask(&mask);
	if (Direction == 0)
	{ /* override for Current [] */
		mask.my_flags.use_circulate_hit = 1;
		mask.my_flags.use_circulate_hit_icon = 1;
		mask.my_flags.use_circulate_hit_shaded = 1;
	}
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}
	if (sf == NULL || Direction == 0)
	{
		sf = get_focus_window();
	}
	if (sf != NULL)
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
			FreeConditionMask(&mask);
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
		if (fw == NULL || fw == &Scr.FvwmRoot)
		{
			if (Direction > 0)
			{
				/* Go to end of list */
				for (fw = Scr.FvwmRoot.next; fw && fw->next;
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
	F_CMD_ARGS, int new_context, int circ_dir, Bool do_use_found,
	Bool do_exec_on_match)
{
	FvwmWindow *found;
	char *restofline;

	found = Circulate(exc->w.fw, action, circ_dir, &restofline);
	if (cond_rc != NULL)
	{
		cond_rc->rc = (found == NULL) ? COND_RC_NO_MATCH : COND_RC_OK;
	}
	if ((!found == !do_exec_on_match) && restofline)
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;
		int flags;

		ecc.w.fw = (do_use_found == True) ? found : NULL;
		if (found != NULL)
		{
			ecc.w.w = FW_W(found);
			flags = FUNC_DONT_DEFER;
		}
		else
		{
			ecc.w.w = None;
			flags = 0;
		}
		ecc.w.wcontext = new_context;
		exc2 = exc_clone_context(
			exc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
		execute_function(cond_rc, exc2, restofline, flags);
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
			cond_rc->rc = COND_RC_ERROR;
		}
		return;
	}
	flags = CreateFlagString(action, &restofline);
	DefaultConditionMask(&mask);
	mask.my_flags.use_circulate_hit = 1;
	mask.my_flags.use_circulate_hit_icon = 1;
	mask.my_flags.use_circulate_hit_shaded = 1;
	CreateConditionMask(flags, &mask);
	if (flags)
	{
		free(flags);
	}
	if (MatchesConditionMask(fw, &mask) && restofline)
	{
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_OK;
		}
		execute_function_override_wcontext(
			cond_rc, exc, restofline, 0, C_WINDOW);
	}
	else if (cond_rc != NULL)
	{
		cond_rc->rc = COND_RC_NO_MATCH;
	}
	FreeConditionMask(&mask);

	return;
}

static Bool cond_check_access(char *file, int type, Bool im)
{
	char *full_file;
	char *path = NULL;

	if (!file || *file == 0)
	{
		return False;
	}
	if (file[0] == '/')
	{
		if (access(file, type) == 0)
		{
			return True;
		}
		else
		{
			return False;
		}
	}
	if (type != X_OK && im == False)
	{
		return False;
	}
	if (im == False)
	{
		path = getenv("PATH");
	}
	else
	{
		path = PictureGetImagePath();
	}
	if (path == NULL || *path == 0)
	{
		return False;
	}
	full_file = searchPath(path, file, NULL, type);
	if (full_file)
	{
		free(full_file);
		return True;
	}
	return False;
}

/* ---------------------------- interface functions ------------------------ */

/*
 * Parses the flag string and returns the text between [ ] or ( )
 * characters.  The start of the rest of the line is put in restptr.
 * Note that the returned string is allocated here and it must be
 * freed when it is not needed anymore.
 * NOTE - exported via .h
 */
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
		char *d;

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

			/* skip quoted string */
			d = SkipQuote(c, NULL, NULL, NULL);
			length += d - c;
			c = d;
		}

		/* We must allocate a new string because we null terminate the
		 * string between the [ ] or ( ) characters. */
		/* TA:  FIXME!  xasprintf() */
		retval = xmalloc(length + 1);
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

/*
 * The name_condition field of the mask is allocated in CreateConditionMask.
 * It must be freed.
 * NOTE - exported via .h
 */
void FreeConditionMask(WindowConditionMask *mask)
{
	struct name_condition *pp,*pp2;
	struct namelist *p,*p2;

	for (pp=mask->name_condition; pp; )
	{
		/* One malloc() is done for all the name strings.
		   The string is tokenised & the name fields point to
		   different parts of the one string.
		   The start of the string is the first name in the string
		   which is actually the last node in the linked list. */
		for (p=pp->namelist; p; )
		{
			p2=p->next;
			if(!p2)
			{
				free(p->name);
			}
			free(p);
			p=p2;
		}
		pp2=pp->next;
		free(pp);
		pp=pp2;
	}
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

/*
 * Note that this function allocates the name field of the mask struct.
 * FreeConditionMask must be called for the mask when the mask is discarded.
 * NOTE - exported via .h
 */
void CreateConditionMask(char *flags, WindowConditionMask *mask)
{
	char *allocated_condition;
	char *next_condition;
	char *condition;
	char *tmp;
	unsigned int state;

	if (flags == NULL)
	{
		return;
	}

	/* Next parse the flags in the string. */
	next_condition = GetNextFullOption(flags, &allocated_condition);
	condition = PeekToken(allocated_condition, &tmp);

	while (condition)
	{
		char *cond;
		int on;

		cond = condition;
		on = 1;
		if (*cond == '!')
		{
			on = 0;
			cond++;
		}
		if (StrEquals(cond,"AcceptsFocus"))
		{
			mask->my_flags.do_accept_focus = on;
			mask->my_flags.use_do_accept_focus = 1;
		}
		else if (StrEquals(cond,"Focused"))
		{
			mask->my_flags.needs_focus =
				(on) ? NEEDS_TRUE : NEEDS_FALSE;
		}
		else if (StrEquals(cond,"HasPointer"))
		{
			mask->my_flags.needs_pointer =
				(on) ? NEEDS_TRUE : NEEDS_FALSE;
		}
		else if (StrEquals(cond,"Iconic"))
		{
			SET_ICONIFIED(mask, on);
			SETM_ICONIFIED(mask, 1);
		}
		else if (StrEquals(cond,"Visible"))
		{
			SET_PARTIALLY_VISIBLE(mask, on);
			SETM_PARTIALLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(cond,"Overlapped"))
		{
			mask->my_flags.needs_overlapped = on;
			mask->my_flags.do_check_overlapped = 1;
		}
		else if (StrEquals(cond,"PlacedByButton"))
		{
			int button;
			int button_mask;

			if (sscanf(tmp, "%d", &button) &&
			    (button >= 1 &&
			     button <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS))
			{
				tmp = SkipNTokens(tmp, 1);
				button_mask = (1<<(button-1));
			}
			else
			{
				button_mask =
				     (1<<NUMBER_OF_EXTENDED_MOUSE_BUTTONS) - 1;
			}
			if (on)
			{
				if (mask->placed_by_button_mask &
				    mask->placed_by_button_set_mask &
				    ~button_mask)
				{
				  	  fvwm_msg(WARN, "PlacedByButton",
						   "Condition always False.");
				}
				mask->placed_by_button_mask |= button_mask;
			}
			else
			{
				mask->placed_by_button_mask &= ~button_mask;
			}
			mask->placed_by_button_set_mask |= button_mask;
		}
		else if (StrEquals(cond,"PlacedByButton3"))
		{
			if (on)
			{
				if (mask->placed_by_button_mask &
				    mask->placed_by_button_set_mask & ~(1<<2))
				{
				  	  fvwm_msg(WARN, "PlacedByButton3",
						   "Condition always False.");
				}
				mask->placed_by_button_mask |= (1<<2);
			}
			else
			{
				mask->placed_by_button_mask &= ~(1<<2);
			}
			mask->placed_by_button_set_mask |= (1<<2);
		}
		else if (StrEquals(cond,"Raised"))
		{
			SET_FULLY_VISIBLE(mask, on);
			SETM_FULLY_VISIBLE(mask, 1);
		}
		else if (StrEquals(cond,"Sticky"))
		{
			SET_STICKY_ACROSS_PAGES(mask, on);
			SET_STICKY_ACROSS_DESKS(mask, on);
			SETM_STICKY_ACROSS_PAGES(mask, 1);
			SETM_STICKY_ACROSS_DESKS(mask, 1);
		}
		else if (StrEquals(cond,"StickyAcrossPages"))
		{
			SET_STICKY_ACROSS_PAGES(mask, on);
			SETM_STICKY_ACROSS_PAGES(mask, 1);
		}
		else if (StrEquals(cond,"StickyAcrossDesks"))
		{
			SET_STICKY_ACROSS_DESKS(mask, on);
			SETM_STICKY_ACROSS_DESKS(mask, 1);
		}
		else if (StrEquals(cond,"StickyIcon"))
		{
			SET_ICON_STICKY_ACROSS_PAGES(mask, on);
			SET_ICON_STICKY_ACROSS_DESKS(mask, on);
			SETM_ICON_STICKY_ACROSS_PAGES(mask, 1);
			SETM_ICON_STICKY_ACROSS_DESKS(mask, 1);
		}
		else if (StrEquals(cond,"StickyAcrossPagesIcon"))
		{
			SET_ICON_STICKY_ACROSS_PAGES(mask, on);
			SETM_ICON_STICKY_ACROSS_PAGES(mask, 1);
		}
		else if (StrEquals(cond,"StickyAcrossDesksIcon"))
		{
			SET_ICON_STICKY_ACROSS_DESKS(mask, on);
			SETM_ICON_STICKY_ACROSS_DESKS(mask, 1);
		}
		else if (StrEquals(cond,"Maximized"))
		{
			SET_MAXIMIZED(mask, on);
			SETM_MAXIMIZED(mask, 1);
		}
		else if (StrEquals(cond,"FixedSize"))
		{
			/* don't set mask here, because we make the test here
			   (and don't compare against window's mask)
			   by checking allowed function */
			SET_SIZE_FIXED(mask, on);
			SETM_SIZE_FIXED(mask, 1);
		}
		else if (StrEquals(cond, "FixedPosition"))
		{
			SET_FIXED(mask, on);
			SETM_FIXED(mask, 1);
		}
		else if (StrEquals(cond,"HasHandles"))
		{
			SET_HAS_HANDLES(mask, on);
			SETM_HAS_HANDLES(mask, 1);
		}
		else if (StrEquals(cond,"Iconifiable"))
		{
			SET_IS_UNICONIFIABLE(mask, !on);
			SETM_IS_UNICONIFIABLE(mask, 1);
		}
		else if (StrEquals(cond,"Maximizable"))
		{
			SET_IS_UNMAXIMIZABLE(mask, !on);
			SETM_IS_UNMAXIMIZABLE(mask, 1);
		}
		else if (StrEquals(cond,"Closable"))
		{
			SET_IS_UNCLOSABLE(mask, !on);
			SETM_IS_UNCLOSABLE(mask, 1);
		}
		else if (StrEquals(cond,"Shaded"))
		{
			SET_SHADED(mask, on);
			SETM_SHADED(mask, 1);
		}
		else if (StrEquals(cond,"Transient"))
		{
			SET_TRANSIENT(mask, on);
			SETM_TRANSIENT(mask, 1);
		}
		else if (StrEquals(cond,"PlacedByFvwm"))
		{
			SET_PLACED_BY_FVWM(mask, on);
			SETM_PLACED_BY_FVWM(mask, 1);
		}
		else if (StrEquals(cond,"CurrentDesk"))
		{
			mask->my_flags.needs_current_desk = on;
			mask->my_flags.do_check_desk = 1;
		}
		else if (StrEquals(cond,"CurrentPage"))
		{
			mask->my_flags.needs_current_desk_and_page = on;
			mask->my_flags.do_check_desk_and_page = 1;
		}
		else if (StrEquals(cond,"CurrentGlobalPage"))
		{
			mask->my_flags.needs_current_desk_and_global_page = on;
			mask->my_flags.do_check_desk_and_global_page = 1;
		}
		else if (StrEquals(cond,"CurrentPageAnyDesk") ||
			 StrEquals(cond,"CurrentScreen"))
		{
			mask->my_flags.needs_current_page = on;
			mask->my_flags.do_check_page = 1;
		}
		else if (StrEquals(cond,"AnyScreen"))
		{
			mask->my_flags.do_not_check_screen = on;
		}
		else if (StrEquals(cond,"CurrentGlobalPageAnyDesk"))
		{
			mask->my_flags.needs_current_global_page = on;
			mask->my_flags.do_check_global_page = 1;
		}
		else if (StrEquals(cond,"CirculateHit"))
		{
			mask->my_flags.use_circulate_hit = on;
		}
		else if (StrEquals(cond,"CirculateHitIcon"))
		{
			mask->my_flags.use_circulate_hit_icon = on;
		}
		else if (StrEquals(cond,"CirculateHitShaded"))
		{
			mask->my_flags.use_circulate_hit_shaded = on;
		}
		else if (StrEquals(cond,"State"))
		{
			if (sscanf(tmp, "%u", &state) && state <= 31)
			{
				state = (1 << state);
				if (on)
				{
					SET_USER_STATES(mask, state);
				}
				else
				{
					CLEAR_USER_STATES(mask, state);
				}
				SETM_USER_STATES(mask, state);
				tmp = SkipNTokens(tmp, 1);
			}
		}
		else if (StrEquals(cond, "Layer"))
		{
			if (sscanf(tmp, "%d", &mask->layer))
			{
				tmp = SkipNTokens(tmp, 1);
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
			mask->my_flags.needs_same_layer = on;
		}
		else if (StrEquals(cond, "Desk"))
		{
			if (sscanf(tmp, "%d", &mask->desk)) {
				tmp = SkipNTokens(tmp, 1);
			}
			mask->my_flags.do_check_cond_desk = on;
		}
		else if (StrEquals(cond, "Screen"))
		{
			if (sscanf(tmp, "%d", &mask->screen)) {
				tmp = SkipNTokens(tmp, 1);
			}
			mask->my_flags.do_check_screen = 1;

			if (!on)
				mask->my_flags.do_not_check_screen = 1;
		}
		else
		{
			struct name_condition *pp;
			struct namelist *p;
			char *condp = xstrdup(cond);

			pp = xmalloc(sizeof *pp);
			pp->invert = (!on ? True : False);
			pp->namelist = NULL;
			pp->next = mask->name_condition;
			mask->name_condition = pp;
			for (;;)
			{
				p = xmalloc(sizeof *p);
				p->name=condp;
				p->next=pp->namelist;
				pp->namelist=p;
				while(*condp && *condp != '|')
				{
					condp++;
				}
				if(!*condp)
				{
					break;
				}
				*condp++='\0';
			}
		}

		if (tmp && *tmp)
		{
			fvwm_msg(OLD, "CreateConditionMask",
				 "Use comma instead of whitespace to "
				 "separate conditions");
		}
		else
		{
			if (allocated_condition != NULL)
			{
				free(allocated_condition);
				allocated_condition = NULL;
			}
			if (next_condition && *next_condition)
			{
				next_condition = GetNextFullOption(
					next_condition, &allocated_condition);
			}
			tmp = allocated_condition;
		}
		condition = PeekToken(tmp, &tmp);
	}

	return;
}

/*
 * Checks whether the given window matches the mask created with
 * CreateConditionMask.
 * NOTE - exported via .h
 */
Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask)
{
	int does_match;
	int is_on_desk;
	int is_on_page;
	int is_on_global_page;
	FvwmWindow *sf = get_focus_window();
	struct name_condition *pp;
	struct namelist *p;
	char *name;

	/* match FixedSize conditional */
	/* special treatment for FixedSize, because more than just
	   the is_size_fixed flag makes a window unresizable (width and height
	   hints etc.) */
	if (IS_SIZE_FIXED(mask) &&
	    mask->flag_mask.common.s.is_size_fixed &&
	    is_function_allowed(F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (!IS_SIZE_FIXED(mask) &&
	    mask->flag_mask.common.s.is_size_fixed &&
	    !is_function_allowed(F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (IS_FIXED(mask) &&
	    mask->flag_mask.common.s.is_fixed &&
	    is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (!IS_FIXED(mask) &&
	    mask->flag_mask.common.s.is_fixed &&
	    !is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (IS_UNICONIFIABLE(mask) &&
	    mask->flag_mask.common.s.is_uniconifiable &&
	    is_function_allowed(F_ICONIFY, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (
		!IS_UNICONIFIABLE(mask) &&
		mask->flag_mask.common.s.is_uniconifiable &&
		!is_function_allowed(
			F_ICONIFY, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (
		IS_UNMAXIMIZABLE(mask) &&
		mask->flag_mask.common.s.is_unmaximizable &&
		is_function_allowed(
			F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (
		!IS_UNMAXIMIZABLE(mask) &&
		mask->flag_mask.common.s.is_unmaximizable &&
		!is_function_allowed(
			F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
	        return False;
	}
	if (
		IS_UNCLOSABLE(mask) &&
		mask->flag_mask.common.s.is_unclosable &&
		(is_function_allowed(
			 F_CLOSE, NULL, fw, RQORIG_PROGRAM_US,False) ||
		 is_function_allowed(
			 F_DELETE, NULL, fw, RQORIG_PROGRAM_US,False) ||
	     is_function_allowed(
		     F_DESTROY, NULL, fw, RQORIG_PROGRAM_US, False)))
	{
	        return False;
	}
	if (
		!IS_UNCLOSABLE(mask) &&
		mask->flag_mask.common.s.is_unclosable &&
		(!is_function_allowed(
			 F_CLOSE, NULL, fw, RQORIG_PROGRAM_US,False) &&
		 !is_function_allowed(
			 F_DELETE, NULL, fw, RQORIG_PROGRAM_US,False) &&
		 !is_function_allowed(
			 F_DESTROY, NULL, fw, RQORIG_PROGRAM_US,False)))
	{
	        return False;
	}
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

	/* desk and page matching */
	is_on_desk = 1;
	if (mask->my_flags.do_check_desk ||
	    mask->my_flags.do_check_desk_and_page ||
	    mask->my_flags.do_check_desk_and_global_page)
	{
		is_on_desk = (fw->Desk == Scr.CurrentDesk);
	}
	is_on_page = 1;
	if (mask->my_flags.do_check_page ||
	    mask->my_flags.do_check_desk_and_page)
	{
		if (FScreenIsEnabled() && !mask->my_flags.do_not_check_screen)
		{
			is_on_page = !!FScreenIsRectangleOnScreen(
				NULL, FSCREEN_CURRENT, &(fw->g.frame));
		}
		else
		{
			is_on_page = !!IsRectangleOnThisPage(
				&(fw->g.frame), Scr.CurrentDesk);
		}
	}
	is_on_global_page = 1;
	if (mask->my_flags.do_check_global_page ||
	    mask->my_flags.do_check_desk_and_global_page)
	{
		is_on_global_page = !!IsRectangleOnThisPage(
			&(fw->g.frame), Scr.CurrentDesk);
	}

	if (mask->my_flags.do_check_desk_and_page)
	{
		int is_on_desk_and_page;

		is_on_desk_and_page = (is_on_desk && is_on_page);
		if (mask->my_flags.needs_current_desk_and_page !=
		    is_on_desk_and_page)
		{
			return False;
		}
	}
	else if (mask->my_flags.do_check_desk_and_global_page)
	{
		int is_on_desk_and_global_page;

		is_on_desk_and_global_page = (is_on_desk && is_on_global_page);
		if (mask->my_flags.needs_current_desk_and_global_page !=
		    is_on_desk_and_global_page)
		{
			return False;
		}
	}
	if (mask->my_flags.do_check_desk &&
	    mask->my_flags.needs_current_desk != is_on_desk)
	{
		return False;
	}
	if (mask->my_flags.do_check_page)
	{
		if (mask->my_flags.needs_current_page != is_on_page)
		{
			return False;
		}
	}
	else if (mask->my_flags.do_check_global_page)
	{
		if (mask->my_flags.needs_current_global_page !=
		    is_on_global_page)
		{
			return False;
		}
	}

	for (pp = mask->name_condition; pp; pp = pp->next)
	{
		does_match = 0;
		for (p = pp->namelist; p; p = p->next)
		{
			name=p->name;
			does_match |= matchWildcards(name, fw->name.name);
			does_match |= matchWildcards(name, fw->icon_name.name);
			if(fw->class.res_class)
				does_match |= matchWildcards(name,
					fw->class.res_class);
			if(fw->class.res_name)
				does_match |= matchWildcards(name,
					fw->class.res_name);
		}
		if(( pp->invert &&  does_match) ||
		   (!pp->invert && !does_match))
		{
			return False;
		}
	}

	if (mask->layer == -1 && sf)
	{
		int is_same_layer;

		is_same_layer = (fw->layer == sf->layer);
		if (mask->my_flags.needs_same_layer != is_same_layer)
		{
			return False;
		}
	}
	if (mask->layer >= 0)
	{
		int is_same_layer;

		is_same_layer = (fw->layer == mask->layer);
		if (mask->my_flags.needs_same_layer != is_same_layer)
		{
			return False;
		}
	}
	if (mask->placed_by_button_set_mask)
	{
		if (!((mask->placed_by_button_set_mask &
			(1<<(fw->placed_by_button - 1))) ==
		     (mask->placed_by_button_set_mask &
		      mask->placed_by_button_mask)))
		{
			return False;
		}
	}
	if (GET_USER_STATES(mask) !=
	    (mask->flag_mask.common.user_states & GET_USER_STATES(fw)))
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
	if (mask->my_flags.do_check_overlapped)
	{
		int is_o;

		is_o = (is_on_top_of_layer(fw) == False);
		if (is_o != mask->my_flags.needs_overlapped)
		{
			return False;
		}
	}
	if (mask->my_flags.do_check_cond_desk)
	{
		if (fw->Desk == mask->desk)
			return True;
		else
			return False;

		if (fw->Desk != mask->desk)
			return True;
		else
			return False;
	}
	if (mask->my_flags.do_check_screen)
	{
		rectangle	 g;
		int		 scr;

		get_unshaded_geometry(fw, &g);
		scr = FScreenOfPointerXY(g.x, g.y);

		if (mask->my_flags.do_not_check_screen) {
			/* Negation of (!screen n) specified. */
			return (scr != mask->screen);
		} else
			return (scr == mask->screen);
	}

	return True;
}

static void direction_cmd(F_CMD_ARGS, Bool is_scan)
{
	rectangle my_g;
	rectangle his_g;
	int my_cx;
	int my_cy;
	int his_cx;
	int his_cy;
	int cross = 0;
	int offset = 0;
	int distance = 0;
	int cycle = False;
	int forward = False;
	int score;
	int best_cross = 0;
	int best_score;
	int worst_score = -1;
	FvwmWindow *tfw;
	FvwmWindow *fw_best;
	int dir;
	int dir2;
	Bool right_handed=False;
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
			cond_rc->rc = COND_RC_ERROR;
		}
		return;
	}
	if (is_scan)
	{
		cycle = True;
		tmp = PeekToken(action, &action);
		if ( ! tmp )
		{
			fvwm_msg(
				ERR, "Direction", "Missing minor direction %s",
				(tmp) ? tmp : "");
			if (cond_rc != NULL)
			{
				cond_rc->rc = COND_RC_ERROR;
			}
			return;
		}
		dir2 = gravity_parse_dir_argument(tmp, NULL, -1);
		/* if enum direction_t changes, this is trashed. */
		if (dir2 == -1 || dir2 > DIR_NW ||
		    (dir < 4) != (dir2 < 4) || (abs(dir - dir2) & 1) != 1)
		{
			fvwm_msg(
				ERR, "Direction", "Invalid minor direction %s",
				(tmp) ? tmp : "");
			if (cond_rc != NULL)
			{
				cond_rc->rc = COND_RC_ERROR;
			}
			return;
		}
		else if (dir2 - dir == 1 || dir2 - dir == -3)
		{
			right_handed=True;
		}
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
			cond_rc->rc = COND_RC_NO_MATCH;
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
	fw_best = NULL;
	best_score = -1;
	for (tfw = Scr.FvwmRoot.next; tfw != NULL; tfw = tfw->next)
	{
		/* Skip every window that does not match conditionals.  Also
		 * skip the currently focused window.  That would be too
		 * close. :) */
		if (tfw == fw || !MatchesConditionMask(tfw, &mask))
		{
			continue;
		}

		/* Calculate relative location of the window. */
		get_visible_window_or_icon_geometry(tfw, &his_g);
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
		case DIR_S:
		case DIR_SW:
			forward = True;
		case DIR_N:
		case DIR_NE:
			cross = -his_cx;
			offset = (his_cx < 0) ? -his_cx : his_cx;
			distance = (dir == DIR_N || dir == DIR_NE) ?
				-his_cy : his_cy;
			break;
		case DIR_E: /* E */
		case DIR_SE: /* SE */
			forward = True;
		case DIR_W: /* W */
		case DIR_NW: /* NW */
			cross = his_cy;
			offset = (his_cy < 0) ? -his_cy : his_cy;
			distance = (dir == DIR_W || dir == DIR_NW) ?
				-his_cx : his_cx;
			break;
		case DIR_C:
			offset = 0;
			tx = (float)his_cx;
			ty = (float)his_cy;
			distance = (int)sqrt(tx * tx + ty * ty);
			break;
		}

		if (cycle)
		{
			offset=0;
		}
		else if (distance < 0)
		{
			/* Target must be in given direction. */
			continue;
		}
		else if (distance == 0 && dir != DIR_C)
		{
			continue;
		}

		/* Calculate score for this window.  The smaller the better. */
		score = distance + offset;
		if (!right_handed)
		{
			cross= -cross;
		}
		if (cycle)
		{
			int ordered = (forward == (cross < best_cross));

			if (distance < 0 && best_score == -1 &&
				(score < worst_score ||
				(score == worst_score && ordered)))
			{
				fw_best = tfw;
				worst_score = score;
				best_cross = cross;
			}
			if (score == 0 && forward == (cross < 0) &&
			    dir != DIR_C)
			{
				continue;
			}
			if (distance >= 0 &&
				(best_score == -1 || score < best_score ||
				 (score == best_score && ordered)))
			{
				fw_best = tfw;
				best_score = score;
				best_cross = cross;
			}
		}
		else
		{
			/* windows more than 45 degrees off the direction are
			 * heavily penalized and will only be chosen if nothing
			 * else within a million pixels */
			if (offset > distance)
			{
				score += 1000000;
			}
			if (best_score == -1 || score < best_score ||
			    (score == best_score && dir == DIR_C))
			{
				fw_best = tfw;
				best_score = score;
			}
		}
	} /* for */

	if (fw_best)
	{
		if (cond_rc != NULL)
		{
			cond_rc->rc = COND_RC_OK;
		}
		execute_function_override_window(
			cond_rc, exc, restofline, 0, fw_best);
	}
	else if (cond_rc != NULL)
	{
		cond_rc->rc = COND_RC_NO_MATCH;
	}
	FreeConditionMask(&mask);

	return;
}

static int __rc_matches_rcstring_consume(
	char **ret_rest, cond_rc_t *cond_rc, char *action)
{
	cond_rc_enum match_rc;
	char *orig_flags;
	char *flags;
	int is_not_reversed = 1;
	int ret;

	/* Create window mask */
	orig_flags = CreateFlagString(action, ret_rest);
	flags = orig_flags;
	if (flags == NULL)
	{
		match_rc = COND_RC_NO_MATCH;
	}
	else
	{
		if (*flags == '!')
		{
			is_not_reversed = 0;
			flags++;
		}
		if (StrEquals(flags, "1") || StrEquals(flags, "match"))
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
		else if (StrEquals(flags, "-2") || StrEquals(flags, "break"))
		{
			match_rc = COND_RC_BREAK;
		}
		else
		{
			match_rc = COND_RC_NO_MATCH;
			/* Does anyone check for other numerical returncode
			 * values? If so, this might have to be changed. */
			fprintf(
				stderr, "Unrecognised condition \"%s\" in"
				" TestRc command.\n", flags);
		}
	}
	if (orig_flags != NULL)
	{
		free(orig_flags);
	}
	ret = ((cond_rc->rc == match_rc) == is_not_reversed);

	return ret;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_Prev(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, -1, True, True);

	return;
}

void CMD_Next(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, 1, True, True);

	return;
}

void CMD_None(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_ROOT, 1, False, False);
	/* invert return code */
	switch (cond_rc->rc)
	{
	case COND_RC_OK:
		cond_rc->rc = COND_RC_NO_MATCH;
		break;
	case COND_RC_NO_MATCH:
		cond_rc->rc = COND_RC_OK;
		break;
	default:
		break;
	}

	return;
}

void CMD_Any(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, exc->w.wcontext, 1, False, True);

	return;
}

void CMD_Current(F_CMD_ARGS)
{
	circulate_cmd(F_PASS_ARGS, C_WINDOW, 0, True, True);

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
	char *token;
	Bool do_reverse = False;
	Bool use_stack = False;

	while (True) /* break when a non-option is found */
	{
		token = PeekToken(action, &restofline);
		if (StrEquals(token, "Reverse"))
		{
			if (!*restofline)
			{
				/* if not any more actions, then Reverse
				 * probably is some user function, so ignore
				 * it and do the old  behaviour */
				break;
			}
			else
			{
				do_reverse = True;
				action = restofline;
			}
		}
		else if (StrEquals(token, "UseStack"))
		{
			if (!*restofline)
			{
				/* if not any more actions, then UseStack
				 * probably is some user function, so ignore
				 * it and do the old behaviour */
				break;
			}
			else
			{
				use_stack = True;
				action = restofline;
			}
		}
		else
		{
			/* No more options -- continue with flags and
			 * commands */
			break;
		}
	}

	flags = CreateFlagString(action, &restofline);
	DefaultConditionMask(&mask);
	mask.my_flags.use_circulate_hit = 1;
	mask.my_flags.use_circulate_hit_icon = 1;
	mask.my_flags.use_circulate_hit_shaded = 1;
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
	g = xmalloc(num * sizeof(FvwmWindow *));
	num = 0;
	if (!use_stack)
	{
		for (t = Scr.FvwmRoot.next; t; t = t->next)
		{
			if (MatchesConditionMask(t, &mask))
			{
				g[num++] = t;
				does_any_window_match = True;
			}
		}
	}
	else
	{
		for (t = Scr.FvwmRoot.stack_next; t && t != &Scr.FvwmRoot;
		     t = t->stack_next)
		{
			if (MatchesConditionMask(t, &mask))
			{
				g[num++] = t;
				does_any_window_match = True;
			}
		}
	}
	if (do_reverse)
	{
		for (i = num-1; i >= 0; i--)
		{
			execute_function_override_window(
				cond_rc, exc, restofline, 0, g[i]);
		}
	}
	else
	{
		for (i = 0; i < num; i++)
		{
			execute_function_override_window(
				cond_rc, exc, restofline, 0, g[i]);
		}
	}
	if (cond_rc != NULL && cond_rc->rc != COND_RC_BREAK)
	{
		cond_rc->rc = (does_any_window_match == False) ?
			COND_RC_NO_MATCH : COND_RC_OK;
	}
	free(g);
	FreeConditionMask(&mask);

	return;
}

/*
 * Execute a function to the closest window in the given
 * direction.
 */
void CMD_Direction(F_CMD_ARGS)
{
	direction_cmd(F_PASS_ARGS,False);
}

void CMD_ScanForWindow(F_CMD_ARGS)
{
	direction_cmd(F_PASS_ARGS,True);
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
				cond_rc->rc = COND_RC_ERROR;
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
					cond_rc->rc = COND_RC_OK;
				}
				execute_function_override_window(
					cond_rc, exc, action, 0, t);
			}
			else if (cond_rc != NULL)
			{
				cond_rc->rc = COND_RC_NO_MATCH;
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
				cond_rc->rc = COND_RC_ERROR;
			}
		}
		else if (XGetGeometry(
				 dpy, win, &JunkRoot, &JunkX, &JunkY,
				 (unsigned int*)&JunkWidth,
				 (unsigned int*)&JunkHeight,
				 (unsigned int*)&JunkBW,
				 (unsigned int*)&JunkDepth) != 0)
		{
			if (cond_rc != NULL)
			{
				cond_rc->rc = COND_RC_OK;
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
					cond_rc, exc2, action,
					FUNC_IS_UNMANAGED);
				exc_destroy_context(exc2);
			}
		}
		else
		{
			/* window id does not exist */
			if (cond_rc != NULL)
			{
				cond_rc->rc = COND_RC_ERROR;
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

void CMD_TestRc(F_CMD_ARGS)
{
	char *rest;

	if (cond_rc == NULL)
	{
		/* useless if no return code to compare to is given */
		return;
	}
	if (__rc_matches_rcstring_consume(&rest, cond_rc, action) &&
	    rest != NULL)
	{
		/* execute the command in root window context; overwrite the
		 * return code with the return code of the command */
		execute_function(cond_rc, exc, rest, 0);
	}

	return;
}

void CMD_Break(F_CMD_ARGS)
{
	int rc;

	if (cond_rc == NULL)
	{
		return;
	}
	rc = GetIntegerArguments(action, &action, &cond_rc->break_levels, 1);
	if (rc != 1 || cond_rc->break_levels <= 0)
	{
		cond_rc->break_levels = -1;
	}
	cond_rc->rc = COND_RC_BREAK;

	return;
}

void CMD_NoWindow(F_CMD_ARGS)
{
	execute_function_override_window(cond_rc, exc, action, 0, NULL);

	return;
}

/* ver() - convert a version string to a floating-point number that
 * can be used to compare different versions.
 * ie. converts "2.5.11" to 2005011 */
static int ver (char *str)
{
	char *n;
	int v;

	str = DoPeekToken(str, &n, NULL, ".", NULL);
	if (!n)
	{
		return -1.0;
	}
	v = atoi(n) * 1000000;
	str = DoPeekToken(str, &n, NULL, ".", NULL);
	if (!n)
	{
		return -1.0;
	}
	v += atoi(n) * 1000;
	str = DoPeekToken(str, &n, NULL, ".", NULL);
	if (!n)
	{
		return -1.0;
	}
	v += atoi(n);

	return v;
}

/* match_version() - compare $version against this version of fvwm
 * using the operator specified by $operator. */
static Bool match_version(char *version, char *operator)
{
	static int fvwm_version = -1;
	const int v = ver(version);

	if (fvwm_version < 0)
	{
		char *tmp = xstrdup(VERSION);
		fvwm_version = ver(tmp);
		free(tmp);
	}
	if (v < 0)
	{
		fprintf(
			stderr, "match_version: Invalid version: %s\n",
			version);
		return False;
	}
	if (strcmp(operator, ">=") == 0)
	{
		return fvwm_version >= v;
	}
	else if (strcmp(operator, ">") == 0)
	{
		return fvwm_version > v;
	}
	else if (strcmp(operator, "<=") == 0)
	{
		return fvwm_version <= v;
	}
	else if (strcmp(operator, "<") == 0)
	{
		return fvwm_version < v;
	}
	else if (strcmp(operator, "==") == 0)
	{
		return (v == fvwm_version);
	}
	else if (strcmp(operator, "!=") == 0)
	{
		return (v != fvwm_version);
	}
	else
	{
		fprintf(
			stderr, "match_version: Invalid operator: %s\n",
			operator);
	}

	return False;
}

void CMD_Test(F_CMD_ARGS)
{
	char *restofline;
	char *flags;
	char *condition;
	char *flags_ptr;
	int match;
	int error;

	flags = CreateFlagString(action, &restofline);

	/* Next parse the flags in the string. */
	flags_ptr = flags;
	flags_ptr = GetNextSimpleOption(flags_ptr, &condition);

	match = 1;
	error = 0;
	while (condition)
	{
		char *cond;
		int reverse;

		cond = condition;
		reverse = 0;
		if (*cond == '!')
		{
			reverse = 1;
			cond++;
		}
		if (StrEquals(cond, "True"))
		{
			match = 1;
		}
		else if (StrEquals(cond, "False"))
		{
			match = 0;
		}
		else if (StrEquals(cond, "Version"))
		{
			char *pattern;
			flags_ptr = GetNextSimpleOption(flags_ptr, &pattern);
			if (pattern)
			{
				char *ver;
				flags_ptr = GetNextSimpleOption(
					flags_ptr, &ver);
				if (ver == NULL)
				{
					match = matchWildcards(
						pattern, VERSION);
				}
				else
				{
					match = match_version(ver, pattern);
					free(ver);
				}
				free(pattern);
			}
			else
			{
				error = 1;
			}
		}
		else if (StrEquals(cond, "Start"))
		{
			match = exc->type == EXCT_INIT ||
				exc->type == EXCT_RESTART;
		}
		else if (StrEquals(cond, "Init"))
		{
			match = exc->type == EXCT_INIT;
		}
		else if (StrEquals(cond, "Restart"))
		{
			match = exc->type == EXCT_RESTART;
		}
		else if (StrEquals(cond, "Exit"))
		{
			match = exc->type == EXCT_QUIT ||
				exc->type == EXCT_TORESTART;
		}
		else if (StrEquals(cond, "Quit"))
		{
			match = exc->type == EXCT_QUIT;
		}
		else if (StrEquals(cond, "ToRestart"))
		{
			match = exc->type == EXCT_TORESTART;
		}
		else if (StrEquals(cond, "x") || StrEquals(cond, "r") ||
			 StrEquals(cond, "w") || StrEquals(cond, "f") ||
			 StrEquals(cond, "i"))
		{
			char *pattern;
			int type = X_OK;
			Bool im = 0;

			switch(cond[0])
			{
			case 'X':
			case 'x':
				type = X_OK;
				break;
			case 'R':
			case 'r':
				type = R_OK;
				break;
			case 'W':
			case 'w':
				type = W_OK;
				break;
			case 'f':
			case 'F':
				type = F_OK;
				break;
			case 'i':
			case 'I':
				im = True;
				type = R_OK;
				break;
			default:
				/* cannot happen */
				break;
			}
			flags_ptr = GetNextSimpleOption(flags_ptr, &pattern);
			if (pattern)
			{
				match = cond_check_access(pattern, type, im);
				free(pattern);
			}
			else
			{
				error = 1;
			}
		}
		else if (StrEquals(cond, "EnvIsSet"))
		{
			char *var_name;

			flags_ptr = GetNextSimpleOption(flags_ptr, &var_name);
			if (var_name)
			{
				const char *value = getenv(var_name);

				match = (value != NULL) ? 1 : 0;
			}
			else
			{
				error = 1;
			}
		}
		else if (StrEquals(cond, "EnvMatch"))
		{
			char *var_name;

			flags_ptr = GetNextSimpleOption(flags_ptr, &var_name);
			if (var_name)
			{
                		const char *value;
				if ( (strlen(var_name) > 10) && (memcmp(var_name,"infostore.",10) == 0)  )
				{
					value = get_metainfo_value(var_name+10);
				}
				else
				{
					value = getenv(var_name);
				}
				char *pattern;
				/* unfortunately, GetNextSimpleOption is
				 * broken, does not accept quoted empty ""
				 *
				 * DV (2-Sep-2014):  It is *not* broken.  The
				 * parsing functions never return empty tokens
				 * by design.
				 */
				flags_ptr = GetNextSimpleOption(
					flags_ptr, &pattern);
				if (!value)
				{
					value = "";
				}
				if (pattern)
				{
					match =
						/* include empty string case */
						(!pattern[0] && !value[0]) ||
						matchWildcards(pattern, value);
				}
				else
				{
					error = 1;
				}
			}
			else
			{
				error = 1;
			}
		}
		else if (StrEquals(cond, "EdgeIsActive"))
		{
			direction_t dir= DIR_NONE;
			char *dirname;
			char *next;
			next = GetNextSimpleOption(flags_ptr, &dirname);
			if (dirname)
			{
				dir = gravity_parse_dir_argument(
					dirname, NULL, DIR_NONE);
				if (dir == DIR_NONE)
				{
					if (!StrEquals(dirname, "Any"))
					{
						next = flags_ptr;
					}
				}
				else if (dir > DIR_MAJOR_MASK)
				{
					error = 1;
				}
				free(dirname);
			}

			if (!error)
			{
				if (((dir == DIR_W || dir == DIR_NONE) &&
					Scr.PanFrameLeft.isMapped) ||
				    ((dir == DIR_N || dir == DIR_NONE) &&
					Scr.PanFrameTop.isMapped) ||
				    ((dir == DIR_S || dir == DIR_NONE) &&
					Scr.PanFrameBottom.isMapped) ||
				    ((dir == DIR_E || dir == DIR_NONE) &&
 				    	Scr.PanFrameRight.isMapped))
				{
				  	match = 1;
				}
				else
				{
				  	match = 0;
				}
			}
			flags_ptr = next;
		}
		else if (StrEquals(cond, "EdgeHasPointer"))
		{
			int x,y;
			Window win;
			direction_t dir = DIR_NONE;
			char *dirname;
			char *next;
			next = GetNextSimpleOption(flags_ptr, &dirname);
			if (dirname)
			{
				dir = gravity_parse_dir_argument(
					dirname, NULL, DIR_NONE);
				if (dir == DIR_NONE)
				{
					if (!StrEquals(dirname, "Any"))
					{
						next = flags_ptr;
					}
				}
				else if (dir > DIR_MAJOR_MASK)
				{
					error = 1;
				}
				free(dirname);
			}

			if (!error)
			{
				if (FQueryPointer(
					dpy, Scr.Root, &JunkRoot, &win,
					&JunkX, &JunkY,	&x, &y, &JunkMask)
 					== False)
				{
					/* pointer is on a different screen */
					match = 0;
				}
				else if (is_pan_frame(win))
				{
					if (dir == DIR_NONE ||
					    (dir == DIR_N &&
					     win == Scr.PanFrameTop.win) ||
					    (dir == DIR_S &&
					     win == Scr.PanFrameBottom.win) ||
					    (dir == DIR_E &&
					     win == Scr.PanFrameRight.win) ||
					    (dir == DIR_W &&
					     win == Scr.PanFrameLeft.win))
					{
						match = 1;
					}
					else
					{
						match = 0;
					}
				}
				else
				{
					match = 0;
				}
			}
			flags_ptr = next;
		}
		else
		{
			/* unrecognized condition */
			error = 1;
			fprintf(
				stderr, "Unrecognised condition \"%s\" in"
				" Test command.\n", cond);
		}

		if (reverse)
		{
			match = !match;
		}
		free(condition);
		if (error || !match)
		{
			break;
		}
		flags_ptr = GetNextSimpleOption(flags_ptr, &condition);
	}

	if (flags != NULL)
	{
		free(flags);
	}
	if (!error && match)
	{
		execute_function(cond_rc, exc, restofline, 0);
	}
	if (cond_rc != NULL)
	{
		if (error)
		{
			cond_rc->rc = COND_RC_ERROR;
		}
		else if (match)
		{
			cond_rc->rc = COND_RC_OK;
		}
		else
		{
			cond_rc->rc = COND_RC_NO_MATCH;
		}
	}

	return;
}
