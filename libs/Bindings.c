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

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "fvwmlib.h"

struct charstring
{
	char key;
	int  value;
};

static Bool is_grabbing_everything = False;

/* The keys must be in lower case! */
static struct charstring win_contexts[]=
{
	{'d', C_EWMH_DESKTOP},
	{'f', C_FRAME},
	{'<', C_F_TOPLEFT},
	{'^', C_F_TOPRIGHT},
	{'>', C_F_BOTTOMRIGHT},
	{'v', C_F_BOTTOMLEFT},
	{'i', C_ICON},
	{'r', C_ROOT},
	{'s', C_SIDEBAR},
	{'[', C_SB_LEFT},
	{']', C_SB_RIGHT},
	{'-', C_SB_TOP},
	{'_', C_SB_BOTTOM},
	{'t', C_TITLE},
	{'w', C_WINDOW},
	{'1', C_L1},
	{'2', C_R1},
	{'3', C_L2},
	{'4', C_R2},
	{'5', C_L3},
	{'6', C_R3},
	{'7', C_L4},
	{'8', C_R4},
	{'9', C_L5},
	{'0', C_R5},
	{'a', C_ALL},
	{0, 0}
};

/* The keys musat be in lower case! */
static struct charstring key_modifiers[]=
{
	{'s',ShiftMask},
	{'c',ControlMask},
	{'l',LockMask},
	{'m',Mod1Mask},
	{'1',Mod1Mask},
	{'2',Mod2Mask},
	{'3',Mod3Mask},
	{'4',Mod4Mask},
	{'5',Mod5Mask},
	{'a',AnyModifier},
	{'n',0},
	{0,0}
};

/* Table to translate a modifier map index to a modifier that we define that
 * generates that index.  This mapping can be chosen by each client, but the
 * settings below try to emulate the usual terminal behaviour. */
static unsigned int modifier_mapindex_to_mask[8] =
{
	ShiftMask,
	Mod3Mask, /* Alt Gr */
	Mod3Mask | ShiftMask,
	/* Just guessing below here - LockMask is not used anywhere*/
	ControlMask,
	Mod1Mask, /* Alt/Meta on XFree86 */
	Mod2Mask, /* Num lock on XFree86 */
	Mod4Mask,
	Mod5Mask, /* Scroll lock on XFree86 */
};

static int key_min;
static int key_max;

/*
 *
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 *
 */
static Bool find_context(
	const char *string, int *output, struct charstring *table)
{
	int i;
	int len = strlen( string );
	Bool error = False;

	*output=0;

	for ( i = 0; i < len; ++i )
	{
		int j = 0, matched = 0;
		char c = string[i];

		/* The following comment strikes me as unlikely, but I leave it
		 * (and  code) intact.  -- Steve Robbins, 28-mar-1999 */
		/* in some BSD implementations, tolower(c) is not defined
		 * unless isupper(c) is true */
		if ( isupper(c) )
		{
			c = tolower( c );
		}
		while ( table[j].key != 0 )
		{
			if ( table[j].key == c )
			{
				*output |= table[j].value;
				matched = 1;
				break;
			}
			++j;
		}
		if (!matched)
		{
			fprintf(
				stderr,
				"find_context: bad context or modifier %c\n",
				c);
			error = True;
		}
	}
	return error;
}

/* Converts the input string into a mask with bits for the contexts */
Bool ParseContext(char *in_context, int *out_context_mask)
{
	return find_context(in_context, out_context_mask, win_contexts);
}

/* Converts the input string into a mask with bits for the modifiers */
Bool ParseModifiers(char *in_modifiers, int *out_modifier_mask)
{
	return find_context(in_modifiers, out_modifier_mask, key_modifiers);
}

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
	void *action, void *action2)
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
			    (tkeysym = XKeycodeToKeysym(dpy, i, m)) == keysym)
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
					    !(modifiers & mask) != mask)
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
				(*pblist) = (Binding *)safemalloc(
					sizeof(Binding));
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
				(*pblist)->NextBinding = temp;
				bound_mask |= bind_mask;
				count++;
			}
		}
	}
	return count;
}

static Bool AreBindingsEqual(Binding *b1, Binding *b2)
{
	if (b1->type != b2->type)
	{
		return False;
	}
	if (b1->Context != b2->Context)
	{
		return False;
	}
	if (b1->Modifier != b2->Modifier)
	{
		return False;
	}
	if (b1->Button_Key != b2->Button_Key)
	{
		return False;
	}
	if (BIND_IS_KEY_BINDING(b1->type) || BIND_IS_MOUSE_BINDING(b1->type))
	{
		return True;
	}
	if (1 STROKE_CODE(&& BIND_IS_STROKE_BINDING(b1->type) &&
			  strcmp(b1->Stroke_Seq, b2->Stroke_Seq) == 0))
	{
		return True;
	}

	return False;
}

/*
 *
 *  Does exactly the opposite of AddBinding: It removes the bindings that
 *  AddBinding would have added to the *pblist_src and collects them in the
 *  *pblist_dest.  This can be used to remove a binding completely from the
 *  list.  The bindings still have to be freed.
 *
 */
void CollectBindingList(
	Display *dpy, Binding **pblist_src, Binding **pblist_dest,
	binding_t type, STROKE_ARG(void *stroke)
	int button, KeySym keysym, int modifiers, int contexts)
{
	Binding *tmplist = NULL;
	Binding *btmp;
	Binding *bold;
	Binding *tmpprev;
	Binding *oldprev;

	/* generate a private list of bindings to be removed */
	AddBinding(
		dpy, &tmplist, type, STROKE_ARG(stroke)
		button, keysym, NULL, modifiers, contexts, NULL, NULL);
	/* now find equivalent bindings in the given binding list and move
	 * them to the new clist */
	for (bold = *pblist_src, oldprev = NULL; bold != NULL;
	     oldprev = bold, bold = bold->NextBinding)
	{
		for (btmp = tmplist, tmpprev = NULL; btmp != NULL;
		     tmpprev = btmp, btmp = btmp->NextBinding)
		{
			if (AreBindingsEqual(btmp, bold))
			{
				/* move matched binding from src list to dest
				 * list */
				UnlinkBinding(pblist_src, bold, oldprev);
				bold->NextBinding = *pblist_dest;
				*pblist_dest = bold;
				/* throw away the tmp binding */
				UnlinkBinding(&tmplist, btmp, tmpprev);
				FreeBindingStruct(btmp);
				/* stop searching for this binding */
				break;
			}
		}
	}
	/* throw away the temporary list */
	FreeBindingList(tmplist);

	return;
}

Bool __compare_binding(
	Binding *b, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier, unsigned int used_modifiers,
	int Context, binding_t type)
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
	    (b->Button_Key == button_keycode || b->Button_Key == 0))
	{
		return True;
	}
	if (BIND_IS_KEY_BINDING(type) &&
	    b->Button_Key == button_keycode)
	{
		return True;
	}
	if (0 STROKE_CODE(|| ((BIND_IS_STROKE_BINDING(type) &&
			       (strcmp(b->Stroke_Seq,stroke) == 0) &&
			       b->Button_Key == button_keycode))))
	{
		return True;
	}

	return False;
}

/* Check if something is bound to a key or button press and return the action
 * to be executed or NULL if not. */
void *CheckBinding(
	Binding *blist, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type)
{
	Binding *b;
	unsigned int used_modifiers = ~dead_modifiers;

	modifier &= (used_modifiers & ALL_MODIFIERS);
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context, type) == True)
		{
			return b->Action;
		}
	}

	return NULL;
}

void *CheckTwoBindings(
	Bool *ret_is_second_binding, Binding *blist, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type, int Context2, binding_t type2)
{
	Binding *b;
	unsigned int used_modifiers = ~dead_modifiers;

	modifier &= (used_modifiers & ALL_MODIFIERS);
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context, type) == True)
		{
			*ret_is_second_binding = False;
			return b->Action;
		}
		if (__compare_binding(
			    b, STROKE_ARG(stroke) button_keycode, modifier,
			    used_modifiers, Context2, type2) == True)
		{
			*ret_is_second_binding = True;
			return b->Action;
		}
	}
	*ret_is_second_binding = False;

	return NULL;
}

/* same as above, but only returns exactly matching bindings, i.e. wildcards for
 * mouse buttons and modifiers must match exactly. */
Bool MatchBindingExactly(
	Binding *b, STROKE_ARG(void *stroke)
	int button, KeyCode keycode, unsigned int modifier, int Context,
	binding_t type)
{
	modifier &= ALL_MODIFIERS;
	if (b->type != type || b->Context != Context || b->Modifier != modifier)
	{
		return False;
	}
	if (BIND_IS_KEY_BINDING(type) && b->Button_Key != keycode)
	{
		return False;
	}
	if (BIND_IS_MOUSE_BINDING(type) && b->Button_Key != button)
	{
		return False;
	}
	if (0 STROKE_CODE(|| ((BIND_IS_STROKE_BINDING(type) &&
			       (b->Button_Key != button ||
				(strcmp(b->Stroke_Seq,stroke) != 0))))))
	{
		return False;
	}

	return True;
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
		int bmax = NUMBER_OF_MOUSE_BUTTONS;
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
		GrabWindowButton(
			dpy, w, blist, contexts, dead_modifiers, cursor, fGrab);
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
		KeySym lowerks;
		KeySym upperks;

		s = alloca(strlen(key) + 1);
		strcpy(s, key);
		*s = tolower(*s);
		lowerks = XStringToKeysym(s);
		*s = toupper(*s);
		upperks = XStringToKeysym(s);
		/* always prefer the lower case spelling if it exists */
		if (lowerks == 0)
		{
			keysym = upperks;
		}
		else
		{
			keysym = lowerks;
		}
	}
	if (keysym == NoSymbol || XKeysymToKeycode(dpy, keysym) == 0)
	{
		return 0;
	}

	return keysym;
}
