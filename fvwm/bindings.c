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

#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/modifiers.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/defaults.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "module_interface.h"
#include "misc.h"
#include "screen.h"
#include "focus.h"
#include "menubindings.h"
#include "move_resize.h"	/* for placement_binding */
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static int mods_unused = DEFAULT_MODS_UNUSED;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void update_nr_buttons(
	int contexts, int *nr_left_buttons, int *nr_right_buttons, Bool do_set)
{
	int i;
	int l = *nr_left_buttons;
	int r = *nr_right_buttons;

	if (contexts == C_ALL)
	{
		return;
	}
	/* check for nr_left_buttons */
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i += 2)
	{
		if ((contexts & (C_L1 << i)))
		{
			if (do_set || *nr_left_buttons <= i / 2)
			{
				*nr_left_buttons = i / 2 + 1;
			}
		}
	}
	/* check for nr_right_buttons */
	for (i = 1; i < NUMBER_OF_TITLE_BUTTONS; i += 2)
	{
		if ((contexts & (C_L1 << i)))
		{
			if (do_set || *nr_right_buttons <= i / 2)
			{
				*nr_right_buttons = i / 2 + 1;
			}
		}
	}
	if (*nr_left_buttons != l || *nr_right_buttons != r)
	{
		Scr.flags.do_need_window_update = 1;
		Scr.flags.has_nr_buttons_changed = 1;
	}

	return;
}

static int activate_binding(Binding *binding, binding_t type, Bool do_grab)
{
	FvwmWindow *t;
	Bool rc = 0;

	if (binding == NULL)
	{
		return rc;
	}
	if (BIND_IS_PKEY_BINDING(type) || binding->Context == C_ALL)
	{
		/* necessary for key bindings that work over unfocused windows
		 */
		GrabWindowKeyOrButton(
			dpy, Scr.Root, binding,
			C_WINDOW | C_DECOR | C_ROOT | C_ICON | C_EWMH_DESKTOP,
			GetUnusedModifiers(), None, do_grab);
		if (do_grab == False)
		{
			rc = 1;
		}
	}
	if (do_grab == False && BIND_IS_KEY_BINDING(type) &&
	    (binding->Context & C_ROOT))
	{
		rc = 1;
	}
	if (fFvwmInStartup == True)
	{
		return rc;
	}

	/* grab keys immediately */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (!IS_EWMH_DESKTOP(FW_W(t)) &&
		    (binding->Context & (C_WINDOW | C_DECOR)) &&
		    BIND_IS_KEY_BINDING(type))
		{
			GrabWindowKey(
				dpy, FW_W_FRAME(t), binding,
				C_WINDOW | C_DECOR, GetUnusedModifiers(),
				do_grab);
		}
		if (binding->Context & C_ICON)
		{
			if (FW_W_ICON_TITLE(t) != None)
			{
				GrabWindowKeyOrButton(
					dpy, FW_W_ICON_TITLE(t), binding,
					C_ICON, GetUnusedModifiers(), None,
					do_grab);
			}
			if (FW_W_ICON_PIXMAP(t) != None)
			{
				GrabWindowKeyOrButton(
					dpy, FW_W_ICON_PIXMAP(t), binding,
					C_ICON, GetUnusedModifiers(), None,
					do_grab);
			}
		}
		if (IS_EWMH_DESKTOP(FW_W(t)) &&
		    (binding->Context & C_EWMH_DESKTOP))
		{
			GrabWindowKeyOrButton(
				dpy, FW_W_PARENT(t), binding, C_EWMH_DESKTOP,
				GetUnusedModifiers(), None, do_grab);
		}
	}

	return rc;
}

static int bind_get_bound_button_contexts(
	Binding **pblist, unsigned short *buttons_grabbed)
{
	int bcontext = 0;
	Binding *b;

	if (buttons_grabbed)
	{
		*buttons_grabbed = 0;
	}
	for (b = *pblist; b != NULL; b = b->NextBinding)
	{
		if (!BIND_IS_MOUSE_BINDING(b->type) &&
		    !BIND_IS_STROKE_BINDING(b->type))
		{
			continue;
		}
		if ((b->Context & (C_WINDOW | C_EWMH_DESKTOP)) &&
		    !(BIND_IS_STROKE_BINDING(b->type) && b->Button_Key == 0) &&
		    buttons_grabbed != NULL)
		{
			if (b->Button_Key == 0)
			{
				*buttons_grabbed |=
					((1 <<
					  NUMBER_OF_EXTENDED_MOUSE_BUTTONS) -
					 1);
			}
			else
			{
				*buttons_grabbed |= (1 << (b->Button_Key - 1));
			}
		}
		if (b->Context != C_ALL && (b->Context & (C_LALL | C_RALL)))
		{
			bcontext |= b->Context;
		}
	}

	return bcontext;
}

static void __rebind_global_key(Binding **pblist, int Button_Key)
{
	Binding *b;

	for (b = *pblist; b != NULL; b = b->NextBinding)
	{
		if (b->Button_Key == Button_Key &&
		    (BIND_IS_PKEY_BINDING(b->type) || b->Context == C_ALL))
		{
			activate_binding(b, b->type, True);

			return;
		}
	}

	return;
}

/* Parses a mouse or key binding */
static int ParseBinding(
	Display *dpy, Binding **pblist, char *tline, binding_t type,
	int *nr_left_buttons, int *nr_right_buttons,
	unsigned short *buttons_grabbed, Bool is_silent)
{
	char *action;
	char context_string[20];
	char modifier_string[20];
	char *ptr;
	char *token;
	char key_string[201] = "";
	char buffer[80];
	char *window_name = NULL;
	char *p;
	int button = 0;
	int n1 = 0;
	int n2 = 0;
	int n3 = 0;
	int context;
	int modifier;
	int rc;
	KeySym keysym = NoSymbol;
	Bool is_unbind_request = False;
	Bool is_pass_through = False;
	Bool are_similar_bindings_left;
	Binding *b;
	Binding *rmlist = NULL;
	STROKE_CODE(char stroke[STROKE_MAX_SEQUENCE + 1] = "");
	STROKE_CODE(int n4 = 0);
	STROKE_CODE(int i);

	/* tline points after the key word "Mouse" or "Key" */
	token = p = PeekToken(tline, &ptr);
 	/* check to see if a window name has been specified. */
	if (p == NULL)
	{
		fvwm_msg(
			ERR, "ParseBinding", "empty %s binding, ignored\n",
			tline);

		return 0;
	}
	if (*p == '(')
	{
		/* A window name has been specified for the binding. */
		sscanf(p + 1, "%79s", buffer);
		p = buffer;
		while (*p != ')')
		{
			if (*p == '\0')
			{
				if (!is_silent)
				{
					fvwm_msg(
						ERR, "ParseBinding",
						"Syntax error in line %s -"
						" missing ')'", tline);
				}

				return 0;
			}
			++p;
		}
		*p++ = '\0';
		window_name = buffer;
		if (*p != '\0')
		{
			if (!is_silent)
			{
				fvwm_msg(
					ERR, "ParseBinding",
					"Syntax error in line %s - trailing"
					" text after specified window", tline);
			}

			return 0;
		}
		token = PeekToken(ptr, &ptr);
	}

	if (token != NULL)
	{
		if (BIND_IS_KEY_BINDING(type))
		{
			/* see len of key_string above */
			n1 = sscanf(token,"%200s", key_string);
		}
#ifdef HAVE_STROKE
		else if (BIND_IS_STROKE_BINDING(type))
		{
			int num = 0;
			int j;

			n1 = 1;
			i = 0;
			if (token[0] == 'N' && token[1] != '\0')
			{
				num = 1;
			}
			j=i+num;
			while (n1 && token[j] != '\0' &&
			       i < STROKE_MAX_SEQUENCE)
			{
				if (!isdigit(token[j]))
				{
					n1 = 0;
				}
				if (num)
				{
					/* Numeric pad to Telephone  */
					if ('7' <= token[j] && token[j] <= '9')
					{
						token[j] -= 6;
					}
					else if ('1' <= token[j] &&
						 token[j] <= '3')
					{
						token[j] += 6;
					}
				}
				stroke[i] = token[j];
				i++;
				j=i+num;
			}
			stroke[i] = '\0';
			if (strlen(token) > STROKE_MAX_SEQUENCE + num)
			{
				if (!is_silent)
				{
					fvwm_msg(
						WARN, "ParseBinding",
						"Too long stroke sequence in"
						" line %s.  Only %i elements"
						" will be taken into"
						" account.\n",
						 tline, STROKE_MAX_SEQUENCE);
				}
			}
		}
#endif /* HAVE_STROKE */
		else
		{
			n1 = sscanf(token, "%d", &button);
			if (button < 0)
			{
				if (!is_silent)
				{
					fvwm_msg(
						ERR, "ParseBinding",
						"Illegal mouse button in line"
						" %s", tline);
				}
				return 0;
			}
			if (button > NUMBER_OF_MOUSE_BUTTONS)
			{
				if (!is_silent)
				{
					fvwm_msg(
						WARN, "ParseBinding",
						"Got mouse button %d when the"
						" maximum is %d.\n  You can't"
						" bind complex functions to"
						" this button.  To suppress"
						" this warning, use:\n"
						"  Silent Mouse %s", button,
						NUMBER_OF_MOUSE_BUTTONS,
						tline);
				}
			}
		}
	}

#ifdef HAVE_STROKE
	if (BIND_IS_STROKE_BINDING(type))
	{
		token = PeekToken(ptr, &ptr);
		if (token != NULL)
		{
			n4 = sscanf(token,"%d", &button);
		}
	}
#endif /* HAVE_STROKE */

	token = PeekToken(ptr, &ptr);
	if (token != NULL)
	{
		n2 = sscanf(token, "%19s", context_string);
	}
	token = PeekToken(ptr, &action);
	if (token != NULL)
	{
		n3 = sscanf(token, "%19s", modifier_string);
	}

	if (n1 != 1 || n2 != 1 || n3 != 1
	    STROKE_CODE(|| (BIND_IS_STROKE_BINDING(type) && n4 != 1)))
	{
		if (!is_silent)
		{
			fvwm_msg(
				ERR, "ParseBinding", "Syntax error in line %s",
				tline);
		}
		return 0;
	}

	if (wcontext_string_to_wcontext(
		    context_string, &context) && !is_silent)
	{
		fvwm_msg(
			WARN, "ParseBinding", "Illegal context in line %s",
			tline);
	}
	if (modifiers_string_to_modmask(modifier_string, &modifier) &&
	    !is_silent)
	{
		fvwm_msg(
			WARN, "ParseBinding", "Illegal modifier in line %s",
			tline);
	}

	if (BIND_IS_KEY_BINDING(type))
	{
		keysym = FvwmStringToKeysym(dpy, key_string);
		/* Don't let a 0 keycode go through, since that means AnyKey
		 * to the XGrabKey call. */
		if (keysym == 0)
		{
			if (!is_silent)
			{
				fvwm_msg(
					ERR, "ParseBinding", "No such key: %s",
					key_string);
			}
			return 0;
		}
	}

	if (action != NULL)
	{
		action = SkipSpaces(action, NULL, 0);
	}
	if (
		action == NULL || *action == 0 ||
		(action[0] == '-' && !is_pass_through))
	{
		is_unbind_request = True;
	}
	else
	{
		is_pass_through = is_pass_through_action(action);
		if (is_pass_through)
		{
			/* pass-through actions indicate that the event be
			 * allowed to pass through to the underlying window. */
			if (window_name == NULL)
			{
				/* It doesn't make sense to have a pass-through
				 * action on global bindings. */
				if (!is_silent)
				{
					fvwm_msg(
						ERR, "ParseBinding",
						"Invalid action for global "
						"binding: %s", tline);
				}

				return 0;
			}
		}
	}

	/* short circuit menu bindings for now. */
	if ((context & C_MENU) == C_MENU)
	{
		menu_binding(
			dpy, type, button, keysym, context, modifier, action,
			window_name);
		/* ParseBinding returns the number of new bindings in pblist
		 * menu bindings does not add to pblist, and should return 0 */

		return 0;
	}
	/* short circuit placement bindings for now. */
	if ((context & C_PLACEMENT) == C_PLACEMENT)
	{
		placement_binding(button,keysym,modifier,action);
		/* ParseBinding returns the number of new bindings in pblist
		 * placement bindings does not add to pblist, and should
		 * return 0 */

		return 0;
	}
	/*
	** Remove the "old" bindings if any
	*/
	/* BEGIN remove */
	CollectBindingList(
		dpy, pblist, &rmlist, &are_similar_bindings_left, type,
		STROKE_ARG((void *)stroke)
		button, keysym, modifier, context, window_name);
	if (rmlist != NULL)
	{
		int bcontext;

		if (is_unbind_request && are_similar_bindings_left == False)
		{
			int rc = 0;

			for (b = rmlist; b != NULL; b = b->NextBinding)
			{
				/* release the grab */
				rc |= activate_binding(b, type, False);
			}
			if (rc)
			{
				__rebind_global_key(
					pblist, rmlist->Button_Key);
			}
		}
		FreeBindingList(rmlist);
		bcontext = bind_get_bound_button_contexts(
			pblist, buttons_grabbed);
		update_nr_buttons(
			bcontext, nr_left_buttons, nr_right_buttons,
			True);
	}
	/* return if it is an unbind request */
	if (is_unbind_request)
	{
		return 0;
	}
	/* END remove */

	update_nr_buttons(context, nr_left_buttons, nr_right_buttons, False);
	if ((modifier & AnyModifier)&&(modifier&(~AnyModifier)))
	{
		fvwm_msg(
			WARN, "ParseBinding", "Binding specified AnyModifier"
			" and other modifers too. Excess modifiers are"
			" ignored.");
		modifier = AnyModifier;
	}
	if (
		(BIND_IS_MOUSE_BINDING(type) ||
		 (BIND_IS_STROKE_BINDING(type) && button != 0)) &&
		(context & (C_WINDOW | C_EWMH_DESKTOP)) &&
		buttons_grabbed != NULL)
	{
		if (button == 0)
		{
			*buttons_grabbed |=
				((1 << NUMBER_OF_EXTENDED_MOUSE_BUTTONS) - 1);
		}
		else
		{
			*buttons_grabbed |= (1 << (button - 1));
		}
	}
	rc = AddBinding(
		dpy, pblist, type, STROKE_ARG((void *)stroke)
		button, keysym, key_string, modifier, context, (void *)action,
		NULL, window_name);

	return rc;
}

static void binding_cmd(F_CMD_ARGS, binding_t type)
{
	Binding *b;
	int count;
	unsigned short btg = Scr.buttons2grab;

	count = ParseBinding(
		dpy, &Scr.AllBindings, action, type, &Scr.nr_left_buttons,
		&Scr.nr_right_buttons, &btg, Scr.flags.are_functions_silent);
	if (btg != Scr.buttons2grab)
	{
		Scr.flags.do_need_window_update = 1;
		Scr.flags.has_mouse_binding_changed = 1;
		Scr.buttons2grab = btg;
	}
	for (
		b = Scr.AllBindings; count > 0 && b != NULL;
		count--, b = b->NextBinding)
	{
		activate_binding(b, type, True);
	}

	return;
}

void print_bindings(void)
{
	Binding *b;

	fprintf(stderr, "Current list of bindings:\n\n");
	for (b = Scr.AllBindings; b != NULL; b = b->NextBinding)
	{
		switch (b->type)
		{
		case BIND_KEYPRESS:
			fprintf(stderr, "Key");
			break;
		case BIND_PKEYPRESS:
			fprintf(stderr, "PointerKey");
			break;
		case BIND_BUTTONPRESS:
		case BIND_BUTTONRELEASE:
			fprintf(stderr, "Mouse");
			break;
		case BIND_STROKE:
			fprintf(stderr, "Stroke");
			break;
		default:
			fvwm_msg(
				ERR, "print_bindings",
				"invalid binding type %d", b->type);
			continue;
		}
		if (b->windowName != NULL)
		{
			fprintf(stderr, " (%s)", b->windowName);
		}
		switch (b->type)
		{
		case BIND_KEYPRESS:
		case BIND_PKEYPRESS:
			fprintf(stderr, "\t%s", b->key_name);
			break;
		case BIND_BUTTONPRESS:
		case BIND_BUTTONRELEASE:
			fprintf(stderr, "\t%d", b->Button_Key);
			break;
		case BIND_STROKE:
			STROKE_CODE(
				fprintf(
					stderr, "\t%s\t%d",
					(char *)b->Stroke_Seq, b->Button_Key));
			break;
		}
		{
			char *mod_string;
			char *context_string;

			mod_string = charmap_table_to_string(
				MaskUsedModifiers(b->Modifier),key_modifiers);
			context_string = charmap_table_to_string(
				b->Context, win_contexts);
			fprintf(
				stderr, "\t%s\t%s\t%s\n", context_string,
				mod_string, (char *)b->Action);
			free(mod_string);
			free(context_string);
		}
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* Removes all unused modifiers from in_modifiers */
unsigned int MaskUsedModifiers(unsigned int in_modifiers)
{
	return in_modifiers & ~mods_unused;
}

unsigned int GetUnusedModifiers(void)
{
	return mods_unused;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_Key(F_CMD_ARGS)
{
	binding_cmd(F_PASS_ARGS, BIND_KEYPRESS);

	return;
}

void CMD_PointerKey(F_CMD_ARGS)
{
	binding_cmd(F_PASS_ARGS, BIND_PKEYPRESS);

	return;
}

void CMD_Mouse(F_CMD_ARGS)
{
	binding_cmd(F_PASS_ARGS, BIND_BUTTONPRESS);

	return;
}

#ifdef HAVE_STROKE
void CMD_Stroke(F_CMD_ARGS)
{
	binding_cmd(F_PASS_ARGS, BIND_STROKE);

	return;
}
#endif /* HAVE_STROKE */

/* Declares which X modifiers are actually locks and should be ignored when
 * testing mouse/key binding modifiers. */
void CMD_IgnoreModifiers(F_CMD_ARGS)
{
	char *token;
	int mods_unused_old = mods_unused;

	token = PeekToken(action, &action);
	if (!token)
	{
		mods_unused = 0;
	}
	else if (StrEquals(token, "default"))
	{
		mods_unused = DEFAULT_MODS_UNUSED;
	}
	else if (modifiers_string_to_modmask(token, &mods_unused))
	{
		fvwm_msg(
			ERR, "ignore_modifiers",
			"illegal modifier in line %s\n", action);
	}
	if (mods_unused != mods_unused_old)
	{
		/* broadcast config to modules */
		broadcast_ignore_modifiers();
	}

	return;
}
