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

/*
 * fvwm built-in functions and complex functions
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#if 1 /*!!!*/
#include <assert.h>
#endif

#include "libs/fvwm_x11.h"
#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/FEvent.h"
#include "libs/Event.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "functable.h"
#include "functable_complex.h"
#include "cmdparser.h"
#include "cmdparser_hooks.h"
#include "cmdparser_old.h"
#include "functions.h"
#include "commands.h"
#include "events.h"
#include "modconf.h"
#include "module_list.h"
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

/* ---------------------------- forward declarations ----------------------- */

static void execute_complex_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, cmdparser_context_t *pc,
	FvwmFunction *func, Bool has_ref_window_moved);

 /* ---------------------------- local variables ---------------------------- */

/* Temporary instance of the hooks functions used in this file.  The goal is
 * to remove all parsing and expansion from functions.c and move it into a
 * separate, replaceable file.  */

static const cmdparser_hooks_t *cmdparser_hooks = NULL;

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

static void __execute_command_line(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *xaction,
	cmdparser_context_t *caller_pc,
	func_flags_t exec_flags, char *all_pos_args_string,
	char *pos_arg_tokens[], Bool has_ref_window_moved)
{
	cmdparser_context_t pc;
	cond_rc_t *func_rc = NULL;
	cond_rc_t dummy_rc;
	Window w;
	char *err_cline;
	const char *err_func;
	cmdparser_execute_type_t exec_type;
	const func_t *bif;
	FvwmFunction *complex_function;
	int set_silent;
	int do_keep_rc = 0;
	/* needed to be able to avoid resize to use moved windows for base */
	extern Window PressedW;
	Window dummy_w;
	int rc;

#if 1 /*!!!*/
	fprintf(stderr, "%s: cpc %p, xaction '%s'\n", __func__, caller_pc, xaction);
#endif
	set_silent = 0;
	/* generate a parsing context; this *must* be destroyed before
	 * returning */
	rc = cmdparser_hooks->create_context(
		&pc, caller_pc, xaction, all_pos_args_string, pos_arg_tokens);
	if (rc != 0)
	{
		goto fn_exit;
	}
cmdparser_hooks->debug(&pc, "!!!A");
	rc = cmdparser_hooks->handle_line_start(&pc);
	if (rc != 0)
	{
		goto fn_exit;
	}
cmdparser_hooks->debug(&pc, "!!!B");

	{
		cmdparser_prefix_flags_t flags;

		flags = cmdparser_hooks->handle_line_prefix(&pc);
cmdparser_hooks->debug(&pc, "!!!BA");
		if (flags & CP_PREFIX_MINUS)
		{
#if 1 /*!!!*/
fprintf(stderr, "!!!do not expand\n");
#endif
			exec_flags |= FUNC_DONT_EXPAND_COMMAND;
		}
		if (flags & CP_PREFIX_SILENT)
		{
#if 1 /*!!!*/
fprintf(stderr, "!!!is silent\n");
#endif
			if (Scr.flags.are_functions_silent == 0)
			{
				set_silent = 1;
				Scr.flags.are_functions_silent = 1;
			}
		}
		if (flags & CP_PREFIX_KEEPRC)
		{
#if 1 /*!!!*/
fprintf(stderr, "!!!do keeprc\n");
#endif
			do_keep_rc = 1;
		}
		if (pc.cline == NULL)
		{
			goto fn_exit;
		}
		err_cline = pc.cline;
	}
cmdparser_hooks->debug(&pc, "!!!C");

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
	if (cond_rc == NULL || do_keep_rc)
	{
		condrc_init(&dummy_rc);
		func_rc = &dummy_rc;
	}
	else
	{
		func_rc = cond_rc;
	}
	{
		const char *func;

		func = cmdparser_hooks->parse_command_name(&pc, func_rc, exc);
		if (func != NULL)
		{
cmdparser_hooks->debug(&pc, "!!!D");
			bif = find_builtin_function(func);
			err_func = func;
		}
		else
		{
cmdparser_hooks->debug(&pc, "!!!E");
			bif = NULL;
			err_func = "";
		}
	}

	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor &&
	    (!bif || !(bif->flags & FUNC_DECOR)))
	{
		fvwm_debug(__func__,
			   "Command can not be added to a decor; executing"
			   " command now: '%s'", err_cline);
	}

	if (!(exec_flags & FUNC_DONT_EXPAND_COMMAND))
	{
		cmdparser_hooks->expand_command_line(
			&pc, (bif) ? !!(bif->flags & FUNC_ADD_TO) : False,
			func_rc, exc);
cmdparser_hooks->debug(&pc, "!!!F");
		if (pc.call_depth <= 1)
		{
			Bool do_free_string_ourselves;

			do_free_string_ourselves = set_repeat_data(
				pc.expline, REPEAT_COMMAND, bif);
			if (do_free_string_ourselves == False)
			{
				cmdparser_hooks->release_expanded_line(&pc);
cmdparser_hooks->debug(&pc, "!!!F");
			}
		}
	}
#if 1 /*!!!*/
fprintf(stderr, "!!!pc.cline: '%s'\n", pc.cline);
#endif

	exec_type = cmdparser_hooks->find_something_to_execute(
		&pc, &bif, &complex_function);
cmdparser_hooks->debug(&pc, "!!!H");
fprintf(stderr, "!!!exec_type %d\n", exec_type);
	switch (exec_type)
	{
	case CP_EXECTYPE_BUILTIN_FUNCTION:
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;
		exec_context_change_mask_t mask;

cmdparser_hooks->debug(&pc, "!!!J");
#if 1 /*!!!*/
		assert(bif && bif->func_c != F_FUNCTION);
#endif
#if 1 /*!!!*/
fprintf(stderr, "!!!pc.cline: '%s'\n", pc.cline);
#endif
		mask = (w != exc->w.w) ? ECC_W : 0;
		ecc.w.fw = exc->w.fw;
		ecc.w.w = w;
		ecc.w.wcontext = exc->w.wcontext;
		if (
			(bif->flags & FUNC_NEEDS_WINDOW) &&
			!(exec_flags & FUNC_DONT_DEFER))
		{
			Bool rc;

			rc = DeferExecution(
				&ecc, &mask, bif->cursor, exc->x.elast->type,
				(bif->flags & FUNC_ALLOW_UNMANAGED));
			if (rc == True)
			{
				break;
			}
#if 1 /*!!!*/
fprintf(stderr, "!!!deferred: %d\n", rc);
#endif
		}
		else if (
			(bif->flags & FUNC_NEEDS_WINDOW) &&
			!__context_has_window(
				exc, bif->flags & FUNC_ALLOW_UNMANAGED))
		{
#if 1 /*!!!*/
fprintf(stderr, "!!!skip no-defer\n");
#endif
			/* no context window and not allowed to defer,
			 * skip command */
			break;
		}
		exc2 = exc_clone_context(exc, &ecc, mask);
		dummy_w = PressedW;
		if (
			has_ref_window_moved &&
			(bif->flags & FUNC_IS_MOVE_TYPE))
		{
#if 1 /*!!!*/
fprintf(stderr, "!!!erase PressedW\n");
#endif
			PressedW = None;
		}
#if 1 /*!!!*/
fprintf(stderr, "!!!execute '%s'\n", bif->keyword);
#endif
		bif->action(func_rc, exc2, pc.cline, &pc);
		PressedW = dummy_w;
		exc_destroy_context(exc2);
		break;
	}
	case CP_EXECTYPE_COMPLEX_FUNCTION:
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;
		exec_context_change_mask_t mask;

		mask = (w != exc->w.w) ? ECC_W : 0;
		ecc.w.fw = exc->w.fw;
		ecc.w.w = w;
		ecc.w.wcontext = exc->w.wcontext;
		exc2 = exc_clone_context(exc, &ecc, mask);
		execute_complex_function(
			func_rc, exc2, &pc, complex_function,
			has_ref_window_moved);
		exc_destroy_context(exc2);
		break;
	}
	case CP_EXECTYPE_COMPLEX_FUNCTION_DOES_NOT_EXIST:
		fvwm_debug(
			__func__, "No such function %s",
			pc.complex_function_name);
		break;
	case CP_EXECTYPE_MODULECONFIG:
		/* Note: the module config command, "*" can not be handled by
		 * the regular command table because there is no required
		 * white space after the asterisk. */
		if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
		{
			fvwm_debug(
				__func__, "Command can not be added to a decor;"
				" executing command now: '%s'", pc.expline);
		}
		/* process a module config command */
		ModuleConfig(pc.expline);
		goto fn_exit;
	case CP_EXECTYPE_UNKNOWN:
		if (executeModuleDesperate(func_rc, exc, pc.cline, &pc) ==
		    NULL && *err_func != 0 && !set_silent)
		{
			fvwm_debug(__func__, "No such command '%s'", err_func);
		}
		break;
#if 1 /*!!!*/
	default:
		assert(!"bad exec_type");
#endif
	}

  fn_exit:
	if (func_rc != NULL && cond_rc != NULL)
	{
		cond_rc->break_levels = func_rc->break_levels;
	}
	if (set_silent)
	{
		Scr.flags.are_functions_silent = 0;
	}
	cmdparser_hooks->destroy_context(&pc);

	return;
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
	const exec_context_t *exc, cmdparser_context_t *caller_pc,
	char *all_pos_args_string, char *pos_arg_tokens[],
	Bool has_ref_window_moved)
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
			__execute_command_line(
				cond_rc, exc, fi->action, caller_pc,
				FUNC_DONT_DEFER, all_pos_args_string,
				pos_arg_tokens, has_ref_window_moved);
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
	int *depth, char *all_pos_args_string, char **pos_arg_tokens,
	cond_rc_t *cond_rc)
{
	int i;

	(*depth)--;
	if (!(*depth))
	{
		Scr.flags.is_executing_complex_function = 0;
	}
	if (all_pos_args_string != NULL)
	{
		free(all_pos_args_string);
	}
	for (i = 0; i < CMDPARSER_NUM_POS_ARGS; i++)
	{
		if (pos_arg_tokens[i] != NULL)
		{
			free(pos_arg_tokens[i]);
		}
	}
	if (cond_rc->break_levels > 0)
	{
		cond_rc->break_levels--;
	}

	return;
}

static void execute_complex_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, cmdparser_context_t *pc,
	FvwmFunction *func, Bool has_ref_window_moved)
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
	int has_immediate = 0;
	int do_run_late_immediate = 0;
	int do_allow_unmanaged = FUNC_ALLOW_UNMANAGED;
	int do_allow_unmanaged_immediate = FUNC_ALLOW_UNMANAGED;
	char *all_pos_args_string;
	char *pos_arg_tokens[CMDPARSER_NUM_POS_ARGS];
	char *taction;
	int x, y ,i;
	XEvent d;
	static int depth = 0;
	const exec_context_t *exc2;
	exec_context_changes_t ecc;
	exec_context_change_mask_t mask;
	int trigger_evtype;
	int button;
	XEvent *te;

#if 1 /*!!!*/
	assert(func != NULL);
#endif
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
	if (!depth)
	{
		Scr.flags.is_executing_complex_function = 1;
	}
	depth++;
	/* duplicate the whole argument list for use as '$*' */
	taction = pc->cline;
	if (taction)
	{
		all_pos_args_string = fxstrdup(taction);
		/* strip trailing newline */
		if (all_pos_args_string[0])
		{
			int l = strlen(all_pos_args_string);

			if (all_pos_args_string[l - 1] == '\n')
			{
				all_pos_args_string[l - 1] = 0;
			}
		}
		/* Get the argument list */
		for (i = 0; i < CMDPARSER_NUM_POS_ARGS; i++)
		{
			taction = GetNextToken(taction, &pos_arg_tokens[i]);
		}
	}
	else
	{
		all_pos_args_string = 0;
		memset(pos_arg_tokens, 0, sizeof(pos_arg_tokens));
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
		if (fi->condition == CF_IMMEDIATE)
		{
			has_immediate = 1;
		}
		if (fi->flags & FUNC_NEEDS_WINDOW)
		{
			NeedsTarget = True;
			do_allow_unmanaged &= fi->flags;
			if (fi->condition == CF_IMMEDIATE)
			{
				do_allow_unmanaged_immediate &= fi->flags;
				ImmediateNeedsTarget = True;
			}
		}
	}

	if (ImmediateNeedsTarget)
	{
		if (DeferExecution(
			    &ecc, &mask, CRS_SELECT, trigger_evtype,
			    do_allow_unmanaged_immediate) == True)
		{
			func->use_depth--;
			__cf_cleanup(
				&depth, all_pos_args_string, pos_arg_tokens,
				cond_rc);
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
		fvwm_debug(
			__func__,
			"Grab failed, unable to execute immediate action");
		__cf_cleanup(
			&depth, all_pos_args_string, pos_arg_tokens, cond_rc);
		return;
	}
	if (has_immediate)
	{
		exc2 = exc_clone_context(exc, &ecc, mask);
		__run_complex_function_items(
			cond_rc, CF_IMMEDIATE, func, exc2, pc,
			all_pos_args_string, pos_arg_tokens,
			has_ref_window_moved);
		exc_destroy_context(exc2);
	}
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
		case CF_LATE_IMMEDIATE:
			do_run_late_immediate = 1;
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
		__cf_cleanup(
			&depth, all_pos_args_string, pos_arg_tokens, cond_rc);
		UngrabEm(GRAB_NORMAL);
		return;
	}

	/* Only defer execution if there is a possibility of needing
	 * a window to operate on */
	if (NeedsTarget)
	{
		if (DeferExecution(
			    &ecc, &mask, CRS_SELECT, trigger_evtype,
			    do_allow_unmanaged) == True)
		{
			func->use_depth--;
			__cf_cleanup(
				&depth, all_pos_args_string, pos_arg_tokens,
				cond_rc);
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
	if (do_run_late_immediate)
	{
		exc2 = exc_clone_context(exc, &ecc, mask);
		__run_complex_function_items(
			cond_rc, CF_LATE_IMMEDIATE, func, exc2, pc,
			all_pos_args_string, pos_arg_tokens,
			has_ref_window_moved);
		exc_destroy_context(exc2);
		do_run_late_immediate = 0;
	}
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
	if (do_run_late_immediate)
	{
		__run_complex_function_items(
			cond_rc, CF_LATE_IMMEDIATE, func, exc2, pc,
			all_pos_args_string, pos_arg_tokens,
			has_ref_window_moved);
	}
	__run_complex_function_items(
		cond_rc, type, func, exc2, pc, all_pos_args_string,
		pos_arg_tokens, has_ref_window_moved);
	exc_destroy_context(exc2);
	/* This is the right place to ungrab the pointer (see comment above).
	 */
	func->use_depth--;
	__cf_cleanup(&depth, all_pos_args_string, pos_arg_tokens, cond_rc);
	UngrabEm(GRAB_NORMAL);

	return;
}

/* ---------------------------- interface functions ------------------------ */

void functions_init(void)
{
	cmdparser_hooks = cmdparser_old_get_hooks();

	return;
}

void execute_function(F_CMD_ARGS, func_flags_t exec_flags)
{
	__execute_command_line(F_PASS_ARGS, exec_flags, NULL, NULL, False);

	return;
}

void execute_function_override_wcontext(
	F_CMD_ARGS, func_flags_t exec_flags, int wcontext)
{
	const exec_context_t *exc2;
	exec_context_changes_t ecc;

	ecc.w.wcontext = wcontext;
	exc2 = exc_clone_context(exc, &ecc, ECC_WCONTEXT);
	execute_function(F_PASS_ARGS_WITH_EXC(exc2), exec_flags);
	exc_destroy_context(exc2);

	return;
}

void execute_function_override_window(
	F_CMD_ARGS, func_flags_t exec_flags, FvwmWindow *fw)
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
	execute_function(F_PASS_ARGS_WITH_EXC(exc2), exec_flags);
	exc_destroy_context(exc2);

	return;
}

void find_func_t(char *action, short *ret_func_c, func_flags_t *flags)
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
				if (ret_func_c)
				{
					*ret_func_c = func_table[j].func_c;
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
	if (ret_func_c)
	{
		*ret_func_c = F_BEEP;
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
	    condition != CF_LATE_IMMEDIATE &&
	    condition != CF_MOTION &&
	    condition != CF_HOLD &&
	    condition != CF_CLICK &&
	    condition != CF_DOUBLE_CLICK)
	{
		fvwm_debug(__func__,
			   "Got '%s' instead of a valid function specifier",
			   token);
		return;
	}
	if (token[0] != 0 && token[1] != 0 &&
	    (find_builtin_function(token) || find_complex_function(token)))
	{
		fvwm_debug(__func__,
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

	tmp = fxmalloc(sizeof *tmp);
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

	token = PeekToken(action, NULL);
	if (!token)
	{
		return;
	}
	func = find_complex_function(token);
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

	return;
}

void CMD_EchoFuncDefinition(F_CMD_ARGS)
{
	FvwmFunction *func;
	const func_t *bif;
	FunctionItem *fi;
	char *token;

	GetNextToken(action, &token);
	if (!token)
	{
		fvwm_debug(__func__, "Missing argument");

		return;
	}
	bif = find_builtin_function(token);
	if (bif != NULL)
	{
		fvwm_debug(__func__,
			   "function '%s' is a built in command", token);
		free(token);

		return;
	}
	func = find_complex_function(token);
	if (!func)
	{
		fvwm_debug(__func__,
			   "function '%s' not defined", token);
		free(token);

		return;
	}
	fvwm_debug(__func__, "definition of function '%s':",
		   token);
	for (fi = func->first_item; fi != NULL; fi = fi->next_item)
	{
		fvwm_debug(__func__, "  %c %s", fi->condition,
			   (fi->action == 0) ? "(null)" : fi->action);
	}
	fvwm_debug(__func__, "end of definition");
	free(token);

	return;
}

/* dummy commands */
void CMD_Title(F_CMD_ARGS) { }
void CMD_TearMenuOff(F_CMD_ARGS) { }
void CMD_KeepRc(F_CMD_ARGS) { }
void CMD_Silent(F_CMD_ARGS) { }
void CMD_Function(F_CMD_ARGS) { }
