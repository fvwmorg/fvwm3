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

#ifndef _FUNCTIONS_
#define _FUNCTIONS_

#include "cursor.h"

struct FvwmFunction;               /* forward declaration */

typedef struct FunctionItem
{
  struct FvwmFunction *func;       /* the function this item is in */
  struct FunctionItem *next_item;  /* next function item */
  char condition;                  /* the character string displayed on left*/
  char *action;                    /* action to be performed */
  short type;                      /* type of built in function */
  unsigned char flags;
} FunctionItem;

typedef struct FvwmFunction
{
  struct FvwmFunction *next_func;  /* next in list of root menus */
  FunctionItem *first_item;        /* first item in function */
  FunctionItem *last_item;         /* last item in function */
  char *name;                      /* function name */
  unsigned int use_depth;
} FvwmFunction;

/* used for parsing commands*/
struct functions
{
  char *keyword;
#ifdef __STDC__
  void (*action)(XEvent *,Window,FvwmWindow *, unsigned long,char *, int *);
#else
  void (*action)();
#endif
  short func_type;
  unsigned char flags;
};

/* Bits for the function flag byte. */
#define FUNC_NEEDS_WINDOW 0x01
#define FUNC_DONT_REPEAT  0x02
#define FUNC_ADD_TO       0x04

/* Types of events for the FUNCTION builtin */
typedef enum
{
  CF_IMMEDIATE =      'i',
  CF_MOTION =         'm',
  CF_HOLD =           'h',
  CF_CLICK =          'c',
  CF_DOUBLE_CLICK =   'd',
  CF_TIMEOUT =        '-'
} cfunc_action_type;

/* for fExpand parameter of ExecuteFunction */
typedef enum
{
  DONT_EXPAND_COMMAND,
  EXPAND_COMMAND
} expand_command_type;

void find_func_type(char *action, short *func_type, unsigned char *flags);
FvwmFunction *FindFunction(const char *function_name);
extern FvwmFunction *NewFvwmFunction(const char *name);
void DestroyFunction(FvwmFunction *func);
extern int DeferExecution(XEvent *, Window *,FvwmWindow **, unsigned long *,
			  cursor_type, int);
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module,
		     expand_command_type expand_cmd);
void ExecuteFunctionSaveTmpWin(char *Action, FvwmWindow *tmp_win,
			       XEvent *eventp, unsigned long context,
			       int Module, expand_command_type expand_cmd);
void AddToFunction(FvwmFunction *func, char *action);

enum
{
  F_UNDEFINED = -1,

  /* functions that need no window */
  F_NOP = 0,
  F_ADDFUNC,
  F_ADDMENU,
  F_ADDMENU2,
  F_ALL,
  F_BEEP,
  F_BUG_OPTS,
  F_BUSY_CURSOR,
  F_BUTTON_STATE,
  F_BUTTON_STYLE,
  F_CHANGE_MENUSTYLE,
  F_CIRCULATE_DOWN,
  F_CIRCULATE_UP,
  F_CLICK,
  F_CLOSE,
  F_COLORMAP_FOCUS,
  F_CONFIG_LIST,
  F_CURRENT,
  F_CURSOR_STYLE,
  F_DESK,
  F_DESTROY_FUNCTION,
  F_DESTROY_MENU,
  F_DESTROY_MENUSTYLE,
  F_DESTROY_STYLE,
  F_DFLT_COLORS,
  F_DFLT_COLORSET,
  F_DFLT_FONT,
  F_DFLT_ICON,
  F_DFLT_LAYERS,
  F_DIRECTION,
  F_EDGE_RES,
  F_EDGE_SCROLL,
  F_EMULATE,
  F_ESCAPE_FUNC,
  F_EXEC,
  F_EXEC_SETUP,
  F_FUNCTION,
  F_GLOBAL_OPTS,
  F_GOTO_PAGE,
  F_HICOLOR,
  F_HICOLORSET,
  F_HIDEGEOMWINDOW,
  F_ICONFONT,
  F_ICON_PATH,
  F_IGNORE_MODIFIERS,
  F_IMAGE_PATH,
  F_KEY,
  F_KILL_MODULE,
  F_LAYER,
  F_MENUSTYLE,
  F_MODULE,
  F_MODULE_PATH,
  F_MODULE_SYNC,
  F_MOUSE,
  F_MOVECURSOR,
  F_MOVE_TO_DESK,
  F_NEXT,
  F_NONE,
  F_OPAQUE,
  F_PICK,
  F_PIXMAP_PATH,
  F_POPUP,
  F_PREV,
  F_QUIT,
  F_QUIT_SESSION,
  F_QUIT_SCREEN,
  F_READ,
  F_RECAPTURE,
  F_RECAPTURE_WINDOW,
  F_REFRESH,
  F_REPEAT,
  F_RESTART,
  F_SAVE_SESSION,
  F_SAVE_QUIT_SESSION,
  F_SCROLL,
  F_SETDESK,
  F_SETENV,
  F_SET_ANIMATION,
  F_SET_MASK,
  F_SET_SYNC_MASK,
  F_SHADE_ANIMATE,
  F_SILENT,
  F_SNAP_ATT,
  F_SNAP_GRID,
  F_STAYSUP,
#ifdef HAVE_STROKE
  F_STROKE,
  F_STROKE_FUNC,
#endif /* HAVE_STROKE */
  F_STYLE,
  F_TITLE,
  F_TITLESTYLE,
  F_TOGGLE_PAGE,
  F_WAIT,
  F_WINDOWFONT,
  F_WINDOWLIST,
  F_XOR,

  /* functions that need a window to operate on */
  F_ADD_BUTTON_STYLE,
  F_ADD_DECOR,
  F_ADD_MOD,
  F_ADD_TITLE_STYLE,
  F_ANIMATED_MOVE,
  F_BORDERSTYLE,
  F_CHANGE_DECOR,
  F_COLOR_LIMIT,
  F_DELETE,
  F_DESTROY,
  F_DESTROY_DECOR,
  F_DESTROY_MOD,
  F_ECHO,
  F_FLIP_FOCUS,
  F_FOCUS,
  F_ICONIFY,
  F_LOWER,
  F_MAXIMIZE,
  F_MOVE,
  F_MOVE_SMOOTHNESS,
  F_MOVE_THRESHOLD,
  F_MOVE_TO_PAGE,
  F_PLACEAGAIN,
  F_RAISE,
  F_RAISELOWER,
  F_RESIZE,
  F_SEND_STRING,
  F_STICK,
  F_UPDATE_DECOR,
  F_WARP,
  F_WINDOWID,
  F_WINDOW_SHADE,

  F_END_OF_LIST = 999,

  /* Functions for use by modules only! */
  F_SEND_WINDOW_LIST = 1000
};

#endif /* _FUNCTIONS_ */
