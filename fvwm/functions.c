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

/***********************************************************************
 *
 * fvwm built-in functions and complex functions
 *
 ***********************************************************************/

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "commands.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "functable.h"
#include "events.h"
#include "modconf.h"
#include "module_interface.h"
#include "misc.h"
#include "screen.h"
#include "geometry.h"
#include "repeat.h"
#include "read.h"
#include "virtual.h"
#include "colorset.h"
#include "ewmh.h"
#include "schedule.h"
#include "expand.h"
#include "menus.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

extern FvwmWindow *Fw;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

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
} cfunc_action_type;

/* ---------------------------- forward declarations ------------------------ */

static void execute_complex_function(F_CMD_ARGS, Bool *desperate);

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/* dummies */
void CMD_Silent(F_CMD_ARGS)
{
	return;
}
void CMD_Function(F_CMD_ARGS)
{
	return;
}
void CMD_Title(F_CMD_ARGS)
{
	return;
}
void CMD_TearMenuOff(F_CMD_ARGS)
{
	return;
}

/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
	return (strcmp((char *)a, ((func_type *)b)->keyword));
}

static const func_type *find_builtin_function(char *func)
{
	static int nfuncs = 0;
	func_type *ret_func;
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
	ret_func = (func_type *)bsearch(
		temp, func_table, nfuncs, sizeof(func_type), func_comp);
	free(temp);

	return ret_func;
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

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
static cfunc_action_type CheckActionType(
	int x, int y, XEvent *d, Bool may_time_out, Bool is_button_pressed)
{
	int xcurrent,ycurrent,total = 0;
	Time t0;
	int dist;
	XEvent old_event;
	extern Time lastTimestamp;
	Bool do_sleep = False;

	xcurrent = x;
	ycurrent = y;
	t0 = lastTimestamp;
	dist = Scr.MoveThreshold;

	while ((total < Scr.ClickTime && lastTimestamp - t0 < Scr.ClickTime) ||
	       !may_time_out)
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
		if (XCheckMaskEvent(
			    dpy, ButtonReleaseMask|ButtonMotionMask|
			    PointerMotionMask|ButtonPressMask|ExposureMask, d))
		{
			do_sleep = 0;
			StashEventTime(d);
			switch (d->xany.type)
			{
			case ButtonRelease:
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
				if (may_time_out)
				{
					is_button_pressed = True;
				}
				break;
			case Expose:
				/* must handle expose here so that raising a
				 * window with "I" works */
				memcpy(&old_event, &Event, sizeof(XEvent));
				memcpy(&Event, d, sizeof(XEvent));
				/* note: handling Expose events never modifies
				 * the global Fw */
				DispatchEvent(True);
				memcpy(&Event, &old_event, sizeof(XEvent));
				break;
			default:
				/* can't happen */
				break;
			}
		}
	}

	return (is_button_pressed) ? CF_HOLD : CF_TIMEOUT;
}

static void cf_cleanup(unsigned int *depth, char **arguments)
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

	return;
}

static void execute_complex_function(F_CMD_ARGS, Bool *desperate)
{
	fvwm_cond_func_rc cond_func_rc = COND_RC_OK;
	cfunc_action_type type = CF_MOTION;
	char c;
	FunctionItem *fi;
	Bool Persist = False;
	Bool HaveDoubleClick = False;
	Bool HaveHold = False;
	Bool NeedsTarget = False;
	Bool ImmediateNeedsTarget = False;
	char *arguments[11], *taction;
	char* func_name;
	int x, y ,i;
	XEvent d, *ev;
	FvwmFunction *func;
	static unsigned int depth = 0;

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
		for (i=1;i<11;i++)
		{
			taction = GetNextToken(taction,&arguments[i]);
		}
	}
	else
	{
		for (i=0;i<11;i++)
		{
			arguments[i] = NULL;
		}
	}
	/* see functions.c to find out which functions need a window to operate
	 * on */
	ev = eventp;
	/* In case we want to perform an action on a button press, we
	 * need to fool other routines */
	if (eventp->type == ButtonPress)
	{
		eventp->type = ButtonRelease;
	}
	func->use_depth++;

	for (fi = func->first_item; fi != NULL; fi = fi->next_item)
	{
		if (fi->flags & FUNC_NEEDS_WINDOW)
		{
			NeedsTarget = True;
			if (fi->condition == CF_IMMEDIATE)
			{
				ImmediateNeedsTarget = True;
				break;
			}
		}
	}

	if (ImmediateNeedsTarget)
	{
		if (DeferExecution(
			    eventp, &w, &fw, &context, CRS_SELECT, ButtonPress))
		{
			func->use_depth--;
			cf_cleanup(&depth, arguments);
			return;
		}
		NeedsTarget = False;
	}

	/* we have to grab buttons before executing immediate actions because
	 * these actions can move the window away from the pointer so that a
	 * button release would go to the application below. */
	if (!GrabEm(CRS_NONE, GRAB_NORMAL))
	{
		func->use_depth--;
		XBell(dpy, 0);
		cf_cleanup(&depth, arguments);
		return;
	}
	for (fi = func->first_item; fi != NULL && cond_func_rc != COND_RC_BREAK;
	     fi = fi->next_item)
	{
		/* c is already lowercase here */
		c = fi->condition;
		switch (c)
		{
		case CF_IMMEDIATE:
			if (fw)
			{
				w = FW_W_FRAME(fw);
			}
			else
			{
				w = None;
			}
			old_execute_function(
				&cond_func_rc, fi->action, fw, eventp, context,
				-1, 0, arguments);
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

	if (!Persist || cond_func_rc == COND_RC_BREAK)
	{
		func->use_depth--;
		cf_cleanup(&depth, arguments);
		UngrabEm(GRAB_NORMAL);
		return;
	}

	/* Only defer execution if there is a possibility of needing
	 * a window to operate on */
	if (NeedsTarget)
	{
		if (DeferExecution(
			    eventp, &w, &fw, &context, CRS_SELECT, ButtonPress))
		{
			func->use_depth--;
			cf_cleanup(&depth, arguments);
			UngrabEm(GRAB_NORMAL);
			return;
		}
	}

	switch (eventp->xany.type)
	{
	case ButtonPress:
	case ButtonRelease:
		x = eventp->xbutton.x_root;
		y = eventp->xbutton.y_root;
		/* Take the click which started this fuction off the
		 * Event queue.  -DDN- Dan D Niles dniles@iname.com */
		XCheckMaskEvent(dpy, ButtonPressMask, &d);
		break;
	default:
		if (XQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y,
			    &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		break;
	}

	/* Wait and see if we have a click, or a move */
	/* wait forever, see if the user releases the button */
	type = CheckActionType(x, y, &d, HaveHold, True);
	if (type == CF_CLICK)
	{
		ev = &d;
		/* If it was a click, wait to see if its a double click */
		if (HaveDoubleClick)
		{
			type = CheckActionType(x, y, &d, True, False);
			switch (type)
			{
			case CF_CLICK:
			case CF_HOLD:
			case CF_MOTION:
				type = CF_DOUBLE_CLICK;
				ev = &d;
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

	/* some functions operate on button release instead of
	 * presses. These gets really weird for complex functions ... */
	if (ev->type == ButtonPress)
	{
		ev->type = ButtonRelease;
	}

	fi = func->first_item;
#ifdef BUGGY_CODE
	/* domivogt (11-Apr-2000): The pointer ***must not*** be ungrabbed
	 * here.  If it is, any window that the mouse enters during the
	 * function will receive MotionNotify events with a button held down!
	 * The results are unpredictable.  E.g. rxvt interprets the
	 * ButtonMotion as user input to select text. */
	UngrabEm(GRAB_NORMAL);
#endif
	while (fi != NULL)
	{
		/* make lower case */
		c = fi->condition;
		if (isupper(c))
		{
			c=tolower(c);
		}
		if (c == type)
		{
			if (fw)
			{
				w = FW_W_FRAME(fw);
			}
			else
			{
				w = None;
			}
			old_execute_function(
				&cond_func_rc, fi->action, fw, ev, context, -1,
				0, arguments);
		}
		fi = fi->next_item;
	}
	/* This is the right place to ungrab the pointer (see comment above). */
	func->use_depth--;
	cf_cleanup(&depth, arguments);
	UngrabEm(GRAB_NORMAL);

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      NewFunction - create a new FvwmFunction
 *
 *  Returned Value:
 *      (FvwmFunction *)
 *
 *  Inputs:
 *      name    - the name of the function
 *
 ***********************************************************************/
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

/* ---------------------------- interface functions ------------------------- */

Bool functions_is_complex_function(const char *function_name)
{
	if (find_complex_function(function_name) != NULL)
	{
		return True;
	}

	return False;
}

/***********************************************************************
 *
 *  Procedure:
 *      execute_function - execute a fvwm built in function
 *
 *  Inputs:
 *      Action  - the action to execute
 *      fw      - the fvwm window structure
 *      eventp  - pointer to the event that caused the function
 *      context - the context in which the button was pressed
 *
 ***********************************************************************/
void execute_function(exec_func_args_type *efa)
{
	static unsigned int func_depth = 0;
	FvwmWindow *s_Fw = Fw;
	Window w;
	int j;
	char *function;
	char *taction;
	char *trash;
	char *trash2;
	char *expaction = NULL;
	char *arguments[11];
	const func_type *bif;
	Bool set_silent;
	Bool must_free_string = False;
	Bool must_free_function = False;

	if (!efa->action)
	{
		/* impossibly short command */
		return;
	}
	/* ignore whitespace at the beginning of all config lines */
	efa->action = SkipSpaces(efa->action, NULL, 0);
	if (!efa->action || efa->action[0] == 0)
	{
		/* impossibly short command */
		return;
	}
	if (efa->action[0] == '#')
	{
		/* a comment */
		return;
	}

	func_depth++;
	if (efa->args)
	{
		for (j=0;j<11;j++)
		{
			arguments[j] = efa->args[j];
		}
	}
	else
	{
		for (j=0;j<11;j++)
		{
			arguments[j] = NULL;
		}
	}

	if (efa->fw == NULL || IS_EWMH_DESKTOP(FW_W(efa->fw)))
	{
		if (efa->flags.is_window_unmanaged)
		{
			w = efa->win;
		}
		else
		{
			w = Scr.Root;
		}
	}
	else
	{
		if (efa->eventp)
		{
			w = GetSubwindowFromEvent(dpy, efa->eventp);
			if (!w)
			{
				w = efa->eventp->xany.window;
			}
		}
		else
		{
			w = FW_W(efa->fw);
		}
	}

	set_silent = False;
	if (efa->action[0] == '-')
	{
		efa->flags.exec |= FUNC_DONT_EXPAND_COMMAND;
		efa->action++;
	}

	taction = efa->action;
	/* parse prefixes */
	trash = PeekToken(taction, &trash2);
	while (trash)
	{
		if (StrEquals(trash, PRE_SILENT))
		{
			if (Scr.flags.silent_functions == 0)
			{
				set_silent = 1;
				Scr.flags.silent_functions = 1;
			}
			taction = trash2;
			trash = PeekToken(taction, &trash2);
		}
		else
			break;
	}
	if (taction == NULL)
	{
		if (set_silent)
			Scr.flags.silent_functions = 0;
		func_depth--;
		return;
	}

	function = PeekToken(taction, NULL);
	if (function)
	{
		function = expand_vars(
			function, arguments, efa->fw, False, False,
			efa->cond_rc);
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
			ERR, "execute_function",
			"Command can not be added to a decor; executing"
			" command now: '%s'", efa->action);
	}
#endif

	if (!(efa->flags.exec & FUNC_DONT_EXPAND_COMMAND))
	{
		expaction = expand_vars(
			taction, arguments, efa->fw, (bif) ?
			!!(bif->flags & FUNC_ADD_TO) :
			False, (taction[0] == '*'), efa->cond_rc);
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
	fvwm_msg(INFO, "LOG", "%s", expaction);
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
				WARN, "execute_function",
				"Command can not be added to a decor;"
				" executing command now: '%s'", expaction);
		}
#endif
		/* process a module config command */
		ModuleConfig(expaction);
	}
	else
	{
		if (bif && bif->func_type != F_FUNCTION)
		{
			char *runaction;

			runaction = SkipNTokens(expaction, 1);
			bif->action(
				efa->cond_rc, efa->eventp, w, efa->fw,
				efa->context, runaction, &efa->module);
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

			execute_complex_function(
				efa->cond_rc, efa->eventp, w, efa->fw,
				efa->context, runaction, &efa->module,
				&desperate);
			if (!bif && desperate)
			{
				if (executeModuleDesperate(
					    efa->cond_rc, efa->eventp, w,
					    efa->fw, efa->context, runaction,
					    &efa->module) == -1 &&
				    *function != 0 && !set_silent)
				{
					fvwm_msg(
						ERR, "execute_function",
						"No such command '%s'",
						function);
				}
			}
		}
	}

	if (set_silent)
	{
		Scr.flags.silent_functions = 0;
	}

	if (must_free_string)
	{
		free(expaction);
	}
	if (must_free_function)
	{
		free(function);
	}
	if (efa->flags.do_save_tmpwin)
	{
		if (check_if_fvwm_window_exists(s_Fw))
		{
			Fw = s_Fw;
		}
		else
		{
			Fw = NULL;
		}
	}
	func_depth--;

	return;
}

void old_execute_function(
	fvwm_cond_func_rc *cond_rc, char *action, FvwmWindow *fw,
	XEvent *eventp, unsigned long context, int Module,
	FUNC_FLAGS_TYPE exec_flags, char *args[])
{
	exec_func_args_type efa;

	memset(&efa, 0, sizeof(efa));
	efa.cond_rc = cond_rc;
	efa.eventp = eventp;
	efa.fw = fw;
	efa.action = action;
	efa.args = args;
	efa.context = context;
	efa.module = Module;
	efa.flags.exec = exec_flags;
	execute_function(&efa);

	return;
}


/***********************************************************************
 *
 *  Procedure:
 *      DeferExecution - defer the execution of a function to the
 *          next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      fw - pointer to FvwmWindow Structure to patch up
 *      context - the context in which the mouse button was pressed
 *      func    - the function to defer
 *      cursor  - the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(
	XEvent *eventp, Window *w, FvwmWindow **fw, unsigned long *context,
	cursor_t cursor, int FinishEvent)
{
	int done;
	int finished = 0;
	Window dummy;
	Window original_w;

	original_w = *w;

	if (*context != C_ROOT && *context != C_NO_CONTEXT && *fw != NULL &&
	    *context != C_EWMH_DESKTOP)
	{
		if (FinishEvent == ButtonPress ||
		    (FinishEvent == ButtonRelease &&
		     eventp->type != ButtonPress))
		{
			return False;
		}
		else if (FinishEvent == ButtonRelease)
		{
			/* We are only waiting until the user releases the
			 * button. Do not change the cursor. */
			cursor = CRS_NONE;
		}
	}
	if (Scr.flags.silent_functions)
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
		XMaskEvent(
			dpy, ButtonPressMask | ButtonReleaseMask |
			ExposureMask | KeyPressMask | VisibilityChangeMask |
			ButtonMotionMask | PointerMotionMask
			/* | EnterWindowMask | LeaveWindowMask*/, eventp);
		StashEventTime(eventp);

		if (eventp->type == KeyPress)
		{
			KeySym keysym = XLookupKeysym(&eventp->xkey,0);
			if (keysym == XK_Escape)
			{
				UngrabEm(GRAB_NORMAL);
				MyXUngrabKeyboard(dpy);
				return True;
			}
			Keyboard_shortcuts(
				eventp, NULL, NULL, NULL, FinishEvent);
		}
		if (eventp->type == FinishEvent)
		{
			finished = 1;
		}
		switch (eventp->type)
		{
		case KeyPress:
		case ButtonPress:
			if (eventp->type != FinishEvent)
			{
				original_w = eventp->xany.window;
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
			DispatchEvent(False);
		}
	}

	MyXUngrabKeyboard(dpy);
	UngrabEm(GRAB_NORMAL);
	*w = eventp->xany.window;
	if ((*w == Scr.Root || *w == Scr.NoFocusWin) &&
	    eventp->xbutton.subwindow != None)
	{
		*w = eventp->xbutton.subwindow;
		eventp->xany.window = *w;
	}
	if (*w == Scr.Root || IS_EWMH_DESKTOP(*w))
	{
		*context = C_ROOT;
		XBell(dpy, 0);
		return True;
	}
	if (XFindContext(dpy, *w, FvwmContext, (caddr_t *)fw) == XCNOENT)
	{
		*fw = NULL;
		XBell(dpy, 0);
		return (True);
	}
	if (*w == FW_W_PARENT(*fw))
	{
		*w = FW_W(*fw);
	}
	if (original_w == FW_W_PARENT(*fw))
	{
		original_w = FW_W(*fw);
	}
	/* this ugly mess attempts to ensure that the release and press
	 * are in the same window. */
	if (*w != original_w && original_w != Scr.Root &&
	    original_w != None && original_w != Scr.NoFocusWin &&
	    !IS_EWMH_DESKTOP(original_w))
	{
		if (*w != FW_W_FRAME(*fw) || original_w != FW_W(*fw))
		{
			*context = C_ROOT;
			XBell(dpy, 0);
			return True;
		}
	}

	if (IS_EWMH_DESKTOP(FW_W(*fw)))
	{
		*context = C_ROOT;
		XBell(dpy, 0);
		return True;
	}

	*context = GetContext(*fw,eventp,&dummy);

	return False;
}


void find_func_type(char *action, short *func_type, unsigned char *flags)
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
				if (func_type)
				{
					*func_type = func_table[j].func_type;
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
	if (func_type)
	{
		*func_type = F_BEEP;
	}
	if (flags)
	{
		*flags = 0;
	}

	return;
}


/***********************************************************************
 *
 *  Procedure:
 *      AddToFunction - add an item to a FvwmFunction
 *
 *  Inputs:
 *      func      - pointer to the FvwmFunction to add the item
 *      action    - the definition string from the config line
 *
 ***********************************************************************/
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

	find_func_type(tmp->action, NULL, &(tmp->flags));

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

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
		AddToDecor(tmp, action);
	}
#endif /* USEDECOR */

	return;
}
