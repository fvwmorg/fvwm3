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

/* I hate things defined in 2 places. */
#ifdef IamTheMain
#define EXTERN
#else
#define EXTERN extern
#endif


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
  Font dt_font;
  XFontStruct *dt_font_struct;
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


/* There is one "form". Start of attempt to change that. */
typedef struct _form {
#if 0
  struct _form *next;                   /* dje hmm. a linklist of forms? */
#endif
  Item def_button;
  int grab_server;                      /* set during parse used on display */
  int server_grabbed;                   /* first time switch */
  int gx, gy, have_geom;
  int warp_pointer;
  int max_width, total_height;          /* frame size */
  unsigned long screen_background;
  Item *cur_input;                      /* current input during parse and run */
  Item *first_input;                    /* forms first input item during parse*/
  Item *last_error;                     /* fvwm2 error message display area */
  DrawTable *roots_dt;                  /* root draw tables linklist */
  int abs_cursor;
  int rel_cursor;
  Window frame;
  int padVText;                         /* vert space for text item */
  char *leading;                        /* part of command to match for data */
  char *file_to_read;                   /* file to read for data */
} Form;

EXTERN Form cur_form;                   /* current form */
#define CF cur_form                     /* I may want to undo this... */

  /* Globals: */
/* These aren't customizable as to shape or color, so for now, they aren't
   in the "form" structure */
EXTERN Cursor xc_ibeam, xc_hand;

/* Link list roots */
EXTERN Item *root_item_ptr;             /* pointer to root of item list */
#ifdef IamTheMain
EXTERN Line root_line = {&root_line,    /* ->next */
                         0,             /* number of items */
                         L_CENTER,0,0,  /* justify, size x/y */
                         0,             /* current array size */
                         0};            /* items ptr */
EXTERN Line *cur_line = &root_line;            /* curr line in parse rtns */
EXTERN char preload_yorn='n';           /* init to non-preload */
#else
EXTERN Line root_line;
EXTERN Line *cur_line;
EXTERN char preload_yorn;
#endif
EXTERN Item *item;                             /* current during parse */
EXTERN Item *cur_sel, *cur_button;             /* current during parse */
EXTERN Display *dpy;
EXTERN Graphics *G;
EXTERN int fd_x;                  /* fd for X connection */
EXTERN Window root, ref;
EXTERN int screen;


/* Font/color stuff
   The colors are:
   . defaults are changed by commands during parse
   . copied into items during parse
   . displayed in the customization dialog
   */
enum { c_bg, c_fg, c_item_bg, c_item_fg, c_itemlo, c_itemhi };
#ifdef IamTheMain
EXTERN char *color_names[4] = {
  "Light Gray", "Black", "Gray50", "Wheat"
};
#else
EXTERN char *color_names[4];
#endif
/*
  Its hard to tell the difference between the txt background color and
  the screen background.  Perhaps there should be a new command for
  the text background...dje */
#ifdef IamTheMain
EXTERN char *screen_background_color = "Light Gray";
EXTERN char bg_state = 'd';                    /* in default state */
  /* d = default state */
  /* s = set by command (must be in "d" state for first "back" cmd to set it) */
  /* u = used (color allocated, too late to accept "back") */
#else
EXTERN char *screen_background_color;
EXTERN char bg_state;
#endif
enum { f_text, f_input, f_button };
#ifdef IamTheMain
EXTERN char *font_names[3] = {
  "8x13bold", "8x13bold", "8x13bold"    /* dje */
/*   "fixed", */
/*   "fixed", */
/*   "fixed" */
};

EXTERN char endDefaultsRead = 'n';
#else
EXTERN char *font_names[3];
EXTERN char endDefaultsRead;
#endif


/* From FvwmAnimate start */
EXTERN char *MyName;
EXTERN int MyNameLen;
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

EXTERN int Channel[2];

/* From FvwmAnimate end */

/* prototypes */
void ReadXServer();                     /* ReadXServer.c */
void RedrawText(Item *item);            /* FvwmForm.c */
void RedrawItem (Item *item, int click); /* FvwmForm.c */
void DoCommand (Item *cmd);             /* FvwmForm.c */
int FontWidth (XFontStruct *xfs);       /* FvwmForm.c */
void RedrawFrame ();                    /* FvwmForm.c */
char * ParseCommand (int, char *, char, int *, char **s); /* ParseCommand.c */

void DeadPipe(int nonsense);            /* FvwmForm.c */

#endif

