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
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"


#define MODS_UNUSED_DEFAULT LockMask
static unsigned int mods_unused = MODS_UNUSED_DEFAULT;


void key_binding(F_CMD_ARGS)
{
  Binding *b;
  FvwmWindow *t;

  b = ParseBinding(dpy, &Scr.AllBindings, action, KEY_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);

  if (fFvwmInStartup == False)
    return;

  /* grab keys immediately */
  if (b != NULL)
    {
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  if (b->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))
	    GrabWindowKey(dpy, t->frame, b,
			  C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR,
			  GetUnusedModifiers(), True);
	  if (b->Context & C_ICON)
	    {
	      if(tmp_win->icon_w != None)
		GrabWindowKey(dpy, tmp_win->icon_w, b, C_ICON,
			      GetUnusedModifiers(), True);
	      if(tmp_win->icon_pixmap_w != None)
		GrabWindowKey(dpy, tmp_win->icon_pixmap_w, b, C_ICON,
			      GetUnusedModifiers(), True);
	    }
	}
    }
}

void mouse_binding(F_CMD_ARGS)
{
  ParseBinding(dpy, &Scr.AllBindings, action, MOUSE_BINDING,
	       &Scr.nr_left_buttons, &Scr.nr_right_buttons, &Scr.buttons2grab);
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
  char *tmp;
  char c;

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
  tmp = token;
  while ((c = *(tmp++)) != '\0')
  {
    switch (c)
    {
    case 's':
    case 'S':
      mods_unused |= ShiftMask;
      break;
    case 'l':
    case 'L':
      mods_unused |= LockMask;
      break;
    case 'c':
    case 'C':
      mods_unused |= ControlMask;
      break;
    case 'm':
    case 'M':
    case '1':
      mods_unused |= Mod1Mask;
      break;
    case '2':
      mods_unused |= Mod2Mask;
      break;
    case '3':
      mods_unused |= Mod3Mask;
      break;
    case '4':
      mods_unused |= Mod4Mask;
      break;
    case '5':
      mods_unused |= Mod5Mask;
      break;
    default:
      fvwm_msg( ERR, "SetModifierLocks", "illegal modifier: %c\n", c);
    }
  }
  free(token);
}
