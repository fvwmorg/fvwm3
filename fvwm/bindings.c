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
