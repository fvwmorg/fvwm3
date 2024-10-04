/* -*-c-*- */
#ifndef IN_FVWMICONMAN_H
#define IN_FVWMICONMAN_H

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "libs/ftime.h"

#include "libs/fvwmlib.h"
#include "libs/Flocale.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"
#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/modifiers.h"

#include "debug.h"
#include "fvwm/fvwm.h"
#include "libs/vpacket.h"
#include "libs/FTips.h"

#ifndef DEFAULT_ACTION
#define DEFAULT_ACTION "Iconify"
#endif

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

extern void PrintMemuse(void);

typedef unsigned long Ulong;
typedef unsigned char Uchar;

typedef signed char Schar;

typedef struct Resolution {
	/* Page/Desk/Screen to show. -1 is current. */
	int pagex_n;
	int pagey_n;
	int desk_n;
	bool invert;
	enum {
		SHOW_ALL = 0x00,
		SHOW_DESK = 0x01,
		SHOW_PAGE = 0x02,
		SHOW_SCREEN = 0x04,
		NO_SHOW_DESK = 0x10,
		NO_SHOW_PAGE = 0x20,
		NO_SHOW_SCREEN = 0x40,
	} type;
} Resolution;

typedef enum {
	REVERSE_NONE,
	REVERSE_ICON,
	REVERSE_NORMAL,
} Reverse;

typedef enum {
	BUTTON_FLAT,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_EDGEUP,
	BUTTON_EDGEDOWN,
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
	DEFAULT = 0,
	FOCUS_CONTEXT = 1,
	SELECT_CONTEXT = 2,
	/* had better be FOCUS_CONTEXT | SELECT_CONTEXT */
	FOCUS_SELECT_CONTEXT = 3,
	PLAIN_CONTEXT = 4,
	TITLE_CONTEXT = 5,
	ICON_CONTEXT = 6,
	ICON_SELECT_CONTEXT = 7,
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
	PrevButton,
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
	rectangle icon_g;
	rectangle real_g; /* geometry of the client possibliy shaded */
	Ulong app_id;
	/* Ulong fvwm_flags; */
	window_flags flags;
	struct win_data *win_prev, *win_next;
	struct win_manager *manager;
	bool app_id_set;
	bool geometry_set;
	Uchar complete;

	/* this data must be freed */
	char *display_string; /* what gets shown in the manager window */
	char *resname;
	char *classname;
	char *titlename;
	char *iconname;
	char *visible_name;
	char *visible_icon_name;
	char *monitor;
} WinData;

#define WINDATA_ICONIFIED(win) ((win)->iconified)

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
		int ex, ey, ew, eh; /* expose damage relatively the main win */
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
	GROW_FIXED = 64,
} GrowDirection;

typedef struct {
	/* Things which we can change go in here.
	 * Things like border width go in WinManager */
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
	SortWeighted,      /* custom sort order */
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
	char *scr; /* RandR monitor name */
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
	int max_button_width;
	int max_button_width_columns;
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
  	Uchar showonlyfocused;
	Uchar shownoiconic;
	Uchar showtransient;
	rectangle managed_g;    /* dimensions of managed screen portion */
	int relief_thickness;	/* relief thickness for each non-flat button */
#define TIPS_NEVER  0
#define TIPS_ALWAYS 1
#define TIPS_NEEDED 2
	int tips;
	char *tips_fontname;
	char *tips_formatstring;
	ftips_config *tips_conf;

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
	Button *select_button, *focus_button, *tipped_button;
	Uchar window_mapped, drawn_mapping;
	ShapeState shape, drawn_shape;
	ButtonArray buttons;

	/* fvwm state */
	int we_are_drawing, configures_expected;
	Bool swallowed;
	Window swallower_win;
	struct
	{
		unsigned is_shaded : 1;
		unsigned needs_resize_after_unshade : 1;
	} flags;
} WinManager;

#define MANAGER_EMPTY(man) ((man)->buttons.num_windows == 0)

typedef struct {
	Ulong desknum;
	Ulong x, y;             /* of the view window */
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
extern int fvwm_fd[2];
extern int x_fd;
extern Display *theDisplay;
extern char *MyName;
extern char *Module;
extern int ModuleLen;
extern ContextDefaults contextDefaults[];
extern int mods_unused;

extern void ReadFvwmPipe(void);
extern void Free (void *p);
extern void ShutMeDown (int flag) __attribute__ ((__noreturn__));
extern RETSIGTYPE DeadPipe (int nothing);
extern char *copy_string (char **target, const char *src);

extern void init_globals (void);
extern int allocate_managers (int num);
extern int expand_weighted_sorts (void);

extern WinData *new_windata (void);
extern void free_windata (WinData *p);
extern int check_win_complete (WinData *p);
int check_resolution(WinManager *man, WinData *win);
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
extern WinData *id_to_win(Ulong id);

#endif /* IN_FVWMICONMAN_H */
