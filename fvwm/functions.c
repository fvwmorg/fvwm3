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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * fvwm built-in functions and complex functions
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <X11/keysym.h>
#include "fvwm.h"
#include "events.h"
#include "style.h"
#include "functions.h"
#include "menus.h"
#include "focus.h"
#include "misc.h"
#include "screen.h"
#include "bindings.h"
#include "repeat.h"
#include "Module.h"
#include "move_resize.h"
#include "virtual.h"
#include "stack.h"
#include "gnome.h"
#include "borders.h"

extern XEvent Event;
extern FvwmWindow *Tmp_win;

/* forward declarations */
static void ComplexFunction(F_CMD_ARGS);
static void execute_complex_function(F_CMD_ARGS, Bool *desperate,
				     expand_command_type expand_cmd);

/*
 * be sure to keep this list properly ordered for bsearch routine!
 *
 * Note: the 3rd column of this table is no longer in use!
 * Remove the field "code" from the structure "functions" in misc.c.
 * Then remove the values from this table.
 * dje 9/25/98
 *
 * Note: above comment is incorrect. The 3rd column is still used in
 * find_func_type which is called from menus.c. Some work has to be done
 * before it can be removed.
 * Dominik Vogt 21/11/98
 *
 * My   mistake. However,  they  are  no  longer  used for their original
 * purpose, and  the   use menus.c is  putting   them to  seems  somewhat
 * unnecessary.  I'm  going to use  F_NOP for EdgeThickness,  and see
 * how that goes.
 * dje 12/19/98.
 */

#define PRE_REPEAT       "repeat"
#define PRE_SILENT       "silent"

/* The function names in the first field *must* be in lowercase or else the
 * function cannot be called. */
static const struct functions func_config[] =
{
  {"+",            add_another_item, F_ADDMENU2,            0},
#ifdef MULTISTYLE
  {"addbuttonstyle",AddButtonStyle,  F_ADD_BUTTON_STYLE,    0},
#endif /* MULTISTYLE */
  {"addmoduleconfig",  AddModConfig, F_ADD_MOD,             0},
#ifdef MULTISTYLE
#ifdef EXTENDED_TITLESTYLE
  {"addtitlestyle",AddTitleStyle,    F_ADD_TITLE_STYLE,     0},
#endif /* EXTENDED_TITLESTYLE */
#endif /* MULTISTYLE */
#ifdef USEDECOR
  {"addtodecor",   add_item_to_decor,F_ADD_DECOR,	    0},
#endif /* USEDECOR */
  {"addtofunc",    add_item_to_func, F_ADDFUNC,             0},
  {"addtomenu",    add_item_to_menu, F_ADDMENU,             0},
  {"all",          AllFunc,          F_ALL,                 0},
  {"animatedmove", animated_move_window,F_ANIMATED_MOVE,    FUNC_NEEDS_WINDOW},
  {"beep",         Bell,             F_BEEP,                0},
#ifdef BORDERSTYLE
  {"borderstyle",  SetBorderStyle,   F_BORDERSTYLE,         0},
#endif /* BORDERSTYLE */
  {"buttonstate",  cmd_button_state, F_BUTTON_STATE,        0},
  {"buttonstyle",  ButtonStyle,      F_BUTTON_STYLE,        0},
#ifdef USEDECOR
  {"changedecor",  ChangeDecor,      F_CHANGE_DECOR,        FUNC_NEEDS_WINDOW},
#endif /* USEDECOR */
  {"changemenustyle", ChangeMenuStyle, F_CHANGE_MENUSTYLE,  0},
  {"clicktime",    SetClick,         F_CLICK,               0},
  {"close",        close_function,   F_CLOSE,               FUNC_NEEDS_WINDOW},
  {"colorlimit",   SetColorLimit,    F_COLOR_LIMIT,         0},
  {"colormapfocus",SetColormapFocus, F_COLORMAP_FOCUS,      0},
  {"current",      CurrentFunc,      F_CURRENT,             0},
  {"cursormove",   movecursor,       F_MOVECURSOR,          0},
  {"cursorstyle",  CursorStyle,      F_CURSOR_STYLE,        0},
  {"defaultbackground",SetDefaultBackground, F_DFLT_BACK,   0},
  {"defaultcolors",SetDefaultColors, F_DFLT_COLORS,         0},
  {"defaultfont",  LoadDefaultFont,  F_DFLT_FONT,           0},
  {"defaulticon",  SetDefaultIcon,   F_DFLT_ICON,           0},
  {"defaultlayers",SetDefaultLayers, F_DFLT_LAYERS,         0},
  {"delete",       delete_function,  F_DELETE,              FUNC_NEEDS_WINDOW},
  {"desk",         changeDesks_func, F_DESK,                0},
  {"desktopsize",  SetDeskSize,      F_SETDESK,             0},
  {"destroy",      destroy_function, F_DESTROY,             FUNC_NEEDS_WINDOW},
#ifdef USEDECOR
  {"destroydecor", DestroyDecor,     F_DESTROY_DECOR,	    0},
#endif /* USEDECOR */
  {"destroyfunc",  destroy_fvwmfunc, F_DESTROY_FUNCTION,    0},
  {"destroymenu",  destroy_menu,     F_DESTROY_MENU,        0},
  {"destroymenustyle", DestroyMenuStyle, F_DESTROY_MENUSTYLE,0},
  {"destroymoduleconfig", DestroyModConfig, F_DESTROY_MOD,  0},
  {"direction",    DirectionFunc,    F_DIRECTION,           0},
  {"echo",         echo_func,        F_ECHO,                0},
  {"edgeresistance",SetEdgeResistance,F_EDGE_RES,           0},
  {"edgescroll",   SetEdgeScroll,    F_EDGE_SCROLL,         0},
  {"edgethickness",setEdgeThickness, F_NOP,                 0},
  {"emulate",      Emulate,          F_EMULATE,             0},
  {"exec",         exec_function,    F_EXEC,                0},
  {"execuseshell", exec_setup,       F_EXEC_SETUP,          0},
  {"flipfocus",    flip_focus_func,  F_FLIP_FOCUS,          FUNC_NEEDS_WINDOW},
  {"focus",        focus_func,       F_FOCUS,               FUNC_NEEDS_WINDOW},
  {"function",     ComplexFunction,  F_FUNCTION,            0},
  {"globalopts",   SetGlobalOptions, F_GLOBAL_OPTS,         0},
#ifdef GNOME
  {"gnomebutton",  GNOME_ButtonFunc, F_MOUSE,               0},
#endif /* GNOME */
  {"gotodesk",     changeDesks_func, F_DESK,                0},
  {"gotodeskandpage",  gotoDeskAndPage_func, F_DESK,        0},
  {"gotopage",     goto_page_func,   F_GOTO_PAGE,           0},
  {"hilightcolor", SetHiColor,       F_HICOLOR,             0},
  {"iconfont",     LoadIconFont,     F_ICONFONT,            0},
  {"iconify",      iconify_function, F_ICONIFY,             FUNC_NEEDS_WINDOW},
  {"iconpath",     iconPath_function,F_ICON_PATH,           0},
  {"ignoremodifiers",ignore_modifiers,F_IGNORE_MODIFIERS,   0},
  {"imagepath",    imagePath_function,F_IMAGE_PATH,         0},
  {"key",          key_binding,      F_KEY,                 0},
  {"killmodule",   module_zapper,    F_KILL_MODULE,         0},
  {"layer",        change_layer,     F_LAYER,               FUNC_NEEDS_WINDOW},
  {"lower",        lower_function,   F_LOWER,               FUNC_NEEDS_WINDOW},
  {"maximize",     Maximize,         F_MAXIMIZE,            FUNC_NEEDS_WINDOW},
  {"maxwindowsize",SetMaxWindowSize, F_MAXWINDOWSIZE,       0},
  {"menu",         staysup_func,     F_STAYSUP,             0},
  {"menustyle",    SetMenuStyle,     F_MENUSTYLE,           0},
  {"module",       executeModule,    F_MODULE,              0},
  {"modulepath",   setModulePath,    F_MODULE_PATH,         0},
  {"mouse",        mouse_binding,    F_MOUSE,               0},
  {"move",         move_window,      F_MOVE,                FUNC_NEEDS_WINDOW},
  {"movethreshold",SetMoveThreshold, F_MOVE_THRESHOLD,      0},
  {"movetodesk",   move_window_to_desk,F_MOVE_TO_DESK,      FUNC_NEEDS_WINDOW},
  {"movetopage",   move_window_to_page,F_MOVE_TO_PAGE,      FUNC_NEEDS_WINDOW},
  {"next",         NextFunc,         F_NEXT,                0},
  {"none",         NoneFunc,         F_NONE,                0},
  {"nop",          Nop_func,         F_NOP,                 FUNC_DONT_REPEAT},
  {"opaquemovesize", SetOpaque,      F_OPAQUE,              0},
  {"pick",         PickFunc,         F_PICK,                0},
  {"piperead",     PipeRead,         F_READ,                0},
  {"pixmappath",   pixmapPath_function,F_PIXMAP_PATH,       0},
  {"placeagain",   PlaceAgain_func,  F_PLACEAGAIN,          FUNC_NEEDS_WINDOW},
  {"popup",        popup_func,       F_POPUP,               0},
  {"prev",         PrevFunc,         F_PREV,                0},
  {"quit",         quit_func,        F_QUIT,                0},
  {"quitscreen",   quit_screen_func, F_QUIT_SCREEN,         0},
  {"raise",        raise_function,   F_RAISE,               FUNC_NEEDS_WINDOW},
  {"raiselower",   raiselower_func,  F_RAISELOWER,          FUNC_NEEDS_WINDOW},
  {"read",         ReadFile,         F_READ,                0},
  {"recapture",    Recapture,        F_RECAPTURE,           0},
  {"recapturewindow",RecaptureWindow,F_RECAPTURE_WINDOW,    0},
  {"refresh",      refresh_function, F_REFRESH,             0},
  {"refreshwindow",refresh_win_function, F_REFRESH,         FUNC_NEEDS_WINDOW},
  {PRE_REPEAT,     repeat_function,  F_REPEAT,              FUNC_DONT_REPEAT},
  {"resize",       resize_window,    F_RESIZE,              FUNC_NEEDS_WINDOW},
  {"restart",      restart_function, F_RESTART,             0},
  {"scroll",       scroll,           F_SCROLL,              0},
  {"send_configinfo",SendDataToModule, F_CONFIG_LIST,       FUNC_DONT_REPEAT},
  {"send_windowlist",send_list_func, F_SEND_WINDOW_LIST,    FUNC_DONT_REPEAT},
  {"sendtomodule", SendStrToModule,  F_SEND_STRING,         FUNC_DONT_REPEAT},
  {"set_mask",     set_mask_function,F_SET_MASK,            FUNC_DONT_REPEAT},
  {"setanimation", set_animation,    F_SET_ANIMATION,	    0},
  {"setenv",       SetEnv,           F_SETENV,              0},
  {PRE_SILENT,     NULL,             F_SILENT,              0},
  {"snapattraction",SetSnapAttraction,F_SNAP_ATT,           0},
  {"snapgrid",     SetSnapGrid,      F_SNAP_GRID,           0},
  {"stick",        stick_function,   F_STICK,               FUNC_NEEDS_WINDOW},
#ifdef HAVE_STROKE
  {"stroke",       stroke_binding,   F_STROKE,              0},
#endif /* HAVE_STROKE */
  {"style",        ProcessNewStyle,  F_STYLE,               0},
  {"title",        Nop_func,         F_TITLE,               0},
  {"titlestyle",   SetTitleStyle,    F_TITLESTYLE,          0},
  {"updatedecor",  UpdateDecor,      F_UPDATE_DECOR,        0},
  {"wait",         wait_func,        F_WAIT,                0},
  {"warptowindow", warp_func,        F_WARP,                FUNC_NEEDS_WINDOW},
  {"windowfont",   LoadWindowFont,   F_WINDOWFONT,          0},
  {"windowid",     WindowIdFunc,     F_WINDOWID,            0},
  {"windowlist",   do_windowList,    F_WINDOWLIST,          0},
/*
  {"windowsdesk",  changeWindowsDesk,F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
*/
  {"windowshade",  WindowShade,      F_WINDOW_SHADE,        FUNC_NEEDS_WINDOW},
  {"windowshadeanimate",setShadeAnim,F_SHADE_ANIMATE,       0},
  {"xorpixmap",    SetXORPixmap,     F_XOR,                 0},
  {"xorvalue",     SetXOR,           F_XOR,                 0},
  {"",0,0,0}
};


/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
  return (strcmp((char *)a, ((struct functions *)b)->keyword));
}

static const struct functions *FindBuiltinFunction(char *func)
{
  struct functions *ret_func;
  char *temp;
  char *s;

  if (!func || func[0] == 0)
    return NULL;

  /* since a lot of lines in a typical rc are probably menu/func continues: */
  if (func[0]=='+' || (func[0] == ' ' && func[1] == '+'))
    return &(func_config[0]);

  if (!(temp = strdup(func)))
    return NULL;
  for (s = temp; *s != 0; s++)
    if (isupper(*s))
      *s = tolower(*s);

  ret_func = (struct functions *)bsearch
    (temp, func_config,
     sizeof(func_config) / sizeof(struct functions) - 1,
     sizeof(struct functions), func_comp);
  free(temp);

  return ret_func;
}


static char *expand(char *input, char *arguments[], FvwmWindow *tmp_win)
{
  int l,i,l2,n,k,j;
  char *out;
  int addto = 0; /*special cas if doing addtofunc */

  l = strlen(input);
  l2 = l;

  if(strncasecmp(input, "AddToFunc", 9) == 0 || input[0] == '+')
  {
    addto = 1;
  }
  i=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  switch (input[i+1])
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	      n = input[i+1] - '0';
	      if(arguments[n] != NULL)
		{
		  l2 += strlen(arguments[n])-2;
		  i++;
		}
	      break;
	    case 'w':
	    case 'd':
	    case 'x':
	    case 'y':
	      l2 += 16;
	      i++;
	      break;
	    }
	}
      i++;
    }

  i=0;
  out = safemalloc(l2+1);
  j=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  switch (input[i+1])
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	      n = input[i+1] - '0';
	      if(arguments[n] != NULL)
		{
		  for(k=0;k<strlen(arguments[n]);k++)
		    out[j++] = arguments[n][k];
		  i++;
		}
            else if (addto == 1)
        {
		out[j++] = '$';
        }
          else
        {
          i++;
          if (isspace(input[i+1]))
            i++; /*eliminates extra white space*/
        }
	      break;
	    case 'w':
	      if(tmp_win)
		sprintf(&out[j],"0x%x",(unsigned int)tmp_win->w);
	      else
		sprintf(&out[j],"$w");
	      j += strlen(&out[j]);
	      i++;
	      break;
	    case 'd':
	      sprintf(&out[j], "%d", Scr.CurrentDesk);
	      j += strlen(&out[j]);
	      i++;
	      break;
	    case 'x':
	      sprintf(&out[j], "%d", Scr.Vx);
	      j += strlen(&out[j]);
	      i++;
	      break;
	    case 'y':
	      sprintf(&out[j], "%d", Scr.Vy);
	      i++;
	      break;
	    case '$':
	      out[j++] = '$';
	      i++;
	      break;
	    default:
	      out[j++] = input[i];
	      break;
	    } /* switch */
	} /* if '$' */
      else
	out[j++] = input[i];
      i++;
    }
  out[j] = 0;
  return out;
}

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
static cfunc_action_type CheckActionType(
  int x, int y, unsigned EndMask, XEvent *d, Bool may_time_out)
{
  int xcurrent,ycurrent,total = 0;
  Time t0;
  int dist;
  Bool is_button_pressed = False;
  extern Time lastTimestamp;

  xcurrent = x;
  ycurrent = y;
  t0 = lastTimestamp;
  dist = Scr.MoveThreshold;

  while ((total < Scr.ClickTime && lastTimestamp - t0 < Scr.ClickTime) ||
	 !may_time_out)
    {
      if (!(x - xcurrent < dist && xcurrent - x < dist &&
	    y - ycurrent < dist && ycurrent - y < dist))
      {
	return CF_MOTION;
      }

      usleep(20000);
      total+=20;
      if(XCheckMaskEvent (dpy,EndMask, d))
	{
	  StashEventTime(d);
	  return CF_CLICK;
	}
      if(XCheckMaskEvent (dpy,ButtonMotionMask|PointerMotionMask, d))
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	  StashEventTime(d);
	}
      else if (may_time_out && XCheckMaskEvent (dpy,ButtonPressMask, d))
	{
	  is_button_pressed = True;
	}
    }

  return (is_button_pressed) ? CF_HOLD : CF_TIMEOUT;
}


/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a fvwm built in function
 *
 *  Inputs:
 *	Action	- the action to execute
 *	tmp_win	- the fvwm window structure
 *	eventp	- pointer to the event that caused the function
 *	context - the context in which the button was pressed
 *
 ***********************************************************************/
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module,
		     expand_command_type expand_cmd)
{
  static unsigned int func_depth = 0;
  Window w;
  int j;
  char *function;
  char *action;
  char *taction;
  char *trash;
  char *trash2;
  char *expaction = NULL;
  char *arguments[10];
  const struct functions *bif;
  Bool set_silent;
  Bool must_free_string = False;
  int skip;

  if (!Action || Action[0] == 0 || Action[1] == 0)
  {
    /* impossibly short command */
    return;                             /* done */
  }
  if (Action[0] == '#') {               /* a comment */
    return;                             /* done */
  }
  /* Note: the module config command, "*" can not be handled by the
     regular command table because there is no required white space after
     the asterisk. */
  if (Action[0] == '*') {               /* a module config command */
    ModuleConfig(NULL,0,0,0,Action,0);  /* process the command */
    return;                             /* done */
  }
  func_depth++;
  for(j=0;j<10;j++)
    arguments[j] = NULL;

  if(tmp_win == NULL)
    w = Scr.Root;
  else
    w = tmp_win->w;

  if((tmp_win) &&(eventp))
    w = eventp->xany.window;
  if((tmp_win)&&(eventp->xbutton.subwindow != None)&&
     (eventp->xany.window != tmp_win->w))
    w = eventp->xbutton.subwindow;

  set_silent = False;
  if (Action[0] == '-')
    {
      expand_cmd = DONT_EXPAND_COMMAND;
      Action++;
    }

  taction = Action;
  /* parse prefixes */
  trash = PeekToken(taction, &trash2);
  while (trash)
    {
      if (StrEquals(trash, PRE_SILENT))
	{
	  if (Scr.flags.silent_functions == 0)
	    {
	      set_silent = 1;
	      Scr.flags.silent_functions = 1;
	    }
	  taction = trash2;
	  trash = PeekToken(taction, &trash2);
	}
      else
	break;
    }
  if (taction == NULL)
    {
      if (set_silent)
	Scr.flags.silent_functions = 0;
      func_depth--;
      return;
    }
  skip = taction - Action;

  if (expand_cmd == EXPAND_COMMAND)
    expaction = expand(Action, arguments, tmp_win);
  else
    expaction = Action;
  taction = expaction + skip;
  j=0;

  action = GetNextToken(taction,&function);
  bif = FindBuiltinFunction(function);
  if (expand_cmd == EXPAND_COMMAND && func_depth <= 1)
  {
    must_free_string = set_repeat_data(expaction, REPEAT_COMMAND, bif);
  }
  if (bif)
    {
      bif->action(eventp,w,tmp_win,context,action,&Module);
    }
  else
    {
      Bool desperate = 1;

      execute_complex_function(eventp,w,tmp_win,context,taction, &Module,
			       &desperate, DONT_EXPAND_COMMAND);
      if(desperate)
	  executeModule(eventp,w,tmp_win,context,taction, &Module);
    }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions, menus or modules. */
  if(Module == -1)
    WaitForButtonsUp();

  free(function);
  if (set_silent)
    Scr.flags.silent_functions = 0;

  if (must_free_string)
  {
    free(expaction);
  }

  func_depth--;
  return;
}


void ExecuteFunctionSaveTmpWin(char *Action, FvwmWindow *tmp_win,
			       XEvent *eventp, unsigned long context,
			       int Module, expand_command_type expand_cmd)
{
  FvwmWindow *s_Tmp_win = Tmp_win;

  ExecuteFunction(Action, tmp_win, eventp, context, Module, expand_cmd);
  Tmp_win = s_Tmp_win;
}


/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to FvwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(XEvent *eventp, Window *w,FvwmWindow **tmp_win,
		   unsigned long *context, cursor_type cursor,
		   int FinishEvent)
{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT)&&(*tmp_win != NULL))
  {
    if((FinishEvent == ButtonPress)||((FinishEvent == ButtonRelease) &&
                                      (eventp->type != ButtonPress)))
    {
      return FALSE;
    }
    else if (FinishEvent == ButtonRelease)
    {
      /* We are only waiting until the user releases the button. Do not change
       * the cursor. */
      cursor = CRS_NONE;
    }
  }
  if (Scr.flags.silent_functions)
  {
    return True;
  }
  if(!GrabEm(cursor))
  {
    XBell(dpy, 0);
    return True;
  }

  while (!finished)
  {
    done = 0;
    /* block until there is an event */
    XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
               ExposureMask |KeyPressMask | VisibilityChangeMask |
               ButtonMotionMask| PointerMotionMask
	       /* | EnterWindowMask | LeaveWindowMask*/, eventp);
    StashEventTime(eventp);

    if(eventp->type == KeyPress)
    {
      KeySym keysym = XLookupKeysym(&eventp->xkey,0);
      if (keysym == XK_Escape)
      {
	UngrabEm();
	return True;
      }
      Keyboard_shortcuts(eventp, NULL, FinishEvent);
    }
    if(eventp->type == FinishEvent)
    {
      finished = 1;
    }
    switch (eventp->type)
    {
    case ButtonPress:
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      /* fall through */
    case KeyPress:
      if (eventp->type != FinishEvent)
	original_w = eventp->xany.window;
      /* fall through */
    case ButtonRelease:
      done = 1;
      break;
    default:
      break;
    }

    if(!done)
    {
      DispatchEvent(False);
    }

  }

  UngrabEm();
  *w = eventp->xany.window;
  if(((*w == Scr.Root)||(*w == Scr.NoFocusWin))
     && (eventp->xbutton.subwindow != (Window)0))
  {
    *w = eventp->xbutton.subwindow;
    eventp->xany.window = *w;
  }
  if (*w == Scr.Root)
  {
    *context = C_ROOT;
    XBell(dpy, 0);
    return TRUE;
  }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
  {
    *tmp_win = NULL;
    XBell(dpy, 0);
    return (TRUE);
  }

  if(*w == (*tmp_win)->Parent)
  {
    *w = (*tmp_win)->w;
  }

  if(original_w == (*tmp_win)->Parent)
  {
    original_w = (*tmp_win)->w;
  }

  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  if((*w != original_w)&&(original_w != Scr.Root)&&
     (original_w != None)&&(original_w != Scr.NoFocusWin))
  {
    if(!((*w == (*tmp_win)->frame)&&
         (original_w == (*tmp_win)->w)))
    {
      *context = C_ROOT;
      XBell(dpy, 0);
      return TRUE;
    }
  }
  *context = GetContext(*tmp_win,eventp,&dummy);

  return FALSE;
}


void find_func_type(char *action, short *func_type, unsigned char *flags)
{
  int j, len = 0;
  char *endtok = action;
  Bool matched;
  int mlen;

  if (action)
  {
    while (*endtok&&!isspace((unsigned char)*endtok))++endtok;
    len = endtok - action;
    j=0;
    matched = FALSE;
    while((!matched)&&((mlen = strlen(func_config[j].keyword))>0))
      {
	if((mlen == len) &&
	   (strncasecmp(action,func_config[j].keyword,mlen)==0))
	  {
	    matched=TRUE;
	    /* found key word */
	    if (func_type)
	      *func_type = func_config[j].func_type;
	    if (flags)
	      *flags = func_config[j].flags;
	    return;
	  }
	else
	  j++;
      }
    /* No clue what the function is. Just return "BEEP" */
  }
  if (func_type)
    *func_type = F_BEEP;
  if (flags)
    *flags = 0;
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	AddToFunction - add an item to a FvwmFunction
 *
 *  Inputs:
 *	func      - pointer to the FvwmFunction to add the item
 *	action    - the definition string from the config line
 *
 ***********************************************************************/
void AddToFunction(FvwmFunction *func, char *action)
{
  FunctionItem *tmp;
  char *token = NULL;
  char condition;

  action = GetNextToken(action, &token);
  if (!token)
    return;
  condition = token[0];
  if (isupper(condition))
    condition = tolower(condition);
  free(token);
  if (!action)
    return;
  while (isspace(*action))
    action++;
  if (*action == 0)
    return;

  tmp = (FunctionItem *)safemalloc(sizeof(FunctionItem));
  tmp->next_item = NULL;
  tmp->func = func;
  if (func->first_item == NULL)
    {
      func->first_item = tmp;
      func->last_item = tmp;
    }
  else
    {
      func->last_item->next_item = tmp;
      func->last_item = tmp;
    }

  tmp->condition = condition;
  tmp->action = stripcpy(action);

  find_func_type(tmp->action, NULL, &(tmp->flags));
}

/***********************************************************************
 *
 *  Procedure:
 *	NewFunction - create a new FvwmFunction
 *
 *  Returned Value:
 *	(FvwmFunction *)
 *
 *  Inputs:
 *	name	- the name of the function
 *
 ***********************************************************************/
FvwmFunction *NewFvwmFunction(const char *name)
{
  FvwmFunction *tmp;

  tmp = (FvwmFunction *)safemalloc(sizeof(FvwmFunction));

  tmp->next_func = Scr.functions;
  tmp->first_item = NULL;
  tmp->last_item = NULL;
  tmp->name = stripcpy(name);
  tmp->use_depth = 0;

  Scr.functions = tmp;
  return (tmp);
}


void DestroyFunction(FvwmFunction *func)
{
  FunctionItem *fi,*tmp2;
  FvwmFunction *tmp, *prev;

  if(func == NULL)
    return;

  tmp = Scr.functions;
  prev = NULL;
  while((tmp != NULL)&&(tmp != func))
    {
      prev = tmp;
      tmp = tmp->next_func;
    }
  if(tmp != func)
    return;

  if (func->use_depth != 0)
    {
      fvwm_msg(ERR,"DestroyFunction", "Function %s is in use (depth %d)",
	       func->name, func->use_depth);
      return;
    }

  if(prev == NULL)
    Scr.functions = func->next_func;
  else
    prev->next_func = func->next_func;

  free(func->name);

  fi = func->first_item;
  while(fi != NULL)
    {
      tmp2 = fi->next_item;
      if (fi->action != NULL)
	free(fi->action);
      free(fi);
      fi = tmp2;
    }
  free(func);
}


/* FindFunction expects a token as the input. Make sure you have used
 * GetNextToken before passing a function name to remove quotes */
FvwmFunction *FindFunction(const char *function_name)
{
  FvwmFunction *func;

  if(function_name == NULL || *function_name == 0)
    return NULL;

  func = Scr.functions;
  while(func != NULL)
  {
    if(func->name != NULL)
      if(strcasecmp(function_name, func->name) == 0)
      {
        return func;
      }
    func = func->next_func;
  }
  return NULL;

}


static void ComplexFunction(F_CMD_ARGS)
{
  Bool desperate = 0;

  execute_complex_function(eventp, w, tmp_win, context, action, Module,
			   &desperate, EXPAND_COMMAND);
}

static void execute_complex_function(F_CMD_ARGS, Bool *desperate,
				     expand_command_type expand_cmd)
{
  static unsigned int cfunc_depth = 0;
  cfunc_action_type type = CF_MOTION;
  char c;
  FunctionItem *fi;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
  Bool HaveHold = False;
  Bool NeedsTarget = False;
  char *arguments[10], *taction;
  char* func_name;
  int x, y ,i;
  XEvent d, *ev;
  FvwmFunction *func;

  /* FindFunction expects a token, not just a quoted string */
  taction = GetNextToken(action,&func_name);
  if (!action || !func_name)
    return;
  func = FindFunction(func_name);
  free(func_name);
  if(func == NULL)
    {
      if(*desperate == 0)
	fvwm_msg(ERR,"ComplexFunction","No such function %s",action);
      return;
    }
  *desperate = 0;
  /* Get the argument list */
  for(i=0;i<10;i++)
    taction = GetNextToken(taction,&arguments[i]);
  /* see functions.c to find out which functions need a window to operate on */
  ev = eventp;
  /* In case we want to perform an action on a button press, we
   * need to fool other routines */
  if(eventp->type == ButtonPress)
    eventp->type = ButtonRelease;
  func->use_depth++;
  cfunc_depth++;
  fi = func->first_item;
  while(fi != NULL)
    {
      /* c is already lowercase here */
      c = fi->condition;
      NeedsTarget = (fi->flags & FUNC_NEEDS_WINDOW) ? True : False;
      switch (c)
      {
      case CF_IMMEDIATE:
	if(tmp_win)
	  w = tmp_win->frame;
	else
	  w = None;
	taction = expand(fi->action,arguments,tmp_win);
	ExecuteFunction(taction,tmp_win,eventp,context,-2,EXPAND_COMMAND);
	free(taction);
	break;
      case CF_DOUBLE_CLICK:
	HaveDoubleClick = True;
	Persist = True;
	break;
      case CF_HOLD:
	HaveHold = True;
	Persist = True;
	break;
      default:
	Persist = True;
	break;
      }
      fi = fi->next_item;
    }

  if(!Persist)
    {
      func->use_depth--;
      cfunc_depth--;
      for(i=0;i<10;i++)
	if(arguments[i] != NULL)
	  free(arguments[i]);
      return;
    }

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if(NeedsTarget)
    {
      if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonPress))
	{
	  func->use_depth--;
	  cfunc_depth--;
	  WaitForButtonsUp();
	  for(i=0;i<10;i++)
	    if(arguments[i] != NULL)
	      free(arguments[i]);
	  return;
	}
    }

  if(!GrabEm(CRS_NONE))
    {
      func->use_depth--;
      cfunc_depth--;
      XBell(dpy, 0);
      for(i=0;i<10;i++)
	if(arguments[i] != NULL)
	  free(arguments[i]);
      return;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x,&y,&JunkX, &JunkY, &JunkMask);
  /* Take the click which started this fuction off the
   * Event queue.  -DDN- Dan D Niles dniles@iname.com */
  XCheckMaskEvent(dpy, ButtonPressMask, &d);

  /* Wait and see if we have a click, or a move */
  /* wait forever, see if the user releases the button */
  type = CheckActionType(x, y, ButtonReleaseMask, &d, HaveHold);
  if (type == CF_CLICK)
    {
      ev = &d;
      /* If it was a click, wait to see if its a double click */
      if(HaveDoubleClick)
	{
	  type = CheckActionType(x,y,ButtonReleaseMask,&d, True);
	  switch (type)
	  {
	  case CF_CLICK:
	  case CF_HOLD:
	    type = CF_DOUBLE_CLICK;
	    ev = &d;
	    break;
	  case CF_MOTION:
	    ev = &d;
	  case CF_TIMEOUT:
	    type = CF_CLICK;
	    break;
	  }
	}
    }
  else if (type == CF_TIMEOUT)
    {
      type = CF_HOLD;
    }

  /* some functions operate on button release instead of
   * presses. These gets really weird for complex functions ... */
  if(ev->type == ButtonPress)
    ev->type = ButtonRelease;

  fi = func->first_item;
  while(fi != NULL)
    {
      /* make lower case */
      c = fi->condition;
      if(isupper(c))
	c=tolower(c);
      if(c == type)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  taction = expand(fi->action,arguments,tmp_win);
	  ExecuteFunction(taction,tmp_win,ev,context,-2,expand_cmd);
	  free(taction);
	}
      fi = fi->next_item;
    }
  WaitForButtonsUp();
  UngrabEm();
  for(i=0;i<10;i++)
    if(arguments[i] != NULL)
      free(arguments[i]);
  func->use_depth--;
  cfunc_depth--;
}
