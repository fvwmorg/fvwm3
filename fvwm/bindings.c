/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "defaults.h"
#include "module_interface.h"
#include "misc.h"
#include "screen.h"
#include "focus.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

static void activate_binding(
  Binding *binding, BindingType type, Bool do_grab, Bool do_grab_root);

static int mods_unused = DEFAULT_MODS_UNUSED;


static void update_nr_buttons(
  int contexts, int *nr_left_buttons, int *nr_right_buttons)
{
  int i;
  int l = (nr_left_buttons) ? *nr_left_buttons : 0;
  int r = (nr_right_buttons) ? *nr_right_buttons : 0;

  if (contexts != C_ALL && (contexts & C_LALL) && nr_left_buttons != NULL)
  {
    /* check for nr_left_buttons */
    for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
    {
      if ((contexts & (C_L1 << i)) && *nr_left_buttons <= i / 2)
      {
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_nr_buttons_changed = 1;
	*nr_left_buttons = i / 2 + 1;
      }
    }
  }
  if(contexts != C_ALL && (contexts & C_RALL) && nr_right_buttons != NULL)
  {
    /* check for nr_right_buttons */
    for (i = 1; i < NUMBER_OF_BUTTONS; i += 2)
    {
      if ((contexts & (C_L1 << i)) && *nr_right_buttons <= i / 2)
      {
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_nr_buttons_changed = 1;
	*nr_right_buttons = i / 2 + 1;
      }
    }
  }
  if ((nr_left_buttons && *nr_left_buttons != l) ||
      (nr_right_buttons && *nr_right_buttons != r))
  {
    Scr.flags.do_need_window_update = 1;
    Scr.flags.has_nr_buttons_changed = 1;
  }
}

/****************************************************************************
 *
 *  Parses a mouse or key binding
 *
 ****************************************************************************/
int ParseBinding(
  Display *dpy, Binding **pblist, char *tline, BindingType type,
  int *nr_left_buttons, int *nr_right_buttons, unsigned char *buttons_grabbed,
  Bool do_ungrab_root, Bool is_silent)
{
  char *action, context_string[20], modifier_string[20], *ptr, *token;
  char key_string[31] = "";
  int button = 0;
  STROKE_CODE(char stroke[STROKE_MAX_SEQUENCE + 1] = "";)
  int n1=0,n2=0,n3=0;
  STROKE_CODE(int n4=0;)
  STROKE_CODE(int i;)
  KeySym keysym = NoSymbol;
  int context;
  int modifier;
  Bool is_unbind_request = False;
  int rc;

  /* tline points after the key word "Mouse" or "Key" */
  ptr = GetNextToken(tline, &token);
  if (token != NULL)
  {
    if (type == KEY_BINDING)
      n1 = sscanf(token,"%30s", key_string); /* see len of key_string above */
#ifdef HAVE_STROKE
    else if (type == STROKE_BINDING)
    {
      int num = 0;
      int j;

      n1 = 1;
      i = 0;
      if (token[0] == 'N' && token[1] != '\0')
	num = 1;
      j=i+num;
      while(n1 && token[j] != '\0' && i < STROKE_MAX_SEQUENCE)
      {
	if (!isdigit(token[j]))
	  n1 = 0;
	if (num)
	{
	  /* Numeric pad to Telephone  */
	  if ('7' <= token[j] && token[j] <= '9')
	    token[j] -= 6;
	  else if ('1' <= token[j] && token[j] <= '3')
	    token[j] += 6;
	}
	stroke[i] = token[j];
	i++;
	j=i+num;
      }
      stroke[i] = '\0';
      if (strlen(token) > STROKE_MAX_SEQUENCE + num)
      {
	if (!is_silent)
	  fvwm_msg(WARN, "ParseBinding",
	    "Too long stroke sequence in line %s"
	    "Only %i elements will be taken into account.\n",
	    tline, STROKE_MAX_SEQUENCE);
      }
    }
#endif /* HAVE_STROKE */
    else
    {
      n1 = sscanf(token, "%d", &button);
      if (button < 0)
      {
	if (!is_silent)
	  fvwm_msg(ERR, "ParseBinding",
	    "Illegal mouse button in line %s", tline);
	free(token);
	return 0;
      }
      if (button > NUMBER_OF_MOUSE_BUTTONS)
      {
	if (!is_silent)
	  fvwm_msg(WARN, "ParseBinding",
	    "Got mouse button %d when the maximum is %d.\n  You can't bind "
	     "complex function to this button.	To suppress the warning, use:"
	     "\n  Silent Mouse %s", button, NUMBER_OF_MOUSE_BUTTONS, tline);
      }
    }
    free(token);
  }

#ifdef HAVE_STROKE
  if (type == STROKE_BINDING)
  {
    ptr = GetNextToken(ptr, &token);
    if (token != NULL)
    {
      n4 = sscanf(token,"%d", &button);
      free(token);
    }
  }
#endif /* HAVE_STROKE */

  ptr = GetNextToken(ptr, &token);
  if (token != NULL)
  {
    n2 = sscanf(token, "%19s", context_string);
    free(token);
  }

  action = GetNextToken(ptr, &token);
  if (token != NULL)
  {
    n3 = sscanf(token, "%19s", modifier_string);
    free(token);
  }

  if (n1 != 1 || n2 != 1 || n3 != 1
     STROKE_CODE(|| (type == STROKE_BINDING && n4 != 1)))
  {
    if (!is_silent)
      fvwm_msg(ERR, "ParseBinding", "Syntax error in line %s", tline);
    return 0;
  }

  if (ParseContext(context_string, &context) && !is_silent)
    fvwm_msg(WARN, "ParseBinding", "Illegal context in line %s", tline);
  if (ParseModifiers(modifier_string, &modifier) && !is_silent)
    fvwm_msg(WARN, "ParseBinding", "Illegal modifier in line %s", tline);

  if (type == KEY_BINDING)
  {
    keysym = FvwmStringToKeysym(dpy, key_string);
    /*
     * Don't let a 0 keycode go through, since that means AnyKey to the
     * XGrabKey call.
     */
    if (keysym == 0)
    {
      if (!is_silent)
	fvwm_msg(ERR, "ParseBinding", "No such key: %s", key_string);
      return 0;
    }
  }

  /*
  ** strip leading whitespace from action if necessary
  */
  while (*action && (*action == ' ' || *action == '\t'))
    action++;
  /* see if it is an unbind request */
  if (!action || action[0] == '-')
    is_unbind_request = True;

  /*
  ** Remove the "old" bindings if any
  */
  {
    Bool is_binding_removed = False;
    Binding *b;
    Binding *rmlist = NULL;

    CollectBindingList(
      dpy, pblist, &rmlist, type, STROKE_ARG((void *)stroke)
      button, keysym, modifier, context);
    if (rmlist != NULL)
    {
      is_binding_removed = True;
      if (is_unbind_request)
      {
	/* remove the grabs for the key for unbind requests */
	for (b = rmlist; b != NULL; b = b->NextBinding)
	{
	  /* release the grab */
	  activate_binding(b, type, False, do_ungrab_root);
	}
      }
      FreeBindingList(rmlist);
    }

    if (is_binding_removed)
    {
      int bcontext = 0;

      if (buttons_grabbed)
	*buttons_grabbed = 0;
      for (b = *pblist; b != NULL; b = b->NextBinding)
      {
	if(b->type == MOUSE_BINDING &&
	   (b->Context & (C_WINDOW|C_EWMH_DESKTOP)) &&
	   (b->Modifier == 0 || b->Modifier == AnyModifier) &&
	   buttons_grabbed != NULL)
	{
	  if (button == 0)
	  {
	    *buttons_grabbed |= ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1);
	  }
	  else
	  {
	    *buttons_grabbed |= (1 << (button - 1));
	  }
	}
	if (nr_left_buttons != NULL || nr_right_buttons != NULL)
	{
	  if (b->Context != C_ALL && (b->Context & (C_LALL | C_RALL)) &&
	      b->type == MOUSE_BINDING)
	  {
	    bcontext |= b->Context;
	  }
	}
      }
      if (nr_left_buttons)
	*nr_left_buttons = 0;
      if (nr_right_buttons)
	*nr_right_buttons = 0;
      update_nr_buttons(bcontext, nr_left_buttons, nr_right_buttons);
    }
  }

  /* return if it is an unbind request */
  if (is_unbind_request)
    return 0;

  update_nr_buttons(context, nr_left_buttons, nr_right_buttons);

  if((modifier & AnyModifier)&&(modifier&(~AnyModifier)))
  {
    fvwm_msg(WARN,"ParseBinding","Binding specified AnyModifier and other "
	     "modifers too. Excess modifiers will be ignored.");
    modifier = AnyModifier;
  }

  if ((type == MOUSE_BINDING)&&
      (context & (C_WINDOW|C_EWMH_DESKTOP))&&
     (((modifier==0)||modifier == AnyModifier)) && (buttons_grabbed != NULL))
  {
    if (button == 0)
    {
      *buttons_grabbed |= ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1);
    }
    else
    {
      *buttons_grabbed |= (1 << (button - 1));
    }
  }

  rc = AddBinding(
    dpy, pblist, type, STROKE_ARG((void *)stroke)
    button, keysym, key_string, modifier, context, (void *)action, NULL);

  return rc;
}

static void activate_binding(
  Binding *binding, BindingType type, Bool do_grab, Bool do_grab_root)
{
  FvwmWindow *t;

  if (binding == NULL)
    return;
  if (do_grab_root)
  {
    /* necessary for key bindings that work over unfocused windows */
    GrabWindowKeyOrButton(
      dpy, Scr.Root, binding,
      C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR|C_ROOT|C_ICON|C_EWMH_DESKTOP,
      GetUnusedModifiers(), None, do_grab);
  }
  if (fFvwmInStartup == True)
    return;

  /* grab keys immediately */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (!IS_EWMH_DESKTOP(FW_W(t)) && (binding->Context & C_WINDOW) &&
	(binding->type == MOUSE_BINDING
	 STROKE_CODE(|| binding->type == STROKE_BINDING)))
    {
      GrabWindowButton(
	dpy, FW_W_PARENT(t), binding, C_WINDOW, GetUnusedModifiers(), None,
	do_grab);
    }
    if (!IS_EWMH_DESKTOP(FW_W(t)) &&
	(binding->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))
	&& binding->type == KEY_BINDING)
    {
      GrabWindowKey(
	dpy, FW_W_FRAME(t), binding,
	C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR,
	GetUnusedModifiers(), do_grab);
    }
    if (binding->Context & C_ICON)
    {
      if (FW_W_ICON_TITLE(t) != None)
      {
	GrabWindowKeyOrButton(
	  dpy, FW_W_ICON_TITLE(t), binding, C_ICON, GetUnusedModifiers(), None,
	  do_grab);
      }
      if (FW_W_ICON_PIXMAP(t) != None)
      {
	GrabWindowKeyOrButton(
	  dpy, FW_W_ICON_PIXMAP(t), binding, C_ICON, GetUnusedModifiers(),
	  None, do_grab);
      }
    }
    if (IS_EWMH_DESKTOP(FW_W(t)) && (binding->Context & C_EWMH_DESKTOP))
    {
      GrabWindowKeyOrButton(
	dpy, FW_W_PARENT(t), binding, C_EWMH_DESKTOP, GetUnusedModifiers(),
	None, do_grab);
    }
  }

  return;
}

static void binding_cmd(F_CMD_ARGS, BindingType type, Bool do_grab_root)
{
  Binding *b;
  int count;
  unsigned char btg = Scr.buttons2grab;

  count = ParseBinding(
    dpy, &Scr.AllBindings, action, type, &Scr.nr_left_buttons,
    &Scr.nr_right_buttons, &btg, do_grab_root, Scr.flags.silent_functions);
  if (btg != Scr.buttons2grab)
  {
    Scr.flags.do_need_window_update = 1;
    Scr.flags.has_mouse_binding_changed = 1;
    Scr.buttons2grab = btg;
  }
  for (b = Scr.AllBindings; count > 0; count--, b = b->NextBinding)
  {
    activate_binding(b, type, True, do_grab_root);
  }

  return;
}

void CMD_Key(F_CMD_ARGS)
{
  binding_cmd(F_PASS_ARGS, KEY_BINDING, False);
}

void CMD_PointerKey(F_CMD_ARGS)
{
  binding_cmd(F_PASS_ARGS, KEY_BINDING, True);
}

void CMD_Mouse(F_CMD_ARGS)
{
  binding_cmd(F_PASS_ARGS, MOUSE_BINDING, False);
}

#ifdef HAVE_STROKE
void CMD_Stroke(F_CMD_ARGS)
{
  binding_cmd(F_PASS_ARGS, STROKE_BINDING, False);
}
#endif /* HAVE_STROKE */

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
  else if (ParseModifiers(token, &mods_unused))
  {
    fvwm_msg(ERR, "ignore_modifiers", "illegal modifier in line %s\n", action);
  }
  if (mods_unused != mods_unused_old)
  {
    /* broadcast config to modules */
    broadcast_ignore_modifiers();
  }

  return;
}
