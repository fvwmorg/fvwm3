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

#include <X11/keysym.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "menus.h"
#include "focus.h"
#include "repeat.h"
#include "move_resize.h"
#include "virtual.h"
#include "stack.h"
#include "gnome.h"
#include "borders.h"
#include "module_interface.h"
#include "conditional.h"
#include "placement.h"
#include "windowlist.h"
#include "modconf.h"
#include "session.h"
#include "icons.h"
#include "read.h"
#include "builtins.h"
#include "events.h"
#include "libs/Colorset.h"

extern XEvent Event;
extern FvwmWindow *Tmp_win;
extern char const * const Fvwm_VersionInfo;

/* forward declarations */
static void execute_complex_function(F_CMD_ARGS, Bool *desperate);

static void dummy_func(F_CMD_ARGS)
{
  return;
}

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
static const func_type func_config[] =
{
  {"+",            add_another_item, F_ADDMENU2,             0},
#ifdef MULTISTYLE
  {"addbuttonstyle",AddButtonStyle,  F_ADD_BUTTON_STYLE,     FUNC_DECOR},
  {"addtitlestyle",AddTitleStyle,    F_ADD_TITLE_STYLE,      FUNC_DECOR},
#endif /* MULTISTYLE */
#ifdef USEDECOR
  {"addtodecor",   add_item_to_decor,F_ADD_DECOR,	     0},
#endif /* USEDECOR */
  {"addtofunc",    add_item_to_func, F_ADDFUNC,              FUNC_ADD_TO},
  {"addtomenu",    add_item_to_menu, F_ADDMENU,              0},
  {"all",          AllFunc,          F_ALL,                  0},
  {"animatedmove", animated_move_window,F_ANIMATED_MOVE,     FUNC_NEEDS_WINDOW},
  {"beep",         Bell,             F_BEEP,                 0},
  {"borderstyle",  SetBorderStyle,   F_BORDERSTYLE,          FUNC_DECOR},
  {"bugopts",      SetBugOptions,    F_BUG_OPTS,             0},
  {"busycursor",   setBusyCursor,    F_BUSY_CURSOR,          0},
  {"buttonstate",  cmd_button_state, F_BUTTON_STATE,         0},
  {"buttonstyle",  ButtonStyle,      F_BUTTON_STYLE,         FUNC_DECOR},
#ifdef USEDECOR
  {"changedecor",  ChangeDecor,      F_CHANGE_DECOR,         FUNC_NEEDS_WINDOW},
#endif /* USEDECOR */
  {"changemenustyle", ChangeMenuStyle, F_CHANGE_MENUSTYLE,   0},
  {"clicktime",    SetClick,         F_CLICK,                0},
  {"close",        close_function,   F_CLOSE,                FUNC_NEEDS_WINDOW},
  {"colorlimit",   SetColorLimit,    F_COLOR_LIMIT,          0},
  {"colormapfocus",SetColormapFocus, F_COLORMAP_FOCUS,       0},
  {"colorset",     HandleColorset,   F_NOP,                  0},
  {"current",      CurrentFunc,      F_CURRENT,              0},
  {"cursormove",   movecursor,       F_MOVECURSOR,           0},
  {"cursorstyle",  CursorStyle,      F_CURSOR_STYLE,         0},
  {"defaultcolors",SetDefaultColors, F_DFLT_COLORS,          0},
  {"defaultcolorset",SetDefaultColorset, F_DFLT_COLORSET,    0},
  {"defaultfont",  LoadDefaultFont,  F_DFLT_FONT,            0},
  {"defaulticon",  SetDefaultIcon,   F_DFLT_ICON,            0},
  {"defaultlayers",SetDefaultLayers, F_DFLT_LAYERS,          0},
  {"delete",       delete_function,  F_DELETE,               FUNC_NEEDS_WINDOW},
  {"desk",         goto_desk_func,   F_GOTO_DESK,            0},
  {"desktopsize",  SetDeskSize,      F_SETDESK,              0},
  {"destroy",      destroy_function, F_DESTROY,              FUNC_NEEDS_WINDOW},
#ifdef USEDECOR
  {"destroydecor", DestroyDecor,     F_DESTROY_DECOR,	     0},
#endif /* USEDECOR */
  {"destroyfunc",  destroy_fvwmfunc, F_DESTROY_FUNCTION,     0},
  {"destroymenu",  destroy_menu,     F_DESTROY_MENU,         0},
  {"destroymenustyle", DestroyMenuStyle, F_DESTROY_MENUSTYLE,0},
  {"destroymoduleconfig", DestroyModConfig, F_DESTROY_MOD,   0},
  {"destroystyle", ProcessDestroyStyle,  F_DESTROY_STYLE,    0},
  {"direction",    DirectionFunc,    F_DIRECTION,            0},
  {"echo",         echo_func,        F_ECHO,                 0},
  {"edgeresistance",SetEdgeResistance,F_EDGE_RES,            0},
  {"edgescroll",   SetEdgeScroll,    F_EDGE_SCROLL,          0},
  {"edgethickness",setEdgeThickness, F_NOP,                  0},
  {"emulate",      Emulate,          F_EMULATE,              0},
  {"escapefunc",   Nop_func,         F_ESCAPE_FUNC,          0},
  {"exec",         exec_function,    F_EXEC,                 0},
  {"execuseshell", exec_setup,       F_EXEC_SETUP,           0},
  {"flipfocus",    flip_focus_func,  F_FLIP_FOCUS,           FUNC_NEEDS_WINDOW},
  {"focus",        focus_func,       F_FOCUS,                FUNC_NEEDS_WINDOW},
  {"function",     NULL,             F_FUNCTION,             0},
  {"globalopts",   SetGlobalOptions, F_GLOBAL_OPTS,          0},
#ifdef GNOME
  {"gnomebutton",  GNOME_ButtonFunc, F_MOUSE,                0},
  {"gnomeshowdesks", GNOME_ShowDesks, F_GOTO_DESK,           0},
#endif /* GNOME */
  {"gotodesk",     goto_desk_func,   F_GOTO_DESK,            0},
  {"gotodeskandpage", gotoDeskAndPage_func, F_GOTO_DESK,     0},
  {"gotopage",     goto_page_func,   F_GOTO_PAGE,            0},
  {"hidegeometrywindow",HideGeometryWindow,F_HIDEGEOMWINDOW, 0},
  {"hilightcolor", SetHiColor,       F_HICOLOR,              0},
  {"hilightcolorset",SetHiColorset,  F_HICOLORSET,           0},
  {"iconfont",     LoadIconFont,     F_ICONFONT,             0},
  {"iconify",      iconify_function, F_ICONIFY,              FUNC_NEEDS_WINDOW},
  {"iconpath",     iconPath_function,F_ICON_PATH,            0},
  {"ignoremodifiers",ignore_modifiers,F_IGNORE_MODIFIERS,    0},
  {"imagepath",    imagePath_function,F_IMAGE_PATH,          0},
  {"key",          key_binding,      F_KEY,                  0},
  {"killmodule",   module_zapper,    F_KILL_MODULE,          0},
  {"layer",        change_layer,     F_LAYER,                FUNC_NEEDS_WINDOW},
  {"lower",        lower_function,   F_LOWER,                FUNC_NEEDS_WINDOW},
  {"maximize",     Maximize,         F_MAXIMIZE,             FUNC_NEEDS_WINDOW},
  {"menu",         staysup_func,     F_STAYSUP,              0},
  {"menustyle",    SetMenuStyle,     F_MENUSTYLE,            0},
  {"module",       executeModule,    F_MODULE,               0},
  {"modulepath",   setModulePath,    F_MODULE_PATH,          0},
  {"modulesynchronous",executeModuleSync,F_MODULE_SYNC,      0},
  {"moduletimeout", setModuleTimeout, F_NOP,                 0},
  {"mouse",        mouse_binding,    F_MOUSE,                0},
  {"move",         move_window,      F_MOVE,                 FUNC_NEEDS_WINDOW},
  {"movethreshold",SetMoveThreshold, F_MOVE_THRESHOLD,       0},
  {"movetodesk",   move_window_to_desk,F_MOVE_TO_DESK,       FUNC_NEEDS_WINDOW},
  {"movetopage",   move_window_to_page,F_MOVE_TO_PAGE,       FUNC_NEEDS_WINDOW},
  {"next",         NextFunc,         F_NEXT,                 0},
  {"none",         NoneFunc,         F_NONE,                 0},
  {"nop",          Nop_func,         F_NOP,                  FUNC_DONT_REPEAT},
  {"opaquemovesize", SetOpaque,      F_OPAQUE,               0},
  {"pick",         PickFunc,         F_PICK,                 0},
  {"piperead",     PipeRead,         F_READ,                 0},
  {"pixmappath",   pixmapPath_function,F_PIXMAP_PATH,        0},
  {"placeagain",   PlaceAgain_func,  F_PLACEAGAIN,           FUNC_NEEDS_WINDOW},
  {"pointerkey",   pointerkey_binding, F_POINTERKEY,         0},
  {"popup",        popup_func,       F_POPUP,                0},
  {"prev",         PrevFunc,         F_PREV,                 0},
  {"quit",         quit_func,        F_QUIT,                 0},
  {"quitscreen",   quit_screen_func, F_QUIT_SCREEN,          0},
  {"quitsession",  quitSession_func, F_QUIT_SESSION,         0},
  {"raise",        raise_function,   F_RAISE,                FUNC_NEEDS_WINDOW},
  {"raiselower",   raiselower_func,  F_RAISELOWER,           FUNC_NEEDS_WINDOW},
  {"read",         ReadFile,         F_READ,                 0},
  {"recapture",    Recapture,        F_RECAPTURE,            0},
  {"recapturewindow",RecaptureWindow,F_RECAPTURE_WINDOW,     0},
  {"refresh",      refresh_function, F_REFRESH,              0},
  {"refreshwindow",refresh_win_function, F_REFRESH,          FUNC_NEEDS_WINDOW},
  {PRE_REPEAT,     repeat_function,  F_REPEAT,               FUNC_DONT_REPEAT},
  {"resize",       resize_window,    F_RESIZE,               FUNC_NEEDS_WINDOW},
  {"resizemove",   resize_move_window,F_RESIZEMOVE,          FUNC_NEEDS_WINDOW},
  {"restart",      restart_function, F_RESTART,              0},
  {"savequitsession",saveQuitSession_func,F_SAVE_QUIT_SESSION,0},
  {"savesession",  saveSession_func, F_SAVE_SESSION,         0},
  {"scroll",       scroll,           F_SCROLL,               0},
  {"send_configinfo",SendDataToModule, F_CONFIG_LIST,        FUNC_DONT_REPEAT},
  {"send_windowlist",send_list_func, F_SEND_WINDOW_LIST,     FUNC_DONT_REPEAT},
  {"sendtomodule", SendStrToModule,  F_SEND_STRING,          FUNC_DONT_REPEAT},
  {"set_mask",     set_mask_function,F_SET_MASK,             FUNC_DONT_REPEAT},
  {"set_nograb_mask",setNoGrabMaskFunc,F_SET_NOGRAB_MASK,    FUNC_DONT_REPEAT},
  {"set_sync_mask",setSyncMaskFunc,  F_SET_SYNC_MASK,        FUNC_DONT_REPEAT},
  {"setanimation", set_animation,    F_SET_ANIMATION,	     0},
  {"setenv",       SetEnv,           F_SETENV,               0},
  {PRE_SILENT,     dummy_func,       F_SILENT,               0},
  {"snapattraction",SetSnapAttraction,F_SNAP_ATT,            0},
  {"snapgrid",     SetSnapGrid,      F_SNAP_GRID,            0},
  {"stick",        stick_function,   F_STICK,                FUNC_NEEDS_WINDOW},
#ifdef HAVE_STROKE
  {"stroke",       stroke_binding,   F_STROKE,               0},
  {"strokefunc",   strokeFunc,       F_STROKE_FUNC,          0},
#endif /* HAVE_STROKE */
  {"style",        ProcessNewStyle,  F_STYLE,                0},
  {"title",        Nop_func,         F_TITLE,                0},
  {"titlestyle",   SetTitleStyle,    F_TITLESTYLE,           FUNC_DECOR},
  {"unsetenv",     UnsetEnv,         F_SETENV,               0},
  {"updatedecor",  UpdateDecor,      F_UPDATE_DECOR,         0},
  {"updatestyles", update_styles_func,F_UPDATE_STYLES,       0},
  {"wait",         wait_func,        F_WAIT,                 0},
  {"warptowindow", warp_func,        F_WARP,                 FUNC_NEEDS_WINDOW},
  {"windowfont",   LoadWindowFont,   F_WINDOWFONT,           0},
  {"windowid",     WindowIdFunc,     F_WINDOWID,             0},
  {"windowlist",   do_windowList,    F_WINDOWLIST,           0},
/*
  {"windowsdesk",  changeWindowsDesk,F_CHANGE_WINDOWS_DESK,  FUNC_NEEDS_WINDOW},
*/
  {"windowshade",  WindowShade,      F_WINDOW_SHADE,         FUNC_NEEDS_WINDOW},
  {"windowshadeanimate",setShadeAnim,F_SHADE_ANIMATE,        0},
  {"xorpixmap",    SetXORPixmap,     F_XOR,                  0},
  {"xorvalue",     SetXOR,           F_XOR,                  0},
  {"",0,0,0}
};


/*
** do binary search on func list
*/
static int func_comp(const void *a, const void *b)
{
  return (strcmp((char *)a, ((func_type *)b)->keyword));
}

static const func_type *FindBuiltinFunction(char *func)
{
  func_type *ret_func;
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

  ret_func = (func_type *)bsearch
    (temp, func_config,
     sizeof(func_config) / sizeof(func_type) - 1,
     sizeof(func_type), func_comp);
  free(temp);

  return ret_func;
}

static char *function_vars[] =
{
  "fg.cs",
  "bg.cs",
  "hilight.cs",
  "shadow.cs",
  "desk.width",
  "desk.height",
  "vp.x",
  "vp.y",
  "vp.width",
  "vp.height",
  "page.nx",
  "page.ny",
  "w.x",
  "w.y",
  "w.width",
  "w.height",
  "screen",
  NULL
};

static int expand_extended_var(
  char *var_name, char *output, FvwmWindow *tmp_win)
{
  char *s;
  char *rest;
  char dummy[64];
  char *target = (output) ? output : dummy;
  int cs = -1;
  int n;
  int i;
  Pixel pixel = 0;
  int val = -12345678;
  Bool is_numeric = False;

  /* allow partial matches for *.cs variables */
  switch ((i = GetTokenIndex(var_name, function_vars, -1, &rest)))
  {
  case 0:
  case 1:
  case 2:
  case 3:
    if (!isdigit(*rest) || (*rest == '0' && *(rest + 1) != 0))
      /* not a non-negative integer without leading zeros */
      return 0;
    if (sscanf(rest, "%d%n", &cs, &n) < 1)
      return 0;
    if (*(rest + n) != 0)
      /* trailing characters */
      return 0;
    if (cs < 0)
      return 0;
    AllocColorset(cs);
    switch (i)
    {
    case 0:
      /* fg.cs */
      pixel = Colorset[cs].fg;
      break;
    case 1:
      /* bg.cs */
      pixel = Colorset[cs].bg;
      break;
    case 2:
      /* hilight.cs */
      pixel = Colorset[cs].hilite;
      break;
    case 3:
      /* shadow.cs */
      pixel = Colorset[cs].shadow;
      break;
    }
    return pixel_to_color_string(dpy, Pcmap, pixel, target, False);
  default:
    break;
  }

  /* only exact matches for all other variables */
  switch ((i = GetTokenIndex(var_name, function_vars, 0, &rest)))
  {
  case 4:
    /* desk.width */
    is_numeric = True;
    val = Scr.VxMax + Scr.MyDisplayWidth;
    break;
  case 5:
    /* desk.height */
    is_numeric = True;
    val = Scr.VyMax + Scr.MyDisplayHeight;
    break;
  case 6:
    /* vp.x */
    is_numeric = True;
    val = Scr.Vx;
    break;
  case 7:
    /* vp.y */
    is_numeric = True;
    val = Scr.Vy;
    break;
  case 8:
    /* vp.width */
    is_numeric = True;
    val = Scr.MyDisplayWidth;
    break;
  case 9:
    /* vp.height */
    is_numeric = True;
    val = Scr.MyDisplayHeight;
    break;
  case 10:
    /* page.nx */
    is_numeric = True;
    val = (int)(Scr.Vx / Scr.MyDisplayWidth);
    break;
  case 11:
    /* page.ny */
    is_numeric = True;
    val = (int)(Scr.Vy / Scr.MyDisplayHeight);
    break;

  case 12:
  case 13:
  case 14:
  case 15:
    if (!tmp_win || IS_ICONIFIED(tmp_win))
      return 0;
    else
    {
      is_numeric = True;
      switch (i)
      {
      case 12:
	/* w.x */
	val = tmp_win->frame_g.x;
	break;
      case 13:
	/* w.y */
	val = tmp_win->frame_g.y;
	break;
      case 14:
	/* w.width */
	val = tmp_win->frame_g.width;
	break;
      case 15:
	/* w.height */
	val = tmp_win->frame_g.height;
	break;
      default:
	return 0;
      }
    }
    break;
  case 16:
    /* screen */
    is_numeric = True;
    val = Scr.screen;
    break;
  default:
    /* unknown variable - try to find it in the environment */
    s = getenv(var_name);
    if (s)
    {
      if (output)
      {
	strcpy(output, s);
      }
      return strlen(s);
    }
    else
    {
      return 0;
    }
  }
  if (is_numeric)
  {
    sprintf(target, "%d", val);
    return strlen(target);
  }

  return 0;
}

static char *expand(
  char *input, char *arguments[], FvwmWindow *tmp_win, Bool addto)
{
  int l,i,l2,n,k,j,m;
  int xlen;
  char *out;
  char *var;
  const char *string = NULL;
  Bool is_string = False;

  l = strlen(input);
  l2 = l;

  if(input[0] == '+' && Scr.last_added_item.type == ADDED_FUNCTION)
  {
    addto = 1;
  }

  /* Calculate best guess at length of expanded string */
  i=0;
  while(i<l)
  {
    if(input[i] == '$')
    {
      switch (input[i+1])
      {
      case '$':
	/* skip the second $, it is not a part of variable */
	i++;
	break;
      case '[':
	/* extended variables */
	var = &input[i+2];
	m = i + 2;
	while (m < l && input[m] != ']' && input[m])
	  m++;
	if (input[m] == ']')
	{
	  input[m] = 0;
	  /* handle variable name */
	  k = strlen(var);
	  if (!addto)
	  {
	    xlen = expand_extended_var(var, NULL, tmp_win);
	    if (xlen > 0)
	      l2 += xlen - (k + 2);
	  }
	  i += k + 2;
	  input[m] = ']';
	}
	break;
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
      case '*':
	if (input[i+1] == '*')
	  n = 0;
	else
	  n = input[i+1] - '0' + 1;
	if(arguments[n] != NULL)
	{
	  l2 += strlen(arguments[n])-2;
	  i++;
	}
	break;
      case '.':
	string = get_current_read_dir();
	break;
      case 'w':
      case 'd':
      case 'x':
      case 'y':
	l2 += 16;
	i++;
	break;
      case 'c':
	if (tmp_win && tmp_win->class.res_class &&
	    tmp_win->class.res_class[0])
	{
	  string = tmp_win->class.res_class;
	}
	break;
      case 'r':
	if (tmp_win && tmp_win->class.res_name &&
	    tmp_win->class.res_name[0])
	{
	  string = tmp_win->class.res_name;
	}
	break;
      case 'n':
	if (tmp_win && tmp_win->name && tmp_win->name[0])
	{
	  string = tmp_win->name;
	}
	break;
      case 'v':
	if (Fvwm_VersionInfo)
	{
	  l2 += strlen(Fvwm_VersionInfo) + 2;
	}
	break;
      }
      if (string)
      {
	for (k = 0; string[k] != 0; k++, l2++)
	{
	  if (string[k] == '\'')
	    l2++;
	}
	string = NULL;
      }

    }
    i++;
  }

  /* Actually create expanded string */
  i=0;
  out = safemalloc(l2+1);
  j=0;
  while(i<l)
  {
    if(input[i] == '$')
    {
      switch (input[i+1])
      {
      case '[':
	/* extended variables */
	if (addto)
	{
	  /* Don't expand these in an 'AddToFunc' command */
	  out[j++] = input[i];
	  break;
	}
	var = &input[i+2];
	m = i + 2;
	while (m < l && input[m] != ']' && input[m])
	  m++;
	if (input[m] == ']')
	{
	  input[m] = 0;
	  /* handle variable name */
	  k = strlen(var);
	  xlen = expand_extended_var(var, &out[j], tmp_win);
	  input[m] = ']';
	  if (xlen > 0)
	  {
	    j += xlen;
	    i += k + 2;
	  }
	  else
	  {
	    out[j++] = input[i];
	  }
	}
	else
	{
	  out[j++] = input[i];
	}
	break;
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
      case '*':
	if (input[i+1] == '*')
	  n = 0;
	else
	  n = input[i+1] - '0' + 1;
	if(arguments[n] != NULL)
	{
	  for(k=0;arguments[n][k];k++)
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
#if 0
	  /* DV: Whoa! Better live with the extra whitespace. */
	  if (isspace(input[i+1]))
	    i++; /*eliminates extra white space*/
#endif
	}
	break;
      case '.':
	string = get_current_read_dir();
	is_string = True;
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
	j += strlen(&out[j]);
	i++;
	break;

      case 'c':
	if (tmp_win && tmp_win->class.res_class &&
	    tmp_win->class.res_class[0])
	{
	  string = tmp_win->class.res_class;
	}
	is_string = True;
	break;
      case 'r':
	if (tmp_win && tmp_win->class.res_name &&
	    tmp_win->class.res_name[0])
	{
	  string = tmp_win->class.res_name;
	}
	is_string = True;
	break;
      case 'n':
	if (tmp_win && tmp_win->name && tmp_win->name[0])
	{
	  string = tmp_win->name;
	}
	is_string = True;
	break;
      case 'v':
	sprintf(&out[j], "%s", (Fvwm_VersionInfo) ? Fvwm_VersionInfo : "");
	j += strlen(&out[j]);
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
      if (is_string && string)
      {
	out[j++] = '\'';
	for(k = 0; string[k]; k++)
	{
	  if (string[k] == '\'')
	    out[j++] = '\\';
	  out[j++] = string[k];
	}
	out[j++] = '\'';
	string = NULL;
	is_string = False;
	i++;
      }
      else if (is_string)
      {
	out[j++] = '$';
	is_string = False;
      }
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
  int x, int y, XEvent *d, Bool may_time_out, Bool is_button_pressed)
{
  int xcurrent,ycurrent,total = 0;
  Time t0;
  int dist;
  XEvent old_event;
  extern Time lastTimestamp;

  xcurrent = x;
  ycurrent = y;
  t0 = lastTimestamp;
  dist = Scr.MoveThreshold;

  while ((total < Scr.ClickTime && lastTimestamp - t0 < Scr.ClickTime) ||
	 !may_time_out)
  {
    if (!(x - xcurrent <= dist && xcurrent - x <= dist &&
	  y - ycurrent <= dist && ycurrent - y <= dist))
    {
      return (is_button_pressed) ? CF_MOTION : CF_TIMEOUT;
    }

    usleep(20000);
    total+=20;
    if(XCheckMaskEvent(dpy, ButtonReleaseMask|ButtonMotionMask|
		       PointerMotionMask|ButtonPressMask|ExposureMask, d))
    {
      StashEventTime(d);
      switch (d->xany.type)
      {
      case ButtonRelease:
	return CF_CLICK;
      case MotionNotify:
	if ((d->xmotion.state & DEFAULT_ALL_BUTTONS_MASK) ||
            !is_button_pressed)
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	}
        else
	{
	  return CF_CLICK;
	}
	break;
      case ButtonPress:
	if (may_time_out)
	  is_button_pressed = True;
	break;
      case Expose:
	/* must handle expose here so that raising a window with "I" works */
	memcpy(&old_event, &Event, sizeof(XEvent));
	memcpy(&Event, d, sizeof(XEvent));
	DispatchEvent(True);
	memcpy(&Event, &old_event, sizeof(XEvent));
	break;
      default:
	/* can't happen */
	break;
      }
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
void ExecuteFunction(F_EXEC_ARGS, unsigned int exec_flags, char *args[])
{
  static unsigned int func_depth = 0;
  Window w;
  int j;
  char *function;
  char *taction;
  char *trash;
  char *trash2;
  char *expaction = NULL;
  char *arguments[11];
  const func_type *bif;
  Bool set_silent;
  Bool must_free_string = False;
  Bool must_free_function = False;

  if (!action)
  {
    /* impossibly short command */
    return;
  }
  /* ignore whitespace at the beginning of all config lines */
  action = SkipSpaces(action, NULL, 0);
  if (!action || action[0] == 0 || action[1] == 0)
  {
    /* impossibly short command */
    return;
  }
  if (action[0] == '#')
  {
    /* a comment */
    return;
  }
  /* Note: the module config command, "*" can not be handled by the
   * regular command table because there is no required white space after
   * the asterisk. */
  if (action[0] == '*')
  {
#ifdef USEDECOR
    if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
    {
      fvwm_msg(
	WARN, "ExecuteFunction",
	"Command can not be added to a decor; executing command now: '%s'",
	action);
    }
#endif
    /* a module config command */
    ModuleConfig(action);  /* process the command */
    return;                             /* done */
  }
  func_depth++;
  if (args)
  {
    for(j=0;j<11;j++)
      arguments[j] = args[j];
  }
  else
  {
    for(j=0;j<11;j++)
      arguments[j] = NULL;
  }

  if(tmp_win == NULL)
  {
    w = Scr.Root;
  }
  else
  {
    if(eventp)
    {
      if(eventp->xbutton.subwindow != None &&
	 eventp->xany.window != tmp_win->w)
      {
	w = eventp->xbutton.subwindow;
      }
      else
      {
	w = eventp->xany.window;
      }
    }
    else
    {
      w = tmp_win->w;
    }
  }

  set_silent = False;
  if (action[0] == '-')
  {
    exec_flags |= FUNC_DONT_EXPAND_COMMAND;
    action++;
  }

  taction = action;
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

  function = PeekToken(taction, NULL);
  if (function)
    function = expand(function, arguments, tmp_win, False);
  if (function)
  {
#if 1
    /* DV: with this piece of code it is impossible to have a complex function
     * with embedded whitespace that begins with a builtin function name, e.g. a
     * function "echo hello". */
    /* DV: ... and without it some of the complex functions will fail */
    char *tmp = function;

    while (*tmp && !isspace(*tmp))
      tmp++;
    *tmp = 0;
#endif
    bif = FindBuiltinFunction(function);
    must_free_function = True;
  }
  else
  {
    bif = NULL;
    function = "";
  }

#ifdef USEDECOR
  if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor &&
      (!bif || !(bif->flags & FUNC_DECOR)))
  {
    fvwm_msg(
      ERR, "ExecuteFunction",
      "Command can not be added to a decor; executing command now: '%s'",
      action);
  }
#endif
  if (!(exec_flags & FUNC_DONT_EXPAND_COMMAND))
  {
    expaction = expand(taction, arguments, tmp_win,
		       (bif) ? !!(bif->flags & FUNC_ADD_TO) : False);
    if (func_depth <= 1)
      must_free_string = set_repeat_data(expaction, REPEAT_COMMAND, bif);
    else
      must_free_string = True;
  }
  else
  {
    expaction = taction;
  }
  j = 0;
  if (bif && bif->func_type != F_FUNCTION)
  {
    char *runaction;

    runaction = SkipNTokens(expaction, 1);
    bif->action(eventp,w,tmp_win,context,runaction,&Module);
  }
  else
  {
    Bool desperate = 1;
    char *runaction;

    if (bif)
    {
      /* strip "function" command */
      runaction = SkipNTokens(expaction, 1);
    }
    else
    {
      runaction = expaction;
    }

    execute_complex_function(
      eventp,w,tmp_win,context,runaction, &Module, &desperate);
    if (!bif && desperate)
    {
      if (executeModuleDesperate(
	eventp, w, tmp_win, context, runaction, &Module) == -1 &&
	  *function != 0)
      {
	fvwm_msg(
	  ERR, "ExecuteFunction", "No such command '%s'", function);
      }
    }
  }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions, menus or modules. */
  if (Module == -1 && (exec_flags & FUNC_DO_SYNC_BUTTONS))
    WaitForButtonsUp(True);

  if (set_silent)
    Scr.flags.silent_functions = 0;

  if (must_free_string)
  {
    free(expaction);
  }
  if (must_free_function)
  {
    free(function);
  }

  func_depth--;
  return;
}


void ExecuteFunctionSaveTmpWin(
  F_EXEC_ARGS, unsigned int exec_flags, char *args[])
{
  FvwmWindow *s_Tmp_win = Tmp_win;

  ExecuteFunction(action, tmp_win, eventp, context, Module, exec_flags, args);
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
  if(!GrabEm(cursor, GRAB_NORMAL))
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
	UngrabEm(GRAB_NORMAL);
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

  UngrabEm(GRAB_NORMAL);
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
  if(*w != original_w && original_w != Scr.Root &&
     original_w != None && original_w != Scr.NoFocusWin)
  {
    if (*w != (*tmp_win)->frame || original_w != (*tmp_win)->w)
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

  if (token[1] != '\0' ||
      (condition != CF_IMMEDIATE && condition != CF_MOTION &&
       condition != CF_HOLD && condition != CF_CLICK &&
       condition != CF_DOUBLE_CLICK))
  {
    fvwm_msg(
      ERR, "AddToFunction",
      "Got '%s' instead of a valid function specifier", token);
    free(token);
    return;
  }
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


static void cf_cleanup(unsigned int *depth, char **arguments)
{
  int i;

  (*depth)--;
  if (*depth == 0)
    WaitForButtonsUp(False);
  for (i = 0; i < 11; i++)
    if(arguments[i] != NULL)
      free(arguments[i]);

  return;
}

static void execute_complex_function(F_CMD_ARGS, Bool *desperate)
{
  cfunc_action_type type = CF_MOTION;
  char c;
  FunctionItem *fi;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
  Bool HaveHold = False;
  Bool NeedsTarget = False;
  Bool ImmediateNeedsTarget = False;
  char *arguments[11], *taction;
  char* func_name;
  int x, y ,i;
  XEvent d, *ev;
  FvwmFunction *func;
  static unsigned int depth = 0;

  /* FindFunction expects a token, not just a quoted string */
  taction = GetNextToken(action,&func_name);
  if (!action || !func_name)
  {
    if (func_name)
      free(func_name);
    return;
  }
  func = FindFunction(func_name);
  free(func_name);
  if(func == NULL)
  {
    if(*desperate == 0)
      fvwm_msg(ERR,"ComplexFunction","No such function %s",action);
    return;
  }
  depth++;
  *desperate = 0;
  /* duplicate the whole argument list for use as '$*' */
  if (taction)
  {
    arguments[0] = strdup(taction);
    /* strip trailing newline */
    if (arguments[0][0])
    {
      int l= strlen(arguments[0]);

      if (arguments[0][l - 1] == '\n')
	arguments[0][l - 1] = 0;
    }
    /* Get the argument list */
    for(i=1;i<11;i++)
      taction = GetNextToken(taction,&arguments[i]);
  }
  else
  {
    for(i=0;i<11;i++)
      arguments[i] = NULL;
  }
  /* see functions.c to find out which functions need a window to operate on */
  ev = eventp;
  /* In case we want to perform an action on a button press, we
   * need to fool other routines */
  if(eventp->type == ButtonPress)
    eventp->type = ButtonRelease;
  func->use_depth++;

  for (fi = func->first_item; fi != NULL; fi = fi->next_item)
  {
    if (fi->flags & FUNC_NEEDS_WINDOW)
    {
      NeedsTarget = True;
      if (fi->condition == CF_IMMEDIATE)
      {
        ImmediateNeedsTarget = True;
        break;
      }
    }
  }

  if(ImmediateNeedsTarget)
  {
    if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonPress))
    {
      func->use_depth--;
      cf_cleanup(&depth, arguments);
      return;
    }
    NeedsTarget = False;
  }

  for (fi = func->first_item; fi != NULL; fi = fi->next_item)
  {
    /* c is already lowercase here */
    c = fi->condition;
    switch (c)
    {
    case CF_IMMEDIATE:
      if (tmp_win)
	w = tmp_win->frame;
      else
	w = None;
      ExecuteFunction(fi->action, tmp_win, eventp, context, -1, 0, arguments);
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
  }

  if(!Persist)
  {
    func->use_depth--;
    for(i=0;i<11;i++)
      if(arguments[i] != NULL)
	free(arguments[i]);
    depth--;
    return;
  }

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if(NeedsTarget)
  {
    if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonPress))
    {
      func->use_depth--;
      cf_cleanup(&depth, arguments);
      return;
    }
  }

  if(!GrabEm(CRS_NONE, GRAB_NORMAL))
  {
    func->use_depth--;
    XBell(dpy, 0);
    cf_cleanup(&depth, arguments);
    return;
  }
  switch (eventp->xany.type)
  {
  case ButtonPress:
  case ButtonRelease:
    x = eventp->xbutton.x_root;
    y = eventp->xbutton.y_root;
    /* Take the click which started this fuction off the
     * Event queue.  -DDN- Dan D Niles dniles@iname.com */
    XCheckMaskEvent(dpy, ButtonPressMask, &d);
    break;
  default:
    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		   &x,&y,&JunkX, &JunkY, &JunkMask);
    break;
  }

  /* Wait and see if we have a click, or a move */
  /* wait forever, see if the user releases the button */
  type = CheckActionType(x, y, &d, HaveHold, True);
  if (type == CF_CLICK)
  {
    ev = &d;
    /* If it was a click, wait to see if its a double click */
    if(HaveDoubleClick)
    {
      type = CheckActionType(x, y, &d, True, False);
      switch (type)
      {
      case CF_CLICK:
      case CF_HOLD:
      case CF_MOTION:
	type = CF_DOUBLE_CLICK;
	ev = &d;
	break;
      case CF_TIMEOUT:
	type = CF_CLICK;
	break;
      default:
	/* can't happen */
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
#ifdef BUGGY_CODE
  /* domivogt (11-Apr-2000): The pointer ***must not*** be ungrabbed here.  If
   * it is, any window that the mouse enters during the function will receive
   * MotionNotify events with a button held down!  The results are
   * unpredictable.  E.g. rxvt interprets the ButtonMotion as user input to
   * select text. */
  UngrabEm(GRAB_NORMAL);
#endif
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
      ExecuteFunction(
	fi->action, tmp_win, ev, context, -1, 0, arguments);
    }
    fi = fi->next_item;
  }
  /* This is the right place to ungrab the pointer (see comment above). */
  func->use_depth--;
  cf_cleanup(&depth, arguments);
  UngrabEm(GRAB_NORMAL);

  return;
}
