#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "fvwm.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include <X11/keysym.h>

#define MODS_UNUSED_DEFAULT LockMask
static unsigned int mods_unused = MODS_UNUSED_DEFAULT;

struct charstring
{
  char key;
  int  value;
};


/* The keys musat be in lower case! */
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

void find_context(char *string, int *output, struct charstring *table,
		  char *tline);

/*
** to remove a binding from the global list (probably needs more processing
** for mouse binding lines though, like when context is a title bar button).
*/
void remove_binding(int contexts, int mods, int button, KeySym keysym,
                    int mouse_binding)
{
  Binding *temp=Scr.AllBindings, *temp2, *prev=NULL;
  KeyCode keycode = 0;

  if (!mouse_binding)
    keycode=XKeysymToKeycode(dpy,keysym);

  while (temp)
  {
    temp2 = temp->NextBinding;
    if (temp->IsMouse == mouse_binding)
    {
      if ((temp->Button_Key == ((mouse_binding)?(button):(keycode))) &&
          (temp->Context == contexts) &&
          (temp->Modifier == mods))
      {
        /* we found it, remove it from list */
        if (prev) /* middle of list */
        {
          prev->NextBinding = temp2;
        }
        else /* must have been first one, set new start */
        {
          Scr.AllBindings = temp2;
        }
        free(temp->key_name);
        free(temp->Action);
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
 *  Parses a mouse or key binding
 *
 ****************************************************************************/
void ParseBindEntry(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long junk, char *tline,int* Module, Bool fKey)
{
  char *action,context[20],modifiers[20],key[20],*ptr, *token;
  Binding *temp;
  int button,i,min,max;
  int n1=0,n2=0,n3=0;
  KeySym keysym;
  KeySym tkeysym;
  int contexts;
  int mods;
  int maxmods;
  int m;

  /* tline points after the key word "Mouse" or "Key" */
  ptr = GetNextToken(tline, &token);
  if(token != NULL)
  {
    if (fKey)
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
    fvwm_msg(ERR,"ParseBindEntry","Syntax error in line %s",tline);
    return;
  }

  find_context(context,&contexts,win_contexts,tline);
  find_context(modifiers,&mods,key_modifiers,tline);

  if (fKey)
    {
      /*
       * Don't let a 0 keycode go through, since that means AnyKey to the
       * XGrabKey call in GrabKeys().
       */
      if ((keysym = XStringToKeysym(key)) == NoSymbol ||
	  (XKeysymToKeycode(dpy, keysym)) == 0)
	return;
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
    if (fKey)
      remove_binding(contexts,mods,0,keysym,0);
    else
      remove_binding(contexts,mods,button,0,1);
    return;
  }

  if (!fKey)
    {
      int j;

      if((contexts != C_ALL) && (contexts & C_LALL))
	{
	  /* check for nr_left_buttons */
	  i=0;
	  j=(contexts &C_LALL)/C_L1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(Scr.nr_left_buttons <i)
	    Scr.nr_left_buttons = i;
	}
      if((contexts != C_ALL) && (contexts & C_RALL))
	{
	  /* check for nr_right_buttons */
	  i=0;
	  j=(contexts&C_RALL)/C_R1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(Scr.nr_right_buttons <i)
	    Scr.nr_right_buttons = i;
	}
    }

  if((mods & AnyModifier)&&(mods&(~AnyModifier)))
  {
    fvwm_msg(WARN,"ParseBindEntry",
             "Binding specified AnyModifier and other modifers too. Excess modifiers will be ignored.");
    mods &= AnyModifier;
  }

  if((!fKey)&&(contexts & C_WINDOW)&&(((mods==0)||mods == AnyModifier)))
  {
    Scr.buttons2grab &= ~(1<<(button-1));
  }

  /*
  ** Unfortunately a keycode can be bound to multiple keysyms and a keysym can
  ** be bound to multiple keycodes. Thus we have to check every keycode with
  ** any single modifier.
  */
  if (fKey)
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
     * zero) and the if condition is always true (fKey is zero). Since min ==
     * max == button there is no loop at all is case of a mouse binding. */
    for (m = 0, tkeysym = XK_Left; m <= maxmods && tkeysym != NoSymbol; m++)
      if (!fKey || (tkeysym = XKeycodeToKeysym(dpy, i, m)) == keysym)
      {
	temp = Scr.AllBindings;
	Scr.AllBindings = (Binding *)safemalloc(sizeof(Binding));
	Scr.AllBindings->IsMouse = !fKey;
	Scr.AllBindings->Button_Key = i;
	Scr.AllBindings->key_name = fKey ? stripcpy(key) : NULL;
	Scr.AllBindings->Context = contexts;
	Scr.AllBindings->Modifier = mods;
	Scr.AllBindings->Action = stripcpy(action);
	Scr.AllBindings->NextBinding = temp;
      }
  }
  return;
}

void ParseMouseEntry(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                     unsigned long junk, char *tline,int* Module)
{
  ParseBindEntry(eventp, w, tmp_win, junk, tline, Module, False);
}

void ParseKeyEntry(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long junk, char *tline,int* Module)
{
  ParseBindEntry(eventp, w, tmp_win, junk, tline, Module, True);
}

/****************************************************************************
 *
 * Turns a  string context of context or modifier values into an array of
 * true/false values (bits)
 *
 ****************************************************************************/
void find_context(char *string, int *output, struct charstring *table,
		  char *tline)
{
  int i=0,j=0;
  Bool matched;
  char tmp1;

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
      fvwm_msg(ERR,"find_context",
               "bad context in line %s",tline);
    }
    i++;
  }
  return;
}


/* Check if something is bount to a key or button press and execute the
 * function if necessary. */
void CheckBinding(int keycode, unsigned int modifier, FvwmWindow *Tmp_win,
		  int Context, int IsMouse)
{
  Binding *key;

  modifier &= ~mods_unused;
  for (key = Scr.AllBindings; key != NULL; key = key->NextBinding)
    {
      if ((key->Button_Key == keycode || (IsMouse && key->Button_Key == 0)) &&
	  ((key->Modifier == MaskUsedModifiers(modifier))||
	   (key->Modifier == AnyModifier)) &&
	  (key->Context & Context)&&
	  (key->IsMouse == IsMouse))
	{
	  ExecuteFunction(key->Action,Tmp_win, &Event,Context,-1);
	  break;
	}
    }
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
