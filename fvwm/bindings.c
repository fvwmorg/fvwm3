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

#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include "fvwm.h"
#include "screen.h"
#include "bindings.h"


#define MODS_UNUSED_DEFAULT LockMask
static unsigned int mods_unused = MODS_UNUSED_DEFAULT;


/****************************************************************************
 *
 *  Parses a mouse or key binding
 *
 ****************************************************************************/
Binding *ParseBinding(Display *dpy, Binding **pblist, char *tline,
		      BindingType type, int *nr_left_buttons,
		      int *nr_right_buttons, unsigned char *buttons_grabbed)
{
  char *action, context[20], modifiers[20], *ptr, *token;
  char key[20] = { '\0' };
  int button = 0;
  int n1=0,n2=0,n3=0;
  int i;
  KeySym keysym = NoSymbol;
  int contexts;
  int mods;

  /* tline points after the key word "Mouse" or "Key" */
  ptr = GetNextToken(tline, &token);
  if(token != NULL)
  {
    if (type == KEY_BINDING)
      n1 = sscanf(token,"%19s",key);
    else
      n1 = sscanf(token,"%d",&button);
    free(token);
  }

  ptr = GetNextToken(ptr,&token);
  if(token != NULL)
  {
    n2 = sscanf(token,"%19s",context);
    free(token);
  }

  action = GetNextToken(ptr,&token);
  if(token != NULL)
  {
    n3 = sscanf(token,"%19s",modifiers);
    free(token);
  }

  if((n1 != 1)||(n2 != 1)||(n3 != 1))
  {
    fprintf(stderr,"ParseBinding: Syntax error in line %s\n", tline);
    return NULL;
  }

  if (ParseContext(context, &contexts))
    fprintf(stderr,"ParseBinding: Illegal context in line %s\n", tline);
  if (ParseModifiers(modifiers, &mods))
    fprintf(stderr,"ParseBinding: Illegal modifier in line %s\n", tline);

  if (type == KEY_BINDING)
    {
      /*
       * Don't let a 0 keycode go through, since that means AnyKey to the
       * XGrabKey call in GrabKeys().
       */
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
	      key[0] = toupper(key[0]);
	      keysym = XStringToKeysym(key);
	    }
	}
      if (keysym == NoSymbol || XKeysymToKeycode(dpy, keysym) == 0)
	return NULL;
    }

  /*
  ** strip leading whitespace from action if necessary
  */
  while (*action && (*action == ' ' || *action == '\t'))
    action++;

  /*
  ** is this an unbind request?
  */
  if (!action || action[0] == '-')
  {
    if (type == KEY_BINDING)
      RemoveBinding(dpy, pblist, KEY_BINDING, 0, keysym, mods, contexts);
    else
      RemoveBinding(dpy, pblist, MOUSE_BINDING, button, 0, mods, contexts);
    return NULL;
  }

  if (type == MOUSE_BINDING)
    {
      int j;

      if((contexts != C_ALL) && (contexts & C_LALL) &&
	 (nr_left_buttons != NULL))
	{
	  /* check for nr_left_buttons */
	  i=0;
	  j=(contexts &C_LALL)/C_L1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(*nr_left_buttons < i)
	    *nr_left_buttons = i;
	}
      if((contexts != C_ALL) && (contexts & C_RALL) &&
	 (nr_right_buttons != NULL))
	{
	  /* check for nr_right_buttons */
	  i=0;
	  j=(contexts&C_RALL)/C_R1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(*nr_right_buttons < i)
	    *nr_right_buttons = i;
	}
    }

  if((mods & AnyModifier)&&(mods&(~AnyModifier)))
  {
    fprintf(stderr,"ParseBinding: Binding specified AnyModifier and other modifers too.\n");
    fprintf(stderr,"Excess modifiers will be ignored.\n");
    mods &= AnyModifier;
  }

  if((type == MOUSE_BINDING)&&(contexts & C_WINDOW)&&
     (((mods==0)||mods == AnyModifier)) && (buttons_grabbed != NULL))
  {
    *buttons_grabbed &= ~(1<<(button-1));
  }

  return AddBinding(dpy, pblist, type, button, keysym, key, mods, contexts,
		    (void *)(stripcpy(action)), NULL);
}

static void activate_binding(Binding *binding, BindingType type)
{
  FvwmWindow *t;

  if (fFvwmInStartup == True || binding == NULL)
    return;

  /* grab keys immediately */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (binding->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))
	GrabWindowKey(dpy, t->frame, binding,
		      C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR,
		      GetUnusedModifiers(), True);
      if (binding->Context & C_ICON)
	{
	  if(t->icon_w != None)
	    GrabWindowKey(dpy, t->icon_w, binding, C_ICON,
			  GetUnusedModifiers(), True);
	  if(t->icon_pixmap_w != None)
	    GrabWindowKey(dpy, t->icon_pixmap_w, binding, C_ICON,
			  GetUnusedModifiers(), True);
	}
    }
}


void key_binding(F_CMD_ARGS)
{
  Binding *b;

  b = ParseBinding(dpy, &Scr.AllBindings, action, KEY_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);
  activate_binding(b, KEY_BINDING);
}

void mouse_binding(F_CMD_ARGS)
{
  Binding *b;

  b = ParseBinding(dpy, &Scr.AllBindings, action, MOUSE_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);
  activate_binding(b, MOUSE_BINDING);
}

/* Removes all unused modifiers from in_modifiers */
unsigned int MaskUsedModifiers(unsigned int in_modifiers)
{
  return in_modifiers & ~mods_unused;
}

unsigned int GetUnusedModifiers(void)
{
  return mods_unused;
}

/***********************************************************************
 *
 *	SetModifierLocks : declares which X modifiers are actually locks
 *		and should be ignored when testing mouse/key binding modifiers.
 *	syntax :
 *		ModifierLocks [S][L][C][M][1][2][3][4][5]
 *
 *	S is shift
 *	L is caps lock
 *	C is control
 *	M is meta, the same as mod1
 *	1-5 are mod1-mod5
 *	The default is L25 (ie. mask out caps lock, num lock & scroll lock
 *	for XFree86 3.3.3.1
 *
 *	Benoit TRIQUET <benoit@adsl-216-100-248-201.dsl.pacbell.net> 2/21/99
 ***********************************************************************/
void ignore_modifiers(F_CMD_ARGS)
{
  char *token;

  mods_unused = 0;
  GetNextToken(action, &token);
  if (!token)
    return;

  if (StrEquals(token, "default"))
    {
      free(token);
      mods_unused = MODS_UNUSED_DEFAULT;
      return;
    }

  if (ParseModifiers(token, &mods_unused))
    fvwm_msg(ERR, "ignore_modifiers", "illegal modifier in line %s\n", action);

  free(token);
  return;
}
