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

#include "fvwm.h"
#include "functions.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

static char *expand(char *input, char *arguments[],FvwmWindow *tmp_win);
static Bool IsClick(int x,int y,unsigned EndMask, XEvent *d,Bool may_time_out);

extern XEvent Event;
extern FvwmWindow *Tmp_win;

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

/* The function names in the first field *must* be in uppercase or else the
 * function cannot be called. */
static struct functions func_config[] =
{
  {"+",            add_another_item, F_ADDMENU2,            FUNC_NO_WINDOW},
#ifdef MULTISTYLE
  {"ADDBUTTONSTYLE",AddButtonStyle,  F_ADD_BUTTON_STYLE,    FUNC_NO_WINDOW},
#endif /* MULTISTYLE */
  {"ADDMODULECONFIG",  AddModConfig, F_ADD_MOD,             FUNC_NO_WINDOW},
#ifdef MULTISTYLE
#ifdef EXTENDED_TITLESTYLE
  {"ADDTITLESTYLE",AddTitleStyle,    F_ADD_TITLE_STYLE,     FUNC_NO_WINDOW},
#endif /* EXTENDED_TITLESTYLE */
#endif /* MULTISTYLE */
#ifdef USEDECOR
  {"ADDTODECOR",   add_item_to_decor,F_ADD_DECOR,	    FUNC_NO_WINDOW},
#endif /* USEDECOR */
  {"ADDTOFUNC",    add_item_to_func, F_ADDFUNC,             FUNC_NO_WINDOW},
  {"ADDTOMENU",    add_item_to_menu, F_ADDMENU,             FUNC_NO_WINDOW},
  {"ALL",          AllFunc,          F_ALL,                 FUNC_NO_WINDOW},
  {"ANIMATEDMOVE", animated_move_window,F_ANIMATED_MOVE,    FUNC_NEEDS_WINDOW},
  {"BEEP",         Bell,             F_BEEP,                FUNC_NO_WINDOW},
#ifdef BORDERSTYLE
  {"BORDERSTYLE",  SetBorderStyle,   F_BORDERSTYLE,         FUNC_NO_WINDOW},
#endif /* BORDERSTYLE */
  {"BUTTONSTYLE",  ButtonStyle,      F_BUTTON_STYLE,        FUNC_NO_WINDOW},
#ifdef USEDECOR
  {"CHANGEDECOR",  ChangeDecor,      F_CHANGE_DECOR,        FUNC_NEEDS_WINDOW},
#endif /* USEDECOR */
  {"CHANGEMENUSTYLE", ChangeMenuStyle, F_CHANGE_MENUSTYLE,  FUNC_NO_WINDOW},
  {"CLICKTIME",    SetClick,         F_CLICK,               FUNC_NO_WINDOW},
  {"CLOSE",        close_function,   F_CLOSE,               FUNC_NEEDS_WINDOW},
  {"COLORLIMIT",   SetColorLimit,    F_COLOR_LIMIT,         FUNC_NO_WINDOW},
  {"COLORMAPFOCUS",SetColormapFocus, F_COLORMAP_FOCUS,      FUNC_NO_WINDOW},
  {"CURRENT",      CurrentFunc,      F_CURRENT,             FUNC_NO_WINDOW},
  {"CURSORMOVE",   movecursor,       F_MOVECURSOR,          FUNC_NO_WINDOW},
  {"CURSORSTYLE",  CursorStyle,      F_CURSOR_STYLE,        FUNC_NO_WINDOW},
  {"DEFAULTCOLORS",SetDefaultColors, F_DFLT_COLORS,         FUNC_NO_WINDOW},
  {"DEFAULTFONT",  LoadDefaultFont,  F_DFLT_FONT,           FUNC_NO_WINDOW},
  {"DEFAULTLAYERS",SetDefaultLayers, F_DFLT_LAYERS,         FUNC_NO_WINDOW},
  {"DELETE",       delete_function,  F_DELETE,              FUNC_NEEDS_WINDOW},
  {"DESK",         changeDesks_func, F_DESK,                FUNC_NO_WINDOW},
  {"DESKTOPSIZE",  SetDeskSize,      F_SETDESK,             FUNC_NO_WINDOW},
  {"DESTROY",      destroy_function, F_DESTROY,             FUNC_NEEDS_WINDOW},
#ifdef USEDECOR
  {"DESTROYDECOR", DestroyDecor,     F_DESTROY_DECOR,	    FUNC_NO_WINDOW},
#endif /* USEDECOR */
  {"DESTROYFUNC",  destroy_fvwmfunc, F_DESTROY_FUNCTION,    FUNC_NO_WINDOW},
  {"DESTROYMENU",  destroy_menu,     F_DESTROY_MENU,        FUNC_NO_WINDOW},
  {"DESTROYMENUSTYLE", DestroyMenuStyle, F_DESTROY_MENUSTYLE,FUNC_NO_WINDOW},
  {"DESTROYMODULECONFIG", DestroyModConfig, F_DESTROY_MOD,  FUNC_NO_WINDOW},
  {"DIRECTION",    DirectionFunc,    F_DIRECTION,           FUNC_NO_WINDOW},
  {"ECHO",         echo_func,        F_ECHO,                FUNC_NO_WINDOW},
  {"EDGERESISTANCE",SetEdgeResistance,F_EDGE_RES,           FUNC_NO_WINDOW},
  {"EDGESCROLL",   SetEdgeScroll,    F_EDGE_SCROLL,         FUNC_NO_WINDOW},
  {"EDGETHICKNESS",setEdgeThickness, F_NOP,                 FUNC_NO_WINDOW},
  {"EMULATE",      Emulate,          F_EMULATE,             FUNC_NO_WINDOW},
  {"EXEC",         exec_function,    F_EXEC,                FUNC_NO_WINDOW},
  {"EXECUSESHELL", exec_setup,       F_EXEC_SETUP,          FUNC_NO_WINDOW},
  {"FLIPFOCUS",    flip_focus_func,  F_FLIP_FOCUS,          FUNC_NEEDS_WINDOW},
  {"FOCUS",        focus_func,       F_FOCUS,               FUNC_NEEDS_WINDOW},
  {"FUNCTION",     ComplexFunction,  F_FUNCTION,            FUNC_NO_WINDOW},
  {"GLOBALOPTS",   SetGlobalOptions, F_GLOBAL_OPTS,         FUNC_NO_WINDOW},
  {"GOTOPAGE",     goto_page_func,   F_GOTO_PAGE,           FUNC_NO_WINDOW},
  {"HILIGHTCOLOR", SetHiColor,       F_HICOLOR,             FUNC_NO_WINDOW},
  {"ICONFONT",     LoadIconFont,     F_ICONFONT,            FUNC_NO_WINDOW},
  {"ICONIFY",      iconify_function, F_ICONIFY,             FUNC_NEEDS_WINDOW},
  {"ICONPATH",     iconPath_function,F_ICON_PATH,           FUNC_NO_WINDOW},
  {"IGNOREMODIFIERS",ignore_modifiers,F_IGNORE_MODIFIERS,   FUNC_NO_WINDOW},
  {"IMAGEPATH",    imagePath_function,F_IMAGE_PATH,         FUNC_NO_WINDOW},
  {"KEY",          ParseKeyEntry,    F_KEY,                 FUNC_NO_WINDOW},
  {"KILLMODULE",   module_zapper,    F_ZAP,                 FUNC_NO_WINDOW},
  {"LAYER",        change_layer,     F_LAYER,               FUNC_NEEDS_WINDOW},
  {"LOWER",        lower_function,   F_LOWER,               FUNC_NEEDS_WINDOW},
  {"MAXIMIZE",     Maximize,         F_MAXIMIZE,            FUNC_NEEDS_WINDOW},
  {"MENU",         staysup_func,     F_STAYSUP,             FUNC_NO_WINDOW},
  {"MENUSTYLE",    SetMenuStyle,     F_MENUSTYLE,           FUNC_NO_WINDOW},
  {"MODULE",       executeModule,    F_MODULE,              FUNC_NO_WINDOW},
  {"MODULEPATH",   setModulePath,    F_MODULE_PATH,         FUNC_NO_WINDOW},
  {"MOUSE",        ParseMouseEntry,  F_MOUSE,               FUNC_NO_WINDOW},
  {"MOVE",         move_window,      F_MOVE,                FUNC_NEEDS_WINDOW},
  {"MOVETODESK",   changeWindowsDesk,F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
  {"MOVETOPAGE",   move_window_to_page,F_MOVE_TO_PAGE,      FUNC_NEEDS_WINDOW},
  {"NEXT",         NextFunc,         F_NEXT,                FUNC_NO_WINDOW},
  {"NONE",         NoneFunc,         F_NONE,                FUNC_NO_WINDOW},
  {"NOP",          Nop_func,         F_NOP,                 FUNC_NO_WINDOW},
  {"OPAQUEMOVESIZE", SetOpaque,      F_OPAQUE,              FUNC_NO_WINDOW},
  {"PICK",         PickFunc,         F_PICK,                FUNC_NO_WINDOW},
  {"PIPEREAD",     PipeRead,         F_READ,                FUNC_NO_WINDOW},
  {"PIXMAPPATH",   pixmapPath_function,F_PIXMAP_PATH,       FUNC_NO_WINDOW},
  {"PLACEAGAIN",   PlaceAgain_func,  F_PLACEAGAIN,          FUNC_NEEDS_WINDOW},
  {"POPUP",        popup_func,       F_POPUP,               FUNC_NO_WINDOW},
  {"PREV",         PrevFunc,         F_PREV,                FUNC_NO_WINDOW},
  {"QUIT",         quit_func,        F_QUIT,                FUNC_NO_WINDOW},
  {"QUITSCREEN",   quit_screen_func, F_QUIT_SCREEN,         FUNC_NO_WINDOW},
  {"RAISE",        raise_function,   F_RAISE,               FUNC_NEEDS_WINDOW},
  {"RAISELOWER",   raiselower_func,  F_RAISELOWER,          FUNC_NEEDS_WINDOW},
  {"READ",         ReadFile,         F_READ,                FUNC_NO_WINDOW},
  {"RECAPTURE",    Recapture,        F_RECAPTURE,           FUNC_NO_WINDOW},
  {"REFRESH",      refresh_function, F_REFRESH,             FUNC_NO_WINDOW},
  {"REFRESHWINDOW",refresh_win_function, F_REFRESH,         FUNC_NEEDS_WINDOW},
  {"RESIZE",       resize_window,    F_RESIZE,              FUNC_NEEDS_WINDOW},
  {"RESTART",      restart_function, F_RESTART,             FUNC_NO_WINDOW},
  {"SCROLL",       scroll,           F_SCROLL,              FUNC_NO_WINDOW},
  {"SENDTOMODULE", SendStrToModule,  F_SEND_STRING,         FUNC_NO_WINDOW},
  {"SEND_CONFIGINFO",SendDataToModule, F_CONFIG_LIST,       FUNC_NO_WINDOW},
  {"SEND_WINDOWLIST",send_list_func, F_SEND_WINDOW_LIST,    FUNC_NO_WINDOW},
  {"SETANIMATION", set_animation,    F_SET_ANIMATION,	    FUNC_NO_WINDOW},
  {"SETENV",       SetEnv,           F_SETENV,              FUNC_NO_WINDOW},
  {"SET_MASK",     set_mask_function,F_SET_MASK,            FUNC_NO_WINDOW},
  {"SILENT",       NULL,             F_SILENT,              FUNC_NO_WINDOW},
  {"SNAPATTRACTION",SetSnapAttraction,F_SNAP_ATT,           FUNC_NO_WINDOW},
  {"SNAPGRID",     SetSnapGrid,      F_SNAP_GRID,           FUNC_NO_WINDOW},
  {"STICK",        stick_function,   F_STICK,               FUNC_NEEDS_WINDOW},
  {"STYLE",        ProcessNewStyle,  F_STYLE,               FUNC_NO_WINDOW},
  {"TITLE",        Nop_func,         F_TITLE,               FUNC_NO_WINDOW},
  {"TITLESTYLE",   SetTitleStyle,    F_TITLESTYLE,          FUNC_NO_WINDOW},
  {"UPDATEDECOR",  UpdateDecor,      F_UPDATE_DECOR,        FUNC_NO_WINDOW},
  {"WAIT",         wait_func,        F_WAIT,                FUNC_NO_WINDOW},
  {"WARPTOWINDOW", warp_func,        F_WARP,                FUNC_NEEDS_WINDOW},
  {"WINDOWFONT",   LoadWindowFont,   F_WINDOWFONT,          FUNC_NO_WINDOW},
  {"WINDOWID",     WindowIdFunc,     F_WINDOWID,            FUNC_NO_WINDOW},
  {"WINDOWLIST",   do_windowList,    F_WINDOWLIST,          FUNC_NO_WINDOW},
/*
  {"WINDOWSDESK",  changeWindowsDesk,F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
*/
#ifdef WINDOWSHADE
  {"WINDOWSHADE",  WindowShade,      F_WINDOW_SHADE,        FUNC_NEEDS_WINDOW},
  {"WINDOWSHADEANIMATE",setShadeAnim,F_SHADE_ANIMATE,       FUNC_NO_WINDOW},
#endif /* WINDOWSHADE */
  {"XORPIXMAP",    SetXORPixmap,     F_XOR,                 FUNC_NO_WINDOW},
  {"XORVALUE",     SetXOR,           F_XOR,                 FUNC_NO_WINDOW},
  {"",0,0,0}
};


/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
  char *key=(char *)a;
  char *f=((struct functions *)b)->keyword;
  return (strcmp(key,f));
}

static struct functions *FindBuiltinFunction(char *func)
{
  static int func_config_size=0;
  struct functions *ret_func;
  char *temp;
  char *s;

  if (!func || func[0] == 0)
    return NULL;

  /* since a lot of lines in a typical rc are probably menu/func continues: */
  if (func[0]=='+')
    return &(func_config[0]);

  if (!func_config_size)
  {
    /* remove finial NULL entry from size */
    func_config_size=((sizeof(func_config))/(sizeof(struct functions)))-1;
  }

  if (!(temp = strdup(func)))
    return NULL;
  for (s = temp; *s != 0; s++)
    if (islower(*s))
      *s = toupper(*s);

  ret_func = (struct functions *)bsearch(temp, func_config, func_config_size,
					 sizeof(struct functions), func_comp);
  free(temp);

  return ret_func;
}


/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a fvwm built in function
 *
 *  Inputs:
 *	Action	- the menu action to execute
 *	tmp_win	- the fvwm window structure
 *	eventp	- pointer to the event that caused the function
 *	context - the context in which the button was pressed
 *
 ***********************************************************************/
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module)
{
  Window w;
  int matched,j;
  char *function;
  char *action, *taction, *expaction;
  char *arguments[10];
  struct functions *bif;
  int func_type;
  Bool set_silent;

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

  taction = expaction = expand(Action,arguments,tmp_win);
  j=0;
  matched = FALSE;
  func_type = F_NOP;
  set_silent = 0;

  do
  {
    action = GetNextToken(taction,&function);
    if (!function)
      {
	if (set_silent)
	  Scr.flags.silent_functions = 0;
	return;
      }
    bif = FindBuiltinFunction(function);
    if (bif)
      {
	func_type = bif->func_type;
	if (func_type == F_SILENT)
	  {
	    taction = action;
	    free(function);
	    if (Scr.flags.silent_functions == 0)
	      {
		set_silent = 1;
		Scr.flags.silent_functions = 1;
	      }
	  }
	else
	  {
	    matched = TRUE;
	    bif->action(eventp,w,tmp_win,context,action,&Module);
	  }
      }
  } while (func_type == F_SILENT);

  if(!matched)
    {
      Bool desperate = 1;

      ComplexFunction2(eventp,w,tmp_win,context,taction, &Module, &desperate);
      if(desperate)
	executeModule(eventp,w,tmp_win,context,taction, &Module);
    }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions or modules. */
  if(Module == -1)
    WaitForButtonsUp();

  if (function)
    free(function);
  if(expaction != NULL)
    free(expaction);
  if (set_silent)
    Scr.flags.silent_functions = 0;

  return;
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
		   unsigned long *context, int cursor, int FinishEvent)

{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT)&&(tmp_win != NULL))
  {
    if((FinishEvent == ButtonPress)||((FinishEvent == ButtonRelease) &&
                                      (eventp->type != ButtonPress)))
    {
      return FALSE;
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
      Keyboard_shortcuts(eventp, NULL, FinishEvent);
    if(eventp->type == FinishEvent)
      finished = 1;
    if(eventp->type == ButtonPress)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      done = 1;
    }
    if(eventp->type == ButtonRelease)
      done = 1;
    if(eventp->type == KeyPress)
      done = 1;

    if(!done)
    {
      DispatchEvent();
    }

  }


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
    UngrabEm();
    return TRUE;
  }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
  {
    *tmp_win = NULL;
    XBell(dpy, 0);
    UngrabEm();
    return (TRUE);
  }

  if(*w == (*tmp_win)->Parent)
    *w = (*tmp_win)->w;

  if(original_w == (*tmp_win)->Parent)
    original_w = (*tmp_win)->w;

  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  if((*w != original_w)&&(original_w != Scr.Root)&&
     (original_w != None)&&(original_w != Scr.NoFocusWin))
    if(!((*w == (*tmp_win)->frame)&&
         (original_w == (*tmp_win)->w)))
    {
      *context = C_ROOT;
      XBell(dpy, 0);
      UngrabEm();
      return TRUE;
    }

  *context = GetContext(*tmp_win,eventp,&dummy);

  UngrabEm();
  return FALSE;
}


void find_func_type(char *action, short *func_type, Bool *func_needs_window)
{
  int j, len = 0;
  char *endtok = action;
  Bool matched;
  int mlen;

  if (action)
  {
    while (*endtok&&!isspace(*endtok))++endtok;
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
	    if (func_needs_window)
	      *func_needs_window = func_config[j].func_needs_window;
	    return;
	  }
	else
	  j++;
      }
    /* No clue what the function is. Just return "BEEP" */
  }
  if (func_type)
    *func_type = F_BEEP;
  if (func_needs_window)
    *func_needs_window = False;
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

  find_func_type(tmp->action, NULL, &(tmp->f_needs_window));
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
FvwmFunction *NewFvwmFunction(char *name)
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
FvwmFunction *FindFunction(char *function_name)
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


/*****************************************************************************
 *
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
static Bool IsClick(int x,int y,unsigned EndMask, XEvent *d, Bool may_time_out)
{
  int xcurrent,ycurrent,total = 0;
  Time t0;
  extern Time lastTimestamp;

  xcurrent = x;
  ycurrent = y;
  t0 = lastTimestamp;

  while(((total < Scr.ClickTime && lastTimestamp - t0 < Scr.ClickTime) ||
	 !may_time_out) &&
	x - xcurrent < 3 && x - xcurrent > -3 &&
	y - ycurrent < 3 && y - ycurrent > -3)
    {
      usleep(20000);
      total+=20;
      if(XCheckMaskEvent (dpy,EndMask, d))
	{
	  StashEventTime(d);
	    return True;
	}
      if(XCheckMaskEvent (dpy,ButtonMotionMask|PointerMotionMask, d))
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	  StashEventTime(d);
	}
    }
  return False;
}

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 *
 ****************************************************************************/
void ComplexFunction(F_CMD_ARGS)
{
  Bool desperate = 0;

  ComplexFunction2(eventp, w, tmp_win, context, action, Module, &desperate);
}

void ComplexFunction2(F_CMD_ARGS, Bool *desperate)
{
  char type = MOTION;
  char c;
  FunctionItem *fi;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
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
  fi = func->first_item;
  while(fi != NULL)
    {
      /* c is already lowercase here */
      c = fi->condition;
      NeedsTarget = fi->f_needs_window;
      if(c==DOUBLE_CLICK)
	{
	  HaveDoubleClick = True;
	  Persist = True;
	}
      else if(c == IMMEDIATE)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  taction = expand(fi->action,arguments,tmp_win);
	  ExecuteFunction(taction,tmp_win,eventp,context,-2);
	  free(taction);
	}
      else
	Persist = True;
      fi = fi->next_item;
    }

  if(!Persist)
    {
      func->use_depth--;
      for(i=0;i<10;i++)
	  if(arguments[i] != NULL)free(arguments[i]);
      return;
    }

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if(NeedsTarget)
    {
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonPress))
	{
	  WaitForButtonsUp();
	  for(i=0;i<10;i++)
	    if(arguments[i] != NULL)free(arguments[i]);
	  return;
	}
    }

  if(!GrabEm(SELECT))
    {
      XBell(dpy, 0);
      for(i=0;i<10;i++)
	  if(arguments[i] != NULL)free(arguments[i]);
      return;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x,&y,&JunkX, &JunkY, &JunkMask);
  /* Take the click which started this fuction off the
   * Event queue.  -DDN- Dan D Niles dniles@iname.com */
  XCheckMaskEvent(dpy, ButtonPressMask, &d);

  /* Wait and see if we have a click, or a move */
  /* wait for0ever, see if the user releases the button */
  if(IsClick(x,y,ButtonReleaseMask,&d,False))
    {
      ev = &d;
      type = CLICK;
    }

  /* If it was a click, wait to see if its a double click */
  if((HaveDoubleClick) && (type == CLICK) &&
     (IsClick(x,y,ButtonPressMask, &d, True)))
    {
      type = DOUBLE_CLICK;
      ev = &d;
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
	  ExecuteFunction(taction,tmp_win,ev,context,-2);
	  free(taction);
	}
      fi = fi->next_item;
    }
  WaitForButtonsUp();
  UngrabEm();
  for(i=0;i<10;i++)
    if(arguments[i] != NULL)free(arguments[i]);
  func->use_depth--;
}


static char *expand(char *input, char *arguments[],FvwmWindow *tmp_win)
{
  int l,i,l2,n,k,j;
  char *out;

  l = strlen(input);
  l2 = l;

  i=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  n = input[i+1] - '0';
	  if((n >= 0)&&(n <= 9)&&(arguments[n] != NULL))
	    {
	      l2 += strlen(arguments[n])-2;
	      i++;
	    }
	  else if(input[i+1] == 'd' || input[i+1] == 'w' ||
		  input[i+1] == 'x' || input[i+1] == 'y')
	    {
	      l2 += 16;
	      i++;
	    }
	}
      i++;
    }

  out = safemalloc(l2+1);
  i=0;
  j=0;
  while(i<l)
    {
      if(input[i] == '$')
	{
	  n = input[i+1] - '0';
	  if((n >= 0)&&(n <= 9)&&(arguments[n] != NULL))
	    {
	      for(k=0;k<strlen(arguments[n]);k++)
		out[j++] = arguments[n][k];
	      i++;
	    }
	  else if(input[i+1] == 'w')
	    {
	      if(tmp_win)
		sprintf(&out[j],"0x%x",(unsigned int)tmp_win->w);
	      else
		sprintf(&out[j],"$w");
	      j += strlen(&out[j]);
	      i++;
	    }
	  else if(input[i+1] == 'd')
	    {
	      sprintf(&out[j], "%d", Scr.CurrentDesk);
	      j += strlen(&out[j]);
	      i++;
	    }
	  else if(input[i+1] == 'x')
	    {
	      sprintf(&out[j], "%d", Scr.Vx);
	      j += strlen(&out[j]);
	      i++;
	    }
	  else if(input[i+1] == 'y')
	    {
	      sprintf(&out[j], "%d", Scr.Vy);
	      i++;
	    }
	  else if(input[i+1] == '$')
	    {
	      out[j++] = '$';
	      i++;
	    }
	  else
	    out[j++] = input[i];
	}
      else
	out[j++] = input[i];
      i++;
    }
  out[j] = 0;
  return out;
}
