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

/* This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation */

/*
 * fvwm built-in functions and complex functions
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "functions.h"
#include "commands.h"
#include "functable.h"
#include "events.h"
#include "modconf.h"
#include "module_interface.h"
#include "misc.h"
#include "screen.h"
#include "repeat.h"
#include "expand.h"
#include "menus.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct FunctionItem
{
	struct FvwmFunction *func;       /* the function this item is in */
	struct FunctionItem *next_item;  /* next function item */
	char condition;                  /* the character string displayed on
					  * left*/
	char *action;                    /* action to be performed */
	short type;                      /* type of built in function */
	FUNC_FLAGS_TYPE flags;
} FunctionItem;

typedef struct FvwmFunction
{
	struct FvwmFunction *next_func;  /* next in list of root menus */
	FunctionItem *first_item;        /* first item in function */
	FunctionItem *last_item;         /* last item in function */
	char *name;                      /* function name */
	unsigned int use_depth;
} FvwmFunction;

/* Types of events for the FUNCTION builtin */
typedef enum
{
	CF_IMMEDIATE =      'i',
	CF_MOTION =         'm',
	CF_HOLD =           'h',
	CF_CLICK =          'c',
	CF_DOUBLE_CLICK =   'd',
	CF_TIMEOUT =        '-'
} cfunc_action_t;

/* ---------------------------- forward declarations ----------------------- */

static void execute_complex_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	Bool *desperate, Bool has_ref_window_moved);

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static int __context_has_window(
	const exec_context_t *exc, execute_flags_t flags)
{
	if (exc->w.fw != NULL)
	{
		return 1;
	}
	else if ((flags & FUNC_ALLOW_UNMANAGED) && exc->w.w != None)
	{
		return 1;
	}

	return 0;
}

/*
 * Defer the execution of a function to the next button press if the context is
 * C_ROOT
 *
 *  Inputs:
 *      cursor  - the cursor to display while waiting
 */
static Bool DeferExecution(
	exec_context_changes_t *ret_ecc, exec_context_change_mask_t *ret_mask,
	cursor_t cursor, int trigger_evtype, int do_allow_unmanaged)
{
	int done;
	int finished = 0;
	int just_waiting_for_finish = 0;
	Window dummy;
	Window original_w;
	static XEvent e;
	Window w;
	int wcontext;
	FvwmWindow *fw;
	int FinishEvent;

	fw = ret_ecc->w.fw;
	w = ret_ecc->w.w;
	original_w = w;
	wcontext = ret_ecc->w.wcontext;
	FinishEvent = ((fw != NULL) ? ButtonRelease : ButtonPress);
	if (wcontext == C_UNMANAGED && do_allow_unmanaged)
	{
		return False;
	}
	if (wcontext != C_ROOT && wcontext != C_NO_CONTEXT && fw != NULL &&
	    wcontext != C_EWMH_DESKTOP)
	{
		if (FinishEvent == ButtonPress ||
		    (FinishEvent == ButtonRelease &&
		     trigger_evtype != ButtonPress))
		{
			return False;
		}
		else if (FinishEvent == ButtonRelease)
		{
			/* We are only waiting until the user releases the
			 * button. Do not change the cursor. */
			cursor = CRS_NONE;
			just_waiting_for_finish = 1;
		}
	}
	if (Scr.flags.are_functions_silent)
	{
		return True;
	}
	if (!GrabEm(cursor, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return True;
	}
	MyXGrabKeyboard(dpy);
	while (!finished)
	{
		done = 0;
		/* block until there is an event */
		FMaskEvent(
			dpy, ButtonPressMask | ButtonReleaseMask |
			ExposureMask | KeyPressMask | VisibilityChangeMask |
			ButtonMotionMask | PointerMotionMask
			/* | EnterWindowMask | LeaveWindowMask*/, &e);

		if (e.type == KeyPress)
		{
			KeySym keysym = XLookupKeysym(&e.xkey, 0);
			if (keysym == XK_Escape)
			{
				ret_ecc->x.etrigger = &e;
				*ret_mask |= ECC_ETRIGGER;
				UngrabEm(GRAB_NORMAL);
				MyXUngrabKeyboard(dpy);
				return True;
			}
			Keyboard_shortcuts(&e, NULL, NULL, NULL, FinishEvent);
		}
		if (e.type == FinishEvent)
		{
			finished = 1;
		}
		switch (e.type)
		{
		case KeyPress:
		case ButtonPress:
			if (e.type != FinishEvent)
			{
				original_w = e.xany.window;
			}
			done = 1;
			break;
		case ButtonRelease:
			done = 1;
			break;
		default:
			break;
		}
		if (!done)
		{
			dispatch_event(&e);
		}
	}
	MyXUngrabKeyboard(dpy);
	UngrabEm(GRAB_NORMAL);
	if (just_waiting_for_finish)
	{
		return False;
	}
	w = e.xany.window;
	ret_ecc->x.etrigger = &e;
	*ret_mask |= ECC_ETRIGGER | ECC_W | ECC_WCONTEXT;
	if ((w == Scr.Root || w == Scr.NoFocusWin) &&
	    e.xbutton.subwindow != None)
	{
		w = e.xbutton.subwindow;
		e.xany.window = w;
	}
	if (w == Scr.Root || IS_EWMH_DESKTOP(w))
	{
		ret_ecc->w.w = w;
		ret_ecc->w.wcontext = C_ROOT;
		XBell(dpy, 0);
		return True;
	}
	*ret_mask |= ECC_FW;
	if (XFindContext(dpy, w, FvwmContext, (caddr_t *)&fw) == XCNOENT)
	{
		ret_ecc->w.fw = NULL;
		ret_ecc->w.w = w;
		ret_ecc->w.wcontext = C_ROOT;
		XBell(dpy, 0);
		return (True);
	}
	if (w == FW_W_PARENT(fw))
	{
		w = FW_W(fw);
	}
	if (original_w == FW_W_PARENT(fw))
	{
		original_w = FW_W(fw);
	}
	/* this ugly mess attempts to ensure that the release and press
	 * are in the same window. */
	if (w != original_w && original_w != Scr.Root &&
	    original_w != None && original_w != Scr.NoFocusWin &&
	    !IS_EWMH_DESKTOP(original_w))
	{
		if (w != FW_W_FRAME(fw) || original_w != FW_W(fw))
		{
			ret_ecc->w.fw = fw;
			ret_ecc->w.w = w;
			ret_ecc->w.wcontext = C_ROOT;
			XBell(dpy, 0);
			return True;
		}
	}

	if (IS_EWMH_DESKTOP(FW_W(fw)))
	{
		ret_ecc->w.fw = fw;
		ret_ecc->w.w = w;
		ret_ecc->w.wcontext = C_ROOT;
		XBell(dpy, 0);
		return True;
	}
	wcontext = GetContext(NULL, fw, &e, &dummy);
	ret_ecc->w.fw = fw;
	ret_ecc->w.w = w;
	ret_ecc->w.wcontext = C_ROOT;

	return False;
}

/* dummies */
void CMD_Dummy(F_CMD_ARGS)
{
	return;
}

/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
	return (strcmp((char *)a, ((func_t *)b)->keyword));
}

static const func_t *find_builtin_function(char *func)
{
	static int nfuncs = 0;
	func_t *ret_func;
	char *temp;
	char *s;

	if (!func || func[0] == 0)
	{
		return NULL;
	}

	/* since a lot of lines in a typical rc are probably menu/func
	 * continues: */
	if (func[0]=='+' || (func[0] == ' ' && func[1] == '+'))
	{
		return &(func_table[0]);
	}

	temp = safestrdup(func);
	for (s = temp; *s != 0; s++)
	{
		if (isupper(*s))
		{
			*s = tolower(*s);
		}
	}
	if (nfuncs == 0)
	{
		for ( ; (func_table[nfuncs]).action != NULL; nfuncs++)
		{
			/* nothing to do here */
		}
	}
	ret_func = (func_t *)bsearch(
		temp, func_table, nfuncs, sizeof(func_t), func_comp);
	free(temp);

	return ret_func;
}

static void __execute_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, char *args[], Bool has_ref_window_moved)
{
	static unsigned int func_depth = 0;
	cond_rc_t *func_rc = NULL;
	cond_rc_t dummy_rc;
	Window w;
	int j;
	char *function;
	char *taction;
	char *trash;
	char *trash2;
	char *expaction = NULL;
	char *arguments[11];
	const func_t *bif;
	Bool set_silent;
	Bool must_free_string = False;
	Bool must_free_function = False;
	Bool do_keep_rc = False;
	/* needed to be able to avoid resize to use moved windows for base */
	extern Window PressedW;
	Window dummy_w;

	if (!action)
	{
		return;
	}
	/* ignore whitespace at the beginning of all config lines */
	action = SkipSpaces(action, NULL, 0);
	if (!action || action[0] == 0)
	{
		/* impossibly short command */
		return;
	}
	if (action[0] == '#')
	{
		/* a comment */
		return;
	}

	func_depth++;
	if (func_depth > MAX_FUNCTION_DEPTH)
	{
		fvwm_msg(
			ERR, "__execute_function",
			"Function '%s' called with a depth of %i, "
			"stopping function execution!",
			action, func_depth);
		func_depth--;
		return;
	}
	if (args)
	{
		for (j = 0; j < 11; j++)
		{
			arguments[j] = args[j];
		}
	}
	else
	{
		for (j = 0; j < 11; j++)
		{
			arguments[j] = NULL;
		}
	}

	if (exc->w.fw == NULL || IS_EWMH_DESKTOP(FW_W(exc->w.fw)))
	{
		if (exec_flags & FUNC_IS_UNMANAGED)
		{
			w = exc->w.w;
		}
		else
		{
			w = Scr.Root;
		}
	}
	else
	{
		FvwmWindow *tw;

		w = GetSubwindowFromEvent(dpy, exc->x.elast);
		if (w == None)
		{
			w = exc->x.elast->xany.window;
		}
		tw = NULL;
		if (w != None)
		{
			if (XFindContext(
				 dpy, w, FvwmContext, (caddr_t *)&tw) ==
			    XCNOENT)
			{
				tw = NULL;
			}
		}
		if (w == None || tw != exc->w.fw)
		{
			w = FW_W(exc->w.fw);
		}
	}

	set_silent = False;
	if (action[0] == '-')
	{
		exec_flags |= FUNC_DONT_EXPAND_COMMAND;
		action++;
	}

	taction = action;
	/* parse prefixes */
	trash = PeekToken(taction, &trash2);
	while (trash)
	{
		if (StrEquals(trash, PRE_SILENT))
		{
			if (Scr.flags.are_functions_silent == 0)
			{
				set_silent = 1;
				Scr.flags.are_functions_silent = 1;
			}
			taction = trash2;
			trash = PeekToken(taction, &trash2);
		}
		else if (StrEquals(trash, PRE_KEEPRC))
		{
			do_keep_rc = True;
			taction = trash2;
			trash = PeekToken(taction, &trash2);
		}
		else
		{
			break;
		}
	}
	if (taction == NULL)
	{
		if (set_silent)
		{
			Scr.flags.are_functions_silent = 0;
		}
		func_depth--;
		return;
	}
	if (cond_rc == NULL || do_keep_rc == True)
	{
		condrc_init(&dummy_rc);
		func_rc = &dummy_rc;
	}
	else
	{
		func_rc = cond_rc;
	}

	GetNextToken(taction, &function);
	if (function)
	{
		char *tmp = function;
		function = expand_vars(
			function, arguments, False, False, func_rc, exc);
		free(tmp);
	}
	if (function && function[0] != '*')
	{
#if 1
		/* DV: with this piece of code it is impossible to have a
		 * complex function with embedded whitespace that begins with a
		 * builtin function name, e.g. a function "echo hello". */
		/* DV: ... and without it some of the complex functions will
		 * fail */
		char *tmp = function;

		while (*tmp && !isspace(*tmp))
		{
			tmp++;
		}
		*tmp = 0;
#endif
		bif = find_builtin_function(function);
		must_free_function = True;
	}
	else
	{
		bif = NULL;
		if (function)
		{
			free(function);
		}
		function = "";
	}

#ifdef USEDECOR
	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor &&
	    (!bif || !(bif->flags & FUNC_DECOR)))
	{
		fvwm_msg(
			ERR, "__execute_function",
			"Command can not be added to a decor; executing"
			" command now: '%s'", action);
	}
#endif

	if (!(exec_flags & FUNC_DONT_EXPAND_COMMAND))
	{
		expaction = expand_vars(
			taction, arguments, (bif) ?
			!!(bif->flags & FUNC_ADD_TO) :
			False, (taction[0] == '*'), func_rc, exc);
		if (func_depth <= 1)
		{
			must_free_string = set_repeat_data(
				expaction, REPEAT_COMMAND, bif);
		}
		else
		{
			must_free_string = True;
		}
	}
	else
	{
		expaction = taction;
	}

#ifdef FVWM_COMMAND_LOG
	fvwm_msg(INFO, "LOG", "%c: %s", (char)exc->type, expaction);
#endif

	/* Note: the module config command, "*" can not be handled by the
	 * regular command table because there is no required white space after
	 * the asterisk. */
	if (expaction[0] == '*')
	{
#ifdef USEDECOR
		if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
		{
			fvwm_msg(
				WARN, "__execute_function",
				"Command can not be added to a decor;"
				" executing command now: '%s'", expaction);
		}
#endif
		/* process a module config command */
		ModuleConfig(expaction);
	}
	else
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;
		exec_context_change_mask_t mask;

		mask = (w != exc->w.w) ? ECC_W : 0;
		ecc.w.fw = exc->w.fw;
		ecc.w.w = w;
		ecc.w.wcontext = exc->w.wcontext;
		if (bif && bif->func_t != F_FUNCTION)
		{
			char *runaction;
			Bool rc = False;

			runaction = SkipNTokens(expaction, 1);
			if ((bif->flags & FUNC_NEEDS_WINDOW) &&
			    !(exec_flags & FUNC_DONT_DEFER))
			{
				rc = DeferExecution(
					&ecc, &mask, bif->cursor,
					exc->x.elast->type,
					(bif->flags & FUNC_ALLOW_UNMANAGED));
			}
			else if ((bif->flags & FUNC_NEEDS_WINDOW) &&
				 !__context_has_window(
					 exc,
					 bif->flags & FUNC_ALLOW_UNMANAGED))
			{
				/* no context window and not allowed to defer,
				 * skip command */
				rc = True;
			}
			if (rc == False)
			{
				exc2 = exc_clone_context(exc, &ecc, mask);
				if (has_ref_window_moved &&
				    (bif->func_t == F_ANIMATED_MOVE ||
				     bif->func_t == F_MOVE ||
				     bif->func_t == F_RESIZE))
				{
					dummy_w = PressedW;
					PressedW = None;
					bif->action(func_rc, exc2, runaction);
					PressedW = dummy_w;
				}
				else
				{
					bif->action(func_rc, exc2, runaction);
				}
				exc_destroy_context(exc2);
			}
		}
		else
		{
			Bool desperate = 1;
			char *runaction;

			if (bif)
			{
				/* strip "function" command */
				runaction = SkipNTokens(expaction, 1);
			}
			else
			{
				runaction = expaction;
			}
			exc2 = exc_clone_context(exc, &ecc, mask);
			execute_complex_function(
				func_rc, exc2, runaction, &desperate,
				has_ref_window_moved);
			if (!bif && desperate)
			{
				if (executeModuleDesperate(
					    func_rc, exc, runaction) == NULL &&
				    *function != 0 && !set_silent)
				{
					fvwm_msg(
						ERR, "__execute_function",
						"No such command '%s'",
						function);
				}
			}
			exc_destroy_context(exc2);
		}
	}

	if (set_silent)
	{
		Scr.flags.are_functions_silent = 0;
	}
	if (cond_rc != NULL)
	{
		cond_rc->break_levels = func_rc->break_levels;
	}
	if (must_free_string)
	{
		free(expaction);
	}
	if (must_free_function)
	{
		free(function);
	}
	func_depth--;

	return;
}

/* find_complex_function expects a token as the input. Make sure you have used
 * GetNextToken before passing a function name to remove quotes */
static FvwmFunction *find_complex_function(const char *function_name)
{
	FvwmFunction *func;

	if (function_name == NULL || *function_name == 0)
	{
		return NULL;
	}

	func = Scr.functions;
	while (func != NULL)
	{
		if (func->name != NULL)
		{
			if (strcasecmp(function_name, func->name) == 0)
			{
				return func;
			}
		}
		func = func->next_func;
	}

	return NULL;

}

/*
 * Builtin which determines if the button press was a click or double click...
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 */
static cfunc_action_t CheckActionType(
	int x, int y, XEvent *d, Bool may_time_out, Bool is_button_pressed,
	int *ret_button)
{
	int xcurrent,ycurrent,total = 0;
	Time t0;
	int dist;
	Bool do_sleep = False;

	xcurrent = x;
	ycurrent = y;
	t0 = fev_get_evtime();
	dist = Scr.MoveThreshold;

	while ((total < Scr.ClickTime &&
		fev_get_evtime() - t0 < Scr.ClickTime) || !may_time_out)
	{
		if (!(x - xcurrent <= dist && xcurrent - x <= dist &&
		      y - ycurrent <= dist && ycurrent - y <= dist))
		{
			return (is_button_pressed) ? CF_MOTION : CF_TIMEOUT;
		}

		if (do_sleep)
		{
			usleep(20000);
		}
		else
		{
			usleep(1);
			do_sleep = 1;
		}
		total += 20;
		if (FCheckMaskEvent(
			    dpy, ButtonReleaseMask|ButtonMotionMask|
			    PointerMotionMask|ButtonPressMask|ExposureMask, d))
		{
			do_sleep = 0;
			switch (d->xany.type)
			{
			case ButtonRelease:
				*ret_button = d->xbutton.button;
				return CF_CLICK;
			case MotionNotify:
				if (d->xmotion.same_screen == False)
				{
					break;
				}
				if ((d->xmotion.state &
				     DEFAULT_ALL_BUTTONS_MASK) ||
				    !is_button_pressed)
				{
					xcurrent = d->xmotion.x_root;
					ycurrent = d->xmotion.y_root;
				}
				else
				{
					return CF_CLICK;
				}
				break;
			case ButtonPress:
				*ret_button = d->xbutton.button;
				if (may_time_out)
				{
					is_button_pressed = True;
				}
				break;
			case Expose:
				/* must handle expose here so that raising a
				 * window with "I" works */
				dispatch_event(d);
				break;
			default:
				/* can't happen */
				break;
			}
		}
	}

	return (is_button_pressed) ? CF_HOLD : CF_TIMEOUT;
}

static void __run_complex_function_items(
	cond_rc_t *cond_rc, char cond, FvwmFunction *func,
	const exec_context_t *exc, char *args[], Bool has_ref_window_moved)
{
	char c;
	FunctionItem *fi;
	int x0, y0, x, y;
	extern Window PressedW;

	if (!(!has_ref_window_moved && PressedW && XTranslateCoordinates(
				  dpy, PressedW , Scr.Root, 0, 0, &x0, &y0,
				  &JunkChild)))
	{
		x0 = y0 = 0;
	}

	for (fi = func->first_item; fi != NULL && cond_rc->break_levels == 0; )
	{
		/* make lower case */
		c = fi->condition;
		if (isupper(c))
		{
			c = tolower(c);
		}
		if (c == cond)
		{
			__execute_function(
				cond_rc, exc, fi->action, FUNC_DONT_DEFER,
				args, has_ref_window_moved);
			if (!has_ref_window_moved && PressedW &&
			    XTranslateCoordinates(
				  dpy, PressedW , Scr.Root, 0, 0, &x, &y,
				  &JunkChild))
			{
				has_ref_window_moved =(x != x0 || y != y0);
			}
		}
		fi = fi->next_item;
	}

	return;
}

static void __cf_cleanup(
	unsigned int *depth, char **arguments, cond_rc_t *cond_rc)
{
	int i;

	(*depth)--;
	if (!(*depth))
	{
		Scr.flags.is_executing_complex_function = 0;
	}
	for (i = 0; i < 11; i++)
	{
		if (arguments[i] != NULL)
		{
			free(arguments[i]);
		}
	}
	if (cond_rc->break_levels > 0)
	{
		cond_rc->break_levels--;
	}

	return;
}

static void execute_complex_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	Bool *desperate, Bool has_ref_window_moved)
{
	cond_rc_t tmp_rc;
	cfunc_action_t type = CF_MOTION;
	char c;
	FunctionItem *fi;
	Bool Persist = False;
	Bool HaveDoubleClick = False;
	Bool HaveHold = False;
	Bool NeedsTarget = False;
	Bool ImmediateNeedsTarget = False;
	int do_allow_unmanaged = FUNC_ALLOW_UNMANAGED;
	int do_allow_unmanaged_immediate = FUNC_ALLOW_UNMANAGED;
	char *arguments[11], *taction;
	char *func_name;
	int x, y ,i;
	XEvent d;
	FvwmFunction *func;
	static unsigned int depth = 0;
	const exec_context_t *exc2;
	exec_context_changes_t ecc;
	exec_context_change_mask_t mask;
	int trigger_evtype;
	int button;
	XEvent *te;

	if (cond_rc == NULL)
	{
		condrc_init(&tmp_rc);
		cond_rc = &tmp_rc;
	}
	cond_rc->rc = COND_RC_OK;
	mask = 0;
	d.type = 0;
	ecc.w.fw = exc->w.fw;
	ecc.w.w = exc->w.w;
	ecc.w.wcontext = exc->w.wcontext;
	/* find_complex_function expects a token, not just a quoted string */
	func_name = PeekToken(action, &taction);
	if (!func_name)
	{
		return;
	}
	func = find_complex_function(func_name);
	if (func == NULL)
	{
		if (*desperate == 0)
		{
			fvwm_msg(
				ERR, "ComplexFunction", "No such function %s",
				action);
		}
		return;
	}
	if (!depth)
	{
		Scr.flags.is_executing_complex_function = 1;
	}
	depth++;
	*desperate = 0;
	/* duplicate the whole argument list for use as '$*' */
	if (taction)
	{
		arguments[0] = safestrdup(taction);
		/* strip trailing newline */
		if (arguments[0][0])
		{
			int l= strlen(arguments[0]);

			if (arguments[0][l - 1] == '\n')
			{
				arguments[0][l - 1] = 0;
			}
		}
		/* Get the argument list */
		for (i = 1; i < 11; i++)
		{
			taction = GetNextToken(taction, &arguments[i]);
		}
	}
	else
	{
		for (i = 0; i < 11; i++)
		{
			arguments[i] = NULL;
		}
	}
	/* In case we want to perform an action on a button press, we
	 * need to fool other routines */
	te = exc->x.elast;
	if (te->type == ButtonPress)
	{
		trigger_evtype = ButtonRelease;
	}
	else
	{
		trigger_evtype = te->type;
	}
	func->use_depth++;

	for (fi = func->first_item; fi != NULL; fi = fi->next_item)
	{
		if (fi->flags & FUNC_NEEDS_WINDOW)
		{
			NeedsTarget = True;
			do_allow_unmanaged &= fi->flags;
			if (fi->condition == CF_IMMEDIATE)
			{
				do_allow_unmanaged_immediate &= fi->flags;
				ImmediateNeedsTarget = True;
				break;
			}
		}
	}

	if (ImmediateNeedsTarget)
	{
		if (DeferExecution(
			    &ecc, &mask, CRS_SELECT, trigger_evtype,
			    do_allow_unmanaged_immediate))
		{
			func->use_depth--;
			__cf_cleanup(&depth, arguments, cond_rc);
			return;
		}
		NeedsTarget = False;
	}
	else
	{
		ecc.w.w = (ecc.w.fw) ? FW_W_FRAME(ecc.w.fw) : None;
		mask |= ECC_W;
	}

	/* we have to grab buttons before executing immediate actions because
	 * these actions can move the window away from the pointer so that a
	 * button release would go to the application below. */
	if (!GrabEm(CRS_NONE, GRAB_NORMAL))
	{
		func->use_depth--;
		fvwm_msg(
			ERR,
			"ComplexFunction", "Grab failed in function %s,"
			" unable to execute immediate action", action);
		__cf_cleanup(&depth, arguments, cond_rc);
		return;
	}
	exc2 = exc_clone_context(exc, &ecc, mask);
	__run_complex_function_items(
		cond_rc, CF_IMMEDIATE, func, exc2, arguments,
		has_ref_window_moved);
	exc_destroy_context(exc2);
	for (fi = func->first_item;
	     fi != NULL && cond_rc->break_levels == 0;
	     fi = fi->next_item)
	{
		/* c is already lowercase here */
		c = fi->condition;
		switch (c)
		{
		case CF_IMMEDIATE:
			break;
		case CF_DOUBLE_CLICK:
			HaveDoubleClick = True;
			Persist = True;
			break;
		case CF_HOLD:
			HaveHold = True;
			Persist = True;
			break;
		default:
			Persist = True;
			break;
		}
	}

	if (!Persist || cond_rc->break_levels != 0)
	{
		func->use_depth--;
		__cf_cleanup(&depth, arguments, cond_rc);
		UngrabEm(GRAB_NORMAL);
		return;
	}

	/* Only defer execution if there is a possibility of needing
	 * a window to operate on */
	if (NeedsTarget)
	{
		if (DeferExecution(
			    &ecc, &mask, CRS_SELECT, trigger_evtype,
			    do_allow_unmanaged))
		{
			func->use_depth--;
			__cf_cleanup(&depth, arguments, cond_rc);
			UngrabEm(GRAB_NORMAL);
			return;
		}
	}

	te = (mask & ECC_ETRIGGER) ? ecc.x.etrigger : exc->x.elast;
	switch (te->xany.type)
	{
	case ButtonPress:
	case ButtonRelease:
		x = te->xbutton.x_root;
		y = te->xbutton.y_root;
		button = te->xbutton.button;
		/* Take the click which started this fuction off the
		 * Event queue.  -DDN- Dan D Niles dniles@iname.com */
		FCheckMaskEvent(dpy, ButtonPressMask, &d);
		break;
	default:
		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y,
			    &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		button = 0;
		break;
	}

	/* Wait and see if we have a click, or a move */
	/* wait forever, see if the user releases the button */
	type = CheckActionType(x, y, &d, HaveHold, True, &button);
	if (type == CF_CLICK)
	{
		int button2;

		/* If it was a click, wait to see if its a double click */
		if (HaveDoubleClick)
		{
			type = CheckActionType(
				x, y, &d, True, False, &button2);
			switch (type)
			{
			case CF_HOLD:
			case CF_MOTION:
			case CF_CLICK:
				if (button == button2)
				{
					type = CF_DOUBLE_CLICK;
				}
				else
				{
					type = CF_CLICK;
				}
				break;
			case CF_TIMEOUT:
				type = CF_CLICK;
				break;
			default:
				/* can't happen */
				break;
			}
		}
	}
	else if (type == CF_TIMEOUT)
	{
		type = CF_HOLD;
	}

	/* some functions operate on button release instead of presses. These
	 * gets really weird for complex functions ... */
	if (d.type == ButtonPress)
	{
		d.type = ButtonRelease;
		if (d.xbutton.button > 0 &&
		    d.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS)
		{
			d.xbutton.state &=
				(~(Button1Mask >> (d.xbutton.button - 1)));
		}
	}

#ifdef BUGGY_CODE
	/* domivogt (11-Apr-2000): The pointer ***must not*** be ungrabbed
	 * here.  If it is, any window that the mouse enters during the
	 * function will receive MotionNotify events with a button held down!
	 * The results are unpredictable.  E.g. rxvt interprets the
	 * ButtonMotion as user input to select text. */
	UngrabEm(GRAB_NORMAL);
#endif
	fev_set_evpos(&d, x, y);
	fev_fake_event(&d);
	ecc.x.etrigger = &d;
	ecc.w.w = (ecc.w.fw) ? FW_W_FRAME(ecc.w.fw) : None;
	mask |= ECC_ETRIGGER | ECC_W;
	exc2 = exc_clone_context(exc, &ecc, mask);
	__run_complex_function_items(
		cond_rc, type, func, exc2, arguments, has_ref_window_moved);
	exc_destroy_context(exc2);
	/* This is the right place to ungrab the pointer (see comment above).
	 */
	func->use_depth--;
	__cf_cleanup(&depth, arguments, cond_rc);
	UngrabEm(GRAB_NORMAL);

	return;
}

/*
 * create a new FvwmFunction
 */
static FvwmFunction *NewFvwmFunction(const char *name)
{
	FvwmFunction *tmp;

	tmp = (FvwmFunction *)safemalloc(sizeof(FvwmFunction));
	tmp->next_func = Scr.functions;
	tmp->first_item = NULL;
	tmp->last_item = NULL;
	tmp->name = stripcpy(name);
	tmp->use_depth = 0;
	Scr.functions = tmp;

	return tmp;
}

static void DestroyFunction(FvwmFunction *func)
{
	FunctionItem *fi,*tmp2;
	FvwmFunction *tmp, *prev;

	if (func == NULL)
	{
		return;
	}

	tmp = Scr.functions;
	prev = NULL;
	while (tmp && tmp != func)
	{
		prev = tmp;
		tmp = tmp->next_func;
	}
	if (tmp != func)
	{
		return;
	}

	if (func->use_depth != 0)
	{
		fvwm_msg(
			ERR,"DestroyFunction",
			"Function %s is in use (depth %d)", func->name,
			func->use_depth);
		return;
	}

	if (prev == NULL)
	{
		Scr.functions = func->next_func;
	}
	else
	{
		prev->next_func = func->next_func;
	}

	free(func->name);

	fi = func->first_item;
	while (fi != NULL)
	{
		tmp2 = fi->next_item;
		if (fi->action != NULL)
		{
			free(fi->action);
		}
		free(fi);
		fi = tmp2;
	}
	free(func);

	return;
}

/* ---------------------------- interface functions ------------------------ */

Bool functions_is_complex_function(const char *function_name)
{
	if (find_complex_function(function_name) != NULL)
	{
		return True;
	}

	return False;
}

void execute_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags)
{
	__execute_function(cond_rc, exc, action, exec_flags, NULL, False);

	return;
}

void execute_function_override_wcontext(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, int wcontext)
{
	const exec_context_t *exc2;
	exec_context_changes_t ecc;

	ecc.w.wcontext = wcontext;
	exc2 = exc_clone_context(exc, &ecc, ECC_WCONTEXT);
	execute_function(cond_rc, exc2, action, exec_flags);
	exc_destroy_context(exc2);

	return;
}

void execute_function_override_window(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, FvwmWindow *fw)
{
	const exec_context_t *exc2;
	exec_context_changes_t ecc;

	ecc.w.fw = fw;
	if (fw != NULL)
	{
		ecc.w.w = FW_W(fw);
		ecc.w.wcontext = C_WINDOW;
		exec_flags |= FUNC_DONT_DEFER;
	}
	else
	{
		ecc.w.w = None;
		ecc.w.wcontext = C_ROOT;
	}
	if (exc != NULL)
	{
		exc2 = exc_clone_context(
			exc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
	}
	else
	{
		ecc.type = EXCT_NULL;
		exc2 = exc_create_context(
			&ecc, ECC_TYPE | ECC_FW | ECC_W | ECC_WCONTEXT);
	}
	execute_function(cond_rc, exc2, action, exec_flags);
	exc_destroy_context(exc2);

	return;
}

void find_func_t(char *action, short *func_t, unsigned char *flags)
{
	int j, len = 0;
	char *endtok = action;
	Bool matched;
	int mlen;

	if (action)
	{
		while (*endtok && !isspace((unsigned char)*endtok))
		{
			++endtok;
		}
		len = endtok - action;
		j=0;
		matched = False;
		while (!matched && (mlen = strlen(func_table[j].keyword)) > 0)
		{
			if (mlen == len &&
			    strncasecmp(action,func_table[j].keyword,mlen) ==
			    0)
			{
				matched=True;
				/* found key word */
				if (func_t)
				{
					*func_t = func_table[j].func_t;
				}
				if (flags)
				{
					*flags = func_table[j].flags;
				}
				return;
			}
			else
			{
				j++;
			}
		}
		/* No clue what the function is. Just return "BEEP" */
	}
	if (func_t)
	{
		*func_t = F_BEEP;
	}
	if (flags)
	{
		*flags = 0;
	}

	return;
}


/*
 *  add an item to a FvwmFunction
 *
 *  Inputs:
 *      func      - pointer to the FvwmFunction to add the item
 *      action    - the definition string from the config line
 */
void AddToFunction(FvwmFunction *func, char *action)
{
	FunctionItem *tmp;
	char *token = NULL;
	char condition;

	token = PeekToken(action, &action);
	if (!token)
		return;
	condition = token[0];
	if (isupper(condition))
		condition = tolower(condition);
	if (condition != CF_IMMEDIATE &&
	    condition != CF_MOTION &&
	    condition != CF_HOLD &&
	    condition != CF_CLICK &&
	    condition != CF_DOUBLE_CLICK)
	{
		fvwm_msg(
			ERR, "AddToFunction",
			"Got '%s' instead of a valid function specifier",
			token);
		return;
	}
	if (token[0] != 0 && token[1] != 0 &&
	    (find_builtin_function(token) || find_complex_function(token)))
	{
		fvwm_msg(
			WARN, "AddToFunction",
			"Got the command or function name '%s' instead of a"
			" function specifier. This may indicate a syntax"
			" error in the configuration file. Using %c as the"
			" specifier.", token, token[0]);
	}
	if (!action)
	{
		return;
	}
	while (isspace(*action))
	{
		action++;
	}
	if (*action == 0)
	{
		return;
	}

	tmp = (FunctionItem *)safemalloc(sizeof(FunctionItem));
	tmp->next_item = NULL;
	tmp->func = func;
	if (func->first_item == NULL)
	{
		func->first_item = tmp;
		func->last_item = tmp;
	}
	else
	{
		func->last_item->next_item = tmp;
		func->last_item = tmp;
	}

	tmp->condition = condition;
	tmp->action = stripcpy(action);

	find_func_t(tmp->action, NULL, &(tmp->flags));

	return;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_DestroyFunc(F_CMD_ARGS)
{
	FvwmFunction *func;
	char *token;

	GetNextToken(action,&token);
	if (!token)
	{
		return;
	}
	func = find_complex_function(token);
	free(token);
	if (!func)
	{
		return;
	}
	if (Scr.last_added_item.type == ADDED_FUNCTION)
	{
		set_last_added_item(ADDED_NONE, NULL);
	}
	DestroyFunction(func);

	return;
}

void CMD_AddToFunc(F_CMD_ARGS)
{
	FvwmFunction *func;
	char *token;

	action = GetNextToken(action,&token);
	if (!token)
	{
		return;
	}
	func = find_complex_function(token);
	if (func == NULL)
	{
		func = NewFvwmFunction(token);
	}

	/* Set + state to last function */
	set_last_added_item(ADDED_FUNCTION, func);

	free(token);
	AddToFunction(func, action);

	return;
}

void CMD_Plus(F_CMD_ARGS)
{
	if (Scr.last_added_item.type == ADDED_MENU)
	{
		add_another_menu_item(action);
	}
	else if (Scr.last_added_item.type == ADDED_FUNCTION)
	{
		AddToFunction(Scr.last_added_item.item, action);
	}
#ifdef USEDECOR
	else if (Scr.last_added_item.type == ADDED_DECOR)
	{
		FvwmDecor *tmp = &Scr.DefaultDecor;
		for ( ; tmp && tmp != Scr.last_added_item.item; tmp = tmp->next)
		{
			/* nothing to do here */
		}
		if (!tmp)
		{
			return;
		}
		AddToDecor(F_PASS_ARGS, tmp);
	}
#endif /* USEDECOR */

	return;
}
