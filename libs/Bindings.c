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
#include <string.h>
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
  {'w',C_WINDOW},
  {'t',C_TITLE},
  {'i',C_ICON},
  {'r',C_ROOT},
  {'f',C_FRAME},
  {'s',C_SIDEBAR},
  {'1',C_L1},
  {'2',C_R1},
  {'3',C_L2},
  {'4',C_R2},
  {'5',C_L3},
  {'6',C_R3},
  {'7',C_L4},
  {'8',C_R4},
  {'9',C_L5},
  {'0',C_R5},
  {'a',C_ALL},
  {0,0}
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


/****************************************************************************
 *
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 *
 ****************************************************************************/
static Bool find_context( const char *string, int *output,
			  struct charstring *table )
{
  int i;
  int len = strlen( string );
  Bool error = False;

  *output=0;

  for ( i = 0; i < len; ++i )
  {
    int j = 0, matched = 0;
    char c = string[i];

    /* The following comment strikes me as unlikely, but I leave it (and
       the code) intact.  -- Steve Robbins, 28-mar-1999 */
    /* in some BSD implementations, tolower(c) is not defined
     * unless isupper(c) is true */
    if ( isupper(c) )
      c = tolower( c );

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
      fprintf( stderr, "find_context: bad context or modifier %c\n", c );
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
    free(b->key_name);
  STROKE_CODE(
    if (b->Stroke_Seq)
      free(b->Stroke_Seq);
    );
  if (b->Action)
    free(b->Action);
  if (b->Action2)
    free(b->Action2);
  free(b);
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
}

/* To remove a binding from the global list (probably needs more processing
 * for mouse binding lines though, like when context is a title bar button).
 * Specify either button or keysym, depending on type. */
Bool RemoveMatchingBinding(
  Display *dpy, Binding **pblist, BindingType type, STROKE_ARG(char *stroke)
  int button, KeySym keysym, int modifiers, int contexts)
{
  Binding *t;
  Binding *prev;
  KeyCode keycode = 0;

  if (!*pblist)
    return False;
  if (type == KEY_BINDING)
    keycode = XKeysymToKeycode(dpy, keysym);

  for (t = *pblist, prev = NULL; t; prev = t, t = t->NextBinding)
  {
    if (MatchBindingExactly(dpy, t, STROKE_ARG(stroke)
                            button, keycode, modifiers, contexts, type))
    {
      /* found a matching binding - remove it */
      UnlinkBinding(pblist, t, prev);
      FreeBindingStruct(t);
      t = NULL;
      /* break out of the loop */
      return True;
    }
  }

  return False;
}


/****************************************************************************
 *
 *  Actually adds a new binding to a list (pblist) of existing bindings.
 *  Specify either button or keysym/key_name, depending on type.
 *  The parameters action and action2 are assumed to reside in malloced memory
 *  that will be freed in RemoveBinding. The key_name is copied into private
 *  memory and has to be freed by the caller.
 *
 ****************************************************************************/
int AddBinding(Display *dpy, Binding **pblist, BindingType type,
	       STROKE_ARG(void *stroke)
	       int button, KeySym keysym, char *key_name,
	       int modifiers, int contexts, void *action, void *action2)
{
  int i;
  int min;
  int max;
  int maxmods;
  int m;
  int count = 0;
  KeySym tkeysym;
  Binding *temp;

  /*
  ** Unfortunately a keycode can be bound to multiple keysyms and a keysym can
  ** be bound to multiple keycodes. Thus we have to check every keycode with
  ** any single modifier.
  */
  if (type == KEY_BINDING)
  {
    XDisplayKeycodes(dpy, &min, &max);
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
    /* If this is a mouse binding we'll fall through the for loop (maxmods is
     * zero) and the if condition is always true (type is zero). Since min ==
     * max == button there is no loop at all is case of a mouse binding. */
    for (m = 0, tkeysym = XK_Left; m <= maxmods && tkeysym != NoSymbol; m++)
    {

      if (type == MOUSE_BINDING ||
	  STROKE_CODE(type == STROKE_BINDING ||)
	  (tkeysym = XKeycodeToKeysym(dpy, i, m)) == keysym)
      {
	temp = *pblist;
	(*pblist) = (Binding *)safemalloc(sizeof(Binding));
	(*pblist)->type = type;
	(*pblist)->Button_Key = i;
	STROKE_CODE((*pblist)->Stroke_Seq =
		    (stroke) ? (void *)stripcpy((char *)stroke) : NULL);
	if (type == KEY_BINDING && key_name != NULL)
	  (*pblist)->key_name = stripcpy(key_name);
	else
	  (*pblist)->key_name = NULL;
	(*pblist)->Context = contexts;
	(*pblist)->Modifier = modifiers;
	(*pblist)->Action = (action) ? stripcpy(action) : NULL;
	(*pblist)->Action2 = (action2) ? stripcpy(action2) : NULL;
	(*pblist)->NextBinding = temp;
	count++;
	/* Add the binding only once for each KeySym value. */
	break;
      }
    }
  }
  return count;
}


/* Check if something is bound to a key or button press and return the action
 * to be executed or NULL if not. */
void *CheckBinding(Binding *blist,
		   STROKE_ARG(char *stroke)
		   int button_keycode,
		   unsigned int modifier,unsigned int dead_modifiers,
		   int Context, BindingType type)
{
  Binding *b;
  unsigned int used_modifiers = ~dead_modifiers;

  modifier &= used_modifiers;

  for (b = blist; b != NULL; b = b->NextBinding)
  {
    if ((
      ((type == MOUSE_BINDING || type == KEY_BINDING) &&
       b->Button_Key == button_keycode) ||
      STROKE_CODE((type == STROKE_BINDING &&
		   (strcmp(b->Stroke_Seq,stroke) == 0) &&
		   b->Button_Key == button_keycode) ||)
      (type == MOUSE_BINDING && b->Button_Key == 0))
	&& (((b->Modifier & used_modifiers) == modifier) ||
	    (b->Modifier == AnyModifier))
	&& (b->Context & Context)
	&& (b->type == type))
    {
      return b->Action;
    }
  }
  return NULL;
}

/* Check if something is bound to a key or button press */
Bool MatchBinding(Display *dpy, Binding *b,
		  STROKE_ARG(void *stroke)
		  int button, KeySym keysym,
		  unsigned int modifier,unsigned int dead_modifiers,
		  int Context, BindingType type)
{
  unsigned int used_modifiers = ~dead_modifiers;
  static int *kc_list = NULL;
  static int kc_len = 0;
  static int kc_maxlen = 0;
  static KeySym last_keysym = NoSymbol;

  modifier &= used_modifiers;
  if (((b->Modifier & used_modifiers) != modifier &&
       b->Modifier != AnyModifier) ||
      (b->Context & Context) == 0 ||
      b->type != type)
  {
    return False;
  }

  /* See comment in AddBinding why this logic is necessary. */
  if (type == KEY_BINDING)
  {
    const int maxmods = 8;
    static int max = 0;
    static int min = 0;
    KeySym tkeysym;
    int i;
    int m;

    if (keysym == NoSymbol)
      return False;
    if (max == 0)
      XDisplayKeycodes(dpy, &min, &max);

    if (keysym != last_keysym)
    {
      /* Rebuild array of key symbols. This is stored between calls to reduce
       * X server traffic. */
      kc_len = 0;
      if (kc_list)
	free(kc_list);
      kc_list = (int *)safemalloc(sizeof(int) * maxmods);
      for (i = min; i <= max; i++)
      {
	for (m = 0, tkeysym = XK_Left; m <= maxmods && tkeysym != NoSymbol; m++)
	{
	  if ((tkeysym = XKeycodeToKeysym(dpy, i, m)) == keysym)
	  {
	    if (kc_len >= kc_maxlen)
	    {
	      kc_maxlen += 8;
	      kc_list =
		(int *)saferealloc((char *)kc_list, sizeof(int) * kc_maxlen);
	    }
	    kc_list[kc_len] = i;
	    kc_len++;
	  }
	}
      }
    }
    for (i = 0; i < kc_len; i++)
    {
      if (kc_list[i] == b->Button_Key)
	return True;
    }
  }
  else if ((
      (type == MOUSE_BINDING && b->Button_Key == button) ||
      STROKE_CODE((type == STROKE_BINDING &&
		   (strcmp(b->Stroke_Seq,stroke) == 0) &&
		   b->Button_Key == button) ||)
      (type == MOUSE_BINDING && b->Button_Key == 0)))
  {
      return True;
  }
  return False;
}

/* same as above, but only returns exactly matching bindings, i.e. wildcards for
 * mouse buttons and modifiers must match exactly. */
Bool MatchBindingExactly(
  Display *dpy, Binding *b, STROKE_ARG(void *stroke)
  int button, KeyCode keycode, unsigned int modifier, int Context,
  BindingType type)
{
  if (b->type == type)
  {
    if (
        (
          (type == KEY_BINDING && b->Button_Key == keycode) ||
          STROKE_CODE(
            (type == STROKE_BINDING && b->Button_Key == button &&
             (strcmp(b->Stroke_Seq,stroke) == 0)) ||
              )
          (type == MOUSE_BINDING && b->Button_Key == button)) &&
        (b->Context == Context) &&
        (b->Modifier == modifier))
    {
      return True;
    }
  }
  return False;
}


/***********************************************************************
 *
 *  Procedure:
 *	GrabWindowKey        - grab needed keys for the window for one binding
 *	GrabAllWindowKeys    - grab needed keys for the window for all bindings
 *                             in blist
 *	GrabWindowButton     - same for mouse buttons
 *	GrabAllWindowButtons - same for mouse buttons
 *	GrabAllWindowKeysAndButtons - both of the above
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
 ***********************************************************************/
void GrabWindowKey(Display *dpy, Window w, Binding *binding,
		   unsigned int contexts, unsigned int dead_modifiers,
		   Bool fGrab)
{
  /* remove unnecessary bits from dead_modifiers */
  dead_modifiers &= ~(binding->Modifier & dead_modifiers);

  if((binding->Context & contexts) && (binding->type == KEY_BINDING))
  {
    if (fGrab)
      XGrabKey(dpy, binding->Button_Key, binding->Modifier, w, True,
	       GrabModeAsync, GrabModeAsync);
    else
      XUngrabKey(dpy, binding->Button_Key, binding->Modifier, w);
    if(binding->Modifier != AnyModifier && dead_modifiers != 0)
    {
      register unsigned int mods;
      register unsigned int max = dead_modifiers;
      register unsigned int living_modifiers = ~dead_modifiers;

      /* handle all bindings for the dead modifiers */
      for (mods = 1; mods <= max; mods++)
      {
	/* Since mods starts with 1 we don't need to test if mods
	 * contains a dead modifier. Otherwise both, dead and living
	 * modifiers would be zero ==> mods == 0 */
	if (mods & living_modifiers)
	  continue;
	if (fGrab)
	  XGrabKey(dpy, binding->Button_Key, mods|binding->Modifier, w,
		   True, GrabModeAsync, GrabModeAsync);
	else
	  XUngrabKey(dpy, binding->Button_Key, mods|binding->Modifier,
		     w);
      }
    }
    if (!is_grabbing_everything)
      XSync(dpy, 0);
  }
  return;
}

void GrabAllWindowKeys(Display *dpy, Window w, Binding *blist,
		       unsigned int contexts, unsigned int dead_modifiers,
		       Bool fGrab)
{
  MyXGrabServer(dpy);
  is_grabbing_everything = True;
  for ( ; blist != NULL; blist = blist->NextBinding)
    GrabWindowKey(dpy, w, blist, contexts, dead_modifiers, fGrab);
  is_grabbing_everything = False;
  XSync(dpy, 0);
  MyXUngrabServer(dpy);
  return;
}


void GrabWindowButton(Display *dpy, Window w, Binding *binding,
		      unsigned int contexts, unsigned int dead_modifiers,
		      Cursor cursor, Bool fGrab)
{
  if (binding->Action == NULL)
    return;

  dead_modifiers &= ~(binding->Modifier & dead_modifiers); /* dje */

  if ((binding->Context & contexts) &&
      ((binding->type == MOUSE_BINDING)
       STROKE_CODE(|| (binding->type == STROKE_BINDING))))
  {
    int bmin = 1;
    int bmax = NUMBER_OF_MOUSE_BUTTONS;
    int button;

    if(binding->Button_Key >0)
      bmin = bmax = binding->Button_Key;
    for (button = bmin; button <= bmax; button++)
    {
      if (fGrab)
	XGrabButton(dpy, button, binding->Modifier, w,
		    True, ButtonPressMask | ButtonReleaseMask,
		    GrabModeSync, GrabModeAsync, None, cursor);
      else
	XUngrabButton(dpy, button, binding->Modifier, w);
      if(binding->Modifier != AnyModifier && dead_modifiers != 0)
      {
	register unsigned int mods;
	register unsigned int max = dead_modifiers;
	register unsigned int living_modifiers = ~dead_modifiers;

	/* handle all bindings for the dead modifiers */
	for (mods = 1; mods <= max; mods++)
	{
	  /* Since mods starts with 1 we don't need to test if mods
	   * contains a dead modifier. Otherwise both, dead and living
	   * modifiers would be zero ==> mods == 0 */
	  if (mods & living_modifiers)
	    continue;
	  if (fGrab)
	    XGrabButton(dpy, button, mods|binding->Modifier, w, True,
			ButtonPressMask | ButtonReleaseMask,
			GrabModeSync, GrabModeAsync, None, cursor);
	  else
	    XUngrabButton(dpy, button, mods|binding->Modifier, w);
	}
      } /* if */
      if (!is_grabbing_everything)
	XSync(dpy, 0);
    } /* for */
  } /* if */
  return;
}

void GrabAllWindowButtons(Display *dpy, Window w, Binding *blist,
			  unsigned int contexts, unsigned int dead_modifiers,
			  Cursor cursor, Bool fGrab)
{
  MyXGrabServer(dpy);
  is_grabbing_everything = True;
  for ( ; blist != NULL; blist = blist->NextBinding)
    GrabWindowButton(dpy, w, blist, contexts, dead_modifiers, cursor, fGrab);
  is_grabbing_everything = False;
  XSync(dpy, 0);
  MyXUngrabServer(dpy);
  return;
}

void GrabAllWindowKeysAndButtons(Display *dpy, Window w, Binding *blist,
				 unsigned int contexts,
				 unsigned int dead_modifiers,
				 Cursor cursor, Bool fGrab)
{
  MyXGrabServer(dpy);
  is_grabbing_everything = True;
  for ( ; blist != NULL; blist = blist->NextBinding)
  {
    if (blist->Context & contexts)
    {
      switch (blist->type)
      {
      case MOUSE_BINDING:
      STROKE_CODE(case STROKE_BINDING:)
	GrabWindowButton(
	  dpy, w, blist, contexts, dead_modifiers, cursor, fGrab);
	break;
      case KEY_BINDING:
	GrabWindowKey(dpy, w, blist, contexts, dead_modifiers, fGrab);
	break;
      default:
	break;
      }
    }
  }
  is_grabbing_everything = False;
  XSync(dpy, 0);
  MyXUngrabServer(dpy);
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FvwmStringToKeysym
 *
 *  Like XStringToKeysym, but allows some typos and does some additional
 *  error checking.
 *
 ***********************************************************************/
KeySym FvwmStringToKeysym(Display *dpy, char *key)
{
  KeySym keysym;

  keysym = XStringToKeysym(key);
  if (keysym == NoSymbol)
  {
    char c = 'X';
    char d = 'X';

    /* If the key name is in the form '<letter><digits>...' it's probably
     * something like 'f10'. Convert the letter to upper case and try
     * again. */
    sscanf(key, "%c%c", &c, &d);
    if (islower(c) && isdigit(d))
    {
      d = key[0];
      key[0] = toupper(c);
      keysym = XStringToKeysym(key);
      key[0] = d;
    }
  }
  if (keysym == NoSymbol || XKeysymToKeycode(dpy, keysym) == 0)
    return 0;
  return keysym;
}
