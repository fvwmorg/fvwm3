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

#include <assert.h>
#include <stdio.h>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include <libs/Flocale.h>
#include <libs/Picture.h>
#include <libs/Colorset.h>
#ifndef FVWM_VERSION
#define FVWM_VERSION 2
#endif

#include "libs/fvwmlib.h"

#include "debug.h"
#include "fvwm/fvwm.h"
#include "libs/vpacket.h"

#ifndef DEFAULT_ACTION
#define DEFAULT_ACTION "Iconify"
#endif

#define RECTANGLES_INTERSECT(x1,y1,w1,h1,x2,y2,w2,h2) \
	  (((x1) + (w1) > (x2) && (x1) < (x2) + (w2)) && \
	   ((y1) + (h1) > (y2) && (y1) < (y2) + (h2)))

#define MAX_ARGS 3

#ifdef TRACE_MEMUSE

#define MALLOC_MAGIC 0xdeadbeaf

#define SET_BIT(field,bit)   ((field) |= (bit))
#define CLEAR_BIT(field,bit) ((field) &= ~(bit))

#define SET_BIT_TO_VAL(field,bit,val) ((val) ? SET_BIT (field,bit) : CLEAR_BIT (field, bit))

extern long MemUsed;

struct malloc_header {
  unsigned long magic, len;
};

#endif

#ifdef DMALLOC
/*  What the heck is this??  */
#include <dmalloc.h>
#endif

extern void PrintMemuse (void);

typedef unsigned long Ulong;
typedef unsigned char Uchar;

typedef signed char Schar;

typedef enum {
  SHOW_GLOBAL,
  SHOW_DESKTOP,
  SHOW_PAGE,
  SHOW_SCREEN,
  NO_SHOW_DESKTOP,      /* "!desk" Show windows not on the current desk */
  NO_SHOW_PAGE,         /* "!page" Show windows not on the current page */
  NO_SHOW_SCREEN        /* "!screen" Show windows not on the current screen */
} Resolution;

typedef enum {
  REVERSE_NONE,
  REVERSE_ICON,
  REVERSE_NORMAL
} Reverse;

typedef enum {
  BUTTON_FLAT,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_EDGEUP,
  BUTTON_EDGEDOWN
} ButtonState;

/* The clicks must be the first three elements in this type, X callbacks
	depend on it! */
typedef enum {
  SELECT,
  MOUSE,
  KEYPRESS,
  NUM_ACTIONS
} Action;

typedef enum {
  PLAIN_CONTEXT = 0,
  FOCUS_CONTEXT = 1,
  SELECT_CONTEXT = 2,
  FOCUS_SELECT_CONTEXT = 3, /* had better be FOCUS_CONTEXT | SELECT_CONTEXT */
  TITLE_CONTEXT = 4,
  ICON_CONTEXT = 5,
  DEFAULT = 6,
  NUM_CONTEXTS
} Contexts;

typedef enum {
  NO_NAME       = 0,
  TITLE_NAME    = 1,
  ICON_NAME     = 2,
  RESOURCE_NAME = 4,
  CLASS_NAME    = 8,
  ALL_NAME      = 15
} NameType;

typedef struct win_list {
  int n;
  struct win_data *head, *tail;
} WinList;

typedef struct string_list {
  NameType type;
  char *string;
  struct string_list *next;
} StringEl;

typedef struct {
  Uchar mask;
  StringEl *list;
} StringList;

typedef enum {
  NoArg,
  IntArg,
  StringArg,
  ButtonArg,
  WindowArg,
  ManagerArg,
  JmpArg
} BuiltinArgType;

typedef enum {
  NoButton,
  SelectButton,
  FocusButton,
  AbsoluteButton,
  UpButton,
  DownButton,
  LeftButton,
  RightButton,
  NextButton,
  PrevButton
} ButtonType; /* doubles for manager too */

typedef struct {
  int offset;
  ButtonType base;
} ButtonValue;

typedef struct builtin_arg {
  BuiltinArgType type;
  union {
    char *string_value;
    ButtonValue button_value;
    int int_value;
  } value;
} BuiltinArg;

typedef struct Function {
  int (*func)(int numargs, BuiltinArg *args);
  int numargs;
  BuiltinArg args[MAX_ARGS];
  struct Function *next;
  struct Function *prev;
} Function;

typedef struct win_data {
  struct button *button;
  /* stuff shadowed in the Button structure */
  FvwmPicture pic;
  FvwmPicture old_pic;
  Uchar iconified, state;

  Ulong desknum;
  long x, y, width, height;
  Ulong app_id;
/*  Ulong fvwm_flags; */
  window_flags flags;
  struct win_data *win_prev, *win_next;
  struct win_manager *manager;
  int app_id_set : 1;
  int geometry_set : 1;
  Uchar complete;

  /* this data must be freed */
  char *display_string; /* what gets shown in the manager window */
  char *resname;
  char *classname;
  char *titlename;
  char *iconname;
  char *visible_name;
  char *visible_icon_name;
} WinData;

typedef struct button {
  int index;      /* index into button array */
  int x, y, w, h; /* current coords of button */
  struct {
    int dirty_flags;
    FvwmPicture pic;
    WinData *win;
    char *display_string;
    int x, y, w, h;
    Uchar iconified, state;
  } drawn_state;
} Button;

typedef struct button_array {
  int dirty_flags;
  int num_buttons, drawn_num_buttons; /* size of buttons array */
  int num_windows, drawn_num_windows; /* number of windows with buttons */
  Button **buttons;
} ButtonArray;

typedef enum {
  GROW_HORIZ = 1,
  GROW_VERT  = 2,
  GROW_UP    = 4,
  GROW_DOWN  = 8,
  GROW_LEFT  = 16,
  GROW_RIGHT = 32,
  GROW_FIXED = 64
} GrowDirection;

typedef struct {
  /* Things which we can change go in here.
     This like border width go in WinManager */
  int x, y, width, height;
  int gravity_x, gravity_y; /* anchor point for window's gravity */
  unsigned int rows, cols;
  int boxheight, boxwidth;
  GrowDirection dir;
} ManGeometry;

typedef struct {
  int num_rects;
  XRectangle rects[2];
} ShapeState;

typedef enum {
  SortNone,          /* no sorting */
  SortId,            /* sort by window id */
  SortName,          /* case insensitive name sorting */
  SortNameCase,      /* case sensitive name sorting */
  SortWeighted       /* custom sort order */
} SortType;

typedef struct {
  char *resname;
  char *classname;
  char *titlename;
  char *iconname;
  int weight;
} WeightedSort;

typedef struct win_manager {
  unsigned int magic;
  int index;

  /* .fvwm2rc options or things set as a result of options */
  Resolution res;
  Reverse rev;
  Pixel backcolor[NUM_CONTEXTS], forecolor[NUM_CONTEXTS];
  Pixel hicolor[NUM_CONTEXTS], shadowcolor[NUM_CONTEXTS];
  GC hiContext[NUM_CONTEXTS], backContext[NUM_CONTEXTS],
    reliefContext[NUM_CONTEXTS];
  GC shadowContext[NUM_CONTEXTS], flatContext[NUM_CONTEXTS];
  FlocaleFont *FButtonFont;
  int draw_icons;
  int shaped;
  StringList show;
  StringList dontshow;
  Binding *bindings[NUM_ACTIONS];
  char *fontname;
  int colorsets[NUM_CONTEXTS];
  Pixmap pixmap[NUM_CONTEXTS];
  char *backColorName[NUM_CONTEXTS];
  char *foreColorName[NUM_CONTEXTS];
  ButtonState buttonState[NUM_CONTEXTS];
  char *geometry_str, *button_geometry_str;
  char *titlename, *iconname;
  char *formatstring;
  NameType format_depend;
  Uchar followFocus;
  Uchar usewinlist;
  SortType sort;
  WeightedSort *weighted_sorts;
  int weighted_sorts_len, weighted_sorts_size;
  char *AnimCommand;
  Uchar showonlyiconic;
  rectangle managed_g;    /* dimensions of managed screen portion */

  /* X11 state */
  Window theWindow, theFrame;
  long sizehints_flags;
  int gravity;
  int fontheight, fontwidth;
  int win_title, win_border;
  int off_x, off_y;
  Uchar cursor_in_window;
  Uchar window_up;
  Uchar can_draw;  /* = 0 until we get our first ConfigureNotify */

  /* button state */
  int dirty_flags;
  ManGeometry geometry, drawn_geometry;
  Button *select_button, *focus_button;
  Uchar window_mapped, drawn_mapping;
  ShapeState shape, drawn_shape;
  ButtonArray buttons;

  /* Fvwm state */
  int we_are_drawing, configures_expected;
  Bool swallowed;
} WinManager;

#define MANAGER_EMPTY(man) ((man)->buttons.num_windows == 0)

typedef struct {
  Ulong desknum;
  Ulong x, y;             /* of the view window */
  rectangle screen_g;     /* screen dimensions */
  WinManager *managers;
  int num_managers;
  int transient;
  WinData *focus_win;
  WinData *select_win;
  int got_window_list;
} GlobalData;

typedef struct {
  char *name;
  ButtonState state;
  char *forecolor[2]; /* 0 is mono, 1 is color */
  char *backcolor[2]; /* 0 is mono, 1 is color */
} ContextDefaults;


extern char *contextNames[NUM_CONTEXTS];

extern GlobalData globals;
extern int Fvwm_fd[2];
extern int x_fd;
extern Display *theDisplay;
extern char *MyName;
extern char *Module;
extern int ModuleLen;
extern ContextDefaults contextDefaults[];
extern int mods_unused;

extern void ReadFvwmPipe(void);
extern void *Malloc (size_t size);
extern void Free (void *p);
extern void ShutMeDown (int flag) __attribute__ ((__noreturn__));
extern void DeadPipe (int nothing) __attribute__ ((__noreturn__));
extern char *copy_string (char **target, const char *src);

extern void init_globals (void);
extern int allocate_managers (int num);
extern int expand_weighted_sorts (void);

extern WinData *new_windata (void);
extern void free_windata (WinData *p);
extern int check_win_complete (WinData *p);
extern WinManager *figure_win_manager (WinData *win, Uchar mask);
extern void init_winlists (void);
extern void delete_win_hashtab (WinData *win);
extern void insert_win_hashtab (WinData *win);
extern WinData *find_win_hashtab (Ulong id);
extern void walk_hashtab (void (*func)(void *));
extern int accumulate_walk_hashtab (int (*func)(void *));
extern void print_stringlist (StringList *list);
extern void add_to_stringlist (StringList *list, char *s);
extern void update_window_stuff (WinManager *man);
extern void print_managers (void);

extern WinManager *find_windows_manager (Window win);
extern int win_in_viewport (WinData *win);
