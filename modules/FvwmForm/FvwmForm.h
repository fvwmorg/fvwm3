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

#ifndef FVWMFORM_H
#define FVWMFORM_H

/*  Modification History

 Created 12/20/98 Dan Espen:

 - FvwmForm.c got too big for my home machine to deal with.

 */

#include "libs/Flocale.h"               /* for font definition stuff */
/*
 * This next stuff should be more specific and customizable.
 * For example padVText (above) was one of the things TXT_SPC
 * controlled. (dje)
 */
#define TEXT_SPC    3
/* This one, box_spc would have to be saved as part of an "item".
   It's used for input boxes, buttons, and I think, choices.
   dje */
#define BOX_SPC     3
/* Item_hspc is only used during the "massage phase"
   has to be saved in items. */
#define ITEM_HSPC   10
#define ITEM_VSPC   5

/* Control the areas that are realloced in chunks: */
#define ITEMS_PER_EXPANSION 32
#define CHOICES_PER_SEL_EXPANSION 8
#define BUTTON_COMMAND_EXPANSION 8

#define I_TEXT          1
#define I_INPUT         2
#define I_SELECT        3
#define I_CHOICE        4
#define I_BUTTON        5

#define IS_SINGLE       1
#define IS_MULTIPLE     2

#define IB_CONTINUE     1
#define IB_RESTART      2
#define IB_QUIT         3

/* There is a "drawtable" for each item drawn in a form */
typedef struct _drawtable {
  struct _drawtable *dt_next;           /* link list, circle */
  char *dt_font_name;                   /* font name */
  char *dt_color_names[4];
  int dt_used;                          /* 1=used as text, 2 used as item */
  unsigned long dt_colors[6];             /* text fore/back
                                           button/choice/input fore/back
                                           hilite and shadow (item only)
                                           Note, need all 6 because choices
                                           use them all */
  GC dt_GC;                             /* graphic ctx used for text */
  GC dt_item_GC;                        /* graphic ctx used for graphics */
  FlocaleFont *dt_Ffont;                /* Fvwm font structure */
} DrawTable;

/* An "item" is something in a form. Part of the structure is common
   for all item types, the rest of the structure is a union. */
typedef union _item {
  int type;                /* item type, one of I_TEXT .. I_BUTTON */
  struct _head {           /* common header */
    int type;
    union _item *next;                  /* for all items linklist */
    int win;               /* X window id */
    char *name;            /* identifier name */
    int size_x, size_y;    /* size of bounding box */
    int pos_x, pos_y;      /* position of top-left corner */
    DrawTable *dt_ptr;                  /* only used for items that need it */
  } header;

  struct {                 /* I_TEXT */
    struct _head head;
    int n;                 /* string length */
    char *value;           /* string to display */
    unsigned long color;                /* color of text */
  } text;

  struct {                 /* I_INPUT */
    struct _head head;
    int buf;               /* input string buffer */
    int n;                 /* string length */
    char *value;           /* input string */
    char *init_value;      /* default string */
    char *blanks;          /* blank string */
    int size;              /* input field size */
    int left;              /* position of the left-most displayed char */
    int o_cursor;          /* store relative cursor position */
    union _item *next_input;            /* a ring of input fields */
    union _item *prev_input;            /* for tabbing */
    int value_history_count;            /* count of input history */
    int value_history_yankat;           /* yank point between restarts */
    char **value_history_ptr;           /* curr insertion point */
  } input;
  struct {                 /* I_SELECT */
    struct _head head;
    int key;               /* one of IS_MULTIPLE, IS_SINGLE */
    int n;                 /* number of choices */
    int choices_array_count;             /* current choices array size */
    union _item **choices; /* list of choices */
  } selection;
  struct {                 /* I_CHOICE */
    struct _head head;
    int on;                /* selected or not */
    int init_on;           /* initially selected or not */
    char *value;           /* value if selected */
    int n;                 /* text string length */
    char *text;            /* text string */
    union _item *sel;      /* selection it belongs to */
  } choice;
  struct {                 /* I_BUTTON */
    struct _head head;
    int key;               /* one of IB_CONTINUE, IB_RESTART, IB_QUIT */
    int n;                 /* # of commands */
    int len;               /* text length */
    char *text;            /* text string */
    int keypress;          /* short cut */
    int button_array_size;              /* current size of next array */
    char **commands;    /* Fvwm command to execute */
  } button;
} Item;

#define L_LEFT        1
#define L_RIGHT       2
#define L_CENTER      3
#define L_LEFTRIGHT   4
#define MICRO_S_FOR_10MS 10000
#define VH_SIZE 50                      /* size of value history */

/* There is one "line" structure for each line in the form.
   "Lines" point at an array of "items". */
typedef struct _line {
  struct _line *next;                   /* Pointer to next line */
  int n;                                /* number of items on the line */
  int justify;                          /* justification */
  int size_x, size_y;                   /* size of bounding rectangle */
  int item_array_size;                  /* track size */
  Item **items;                         /* ptr to array of items */
} Line;

/* Need a struct for the pointer color.
   Since colors are unsigned, we need a flag to indicate whether
   the color was ever set. */
typedef struct _ptrc {
  XColor pointer_color;
  char used;
} Ptr_color;

/* There is one "form". Start of attempt to change that. */
typedef struct _form {
#if 0
  struct _form *next;                   /* dje hmm. a linklist of forms? */
#endif
  Item def_button;
  int grab_server;                      /* set during parse used on display */
  int server_grabbed;                   /* first time switch */
  int gx, gy, have_geom;
  int xneg, yneg;
  int warp_pointer;
  int max_width, total_height;          /* frame size */
  unsigned long screen_background;
  Item *cur_input;                      /* current input during parse and run */
  Item *first_input;                    /* forms first input item during parse*/
  Item *last_error;                     /* fvwm error message display area */
  DrawTable *roots_dt;                  /* root draw tables linklist */
  int abs_cursor;
  int rel_cursor;
  Window frame;
  int padVText;                         /* vert space for text item */
  char *leading;                        /* part of command to match for data */
  char *file_to_read;                   /* file to read for data */
  char *title;                          /* form title, NULL, use alias */
  char *expand_buffer;                  /* buffer to expand commands in */
  int expand_buffer_size;               /* current size */
  char have_env_var;                    /* at least one env var on cmd line */
  Cursor pointer[3];
  Ptr_color p_c[6];                     /* fg/bg for pointers, used flag */
  int activate_on_press;                /* activate button on press */
} Form;

enum {input_pointer,button_pointer,button_in_pointer};
enum {input_fore,input_back,
      button_fore,button_back,
      button_in_fore,button_in_back};

extern Form cur_form;                   /* current form */
#define CF cur_form                     /* I may want to undo this... */

  /* Globals: */

/* Link list roots */
extern Item *root_item_ptr;             /* pointer to root of item list */
extern Line root_line;
extern Line *cur_line;
extern char preload_yorn;
extern Item *item;                             /* current during parse */
extern Item *cur_sel, *cur_button;             /* current during parse */
extern Display *dpy;
extern int fd_x;                  /* fd for X connection */
extern Window root, ref;
extern int screen;

enum { c_bg, c_fg, c_item_bg, c_item_fg, c_itemlo, c_itemhi };
extern char *color_names[4];
extern char bg_state;
extern char endDefaultsRead;
enum { f_text, f_input, f_button };
extern char *font_names[3];
extern char *screen_background_color;

extern int colorset;
extern int itemcolorset;

/* From FvwmAnimate start */
extern char *MyName;
extern int MyNameLen;
/* here is the old double parens trick. */
/* #define DEBUG */
#ifdef DEBUG
#define myfprintf(X) \
  fprintf X;\
  fflush (stderr);
#else
#define myfprintf(X)
#endif

#define tempmyfprintf(X) \
  fprintf X;\
  fflush (stderr);

extern int Channel[2];

/* From FvwmAnimate end */

/* prototypes */
void ReadXServer();                     /* ReadXServer.c */
void RedrawText(Item *item);            /* FvwmForm.c */
void RedrawItem (Item *item, int click); /* FvwmForm.c */
void UpdateRootTransapency(void);        /* FvwmForm.c */
void DoCommand (Item *cmd);             /* FvwmForm.c */
int FontWidth (XFontStruct *xfs);       /* FvwmForm.c */
void RedrawFrame ();                    /* FvwmForm.c */
char * ParseCommand (int, char *, char, int *, char **s); /* ParseCommand.c */

void DeadPipe(int nonsense);            /* FvwmForm.c */

#endif
