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
  {'a',C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|
   C_L1|C_L2|C_L3|C_L4|C_L5|C_R1|C_R2|C_R3|C_R4|C_R5},
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
static Bool find_context(char *string, int *output, struct charstring *table)
{
  int i=0,j=0;
  Bool matched;
  char tmp1;
  Bool error = False;

  *output=0;
  i=0;
  while(i<strlen(string))
  {
    j=0;
    matched = FALSE;
    while((!matched)&&(table[j].key != 0))
    {
      /* in some BSD implementations, tolower(c) is not defined
       * unless isupper(c) is true */
      tmp1=string[i];
      if(isupper(tmp1))
        tmp1=tolower(tmp1);
      /* end of ugly BSD patch */

      if(tmp1 == table[j].key)
      {
        *output |= table[j].value;
        matched = TRUE;
      }
      j++;
    }
    if(!matched)
    {
      fprintf(stderr,"find_context: bad context or modifier %c\n", tmp1);
      error = True;
    }
    i++;
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

/*
** to remove a binding from the global list (probably needs more processing
** for mouse binding lines though, like when context is a title bar button).
** Specify either button or keysym, depending on type.
*/
void RemoveBinding(Display *dpy, Binding **pblist, BindingType type,
		   int button, KeySym keysym, int modifiers, int contexts)
{
  Binding *temp=*pblist, *temp2, *prev=NULL;
  KeyCode keycode = 0;

  if (type == KEY_BINDING)
    keycode=XKeysymToKeycode(dpy,keysym);

  while (temp)
  {
    temp2 = temp->NextBinding;
    if (temp->type == type)
    {
      if ((temp->Button_Key == (type == MOUSE_BINDING ? button : keycode)) &&
          (temp->Context == contexts) &&
          (temp->Modifier == modifiers))
      {
        /* we found it, remove it from list */
        if (prev) /* middle of list */
        {
          prev->NextBinding = temp2;
        }
        else /* must have been first one, set new start */
        {
          *pblist = temp2;
        }
	if (temp->key_name)
	  free(temp->key_name);
	if (temp->Action)
	  free(temp->Action);
	if (temp->Action2)
	  free(temp->Action2);
        free(temp);
        temp=NULL;
      }
    }
    if (temp)
      prev=temp;
    temp=temp2;
  }
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
Binding *AddBinding(Display *dpy, Binding **pblist, BindingType type,
		    int button, KeySym keysym, char *key_name, int modifiers,
		    int contexts, void *action, void *action2)
{
  int i;
  int min;
  int max;
  int maxmods;
  int m;
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
  for (i=min; i<=max; i++)
  {
    /* If this is a mouse binding we'll fall through the for loop (maxmods is
     * zero) and the if condition is always true (type is zero). Since min ==
     * max == button there is no loop at all is case of a mouse binding. */
    for (m = 0, tkeysym = XK_Left; m <= maxmods && tkeysym != NoSymbol; m++)
      if (type == MOUSE_BINDING ||
	  (tkeysym = XKeycodeToKeysym(dpy, i, m)) == keysym)
      {
	temp = *pblist;
	(*pblist) = (Binding *)safemalloc(sizeof(Binding));
	(*pblist)->type = type;
	(*pblist)->Button_Key = i;
	if (type == KEY_BINDING && key_name != NULL)
	  (*pblist)->key_name = stripcpy(key_name);
	else
	  (*pblist)->key_name = NULL;
	(*pblist)->Context = contexts;
	(*pblist)->Modifier = modifiers;
	(*pblist)->Action = action;
	(*pblist)->Action2 = action2;
	(*pblist)->NextBinding = temp;
      }
  }
  return *pblist;
}


/****************************************************************************
 *
 *  Parses a mouse or key binding
 *
 ****************************************************************************/
Binding *ParseBinding(Display *dpy, Binding **pblist, char *tline,
		      BindingType type, int *nr_left_buttons,
		      int *nr_right_buttons, unsigned char *buttons_grabbed)
{
  char *action, context[20], modifiers[20], key[20], *ptr, *token;
  int button;
  int n1=0,n2=0,n3=0;
  int i;
  KeySym keysym;
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

/* Check if something is bound to a key or button press and return the action
 * to be executed or NULL if not. */
void *CheckBinding(Binding *blist, int button_keycode, unsigned int modifier,
		   unsigned int dead_modifiers, int Context, BindingType type)
{
  Binding *b;
  unsigned int used_modifiers = ~dead_modifiers;

  modifier &= used_modifiers;

  for (b = blist; b != NULL; b = b->NextBinding)
    {
      if ((b->Button_Key == button_keycode ||
	   (type == MOUSE_BINDING && b->Button_Key == 0))
	  && (((b->Modifier & used_modifiers) == modifier) ||
	      (b->Modifier == AnyModifier))
	  && (b->Context & Context)
	  && (b->type == type))
	{
	  /* Make throw away excess events before executing our binding */
	  return b->Action;
	}
    }
  return NULL;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabWindowKey        - grab needed keys for the window for one binding
 *	GrabAllWindowKeys    - grab needed keys for the window for all bindings
 *                             in blist
 *	GrabWindowButton     - same for mouse buttons
 *	GrabAllWindowButtons - same for mouse buttons
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
      if(binding->Modifier != AnyModifier && binding->Modifier != 0 &&
	 dead_modifiers != 0)
	{
	  register unsigned int mods;
	  register unsigned int max = dead_modifiers | binding->Modifier;

	  /* handle all bindings for the dead modifiers */
	  for (mods = 1; mods <= max; mods++)
	    {
	      if ((mods & ~dead_modifiers) != binding->Modifier)
		continue;
	      if (fGrab)
		XGrabKey(dpy, binding->Button_Key, mods|binding->Modifier, w,
			 True, GrabModeAsync, GrabModeAsync);
	      else
		XUngrabKey(dpy, binding->Button_Key, mods|binding->Modifier,
			   w);
	    }
	}
    }
  return;
}

void GrabAllWindowKeys(Display *dpy, Window w, Binding *blist,
		       unsigned int contexts, unsigned int dead_modifiers,
		       Bool fGrab)
{
  MyXGrabServer(dpy);
  for ( ; blist != NULL; blist = blist->NextBinding)
    GrabWindowKey(dpy, w, blist, contexts, dead_modifiers, fGrab);
  MyXUngrabServer(dpy);
  XSync(dpy, 0);
  return;
}


void GrabWindowButton(Display *dpy, Window w, Binding *binding,
		      unsigned int contexts, unsigned int dead_modifiers,
		      Cursor cursor, Bool fGrab)
{
  Binding *b;

  if (binding->Action == NULL)
    return;

  if((binding->Context & contexts) && (binding->type == MOUSE_BINDING))
    {
      int bmin = 1;
      int bmax = 3;
      int button;

      if(binding->Button_Key >0)
	bmin = bmax = binding->Button_Key;

      for (button = bmin; button <= bmax; button++)
	{
	  if (fGrab)
	    XGrabButton(dpy, button, binding->Modifier, w,
			True, ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, None, cursor);
	  else
	    XUngrabButton(dpy, button, binding->Modifier, w);
	  if(binding->Modifier != AnyModifier && binding->Modifier != 0 &&
	     dead_modifiers != 0)
	    {
	      register unsigned int mods;
	      register unsigned int max = dead_modifiers | binding->Modifier;

	      /* handle all bindings for the dead modifiers */
	      for (mods = 1; mods <= max; mods++)
		{
		  if ((mods & ~dead_modifiers) != binding->Modifier)
		    continue;
		  if (fGrab)
		    XGrabButton(dpy, button, mods|binding->Modifier, w, True,
				ButtonPressMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, None, cursor);
		  else
		    XUngrabButton(dpy, button, mods|binding->Modifier, w);
		}
	    } /* if */
	} /* for */
    } /* if */
  return;
}

void GrabAllWindowButtons(Display *dpy, Window w, Binding *blist,
			  unsigned int contexts, unsigned int dead_modifiers,
			  Cursor cursor, Bool fGrab)
{
  MyXGrabServer(dpy);
  for ( ; blist != NULL; blist = blist->NextBinding)
    GrabWindowButton(dpy, w, blist, contexts, dead_modifiers, cursor, fGrab);
  XSync(dpy, 0);
  MyXUngrabServer(dpy);
  XSync(dpy, 0);
  return;
}
