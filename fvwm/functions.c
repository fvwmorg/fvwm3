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
#include "add_window.h"
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
 */

#define PRE_REPEAT       "repeat"
#define PRE_SILENT       "silent"

/* The function names in the first field *must* be in lowercase or else the
 * function cannot be called.  The cmd_mixed parameter of the macro is only
 * intended for parsing by modules that rely on the old case of the commands.
 */
#define CMD_ENTRY(cmd_lowercase, cmd_mixed, func, cmd_id, flags) \
 {cmd_lowercase, func, cmd_id, flags}
/* the next line must be blank for modules to properly parse this file */

static const func_type func_config[] =
{
  CMD_ENTRY("+", +,
	    add_another_item, F_ADDMENU2, 0),
#ifdef MULTISTYLE
  CMD_ENTRY("addbuttonstyle", AddButtonStyle,
	    AddButtonStyle, F_ADD_BUTTON_STYLE, FUNC_DECOR),
  CMD_ENTRY("addtitlestyle", AddTitleStyle,
	    AddTitleStyle, F_ADD_TITLE_STYLE, FUNC_DECOR),
#endif /* MULTISTYLE */
#ifdef USEDECOR
  CMD_ENTRY("addtodecor", AddToDecor,
	    add_item_to_decor, F_ADD_DECOR, 0),
#endif /* USEDECOR */
  CMD_ENTRY("addtofunc", AddToFunc,
	    add_item_to_func, F_ADDFUNC, FUNC_ADD_TO),
  CMD_ENTRY("addtomenu", AddToMenu,
	    add_item_to_menu, F_ADDMENU, 0),
  CMD_ENTRY("all", All,
	    AllFunc, F_ALL, 0),
  CMD_ENTRY("animatedmove", AnimatedMove,
	    animated_move_window, F_ANIMATED_MOVE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("beep", Beep,
	    Bell, F_BEEP, 0),
  CMD_ENTRY("borderstyle", BorderStyle,
	    SetBorderStyle, F_BORDERSTYLE, FUNC_DECOR),
  CMD_ENTRY("bugopts", BugOpts,
	    SetBugOptions, F_BUG_OPTS, 0),
  CMD_ENTRY("busycursor", BusyCursor,
	    setBusyCursor, F_BUSY_CURSOR, 0),
  CMD_ENTRY("buttonstate", ButtonState,
	    cmd_button_state, F_BUTTON_STATE, 0),
  CMD_ENTRY("buttonstyle", ButtonStyle,
	    ButtonStyle, F_BUTTON_STYLE, FUNC_DECOR),
#ifdef USEDECOR
  CMD_ENTRY("changedecor", ChangeDecor,
	    ChangeDecor, F_CHANGE_DECOR, FUNC_NEEDS_WINDOW),
#endif /* USEDECOR */
  CMD_ENTRY("changemenustyle", ChangeMenuStyle,
	    ChangeMenuStyle, F_CHANGE_MENUSTYLE, 0),
  CMD_ENTRY("clicktime", ClickTime,
	    SetClick, F_CLICK, 0),
  CMD_ENTRY("close", Close,
	    close_function, F_CLOSE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("colorlimit", ColorLimit,
	    SetColorLimit, F_COLOR_LIMIT, 0),
  CMD_ENTRY("colormapfocus", ColormapFocus,
	    SetColormapFocus, F_COLORMAP_FOCUS, 0),
  CMD_ENTRY("colorset", Colorset,
	    HandleColorset, F_NOP, 0),
  CMD_ENTRY("copymenustyle", CopyMenuStyle,
	    CopyMenuStyle, F_COPY_MENU_STYLE, 0),
  CMD_ENTRY("current", Current,
	    CurrentFunc, F_CURRENT, 0),
  CMD_ENTRY("cursormove", CursorMove,
	    movecursor, F_MOVECURSOR, 0),
  CMD_ENTRY("cursorstyle", CursorStyle,
	    CursorStyle, F_CURSOR_STYLE, 0),
  CMD_ENTRY("defaultcolors", DefaultColors,
	    SetDefaultColors, F_DFLT_COLORS, 0),
  CMD_ENTRY("defaultcolorset", DefaultColorset,
	    SetDefaultColorset, F_DFLT_COLORSET, 0),
  CMD_ENTRY("defaultfont", DefaultFont,
	    LoadDefaultFont, F_DFLT_FONT, 0),
  CMD_ENTRY("defaulticon", DefaultIcon,
	    SetDefaultIcon, F_DFLT_ICON, 0),
  CMD_ENTRY("defaultlayers", DefaultLayers,
	    SetDefaultLayers, F_DFLT_LAYERS, 0),
  CMD_ENTRY("delete", Delete,
	    delete_function, F_DELETE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("desk", Desk,
	    goto_desk_func, F_GOTO_DESK, 0),
  CMD_ENTRY("desktopsize", DesktopSize,
	    SetDeskSize, F_SETDESK, 0),
  CMD_ENTRY("destroy", Destroy,
	    destroy_function, F_DESTROY, FUNC_NEEDS_WINDOW),
#ifdef USEDECOR
  CMD_ENTRY("destroydecor", DestroyDecor,
	    DestroyDecor, F_DESTROY_DECOR, 0),
#endif /* USEDECOR */
  CMD_ENTRY("destroyfunc", DestroyFunc,
	    destroy_fvwmfunc, F_DESTROY_FUNCTION, 0),
  CMD_ENTRY("destroymenu", DestroyMenu,
	    destroy_menu, F_DESTROY_MENU, 0),
  CMD_ENTRY("destroymenustyle", DestroyMenuStyle,
	    DestroyMenuStyle, F_DESTROY_MENUSTYLE, 0),
  CMD_ENTRY("destroymoduleconfig", DestroyModuleConfig,
	    DestroyModConfig, F_DESTROY_MOD, 0),
  CMD_ENTRY("destroystyle", DestroyStyle,
	    ProcessDestroyStyle, F_DESTROY_STYLE, 0),
  CMD_ENTRY("direction", Direction,
	    DirectionFunc, F_DIRECTION, 0),
  CMD_ENTRY("echo", Echo,
	    echo_func, F_ECHO, 0),
  CMD_ENTRY("edgeresistance", EdgeResistance,
	    SetEdgeResistance, F_EDGE_RES, 0),
  CMD_ENTRY("edgescroll", EdgeScroll,
	    SetEdgeScroll, F_EDGE_SCROLL, 0),
  CMD_ENTRY("edgethickness", EdgeThickness,
	    setEdgeThickness, F_NOP, 0),
  CMD_ENTRY("emulate", Emulate,
	    Emulate, F_EMULATE, 0),
  CMD_ENTRY("escapefunc", EscapeFunc,
	    Nop_func, F_ESCAPE_FUNC, 0),
  CMD_ENTRY("exec", Exec,
	    exec_function, F_EXEC, 0),
  CMD_ENTRY("execuseshell", ExecUseShell,
	    exec_setup, F_EXEC_SETUP, 0),
  CMD_ENTRY("fakeclick", FakeClick,
	    fake_click, F_FAKE_CLICK, 0),
  CMD_ENTRY("flipfocus", FlipFocus,
	    flip_focus_func, F_FLIP_FOCUS, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("focus", Focus,
	    focus_func, F_FOCUS, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("function", Function,
	    NULL, F_FUNCTION, 0),
  CMD_ENTRY("globalopts", GlobalOpts,
	    SetGlobalOptions, F_GLOBAL_OPTS, 0),
#ifdef GNOME
  CMD_ENTRY("gnomebutton", GnomeButton,
	    GNOME_ButtonFunc, F_MOUSE, 0),
  CMD_ENTRY("gnomeshowdesks", GnomeShowDesks,
	    GNOME_ShowDesks, F_GOTO_DESK, 0),
#endif /* GNOME */
  CMD_ENTRY("gotodesk", GotoDesk,
	    goto_desk_func, F_GOTO_DESK, 0),
  CMD_ENTRY("gotodeskandpage", GotoDeskAndPage,
	    gotoDeskAndPage_func, F_GOTO_DESK, 0),
  CMD_ENTRY("gotopage", GotoPage,
	    goto_page_func, F_GOTO_PAGE, 0),
  CMD_ENTRY("hidegeometrywindow", HideGeometryWindow,
	    HideGeometryWindow, F_HIDEGEOMWINDOW, 0),
  CMD_ENTRY("hilightcolor", HilightColor,
	    SetHiColor, F_HICOLOR, 0),
  CMD_ENTRY("hilightcolorset", HilightColorset,
	    SetHiColorset, F_HICOLORSET, 0),
  CMD_ENTRY("iconfont", IconFont,
	    LoadIconFont, F_ICONFONT, 0),
  CMD_ENTRY("iconify", Iconify,
	    iconify_function, F_ICONIFY, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("iconpath", Iconpath,
	    iconPath_function, F_ICON_PATH, 0),
  CMD_ENTRY("ignoremodifiers", IgnoreModifiers,
	    ignore_modifiers, F_IGNORE_MODIFIERS, 0),
  CMD_ENTRY("imagepath", ImagePath,
	    imagePath_function, F_IMAGE_PATH, 0),
  CMD_ENTRY("key", Key,
	    key_binding, F_KEY, 0),
  CMD_ENTRY("killmodule", KillModule,
	    module_zapper, F_KILL_MODULE, 0),
  CMD_ENTRY("layer", Layer,
	    change_layer, F_LAYER, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("lower", Lower,
	    lower_function, F_LOWER, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("maximize", Maximize,
	    Maximize, F_MAXIMIZE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("menu", Menu,
	    staysup_func, F_STAYSUP, 0),
  CMD_ENTRY("menustyle", MenuStyle,
	    SetMenuStyle, F_MENUSTYLE, 0),
  CMD_ENTRY("module", Module,
	    executeModule, F_MODULE, 0),
  CMD_ENTRY("modulepath", ModulePath,
	    setModulePath, F_MODULE_PATH, 0),
  CMD_ENTRY("modulesynchronous", ModuleSynchronous,
	    executeModuleSync, F_MODULE_SYNC, 0),
  CMD_ENTRY("moduletimeout", ModuleTimeout,
	    setModuleTimeout, F_NOP, 0),
  CMD_ENTRY("mouse", Mouse,
	    mouse_binding, F_MOUSE, 0),
  CMD_ENTRY("move", Move,
	    move_window, F_MOVE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("movethreshold", MoveThreshold,
	    SetMoveThreshold, F_MOVE_THRESHOLD, 0),
  CMD_ENTRY("movetodesk", MoveToDesk,
	    move_window_to_desk, F_MOVE_TO_DESK, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("movetopage", MoveToPage,
	    move_window_to_page, F_MOVE_TO_PAGE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("next", Next,
	    NextFunc, F_NEXT, 0),
  CMD_ENTRY("none", None,
	    NoneFunc, F_NONE, 0),
  CMD_ENTRY("nop", Nop,
	    Nop_func, F_NOP, FUNC_DONT_REPEAT),
  CMD_ENTRY("opaquemovesize", OpaqueMoveSize,
	    SetOpaque, F_OPAQUE, 0),
  CMD_ENTRY("pick", Pick,
	    PickFunc, F_PICK, 0),
  CMD_ENTRY("piperead", PipeRead,
	    PipeRead, F_READ, 0),
  CMD_ENTRY("pixmappath", PixmapPath,
	    pixmapPath_function, F_PIXMAP_PATH, 0),
  CMD_ENTRY("placeagain", PlaceAgain,
	    PlaceAgain_func, F_PLACEAGAIN, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("pointerkey", PointerKey,
	    pointerkey_binding, F_POINTERKEY, 0),
  CMD_ENTRY("popup", Popup,
	    popup_func, F_POPUP, 0),
  CMD_ENTRY("prev", Prev,
	    PrevFunc, F_PREV, 0),
  CMD_ENTRY("quit", Quit,
	    quit_func, F_QUIT, 0),
  CMD_ENTRY("quitscreen", QuitScreen,
	    quit_screen_func, F_QUIT_SCREEN, 0),
  CMD_ENTRY("quitsession", QuitSession,
	    quitSession_func, F_QUIT_SESSION, 0),
  CMD_ENTRY("raise", Raise,
	    raise_function, F_RAISE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("raiselower", RaiseLower,
	    raiselower_func, F_RAISELOWER, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("read", Read,
	    ReadFile, F_READ, 0),
  CMD_ENTRY("recapture", Recapture,
	    Recapture, F_RECAPTURE, 0),
  CMD_ENTRY("recapturewindow", RecaptureWindow,
	    RecaptureWindow, F_RECAPTURE_WINDOW, 0),
  CMD_ENTRY("refresh", Refresh,
	    refresh_function, F_REFRESH, 0),
  CMD_ENTRY("refreshwindow", RefreshWindow,
	    refresh_win_function, F_REFRESH, FUNC_NEEDS_WINDOW),
  CMD_ENTRY(PRE_REPEAT, Repeat,
	    repeat_function, F_REPEAT, FUNC_DONT_REPEAT),
  CMD_ENTRY("resize", Resize,
	    resize_window, F_RESIZE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("resizemove", ResizeMove,
	    resize_move_window, F_RESIZEMOVE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("restart", Restart,
	    restart_function, F_RESTART, 0),
  CMD_ENTRY("savequitsession", SaveQuitSession,
	    saveQuitSession_func, F_SAVE_QUIT_SESSION, 0),
  CMD_ENTRY("savesession", SaveSession,
	    saveSession_func, F_SAVE_SESSION, 0),
  CMD_ENTRY("scroll", Scroll,
	    scroll, F_SCROLL, 0),
  CMD_ENTRY("send_configinfo", Send_ConfigInfo,
	    SendDataToModule, F_CONFIG_LIST, FUNC_DONT_REPEAT),
  CMD_ENTRY("send_windowlist", Send_WindowList,
	    send_list_func, F_SEND_WINDOW_LIST, FUNC_DONT_REPEAT),
  CMD_ENTRY("sendtomodule", SendToModule,
	    SendStrToModule, F_SEND_STRING, FUNC_DONT_REPEAT),
  CMD_ENTRY("set_mask", set_mask,
	    set_mask_function, F_SET_MASK, FUNC_DONT_REPEAT),
  CMD_ENTRY("set_nograb_mask", set_nograb_mask,
	    setNoGrabMaskFunc, F_SET_NOGRAB_MASK, FUNC_DONT_REPEAT),
  CMD_ENTRY("set_sync_mask", set_sync_mask,
	    setSyncMaskFunc, F_SET_SYNC_MASK, FUNC_DONT_REPEAT),
  CMD_ENTRY("setanimation", SetAnimation,
	    set_animation, F_SET_ANIMATION, 0),
  CMD_ENTRY("setenv", SetEnv,
	    SetEnv, F_SETENV, 0),
  CMD_ENTRY(PRE_SILENT, Silent,
	    dummy_func, F_SILENT, 0),
  CMD_ENTRY("snapattraction", SnapAttraction,
	    SetSnapAttraction, F_SNAP_ATT, 0),
  CMD_ENTRY("snapgrid", SnapGrid,
	    SetSnapGrid, F_SNAP_GRID, 0),
  CMD_ENTRY("stick", Stick,
	    stick_function, F_STICK, FUNC_NEEDS_WINDOW),
#ifdef HAVE_STROKE
  CMD_ENTRY("stroke", Stroke,
	    stroke_binding, F_STROKE, 0),
  CMD_ENTRY("strokefunc", StrokeFunc,
	    strokeFunc, F_STROKE_FUNC, 0),
#endif /* HAVE_STROKE */
  CMD_ENTRY("style", Style,
	    ProcessNewStyle, F_STYLE, 0),
  CMD_ENTRY("title", Title,
	    Nop_func, F_TITLE, 0),
  CMD_ENTRY("titlestyle", TitlesTyle,
	    SetTitleStyle, F_TITLESTYLE, FUNC_DECOR),
  CMD_ENTRY("unsetenv", UnsetEnv,
	    UnsetEnv, F_SETENV, 0),
  CMD_ENTRY("updatedecor", UpdateDecor,
	    UpdateDecor, F_UPDATE_DECOR, 0),
  CMD_ENTRY("updatestyles", UpdateStyles,
	    update_styles_func, F_UPDATE_STYLES, 0),
  CMD_ENTRY("wait", Wait,
	    wait_func, F_WAIT, 0),
  CMD_ENTRY("warptowindow", WarpToWindow,
	    warp_func, F_WARP, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("windowfont", WindowFont,
	    LoadWindowFont, F_WINDOWFONT, 0),
  CMD_ENTRY("windowid", WindowId,
	    WindowIdFunc, F_WINDOWID, 0),
  CMD_ENTRY("windowlist", WindowList,
	    do_windowList, F_WINDOWLIST, 0),
  CMD_ENTRY("windowshade", WindowShade,
	    WindowShade, F_WINDOW_SHADE, FUNC_NEEDS_WINDOW),
  CMD_ENTRY("windowshadeanimate", WindowShadeAnimate,
	    setShadeAnim, F_SHADE_ANIMATE, 0),
  CMD_ENTRY("xorpixmap", XorPixmap,
	    SetXORPixmap, F_XOR, 0),
  CMD_ENTRY("xorvalue", XorValue,
	    SetXOR,F_XOR,0),
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

  temp = safestrdup(func);
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
#define DEBUG_CAT 0
static cfunc_action_type CheckActionType(
  int x, int y, XEvent *d, Bool may_time_out, Bool is_button_pressed)
{
  int xcurrent,ycurrent,total = 0;
  Time t0;
  int dist;
  XEvent old_event;
  extern Time lastTimestamp;
  Bool do_sleep = False;
#if DEBUG_CAT
int cev = 0;
int clp = 0;
int csleep = 0;
int cc[5] = {0,0,0,0,0};
#endif

  xcurrent = x;
  ycurrent = y;
  t0 = lastTimestamp;
  dist = Scr.MoveThreshold;

  while ((total < Scr.ClickTime && lastTimestamp - t0 < Scr.ClickTime) ||
	 !may_time_out)
  {
#if DEBUG_CAT
clp++;
#endif
    if (!(x - xcurrent <= dist && xcurrent - x <= dist &&
	  y - ycurrent <= dist && ycurrent - y <= dist))
    {
#if DEBUG_CAT
fprintf(stderr, "cat stats (1): clp: %d (%d/%d/%d/%d/%d)  cev: %d csleep: %d\n", clp, cc[0], cc[1], cc[2], cc[3], cc[4], cev, csleep);
#endif
      return (is_button_pressed) ? CF_MOTION : CF_TIMEOUT;
    }

    if (do_sleep)
    {
#if DEBUG_CAT
csleep += 20000;
#endif
      usleep(20000);
    }
    else
    {
#if DEBUG_CAT
csleep += 1;
#endif
      usleep(1);
      do_sleep = 1;
    }
    total += 20;
    if (XCheckMaskEvent(dpy, ButtonReleaseMask|ButtonMotionMask|
			PointerMotionMask|ButtonPressMask|ExposureMask, d))
    {
#if DEBUG_CAT
cev++;
#endif
      do_sleep = 0;
      StashEventTime(d);
      switch (d->xany.type)
      {
      case ButtonRelease:
#if DEBUG_CAT
cc[0]++;
fprintf(stderr, "cat stats (2): clp: %d (%d/%d/%d/%d/%d)  cev: %d csleep: %d\n", clp, cc[0], cc[1], cc[2], cc[3], cc[4], cev, csleep);
#endif
	return CF_CLICK;
      case MotionNotify:
#if DEBUG_CAT
cc[1]++;
#endif
	if ((d->xmotion.state & DEFAULT_ALL_BUTTONS_MASK) ||
            !is_button_pressed)
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	}
        else
	{
#if DEBUG_CAT
fprintf(stderr, "cat stats (3): clp: %d (%d/%d/%d/%d/%d)  cev: %d csleep: %d\n", clp, cc[0], cc[1], cc[2], cc[3], cc[4], cev, csleep);
#endif
	  return CF_CLICK;
	}
	break;
      case ButtonPress:
#if DEBUG_CAT
cc[2]++;
#endif
	if (may_time_out)
	  is_button_pressed = True;
	break;
      case Expose:
#if DEBUG_CAT
cc[3]++;
#endif
	/* must handle expose here so that raising a window with "I" works */
	memcpy(&old_event, &Event, sizeof(XEvent));
	memcpy(&Event, d, sizeof(XEvent));
	/* note: handling Expose events will never modify the global Tmp_win */
	DispatchEvent(True);
	memcpy(&Event, &old_event, sizeof(XEvent));
	break;
      default:
#if DEBUG_CAT
cc[4]++;
fprintf(stderr,"%d ", d->xany.type);
#endif
	/* can't happen */
	break;
      }
    }
  }

#if DEBUG_CAT
fprintf(stderr, "cat stats (4): clp: %d (%d/%d/%d/%d/%d)  cev: %d csleep: %d\n", clp, cc[0], cc[1], cc[2], cc[3], cc[4], cev, csleep);
#endif
  return (is_button_pressed) ? CF_HOLD : CF_TIMEOUT;
}


/***********************************************************************
 *
 *  Procedure:
 *	execute_function - execute a fvwm built in function
 *
 *  Inputs:
 *	Action	- the action to execute
 *	tmp_win	- the fvwm window structure
 *	eventp	- pointer to the event that caused the function
 *	context - the context in which the button was pressed
 *
 ***********************************************************************/
void execute_function(exec_func_args_type *efa)
{
  static unsigned int func_depth = 0;
  FvwmWindow *s_Tmp_win = Tmp_win;
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

  if (!efa->action)
  {
    /* impossibly short command */
    return;
  }
  /* ignore whitespace at the beginning of all config lines */
  efa->action = SkipSpaces(efa->action, NULL, 0);
  if (!efa->action || efa->action[0] == 0)
  {
    /* impossibly short command */
    return;
  }
  if (efa->action[0] == '#')
  {
    /* a comment */
    return;
  }
  /* Note: the module config command, "*" can not be handled by the
   * regular command table because there is no required white space after
   * the asterisk. */
  if (efa->action[0] == '*')
  {
#ifdef USEDECOR
    if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
    {
      fvwm_msg(
	WARN, "execute_function",
	"Command can not be added to a decor; executing command now: '%s'",
	efa->action);
    }
#endif
    /* process a module config command */
    ModuleConfig(efa->action);
    return;
  }
  func_depth++;
  if (efa->args)
  {
    for(j=0;j<11;j++)
      arguments[j] = efa->args[j];
  }
  else
  {
    for(j=0;j<11;j++)
      arguments[j] = NULL;
  }

  if (efa->tmp_win == NULL)
  {
    if (efa->flags.is_window_unmanaged)
      w = efa->win;
    else
      w = Scr.Root;
  }
  else
  {
    if(efa->eventp)
    {
      if(efa->eventp->xbutton.subwindow != None &&
	 efa->eventp->xany.window != efa->tmp_win->w)
      {
	w = efa->eventp->xbutton.subwindow;
      }
      else
      {
	w = efa->eventp->xany.window;
      }
    }
    else
    {
      w = efa->tmp_win->w;
    }
  }

  set_silent = False;
  if (efa->action[0] == '-')
  {
    efa->flags.exec |= FUNC_DONT_EXPAND_COMMAND;
    efa->action++;
  }

  taction = efa->action;
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
    function = expand(function, arguments, efa->tmp_win, False);
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
      ERR, "execute_function",
      "Command can not be added to a decor; executing command now: '%s'",
      efa->action);
  }
#endif
  if (!(efa->flags.exec & FUNC_DONT_EXPAND_COMMAND))
  {
    expaction = expand(taction, arguments, efa->tmp_win,
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
    bif->action(efa->eventp,w,efa->tmp_win,efa->context,runaction,&efa->module);
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
      efa->eventp,w,efa->tmp_win,efa->context,runaction, &efa->module,
      &desperate);
    if (!bif && desperate)
    {
      if (executeModuleDesperate(
	efa->eventp, w, efa->tmp_win, efa->context, runaction, &efa->module) ==
	  -1 && *function != 0)
      {
	fvwm_msg(
	  ERR, "execute_function", "No such command '%s'", function);
      }
    }
  }

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
  if (efa->flags.do_save_tmpwin)
  {
    if (check_if_fvwm_window_exists(s_Tmp_win))
      Tmp_win = s_Tmp_win;
    else
      Tmp_win = NULL;
  }
  func_depth--;

  return;
}

void old_execute_function(
  char *action, FvwmWindow *tmp_win, XEvent *eventp, unsigned long context,
  int Module, FUNC_FLAGS_TYPE exec_flags, char *args[])
{
  exec_func_args_type efa;

  memset(&efa, 0, sizeof(efa));
  efa.eventp = eventp;
  efa.tmp_win = tmp_win;
  efa.action = action;
  efa.args = args;
  efa.context = context;
  efa.module = Module;
  efa.flags.exec = exec_flags;
  execute_function(&efa);

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
int DeferExecution(
  XEvent *eventp, Window *w,FvwmWindow **tmp_win, unsigned long *context,
  cursor_type cursor, int FinishEvent)
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
               ExposureMask | KeyPressMask | VisibilityChangeMask |
               ButtonMotionMask | PointerMotionMask
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
    case KeyPress:
      if (eventp->type != FinishEvent)
	original_w = eventp->xany.window;
      /* fall through */
    case ButtonPress:
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

  if (func == NULL)
    return;

  tmp = Scr.functions;
  prev = NULL;
  while (tmp && tmp != func)
  {
    prev = tmp;
    tmp = tmp->next_func;
  }
  if (tmp != func)
  {
    return;
  }

  if (func->use_depth != 0)
  {
    fvwm_msg(ERR,"DestroyFunction", "Function %s is in use (depth %d)",
	     func->name, func->use_depth);
    return;
  }

  if (prev == NULL)
  {
    Scr.functions = func->next_func;
  }
  else
  {
    prev->next_func = func->next_func;
  }

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
  if (!(*depth))
  {
    Scr.flags.is_executing_complex_function = 0;
  }
  for (i = 0; i < 11; i++)
  {
    if(arguments[i] != NULL)
    {
      free(arguments[i]);
    }
  }

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
  if (!depth)
    Scr.flags.is_executing_complex_function = 1;
  depth++;
  *desperate = 0;
  /* duplicate the whole argument list for use as '$*' */
  if (taction)
  {
    arguments[0] = safestrdup(taction);
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

  /* we have to grab buttons before executing immediate actions because these
   * actions can move the window away from the pointer so that a button
   * release would go to the application below. */
  if (!GrabEm(CRS_NONE, GRAB_NORMAL))
  {
    func->use_depth--;
    XBell(dpy, 0);
    cf_cleanup(&depth, arguments);
    return;
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
      old_execute_function(
	fi->action, tmp_win, eventp, context, -1, 0, arguments);
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
    cf_cleanup(&depth, arguments);
    UngrabEm(GRAB_NORMAL);
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
      UngrabEm(GRAB_NORMAL);
      return;
    }
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
      old_execute_function(
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
