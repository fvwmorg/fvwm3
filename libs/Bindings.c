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

#include "config.h"

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "libs/fvwmlib.h"
#include "libs/Strings.h"
#include "libs/wild.h"
#include "libs/Grab.h"
#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/modifiers.h"

static Bool is_grabbing_everything = False;

static int key_min = 0;
static int key_max = 0;

/* Free the memory use by a binding. */
void FreeBindingStruct(Binding *b)
{
	if (b->key_name)
	{
		free(b->key_name);
	}
	STROKE_CODE(
		if (b->Stroke_Seq)
		free(b->Stroke_Seq);
		);
	if (b->Action)
	{
		free(b->Action);
	}
	if (b->Action2)
	{
		free(b->Action2);
	}
	if (b->windowName)
	{
		free(b->windowName);
	}
	free(b);

	return;
}

void FreeBindingList(Binding *b)
{
	Binding *t;

	for (; b != NULL; b = t)
	{
		t = b->NextBinding;
		FreeBindingStruct(b);
	}

	return;
}

/* Unlink a binding b from a binding list pblist.  The previous binding in the
 * list (prev) must be given also.  Pass NULL at the beginning of the list.
 * The *pblist pointer may be modified by this function. */
static void UnlinkBinding(Binding **pblist, Binding *b, Binding *prev)
{
	Binding *t;

	if (!prev && b != *pblist)
	{
		for (t = *pblist; t && t != b; prev = t, t = t->NextBinding)
		{
			/* Find the previous binding in the list. */
		}
		if (t == NULL)
		{
			/* Binding not found */
			return;
		}
	}

	if (prev)
	{
		/* middle of list */
		prev->NextBinding = b->NextBinding;
	}
	else
	{
		/* must have been first one, set new start */
		*pblist = b->NextBinding;
	}

	return;
}

/* To remove a binding from the global list (probably needs more processing
 * for mouse binding lines though, like when context is a title bar button).
 * Specify either button or keysym, depending on type. */
void RemoveBinding(Binding **pblist, Binding *b, Binding *prev)
{
	UnlinkBinding(pblist, b, NULL);
	FreeBindingStruct(b);

	return;
}

/*
 *
 *  Actually adds a new binding to a list (pblist) of existing bindings.
 *  Specify either button or keysym/key_name, depending on type.
 *  The parameters action and action2 are assumed to reside in malloced memory
 *  that will be freed in RemoveBinding. The key_name is copied into private
 *  memory and has to be freed by the caller.
 *
 */
int AddBinding(
	Display *dpy, Binding **pblist, binding_t type,
	STROKE_ARG(void *stroke)
	int button, KeySym keysym, char *key_name, int modifiers, int contexts,
	void *action, void *action2, char *windowName)
{
	int i;
	int min;
	int max;
	int maxmods;
	int m;
	int mask;
	int count = 0;
	KeySym tkeysym;
	Binding *temp;

	/*
	** Unfortunately a keycode can be bound to multiple keysyms and a
	** keysym can bound to multiple keycodes. Thus we have to check every
	** keycode with any single modifier.
	*/
	if (BIND_IS_KEY_BINDING(type))
	{
		if (key_max == 0)
		{
			XDisplayKeycodes(dpy, &key_min, &key_max);
		}
		min=key_min;
		max=key_max;
		maxmods = 8;
	}
	else
	{
		min = button;
		max = button;
		maxmods = 0;
	}
	for (i = min; i <= max; i++)
	{
		unsigned int bound_mask = 0;

		/* If this is a mouse binding we'll fall through the for loop
		 * (maxmods is zero) and the if condition is always true (type
		 * is zero). Since min == max == button there is no loop at all
		 * is case of a mouse binding. */
		for (m = 0, tkeysym = XK_Left;
		     m <= maxmods && tkeysym != NoSymbol; m++)
		{
			if (BIND_IS_MOUSE_BINDING(type) ||
			    STROKE_CODE(BIND_IS_STROKE_BINDING(type) ||)
			    (tkeysym = fvwm_KeycodeToKeysym(dpy, i, m, 0)) == keysym)
			{
				unsigned int add_modifiers = 0;
				unsigned int bind_mask = 1;
				unsigned int check_bound_mask = 0;

				switch (m)
				{
				case 0:
					/* key generates the key sym with no
					 * modifiers depressed - bind it */
					break;
				case 1:
					/* key generates the key sym with shift
					 * depressed */
					if (modifiers != AnyModifier &&
					    !(modifiers & ShiftMask))
					{
						add_modifiers = ShiftMask;
						bind_mask = (1 << m);
						/* but don't bind it again if
						 * already bound without
						 * modifiers */
						check_bound_mask = 1;
					}
					break;
				default:
					/* key generates the key sym with
					 * undefined modifiers depressed -
					 * let's make an educated guess at what
					 * modifiers the user expected
					 * based on the XFree86 default
					 * configuration. */
					mask = modifier_mapindex_to_mask[m - 1];
					if (modifiers != AnyModifier &&
					    !((modifiers & mask) != mask))
					{
						add_modifiers = mask;
						bind_mask = (1 << m);
						/* but don't bind it again if
						 * already bound without
						 * modifiers */
						check_bound_mask = 1;
					}
					break;
				}
				if ((bind_mask & bound_mask) ||
				    (check_bound_mask & bound_mask))
				{
					/* already bound, break out */
					break;
				}
				temp = *pblist;
				(*pblist) = xmalloc(sizeof(Binding));
				(*pblist)->type = type;
				(*pblist)->Button_Key = i;
				STROKE_CODE((*pblist)->Stroke_Seq = (stroke) ?
					    (void *)stripcpy((char *)stroke) :
					    NULL);
				if (BIND_IS_KEY_BINDING(type) &&
				    key_name != NULL)
				{
					(*pblist)->key_name =
						stripcpy(key_name);
				}
				else
				{
					(*pblist)->key_name = NULL;
				}
				(*pblist)->Context = contexts;
				(*pblist)->Modifier = modifiers | add_modifiers;
				(*pblist)->Action =
					(action) ? stripcpy(action) : NULL;
				(*pblist)->Action2 =
					(action2) ? stripcpy(action2) : NULL;
				(*pblist)->windowName =
					windowName ? stripcpy(windowName) :
					NULL;
				(*pblist)->NextBinding = temp;
				bound_mask |= bind_mask;
				count++;
			}
		}
	}
	return count;
}

/* returns 1 if binding b1 is identical to binding b2 (except action)
 * returns 2 if b1 and b2 are identical except that b1 has a different window
 * name than b2 and both are not NULL
 * returns 0 otherwise
 */
static int compare_bindings(Binding *b1, Binding *b2)
{
	if (b1->type != b2->type)
	{
		return 0;
	}
	if (b1->Context != b2->Context)
	{
		return 0;
	}
	if (b1->Modifier != b2->Modifier)
	{
		return 0;
	}
	if (b1->Button_Key != b2->Button_Key)
	{
		return 0;
	}

	/* definition: "global binding" => b->windowName == NULL
	 * definition: "window-specific binding" => b->windowName != NULL
	 */
	if (b1->windowName && b2->windowName)
	{
		/* Both bindings are window-specific. The existing binding, b2,
		 * is only replaced (by b1) if it applies to the same window */
		if (strcmp(b1->windowName, b2->windowName) != 0)
		{
			return 2;
		}
	}
	else if (b1->windowName || b2->windowName)
	{
		/* 1 binding is window-specific, the other is global - no need
		 * to replace this binding. */
		return 0;
	}

	if (BIND_IS_KEY_BINDING(b1->type) || BIND_IS_MOUSE_BINDING(b1->type))
	{
		return 1;
	}
	if (1 STROKE_CODE(&& BIND_IS_STROKE_BINDING(b1->type) &&
			  strcmp(b1->Stroke_Seq, b2->Stroke_Seq) == 0))
	{
		return 1;
	}

	return 0;
}

/*
 *  Does exactly the opposite of AddBinding: It removes the bindings that
 *  AddBinding would have added to the *pblist_src and collects them in the
 *  *pblist_dest.  This can be used to remove a binding completely from the
 *  list.  The bindings still have to be freed.
 */
void CollectBindingList(
	Display *dpy, Binding **pblist_src, Binding **pblist_dest,
	Bool *ret_are_similar_bindings_left, binding_t type,
	STROKE_ARG(void *stroke) int button, KeySym keysym,
	int modifiers, int contexts, char *windowName)
{
	Binding *tmplist = NULL;
	Binding *bold;
	Binding *oldprev;

	*ret_are_similar_bindings_left = False;
	/* generate a private list of bindings to be removed */
	AddBinding(
		dpy, &tmplist, type, STROKE_ARG(stroke)
		button, keysym, NULL, modifiers, contexts, NULL, NULL,
		windowName);
	/* now find equivalent bindings in the given binding list and move
	 * them to the new clist */
	for (bold = *pblist_src, oldprev = NULL; bold != NULL; )
	{
		Binding *btmp;
		Binding *bfound;

		bfound = NULL;
		for (btmp = tmplist;
		     btmp != NULL && (
			     bfound == NULL ||
			     *ret_are_similar_bindings_left == False);
		     btmp = btmp->NextBinding)
		{
			int rc;

			rc = compare_bindings(btmp, bold);
			if (rc == 1)
			{
				bfound = btmp;
			}
			else if (rc == 2)
			{
				*ret_are_similar_bindings_left = True;
				if (bfound != NULL)
				{
					break;
				}
			}
		}
		if (bfound != NULL)
		{
			Binding *next;

			/* move matched binding from src list to dest list */
			UnlinkBinding(pblist_src, bold, oldprev);
			next = bold->NextBinding;
			bold->NextBinding = *pblist_dest;
			*pblist_dest = bold;
			/* oldprev is unchanged */
			bold = next;
		}
		else
		{
			oldprev = bold;
			bold = bold->NextBinding;
		}
	}
	/* throw away the temporary list */
	FreeBindingList(tmplist);

	return;
}

/*
 * bindingAppliesToWindow()
 *
 * The Key/Mouse/PointerKey syntax (optionally) allows a window name
 * (or class or resource) to be specified with the binding, denoting
 * which windows the binding can be invoked in. This function determines
 * if the binding actually applies to a window based on its
 * name/class/resource.
 */
static Bool does_binding_apply_to_window(
	Binding *binding, const XClassHint *win_class, const char *win_name)
{
	/* If no window name is specified with the binding then that means
	 * the binding applies to ALL windows. */
	if (binding->windowName == NULL)
	{
		return True;
	}
	else if (win_class == NULL || win_name == NULL)
	{
		return False;
	}
	if (matchWildcards(binding->windowName, win_name) == True ||
	    matchWildcards(binding->windowName, win_class->res_name) == True ||
	    matchWildcards(binding->windowName, win_class->res_class) == True)
	{
		return True;
	}

	return False;
}

static Bool __compare_binding(
	Binding *b, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier, unsigned int used_modifiers,
	int Context, binding_t type, const XClassHint *win_class,
	const char *win_name)
{
	if (b->type != type || !(b->Context & Context))
	{
		return False;
	}
	if ((b->Modifier & used_modifiers) != modifier &&
	    b->Modifier != AnyModifier)
	{
		return False;
	}
	if (BIND_IS_MOUSE_BINDING(type) &&
	    (b->Button_Key != button_keycode && b->Button_Key != 0))
	{
		return False;
	}
	else if (BIND_IS_KEY_BINDING(type) && b->Button_Key != button_keycode)
	{
		return False;
	}
#ifdef HAVE_STROKE
	else if (BIND_IS_STROKE_BINDING(type) &&
		((strcmp(b->Stroke_Seq, stroke) != 0) ||
		 b->Button_Key != button_keycode))
	{
		return False;
	}
#endif
	if (!does_binding_apply_to_window(b, win_class, win_name))
	{
		return False;
	}

	return True;
}

/* is_pass_through_action() - returns true if the action indicates that the
 * binding should be ignored by fvwm & passed through to the underlying
 * window.
 * Note: it is only meaningful to check for pass-thru actions on
 * window-specific bindings. */
Bool is_pass_through_action(const char *action)
{
	/* action should never be NULL. */
	return (strncmp(action, "--", 2) == 0);
}

/* Check if something is bound to a key or button press and return the action
 * to be executed or NULL if not. */
void *CheckBinding(
	Binding *blist, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type, const XClassHint *win_class,
	const char *win_name)
{
	Binding *b;
	unsigned int used_modifiers = ~dead_modifiers;
	void *action = NULL;

	modifier &= (used_modifiers & ALL_MODIFIERS);
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context, type, win_class,
			    win_name) == True)
		{
			/* If this is a global binding, keep searching <blist>
			 * in the hope of finding a window-specific binding.
			 * If we don't find a win-specific binding, we use the
			 * _first_ matching global binding we hit. */
			if (action == NULL || b->windowName)
			{
				action = b->Action;
				if (b->windowName)
				{
					if (is_pass_through_action(action))
					{
						action = NULL;
					}
					break;
				}
			}
		}
	}

	return action;
}

void *CheckTwoBindings(
	Bool *ret_is_second_binding, Binding *blist, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type, const XClassHint *win_class,
	const char *win_name, int Context2, binding_t type2,
	const XClassHint *win_class2, const char *win_name2)
{
	Binding *b;
	unsigned int used_modifiers = ~dead_modifiers;
	void *action = NULL;

	modifier &= (used_modifiers & ALL_MODIFIERS);
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context, type, win_class, win_name)
		    == True)
		{
			if (action == NULL || b->windowName)
			{
				*ret_is_second_binding = False;
				action = b->Action;
				if (b->windowName)
				{
					if (is_pass_through_action(action))
						action = NULL;
					break;
				}
			}
		}
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context2, type2, win_class2,
			    win_name2) == True)
		{
			if (action == NULL || b->windowName)
			{
				*ret_is_second_binding = True;
				action = b->Action;
				if (b->windowName)
				{
					if (is_pass_through_action(action))
					{
						action = NULL;
					}
					break;
				}
			}
		}
	}

	return action;
}

/*
 *      GrabWindowKey        - grab needed keys for the window for one binding
 *      GrabAllWindowKeys    - grab needed keys for the window for all bindings
 *                             in blist
 *      GrabWindowButton     - same for mouse buttons
 *      GrabAllWindowButtons - same for mouse buttons
 *      GrabAllWindowKeysAndButtons - both of the above
 *
 *  Inputs:
 *   w              - the window to use (the frame window)
 *   grab           - 1 to grab, 0 to ungrab
 *   binding        - pointer to the bindinge to grab/ungrab
 *   contexts       - all context bits that shall receive bindings
 *   dead_modifiers - modifiers to ignore for 'AnyModifier'
 *   cursor         - the mouse cursor to use when the pointer is on the
 *                    grabbed area (mouse bindings only)
 *
 */
void GrabWindowKey(Display *dpy, Window w, Binding *binding,
		   unsigned int contexts, unsigned int dead_modifiers,
		   Bool fGrab)
{
	/* remove unnecessary bits from dead_modifiers */
	dead_modifiers &= ~(binding->Modifier & dead_modifiers);
	dead_modifiers &= ALL_MODIFIERS;

	if((binding->Context & contexts) && BIND_IS_KEY_BINDING(binding->type))
	{
		if (fGrab)
		{
			XGrabKey(
				dpy, binding->Button_Key, binding->Modifier, w,
				True, GrabModeAsync, GrabModeAsync);
		}
		else
		{
			XUngrabKey(
				dpy, binding->Button_Key, binding->Modifier, w);
		}
		if(binding->Modifier != AnyModifier && dead_modifiers != 0)
		{
			register unsigned int mods;
			register unsigned int max = dead_modifiers;
			register unsigned int living_modifiers =
				~dead_modifiers;

			/* handle all bindings for the dead modifiers */
			for (mods = 1; mods <= max; mods++)
			{
				/* Since mods starts with 1 we don't need to
				 * test if mods contains a dead modifier.
				 * Otherwise both, dead and living modifiers
				 * would be zero ==> mods == 0 */
				if (mods & living_modifiers)
				{
					continue;
				}
				if (fGrab)
				{
					XGrabKey(
						dpy, binding->Button_Key,
						mods|binding->Modifier, w,
						True, GrabModeAsync,
						GrabModeAsync);
				}
				else
				{
					XUngrabKey(
						dpy, binding->Button_Key,
						mods|binding->Modifier, w);
				}
			}
		}
		if (!is_grabbing_everything)
		{
			XSync(dpy, 0);
		}
	}

	return;
}

void GrabAllWindowKeys(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Bool fGrab)
{
	MyXGrabServer(dpy);
	is_grabbing_everything = True;
	for ( ; blist != NULL; blist = blist->NextBinding)
	{
		GrabWindowKey(dpy, w, blist, contexts, dead_modifiers, fGrab);
	}
	is_grabbing_everything = False;
	MyXUngrabServer(dpy);

	return;
}

void GrabWindowButton(
	Display *dpy, Window w, Binding *binding, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab)
{
	if (binding->Action == NULL)
	{
		return;
	}

	dead_modifiers &= ~(binding->Modifier & dead_modifiers); /* dje */
	dead_modifiers &= ALL_MODIFIERS;

	if ((binding->Context & contexts) &&
	    ((BIND_IS_MOUSE_BINDING(binding->type) ||
	      (BIND_IS_STROKE_BINDING(binding->type) &&
	       binding->Button_Key !=0))))
	{
		int bmin = 1;
		int bmax = NUMBER_OF_EXTENDED_MOUSE_BUTTONS;
		int button;

		if(binding->Button_Key >0)
		{
			bmin = bmax = binding->Button_Key;
		}
		for (button = bmin; button <= bmax; button++)
		{
			if (fGrab)
			{
				XGrabButton(
					dpy, button, binding->Modifier, w,
					True, ButtonPressMask |
					ButtonReleaseMask, GrabModeSync,
					GrabModeAsync, None, cursor);
			}
			else
			{
				XUngrabButton(
					dpy, button, binding->Modifier, w);
			}
			if (binding->Modifier != AnyModifier &&
			    dead_modifiers != 0)
			{
				register unsigned int mods;
				register unsigned int max = dead_modifiers;
				register unsigned int living_modifiers =
					~dead_modifiers;

				/* handle all bindings for the dead modifiers */
				for (mods = 1; mods <= max; mods++)
				{
					/* Since mods starts with 1 we don't
					 * need to test if mods contains a
					 * dead modifier. Otherwise both, dead
					 * and living modifiers would be zero
					 * ==> mods == 0 */
					if (mods & living_modifiers)
					{
						continue;
					}
					if (fGrab)
					{
						XGrabButton(
							dpy, button,
							mods|binding->Modifier,
							w, True,
							ButtonPressMask |
							ButtonReleaseMask,
							GrabModeSync,
							GrabModeAsync, None,
							cursor);
					}
					else
					{
						XUngrabButton(
							dpy, button,
							mods|binding->Modifier,
							w);
					}
				}
			}
			if (!is_grabbing_everything)
			{
				XSync(dpy, 0);
			}
		}
	}

	return;
}

void GrabAllWindowButtons(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab)
{
	MyXGrabServer(dpy);
	is_grabbing_everything = True;
	for ( ; blist != NULL; blist = blist->NextBinding)
	{
		GrabWindowButton(dpy, w, blist, contexts, dead_modifiers,
			cursor, fGrab);
	}
	is_grabbing_everything = False;
	MyXUngrabServer(dpy);

	return;
}

void GrabAllWindowKeysAndButtons(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab)
{
	MyXGrabServer(dpy);
	is_grabbing_everything = True;
	for ( ; blist != NULL; blist = blist->NextBinding)
	{
		if (blist->Context & contexts)
		{
			if (BIND_IS_MOUSE_BINDING(blist->type) ||
			    BIND_IS_STROKE_BINDING(blist->type))
			{
				GrabWindowButton(
					dpy, w, blist, contexts,
					dead_modifiers, cursor, fGrab);
			}
			else if (BIND_IS_KEY_BINDING(blist->type))
			{
				GrabWindowKey(
					dpy, w, blist, contexts,
					dead_modifiers, fGrab);
			}
		}
	}
	is_grabbing_everything = False;
	MyXUngrabServer(dpy);

	return;
}

void GrabWindowKeyOrButton(
	Display *dpy, Window w, Binding *binding, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab)
{
	if (BIND_IS_MOUSE_BINDING(binding->type) ||
	    BIND_IS_STROKE_BINDING(binding->type))
	{
		GrabWindowButton(
			dpy, w, binding, contexts, dead_modifiers, cursor,
			fGrab);
	}
	else if (BIND_IS_KEY_BINDING(binding->type))
	{
		GrabWindowKey(
			dpy, w, binding, contexts, dead_modifiers, fGrab);
	}

	return;
}

/*
 *
 *  Like XStringToKeysym, but allows some typos and does some additional
 *  error checking.
 *
 */
KeySym FvwmStringToKeysym(Display *dpy, char *key)
{
	KeySym keysym;
	char *s;

	if (!isalpha(*key))
	{
		keysym = XStringToKeysym(key);
	}
	else
	{
		s = alloca(strlen(key) + 1);
		strcpy(s, key);
		/* always prefer the lower case spelling if it exists */
		*s = tolower(*s);
		keysym = XStringToKeysym(s);
		if (keysym == NoSymbol)
		{
			*s = toupper(*s);
			keysym = XStringToKeysym(s);
		}
	}
	if (keysym == NoSymbol || XKeysymToKeycode(dpy, keysym) == 0)
	{
		return 0;
	}

	return keysym;
}
