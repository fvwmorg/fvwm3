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
#include <X11/keysym.h>

#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/Flocale.h"
#include "fvwm.h"
#include "bindings.h"
#include "misc.h"
#include "wcontext.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* menu bindings are kept in a local binding list separate from other
 * bindings*/
static Binding *menu_bindings_regular = NULL;
static Binding *menu_bindings_fallback = NULL;
static Binding **menu_bindings = NULL;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static Binding *__menu_binding_is_mouse(
	Binding *blist, XEvent* event, int context)
{
	Binding *b;
	int real_mod;

	real_mod = event->xbutton.state - (1 << (7 + event->xbutton.button));
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (
			BIND_IS_MOUSE_BINDING(b->type) &&
			(b->Button_Key == 0 ||
			 event->xbutton.button == b->Button_Key) &&
			(b->Modifier == AnyModifier ||
			 MaskUsedModifiers(b->Modifier) ==
			 MaskUsedModifiers(real_mod)) &&
			(b->Context == C_MENU ||
			 (b->Context & context) != C_MENU))
		{
			break;
		}
	}

	return b;
}

static Binding *__menu_binding_is_key(
	Binding *blist, XEvent* event, int context)
{
	Binding *b;

	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (
			BIND_IS_KEY_BINDING(b->type) &&
			event->xkey.keycode == b->Button_Key &&
			(b->Modifier == AnyModifier ||
			 MaskUsedModifiers(b->Modifier) ==
			 MaskUsedModifiers(event->xkey.state)) &&
			(b->Context == C_MENU ||
			 (b->Context & context) != C_MENU))
		{
			break;
		}
	}

	return b;
}

/* ---------------------------- interface functions ------------------------ */

void menu_bindings_startup_complete(void)
{
	menu_bindings = &menu_bindings_regular;

	return;
}

Binding *menu_binding_is_mouse(XEvent* event, int context)
{
	Binding *b;

	b = __menu_binding_is_mouse(menu_bindings_regular, event, context);
	if (b == NULL)
	{
		b = __menu_binding_is_mouse(
			menu_bindings_fallback, event, context);
	}

	return b;
}

Binding *menu_binding_is_key(XEvent* event, int context)
{
	Binding *b;

	b = __menu_binding_is_key(menu_bindings_regular, event, context);
	if (b == NULL)
	{
		b = __menu_binding_is_key(
			menu_bindings_fallback, event, context);
	}

	return b;
}

int menu_binding(
	Display *dpy, binding_t type, int button, KeySym keysym,
	int context, int modifier, char *action, char *menuStyle)
{
	Binding *rmlist;
	int rc;

	if (menu_bindings == NULL)
	{
		menu_bindings = &menu_bindings_fallback;
	}
	rmlist = NULL;
	if (~(~context | C_MENU | C_TITLE) != 0)
	{
		fvwm_msg(
			ERR, "menu_binding",
			"invalid context in combination with menu context.");

		return 1;
	}
	if (menuStyle != NULL)
	{
		/* fixme - make either match a menuStyle or a menu name */
		fvwm_msg(
			ERR, "menu_binding",
			"a window name may not be specified with menu context."
			);

		return 1;
	}
	/*
	 * Remove the "old" bindings if any
	 */
	/* BEGIN remove */
	CollectBindingList(
		dpy, menu_bindings, &rmlist, type, STROKE_ARG(NULL)
		button, keysym, modifier, context, menuStyle);
	if (rmlist != NULL)
	{
		FreeBindingList(rmlist);
	}
	else if (
		keysym == 0 && button != 0 && modifier == 0 &&
		strcmp(action,"-") == 0 && context == C_MENU)
	{
		/* Warn if Mouse n M N - occurs without removing any binding.
		 The user most likely want Mouse n MT A - instead. */
		fvwm_msg(WARN, "menu_binding",
			 "The syntax for disabling the tear off button has "
			 "changed.");
	}
	if (strcmp(action,"-") == 0)
	{
		return 0;
	}
	/* END remove */
	if ((modifier & AnyModifier) && (modifier & (~AnyModifier)))
	{
		fvwm_msg(
			WARN, "menu_binding", "Binding specified AnyModifier"
			" and other modifers too. Excess modifiers are"
			" ignored.");
		modifier = AnyModifier;
	}
	/* Warn about Mouse n M N TearOff. */
	if (
		keysym == 0 && button != 0 && modifier == 0 &&
		strcasecmp(action,"tearoff") == 0 && context == C_MENU)
	{
		fvwm_msg(OLD, "menu_binding",
			 "The syntax for disabling the tear off button has "
			 "changed. The TearOff action is no longer possible "
			 "in menu bindings.");
	}
	rc = AddBinding(
		dpy, menu_bindings, type, STROKE_ARG(NULL) button, keysym,
		NULL, modifier, context, (void *)action, NULL, menuStyle);

	return rc;
}
