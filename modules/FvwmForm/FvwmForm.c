/* FvwmForm is original work of Thomas Zuwei Feng.
 *
 * Copyright Feb 1995, Thomas Zuwei Feng.  No guarantees or warantees are
 * provided or implied in any way whatsoever.  Use this program at your own
 * risk.  Permission to use, modify, and redistribute this program is hereby
 * given, provided that this copyright is kept intact.
 */
#include "config.h"

#include "../../libs/fvwmlib.h"

#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

void dummy () {
}

#ifdef DEBUG
#define fprintf fprintf
#else
#define fprintf dummy
#endif


#define TEXT_SPC    3
#define BOX_SPC     3
#define ITEM_HSPC   10
#define ITEM_VSPC   5

/* tba: use dynamic buffer expanding */
#define MAX_LINES 16
#define MAX_ITEMS 64
#define ITEMS_PER_LINE 64
#define CHOICES_PER_SEL 64

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

typedef union _item {
  int type;                /* item type, one of I_TEXT .. I_BUTTON */
  struct _head {           /* common header */
    int type;
    int win;               /* X window id */
    char *name;            /* identifier name */
    int size_x, size_y;    /* size of bounding box */
    int pos_x, pos_y;      /* position of top-left corner */
  } header;
  struct {                 /* I_TEXT */
    struct _head head;
    int n;                 /* string length */
    char *value;           /* string to display */
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
  } input;
  struct {                 /* I_SELECT */
    struct _head head;
    int key;               /* one of IS_MULTIPLE, IS_SINGLE */
    int n;                 /* number of choices */
    union _item **choices; /* list of choices */
  } select;
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
    /* Fvwm command to execute */
    char **commands;
  } button;
} Item;


#define L_LEFT        1
#define L_RIGHT       2
#define L_CENTER      3
#define L_LEFTRIGHT   4

typedef struct _line {
  int n;                   /* number of items on the line */
  int justify;             /* justification */
  int size_x, size_y;      /* size of bounding rectangle */
  Item **items;             /* list of items */
} Line;


/* global variables */
char *prog_name;           /* program name, e.g. FvwmForm */

int fd_in;                 /* fd for Fvwm->Module packets */
int fd_out;                /* fd for Module->Fvwm packets */
int fd[2]; /* pipe pair */
int fd_err;
FILE *fp_err;

Line lines[MAX_LINES];
int n_lines;
Item items[MAX_ITEMS];
int n_items;
Item def_button;

int grab_server = 0, server_grabbed = 0;
int gx, gy, geom = 0;
int warp_pointer = 0;

Display *dpy;
int fd_x;                  /* fd for X connection */

Window root, frame, ref;
Colormap d_cmap;
int screen;
int scr_depth;

int max_width, total_height;  /* frame size */

enum { c_back, c_fore, c_itemback, c_itemfore, c_itemlo, c_itemhi };
char *color_names[4] = {
  "Light Gray", "Black", "Gray50", "Wheat"
};
unsigned long colors[6];

enum { f_text, f_input, f_button };
char *font_names[3] = {
  "fixed",
  "fixed",
  "fixed"
};
Font fonts[3];
XFontStruct *xfs[3];

Cursor xc_ibeam, xc_hand;

GC gc_text, gc_input, gc_button;

Item *cur_text;
int abs_cursor;
int rel_cursor;

static char *buf;
static int N = 8;

/* copy a string until '\0', or up to n chars, and delete trailing spaces */
char *CopyNString (char *cp, int n)
{
  char *dp, *bp;
  if (n == 0)
    n = strlen(cp);
  bp = dp = (char *)malloc(n+1);
  while (n-- > 0)
    *dp++ = *cp++;
  while (isspace(*(--dp)));
  *(++dp) = '\0';
  return bp;
}

/* copy a string until '"', or '\n', or '\0' */
char *CopyQuotedString (char *cp)
{
  char *dp, *bp, c;
  bp = dp = (char *)malloc(strlen(cp) + 1);
  while (1) {
    switch (c = *(cp++)) {
    case '\\':
      *(dp++) = *(cp++);
      break;
    case '\"':
    case '\n':
    case '\0':
      *dp = '\0';
      return bp;
      break;
    default:
      *(dp++) = c;
      break;
    }
  }
}

/* copy a string until the first space */
char *CopySolidString (char *cp)
{
  char *dp, *bp, c;
  bp = dp = (char *)malloc(strlen(cp) + 1);
  while (1) {
    c = *(cp++);
    if (c == '\\') {
      *(dp++) = '\\';
      *(dp++) = *(cp++);
    } else if (isspace(c) || c == '\0') {
      *dp = '\0';
      return bp;
    } else
      *(dp++) = c;
  }
}

/* get the font height */
int FontHeight (XFontStruct *xfs)
{
  return (xfs->ascent + xfs->descent);
}

/* get the font width, for fixed-width font only */
int FontWidth (XFontStruct *xfs)
{
  return (xfs->per_char[0].width);
}

/* read the configuration file */
void ReadConfig ()
{
  FILE *fopen();
  int prog_name_len, i, j, l, extra;
  char *line_buf;
  char *cp;
  Line *cur_line, *line;
  Item *item, *cur_sel, *cur_button;

#define AddToLine(item) { cur_line->items[cur_line->n++] = item; cur_line->size_x += item->header.size_x; if (cur_line->size_y < item->header.size_y) cur_line->size_y = item->header.size_y; }

  n_items = 0;
  n_lines = 0;

  /* default line in case the first *FFLine is missing */
  lines[0].n = 0;
  lines[0].justify = L_CENTER;
  lines[0].size_x = lines[0].size_y = 0;
  lines[0].items = (Item **)malloc(sizeof(Item *) * ITEMS_PER_LINE);
  cur_line = lines;

  /* default button is for initial functions */
  cur_button = &def_button;
  def_button.button.n = 0;
  def_button.button.commands = (char **)malloc(sizeof(char *) * MAX_ITEMS);
  def_button.button.key = IB_CONTINUE;

  /* default fonts in case the *FFFont's are missing */
  xfs[f_text] = xfs[f_input] = xfs[f_button] =
    GetFontOrFixed(dpy, "fixed");
  fonts[f_text] = fonts[f_input] = fonts[f_button] = xfs[f_text]->fid;

  prog_name_len = strlen(prog_name);

  while (GetConfigLine(fd,&line_buf),line_buf) {
    cp = line_buf;
    while (isspace(*cp)) cp++;  /* skip blanks */
    if (*cp != '*') continue;
    if (strncmp(++cp, prog_name, prog_name_len) != 0) continue;
    cp += prog_name_len;
    /* at this point we have recognized "*FvwmForm" */
    if (strncmp(cp, "GrabServer", 10) == 0) {
      grab_server = 1;
      continue;
    }
    else if (strncmp(cp, "WarpPointer", 11) == 0) {
      warp_pointer = 1;
    }
    else if (strncmp(cp, "Position", 8) == 0) {
      cp += 8;
      geom = 1;
      while (isspace(*cp)) cp++;
      gx = atoi(cp);
      while (!isspace(*cp)) cp++;
      while (isspace(*cp)) cp++;
      gy = atoi(cp);
      fprintf(fp_err, "Position @ (%d, %d)\n", gx, gy);
      continue;
    }
    else if (strncmp(cp, "Fore", 4) == 0) {
      cp += 4;
      while (isspace(*cp)) cp++;
      color_names[c_fore] = CopyNString(cp, 0);
      fprintf(fp_err, "ColorFore: %s\n", color_names[c_fore]);
      continue;
    } else if (strncmp(cp, "Back", 4) == 0) {
      cp += 4;
      while (isspace(*cp)) cp++;
      color_names[c_back] = CopyNString(cp, 0);
      fprintf(fp_err, "ColorBack: %s\n", color_names[c_back]);
      continue;
    } else if (strncmp(cp, "ItemFore", 8) == 0) {
      cp += 8;
      while (isspace(*cp)) cp++;
      color_names[c_itemfore] = CopyNString(cp, 0);
      fprintf(fp_err, "ColorItemFore: %s\n", color_names[c_itemfore]);
      continue;
    } else if (strncmp(cp, "ItemBack", 8) == 0) {
      cp += 8;
      while (isspace(*cp)) cp++;
      color_names[c_itemback] = CopyNString(cp, 0);
      fprintf(fp_err, "ColorItemBack: %s\n", color_names[c_itemback]);
      continue;
    } else if (strncmp(cp, "Font", 4) == 0) {
      cp += 4;
      while (isspace(*cp)) cp++;
      font_names[f_text] = CopyNString(cp, 0);
      fprintf(fp_err, "Font: %s\n", font_names[f_text]);
      xfs[f_text] = GetFontOrFixed(dpy, font_names[f_text]);
      fonts[f_text] = xfs[f_text]->fid;
      continue;
    } else if (strncmp(cp, "ButtonFont", 10) == 0) {
      cp += 10;
      while (isspace(*cp)) cp++;
      font_names[f_button] = CopyNString(cp, 0);
      fprintf(fp_err, "ButtonFont: %s\n", font_names[f_button]);
      xfs[f_button] = GetFontOrFixed(dpy, font_names[f_button]);
      fonts[f_button] = xfs[f_button]->fid;
      continue;
    } else if (strncmp(cp, "InputFont", 9) == 0) {
      cp += 9;
      while (isspace(*cp)) cp++;
      font_names[f_input] = CopyNString(cp, 0);
      fprintf(fp_err, "InputFont: %s\n", font_names[f_input]);
      xfs[f_input] = GetFontOrFixed(dpy, font_names[f_input]);
      fonts[f_input] = xfs[f_input]->fid;
      continue;
    } else if (strncmp(cp, "Line", 4) == 0) {
      cp += 4;
      cur_line = lines + n_lines++;
      while (isspace(*cp)) cp++;
      if (strncmp(cp, "left", 4) == 0)
	cur_line->justify = L_LEFT;
      else if (strncmp(cp, "right", 5) == 0)
	cur_line->justify = L_RIGHT;
      else if (strncmp(cp, "center", 6) == 0)
	cur_line->justify = L_CENTER;
      else
	cur_line->justify = L_LEFTRIGHT;
      cur_line->n = 0;
      cur_line->items = (Item **)malloc(sizeof(Item *) * ITEMS_PER_LINE);
      continue;
    } else if (strncmp(cp, "Text", 4) == 0) {
/* syntax: *FFText "<text>" */
      cp += 4;
      item = items + n_items++;
      item->type = I_TEXT;
      item->header.name = "";
      while (isspace(*cp)) cp++;
      if (*cp == '\"')
	item->text.value = CopyQuotedString(++cp);
      else
	item->text.value = "";
      item->text.n = strlen(item->text.value);
      item->header.size_x = XTextWidth(xfs[f_text], item->text.value,
				     item->text.n) + 2 * TEXT_SPC;
      item->header.size_y = FontHeight(xfs[f_text]) + 2 * TEXT_SPC;
      fprintf(fp_err, "Text \"%s\" [%d, %d]\n", item->text.value,
	     item->header.size_x, item->header.size_y);
      AddToLine(item);
      continue;
    }
    else if (strncmp(cp, "Input", 5) == 0) {
/* syntax: *FFInput <name> <size> "<init_value>" */
      cp += 5;
      item = items + n_items++;
      item->type = I_INPUT;
      while (isspace(*cp)) cp++;
      item->header.name = CopySolidString(cp);
      cp += strlen(item->header.name);
      while (isspace(*cp)) cp++;
      item->input.size = atoi(cp);
      while (!isspace(*cp)) cp++;
      while (isspace(*cp)) cp++;
      if (*cp == '\"')
	item->input.init_value = CopyQuotedString(++cp);
      else
	item->input.init_value = "";
      item->input.blanks = (char *)malloc(item->input.size);
      for (j = 0; j < item->input.size; j++)
	item->input.blanks[j] = ' ';
      item->input.buf = strlen(item->input.init_value) + 1;
      item->input.value = (char *)malloc(item->input.buf);
      item->header.size_x = FontWidth(xfs[f_input]) * item->input.size
	+ 2 * TEXT_SPC + 2 * BOX_SPC;
      item->header.size_y = FontHeight(xfs[f_input]) + 3 * TEXT_SPC
	+ 2 * BOX_SPC;
      fprintf(fp_err, "Input, %s, [%d], \"%s\"\n", item->header.name,
	      item->input.size, item->input.init_value);
      AddToLine(item);
    }
    else if (strncmp(cp, "Selection", 9) == 0) {
/* syntax: *FFSelection <name> single | multiple */
      cp += 9;
      cur_sel = items + n_items++;
      cur_sel->type = I_SELECT;
      while (isspace(*cp)) cp++;
      cur_sel->header.name = CopySolidString(cp);
      cp += strlen(cur_sel->header.name);
      while (isspace(*cp)) cp++;
      if (strncmp(cp, "multiple", 8) == 0)
	cur_sel->select.key = IS_MULTIPLE;
      else
	cur_sel->select.key = IS_SINGLE;
      cur_sel->select.n = 0;
      cur_sel->select.choices =
	(Item **)malloc(sizeof(Item *) * CHOICES_PER_SEL);
      continue;
    } else if (strncmp(cp, "Choice", 6) == 0) {
/* syntax: *FFChoice <name> <value> [on | _off_] ["<text>"] */
      cp += 6;
      item = items + n_items++;
      item->type = I_CHOICE;
      while (isspace(*cp)) cp++;
      item->header.name = CopySolidString(cp);
      cp += strlen(item->header.name);
      while (isspace(*cp)) cp++;
      item->choice.value = CopySolidString(cp);
      cp += strlen(item->choice.value);
      while (isspace(*cp)) cp++;
      if (strncmp(cp, "on", 2) == 0)
	item->choice.init_on = 1;
      else
	item->choice.init_on = 0;
      while (!isspace(*cp)) cp++;
      while (isspace(*cp)) cp++;
      if (*cp == '\"')
	item->choice.text = CopyQuotedString(++cp);
      else
	item->choice.text = "";
      item->choice.n = strlen(item->choice.text);
      item->choice.sel = cur_sel;
      cur_sel->select.choices[cur_sel->select.n++] = item;
      item->header.size_y = FontHeight(xfs[f_text]) + 2 * TEXT_SPC;
      item->header.size_x = FontHeight(xfs[f_text]) + 4 * TEXT_SPC +
	XTextWidth(xfs[f_text], item->choice.text, item->choice.n);
      fprintf(fp_err, "Choice %s, \"%s\", [%d, %d]\n", item->header.name,
	     item->choice.text, item->header.size_x, item->header.size_y);
      AddToLine(item);
      continue;
    } else if (strncmp(cp, "Button", 6) == 0) {
/* syntax: *FFButton continue | restart | quit "<text>" */
      cp += 6;
      item = items + n_items++;
      item->type = I_BUTTON;
      item->header.name = "";
      while (isspace(*cp)) cp++;
      if (strncmp(cp, "restart", 7) == 0)
	item->button.key = IB_RESTART;
      else if (strncmp(cp, "quit", 4) == 0)
	item->button.key = IB_QUIT;
      else
	item->button.key = IB_CONTINUE;
      while (!isspace(*cp)) cp++;
      while (isspace(*cp)) cp++;
      if (*cp == '\"') {
	item->button.text = CopyQuotedString(++cp);
	cp += strlen(item->button.text) + 1;
	while (isspace(*cp)) cp++;
      } else
	item->button.text = "";
      if (*cp == '^')
	item->button.keypress = *(++cp) - '@';
      else if (*cp == 'F')
	item->button.keypress = 256 + atoi(++cp);
      else
	item->button.keypress = -1;
      item->button.len = strlen(item->button.text);
      item->button.n = 0;
      item->button.commands = (char **)malloc(sizeof(char *) * MAX_ITEMS);
      item->header.size_y = FontHeight(xfs[f_button]) + 2 * TEXT_SPC
	+ 2 * BOX_SPC;
      item->header.size_x = 2 * TEXT_SPC + 2 * BOX_SPC
	+ XTextWidth(xfs[f_button], item->button.text, item->button.len);
      AddToLine(item);
      cur_button = item;
      continue;
    } else if (strncmp(cp, "Command", 7) == 0) {
/* syntax: *FFCommand <command> */
      cp += 7;
      while (isspace(*cp)) cp++;
      cur_button->button.commands[cur_button->button.n++] =
	CopyNString(cp, 0);
    }
  }  /* end of switch() */
  /* get the geometry right */
  max_width = 0;
  total_height = ITEM_VSPC;
  for (l = 0; l < n_lines; l++) {
    line = lines + l;
    for (i = 0; i < line->n; i++) {
      line->items[i]->header.pos_y = total_height;
      if (line->items[i]->header.size_y < line->size_y)
        line->items[i]->header.pos_y += (line->size_y - line->items[i]->header.size_y) / 2 + 1 ;
    }
    total_height += ITEM_VSPC + line->size_y;
    line->size_x += (line->n + 1) * ITEM_HSPC;
    if (line->size_x > max_width)
      max_width = line->size_x;
  }
  for (l = 0; l < n_lines; l++) {
    int width;
    line = lines + l;
    fprintf(fp_err, "Line[%d], %d, %d items\n", l, line->justify, line->n);
    switch (line->justify) {
    case L_LEFT:
      width = ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_RIGHT:
      width = max_width - line->size_x + ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_CENTER:
      width = (max_width - line->size_x) / 2 + ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	fprintf(fp_err, "Line[%d], Item[%d] @ (%d, %d)\n", l, i,
	       line->items[i]->header.pos_x, line->items[i]->header.pos_y);
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_LEFTRIGHT:
    /* count the number of inputs on the line - the extra space will be
     * shared amongst these if there are any, otherwise it will be added
     * as space in between the elements
     */
      extra = 0 ;
      for (i = 0 ; i < line->n ; i++) {
        if (line->items[i]->type == I_INPUT)
          extra++ ;
      }
      if (extra == 0) {
        if (line->n < 2) {  /* same as L_CENTER */
	  width = (max_width - line->size_x) / 2 + ITEM_HSPC;
	  for (i = 0; i < line->n; i++) {
	    line->items[i]->header.pos_x = width;
	    width += ITEM_HSPC + line->items[i]->header.size_x;
	  }
        } else {
	  extra = (max_width - line->size_x) / (line->n - 1);
	  width = ITEM_HSPC;
	  for (i = 0; i < line->n; i++) {
	    line->items[i]->header.pos_x = width;
	    width += ITEM_HSPC + line->items[i]->header.size_x + extra;
	  }
        }
      } else {
        extra = (max_width - line->size_x) / extra ;
        width = ITEM_HSPC ;
        for (i = 0 ; i < line->n ; i++) {
          line->items[i]->header.pos_x = width ;
          if (line->items[i]->type == I_INPUT)
            line->items[i]->header.size_x += extra ;
          width += ITEM_HSPC + line->items[i]->header.size_x ;
        }
      }
      break;
    }
  }
}

#define MAX_INTENSITY 65535

/* allocate color cells */
void GetColors ()
{
  Visual* visual = DefaultVisual(dpy, screen);
  XColor xc_item;
  int red, green, blue, tmp1, tmp2 ;
  if (scr_depth < 8) {
    colors[c_back] = colors[c_itemback] = WhitePixel(dpy, screen);
    colors[c_fore] = colors[c_itemfore] = colors[c_itemlo] = colors[c_itemhi]
      = BlackPixel(dpy, screen);
  } else if (visual->class == TrueColor ||
                        visual->class == StaticColor ||
                        visual->class == StaticGray) {
    if (XParseColor(dpy, d_cmap, color_names[c_fore], &xc_item) &&
               XAllocColor(dpy, d_cmap, &xc_item))
         colors[c_fore] = xc_item.pixel;
       else
         colors[c_fore] = BlackPixel(dpy, screen);

    if (XParseColor(dpy, d_cmap, color_names[c_back], &xc_item) &&
               XAllocColor(dpy, d_cmap, &xc_item))
         colors[c_back] = xc_item.pixel;
       else
         colors[c_back] = WhitePixel(dpy, screen);

    if (XParseColor(dpy, d_cmap, color_names[c_itemfore], &xc_item) &&
               XAllocColor(dpy, d_cmap, &xc_item))
         colors[c_itemfore] = xc_item.pixel;
       else
         colors[c_itemfore] = BlackPixel(dpy, screen);

    if (XParseColor(dpy, d_cmap, color_names[c_itemback], &xc_item) &&
               XAllocColor(dpy, d_cmap, &xc_item))
         colors[c_itemback] = xc_item.pixel;
       else
         colors[c_itemback] = WhitePixel(dpy, screen);

    InitPictureCMap(dpy,root);          /* for shadow routines */
    colors[c_itemlo] = GetShadow(colors[c_itemback]); /* alloc shadow */
    colors[c_itemhi] = GetHilite(colors[c_itemback]); /* alloc shadow */
  } else if (!XAllocColorCells(dpy, d_cmap, 0, NULL, 0, colors, 6)) {
    colors[c_back] = colors[c_itemback] = WhitePixel(dpy, screen);
    colors[c_fore] = colors[c_itemfore] = colors[c_itemlo] = colors[c_itemhi]
      = BlackPixel(dpy, screen);
  } else {
    XStoreNamedColor(dpy, d_cmap, color_names[c_fore], colors[c_fore],
		     DoRed | DoGreen | DoBlue);
    XStoreNamedColor(dpy, d_cmap, color_names[c_back], colors[c_back],
		     DoRed | DoGreen | DoBlue);
    XStoreNamedColor(dpy, d_cmap, color_names[c_itemfore],
		     colors[c_itemfore], DoRed | DoGreen | DoBlue);
    XStoreNamedColor(dpy, d_cmap, color_names[c_itemback],
		     colors[c_itemback], DoRed | DoGreen | DoBlue);
    XParseColor(dpy, d_cmap, color_names[c_itemback], &xc_item);
    red = (int) xc_item.red ;
    green = (int) xc_item.green ;
    blue = (int) xc_item.blue ;
    xc_item.red = (60 * red) / 100 ;
    xc_item.green = (60 * green) / 100 ;
    xc_item.blue = (60 * blue) / 100 ;
    xc_item.pixel = colors[c_itemlo];
    xc_item.flags = DoRed | DoGreen | DoBlue;
    XStoreColor(dpy, d_cmap, &xc_item);
    XParseColor(dpy, d_cmap, color_names[c_itemback], &xc_item);
    tmp1 = (14 * red) / 10 ;
    if (tmp1 > MAX_INTENSITY) tmp1 = MAX_INTENSITY ;
    tmp2 = (MAX_INTENSITY + red) / 2 ;
    xc_item.red = (tmp1 > tmp2) ? tmp1 : tmp2 ;
    tmp1 = (14 * green) / 10 ;
    if (tmp1 > MAX_INTENSITY) tmp1 = MAX_INTENSITY ;
    tmp2 = (MAX_INTENSITY + green) / 2 ;
    xc_item.green = (tmp1 > tmp2) ? tmp1 : tmp2 ;
    tmp1 = (14 * blue) / 10 ;
    if (tmp1 > MAX_INTENSITY) tmp1 = MAX_INTENSITY ;
    tmp2 = (MAX_INTENSITY + blue) / 2 ;
    xc_item.blue = (tmp1 > tmp2) ? tmp1 : tmp2 ;
    xc_item.pixel = colors[c_itemhi];
    xc_item.flags = DoRed | DoGreen | DoBlue;
    XStoreColor(dpy, d_cmap, &xc_item);
  }
}

/* reset all the values */
void Restart ()
{
  int i;
  Item *item;

  cur_text = NULL;
  abs_cursor = rel_cursor = 0;
  for (i = 0; i < n_items; i++) {
    item = items + i;
    switch (item->type) {
    case I_INPUT:
      if (!cur_text)
	cur_text = item;
      item->input.n = strlen(item->input.init_value);
      strcpy(item->input.value, item->input.init_value);
      item->input.left = 0;
      item->input.o_cursor = 0;
      break;
    case I_CHOICE:
      item->choice.on = item->choice.init_on;
      break;
    }
  }
}

/* redraw the frame */
void RedrawFrame ()
{
  int i, x, y;
  Item *item;

  for (i = 0; i < n_items; i++) {
    item = items + i;
    switch (item->type) {
    case I_TEXT:
      x = item->header.pos_x + TEXT_SPC;
      y = item->header.pos_y + TEXT_SPC + xfs[f_text]->ascent;
      XDrawImageString(dpy, frame, gc_text, x, y, item->text.value,
		       item->text.n);
      break;
    case I_CHOICE:
      x = item->header.pos_x + TEXT_SPC + item->header.size_y;
      y = item->header.pos_y + TEXT_SPC + xfs[f_text]->ascent;
      XDrawImageString(dpy, frame, gc_text, x, y, item->choice.text,
		       item->choice.n);
      break;
    }
  }
}

/* redraw an item */
void RedrawItem (Item *item, int click)
{
  int dx, dy, len, x;
  static XSegment xsegs[4];

  switch (item->type) {
  case I_INPUT:
    dx = item->header.size_x - 1;
    dy = item->header.size_y - 1;
    XSetForeground(dpy, gc_button, colors[c_itemlo]);
    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    XSetForeground(dpy, gc_button, colors[c_itemhi]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    if (click) {
      x = BOX_SPC + TEXT_SPC + FontWidth(xfs[f_input]) * abs_cursor - 1;
      XSetForeground(dpy, gc_button, colors[c_itemback]);
      XDrawLine(dpy, item->header.win, gc_button,
		x, BOX_SPC, x, dy - BOX_SPC);
    }
    len = item->input.n - item->input.left;
    if (len > item->input.size)
      len = item->input.size;
    else
      XDrawImageString(dpy, item->header.win, gc_input,
		       BOX_SPC + TEXT_SPC + FontWidth(xfs[f_input]) * len,
		       BOX_SPC + TEXT_SPC + xfs[f_input]->ascent,
		       item->input.blanks, item->input.size - len);
    XDrawImageString(dpy, item->header.win, gc_input,
		     BOX_SPC + TEXT_SPC,
		     BOX_SPC + TEXT_SPC + xfs[f_input]->ascent,
		     item->input.value + item->input.left, len);
    if (item == cur_text && !click) {
      x = BOX_SPC + TEXT_SPC + FontWidth(xfs[f_input]) * abs_cursor - 1;
      XDrawLine(dpy, item->header.win, gc_input,
		x, BOX_SPC, x, dy - BOX_SPC);
    }
    break;
  case I_CHOICE:
    dx = dy = item->header.size_y - 1;
    if (item->choice.on) {
	XSetForeground(dpy, gc_button, colors[c_itemfore]);
      if (item->choice.sel->select.key == IS_MULTIPLE) {
	xsegs[0].x1 = 5, xsegs[0].y1 = 5;
	xsegs[0].x2 = dx - 5, xsegs[0].y2 = dy - 5;
	xsegs[1].x1 = 5, xsegs[1].y1 = dy - 5;
	xsegs[1].x2 = dx - 5, xsegs[1].y2 = 5;
	XDrawSegments(dpy, item->header.win, gc_button, xsegs, 2);
      } else {
	XDrawArc(dpy, item->header.win, gc_button,
		 5, 5, dx - 10, dy - 10, 0, 360 * 64);
      }
    } else
      XClearWindow(dpy, item->header.win);
    if (item->choice.on)
      XSetForeground(dpy, gc_button, colors[c_itemlo]);
    else
      XSetForeground(dpy, gc_button, colors[c_itemhi]);
    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    if (item->choice.on)
      XSetForeground(dpy, gc_button, colors[c_itemhi]);
    else
      XSetForeground(dpy, gc_button, colors[c_itemlo]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    break;
  case I_BUTTON:
    dx = item->header.size_x - 1;
    dy = item->header.size_y - 1;
    if (click)
      XSetForeground(dpy, gc_button, colors[c_itemlo]);
    else
      XSetForeground(dpy, gc_button, colors[c_itemhi]);
    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    if (click)
      XSetForeground(dpy, gc_button, colors[c_itemhi]);
    else
      XSetForeground(dpy, gc_button, colors[c_itemlo]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, gc_button, xsegs, 4);
    XSetForeground(dpy, gc_button, colors[c_itemfore]);
    XDrawImageString(dpy, item->header.win, gc_button,
		     BOX_SPC + TEXT_SPC,
		     BOX_SPC + TEXT_SPC + xfs[f_button]->ascent,
		     item->button.text, item->button.len);
    break;
  }
  XFlush(dpy);
}

void ToggleChoice (Item *item)
{
  int i;
  Item *sel = item->choice.sel;

  if (sel->select.key == IS_SINGLE) {
    if (!item->choice.on) {
      for (i = 0; i < sel->select.n; i++) {
	if (sel->select.choices[i]->choice.on) {
	  sel->select.choices[i]->choice.on = 0;
	  RedrawItem(sel->select.choices[i], 0);
	}
      }
      item->choice.on = 1;
      RedrawItem(item, 0);
    }
  } else {  /* IS_MULTIPLE */
    item->choice.on = !item->choice.on;
    RedrawItem(item, 0);
  }
}

/* do var substitution for command string */
void ParseCommand (int dn, char *sp, char end, int *dn1, char **sp1)
#define AddChar(chr) { if (dn >= N) { N *= 2; buf = (char *)realloc(buf, N); } buf[dn++] = (chr); }
{
  static char var[256];
  char c, x, *wp, *cp, *vp;
  int i, j, dn2;
  Item *item;

  while (1) {
    c = *(sp++);
    if (c == '\0' || c == end) {  /* end of substitution */
      *dn1 = dn;
      *sp1 = sp;
      return;
    } if (c == '\\') {  /* escape char */
      AddChar('\\');
      AddChar(*(sp++));
      goto next_loop;
    }
    if (c == '$') {  /* variable */
      if (*sp != '(')
	goto normal_char;
      wp = ++sp;
      vp = var;
      while (1) {
	x = *(sp++);
	if (x == '\\') {
	  *(vp++) = '\\';
	  *(vp++) = *(sp++);
	}
	else if (x == ')' || x == '?' || x == '!') {
	  *(vp++) = '\0';
	  break;
	}
	else if (!isspace(x))
	  *(vp++) = x;
      }
      for (i = 0; i < n_items; i++) {
	item = items + i;
	if (strcmp(var, item->header.name) == 0) {
	  switch (item->type) {
	  case I_INPUT:
	    if (x == ')') {
	      for (cp = item->input.value; *cp != '\0'; cp++) {
		if (*cp == '\"' || *cp == '\'' || *cp == '\\')
		  AddChar('\\');
		AddChar(*cp);
	      }
	    } else {
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	      if ((x == '?' && strlen(item->input.value) > 0) ||
		  (x == '!' && strlen(item->input.value) == 0))
		dn = dn2;
	    }
	    break;
	  case I_CHOICE:
	    if (x == ')') {
	      for (cp = item->choice.value; *cp != '\0'; cp++)
		AddChar(*cp);
	    } else {
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	      if ((x == '?' && item->choice.on) ||
		  (x == '!' && !item->choice.on))
		dn = dn2;
	    }
	    break;
	  case I_SELECT:
	    if (x != ')')
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	    AddChar(' ');
	    for (j = 0; j < item->select.n; j++) {
	      if (item->select.choices[j]->choice.on) {
		for (cp = item->select.choices[j]->choice.value;
		     *cp != '\0'; cp++)
		  AddChar(*cp);
		AddChar(' ');
	      }
	    }
	    break;
	  }
	  goto next_loop;
	}
      }
      goto next_loop;
    }
  normal_char:
    AddChar(c);
  next_loop:
      ;
  }
}

/* execute a command */
void DoCommand (Item *cmd)
{
  int i, k, dn, len;
  char *sp;

  /* pre-command */
  if (cmd->button.key == IB_QUIT)
    XWithdrawWindow(dpy, frame, screen);

  for (k = 0; k < cmd->button.n; k++) {
    /* construct command */
    ParseCommand(0, cmd->button.commands[k], '\0', &dn, &sp);
    AddChar('\0');
    fprintf(fp_err, "Final command[%d]: [%s]\n", k, buf);

    /* send command */
    write(fd_out, &ref, sizeof(Window));
    len = strlen(buf);
    write(fd_out, &len, sizeof(int));
    write(fd_out, buf, len);
    len = 1;
    write(fd_out, &len, sizeof(int));
  }

  /* post-command */
  if (cmd->button.key == IB_QUIT) {
    if (grab_server)
      XUngrabServer(dpy);
    exit(0);
  }
  if (cmd->button.key == IB_RESTART) {
    Restart();
    for (i = 0; i < n_items; i++) {
      if (items[i].type == I_INPUT) {
	XClearWindow(dpy, items[i].header.win);
	RedrawItem(items + i, 0);
      }
      if (items[i].type == I_CHOICE)
	RedrawItem(items + i, 0);
    }
  }
}

/* open the windows */
void OpenWindows ()
{
  int i, x, y;
  Item *item;
  static XColor xcf, xcb;
  static XSetWindowAttributes xswa;
  static XGCValues xgcv;
  static XWMHints wmh = { InputHint, True };
  static XSizeHints sh = { PPosition | PSize | USPosition | USSize };
  static int xgcv_mask = GCBackground | GCForeground | GCFont;

  xc_ibeam = XCreateFontCursor(dpy, XC_xterm);
  xc_hand = XCreateFontCursor(dpy, XC_hand2);
  xcf.pixel = WhitePixel(dpy, screen);
  XQueryColor(dpy, d_cmap, &xcf);
  xcb.pixel = colors[c_itemback];
  XQueryColor(dpy, d_cmap, &xcb);
  XRecolorCursor(dpy, xc_ibeam, &xcf, &xcb);

  /* the frame window first */
  if (geom) {
    if (gx >= 0)
      x = gx;
    else
      x = DisplayWidth(dpy, screen) - max_width + gx;
    if (gy >= 0)
      y = gy;
    else
      y = DisplayHeight(dpy, screen) - total_height + gy;
  } else {
    x = (DisplayWidth(dpy, screen) - max_width) / 2;
    y = (DisplayHeight(dpy, screen) - total_height) / 2;
  }
  frame = XCreateSimpleWindow(dpy, root, x, y, max_width, total_height,
			      0, BlackPixel(dpy, screen), colors[c_back]);
  XSelectInput(dpy, frame, KeyPressMask | ExposureMask);
  XStoreName(dpy, frame, prog_name);
  XSetWMHints(dpy, frame, &wmh);
  sh.x = x, sh.y = y;
  sh.width = max_width, sh.height = total_height;
  XSetWMNormalHints(dpy, frame, &sh);

  xgcv.foreground = colors[c_fore];
  xgcv.background = colors[c_back];
  xgcv.font = fonts[f_text];
  gc_text = XCreateGC(dpy, frame, xgcv_mask, &xgcv);
  xgcv.background = colors[c_itemback];
  xgcv.foreground = colors[c_itemfore];
  xgcv.font = fonts[f_input];
  gc_input = XCreateGC(dpy, frame, xgcv_mask, &xgcv);
  xgcv.font = fonts[f_button];
  gc_button = XCreateGC(dpy, frame, xgcv_mask, &xgcv);

  for (i = 0; i < n_items; i++) {
    item = items + i;
    switch (item->type) {
    case I_INPUT:
      item->header.win =
	XCreateSimpleWindow(dpy, frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_x, item->header.size_y,
			    0, colors[c_back], colors[c_itemback]);
      XSelectInput(dpy, item->header.win, ButtonPressMask | ExposureMask);
      xswa.cursor = xc_ibeam;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      break;
    case I_CHOICE:
      item->header.win =
	XCreateSimpleWindow(dpy, frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_y, item->header.size_y,
			    0, colors[c_back], colors[c_itemback]);
      XSelectInput(dpy, item->header.win, ButtonPressMask | ExposureMask);
      xswa.cursor = xc_hand;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      break;
    case I_BUTTON:
      item->header.win =
	XCreateSimpleWindow(dpy, frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_x, item->header.size_y,
			    0, colors[c_back], colors[c_itemback]);
      XSelectInput(dpy, item->header.win,
		   ButtonPressMask | ExposureMask);
      xswa.cursor = xc_hand;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      break;
    }
  }
  Restart();
  XMapRaised(dpy, frame);
  XMapSubwindows(dpy, frame);
  if (warp_pointer) {
    XWarpPointer(dpy, None, frame, 0, 0, 0, 0,
		 max_width / 2, total_height - 1);
  }
  DoCommand(&def_button);
}

/* read something from Fvwm */
void ReadFvwm ()
{
  static char buffer[32];
  int n;

  n = read(fd_in, buffer, 32);
  if (n == 0) {
    if (grab_server)
      XUngrabServer(dpy);
    exit(0);
  }
}

/* read an X event */
void ReadXServer ()
{
  static XEvent event;
  int i, old_cursor, keypress;
  Item *item, *old_item;
  KeySym ks;
  char *sp, *dp, *ep;
  static char buf[10], n;

  while (XEventsQueued(dpy, QueuedAfterReading)) {
    XNextEvent(dpy, &event);
    if (event.xany.window == frame) {
      switch (event.type) {
      case Expose:
	RedrawFrame();
	if (grab_server && !server_grabbed) {
	  if (GrabSuccess ==
	      XGrabPointer(dpy, frame, True, 0, GrabModeAsync, GrabModeAsync,
			   None, None, CurrentTime))
	    server_grabbed = 1;
	}
	break;
      case KeyPress:  /* we do text input here */
	n = XLookupString(&event.xkey, buf, 10, &ks, NULL);
	keypress = buf[0];
	fprintf(fp_err, "Keypress [%s]\n", buf);
	if (n == 0) {  /* not a regular key, translate it into one */
	  switch (ks) {
	  case XK_Home:
	  case XK_Begin:
	    buf[0] = '\001';  /* ^A */
	    break;
	  case XK_End:
	    buf[0] = '\005';  /* ^E */
	    break;
	  case XK_Left:
	    buf[0] = '\002';  /* ^B */
	    break;
	  case XK_Right:
	    buf[0] = '\006';  /* ^F */
	    break;
	  case XK_Up:
	    buf[0] = '\020';  /* ^P */
	    break;
	  case XK_Down:
	    buf[0] = '\016';  /* ^N */
	    break;
	  default:
	    if (ks >= XK_F1 && ks <= XK_F35) {
	      buf[0] = '\0';
	      keypress = 257 + ks - XK_F1;
	    } else
	      goto no_redraw;  /* no action for this event */
	  }
	}
	if (!cur_text) {  /* no text input fields */
	  for (i = 0; i < n_items; i++) {
	    item = items + i;
	    fprintf(fp_err, "Button[%d], keypress==%d\n", i,
		    item->button.keypress);
	    if (item->type == I_BUTTON && item->button.keypress == buf[0]) {
	      RedrawItem(item, 1);
	      sleep(1);
	      RedrawItem(item, 0);
	      DoCommand(item);
	      goto no_redraw;
	    }
	  }
	  break;
	}
	switch (buf[0]) {
	case '\001':  /* ^A */
	  old_cursor = abs_cursor;
	  rel_cursor = 0;
	  abs_cursor = 0;
	  cur_text->input.left = 0;
	  goto redraw_newcursor;
	  break;
	case '\005':  /* ^E */
	  old_cursor = abs_cursor;
	  rel_cursor = cur_text->input.n;
	  if ((cur_text->input.left = rel_cursor - cur_text->input.size) < 0)
	    cur_text->input.left = 0;
	  abs_cursor = rel_cursor - cur_text->input.left;
	  goto redraw_newcursor;
	  break;
	case '\002':  /* ^B */
	  old_cursor = abs_cursor;
	  if (rel_cursor > 0) {
	    rel_cursor--;
	    abs_cursor--;
	    if (abs_cursor <= 0 && rel_cursor > 0) {
	      abs_cursor++;
	      cur_text->input.left--;
	    }
	  }
	  goto redraw_newcursor;
	  break;
	case '\006':  /* ^F */
	  old_cursor = abs_cursor;
	  if (rel_cursor < cur_text->input.n) {
	    rel_cursor++;
	    abs_cursor++;
	    if (abs_cursor >= cur_text->input.size &&
		rel_cursor < cur_text->input.n) {
	      abs_cursor--;
	      cur_text->input.left++;
	    }
	  }
	  goto redraw_newcursor;
	  break;
	case '\010':  /* ^H */
	  old_cursor = abs_cursor;
	  if (rel_cursor > 0) {
	    sp = cur_text->input.value + rel_cursor;
	    dp = sp - 1;
	    for (; *dp = *sp, *sp != '\0'; dp++, sp++);
	    cur_text->input.n--;
	    rel_cursor--;
	    if (rel_cursor < abs_cursor) {
	      abs_cursor--;
	      if (abs_cursor <= 0 && rel_cursor > 0) {
		abs_cursor++;
		cur_text->input.left--;
	      }
	    } else
	      cur_text->input.left--;
	  }
	  goto redraw_newcursor;
	  break;
	case '\177':  /* DEL */
	case '\004':  /* ^D */
	  if (rel_cursor < cur_text->input.n) {
	    sp = cur_text->input.value + rel_cursor + 1;
	    dp = sp - 1;
	    for (; *dp = *sp, *sp != '\0'; dp++, sp++);
	    cur_text->input.n--;
	    goto redraw;
	  }
	  break;
	case '\013':  /* ^K */
	  cur_text->input.value[rel_cursor] = '\0';
	  cur_text->input.n = rel_cursor;
	  goto redraw;
	case '\025':  /* ^U */
	  old_cursor = abs_cursor;
	  cur_text->input.value[0] = '\0';
	  cur_text->input.n = cur_text->input.left = 0;
	  rel_cursor = abs_cursor = 0;
	  goto redraw_newcursor;
	case '\t':
	case '\n':
	case '\015':
	case '\016':  /* LINEFEED, TAB, RETURN, ^N, jump to the next field */
	  for (i = (cur_text - items) + 1; i < n_items; i++) {
	    item = items + i;
	    if (item->type == I_INPUT) {
	      old_item = cur_text;
	      old_item->input.o_cursor = rel_cursor;
	      cur_text = item;
	      RedrawItem(old_item, 1);
	      rel_cursor = item->input.o_cursor;
	      abs_cursor = rel_cursor - item->input.left;
	      goto redraw;
	    }
	  }
	  /* end of all text input fields, check for buttons */
	  for (i = 0; i < n_items; i++) {
	    item = items + i;
	    fprintf(fp_err, "Button[%d], keypress==%d\n", i,
		    item->button.keypress);
	    if (item->type == I_BUTTON && item->button.keypress == buf[0]) {
	      RedrawItem(item, 1);
	      sleep(1);
	      RedrawItem(item, 0);
	      DoCommand(item);
	      goto no_redraw;
	    }
	  }
	  /* goto the first text input field */
	  for (i = 0; i < n_items; i++) {
	    item = items + i;
	    if (item->type == I_INPUT) {
	      old_item = cur_text;
	      old_item->input.o_cursor = rel_cursor;
	      cur_text = item;
	      RedrawItem(old_item, 1);
	      rel_cursor = item->input.o_cursor;
	      abs_cursor = rel_cursor - item->input.left;
	      goto redraw;
	    }
	  }
	  break;
	default:
	  old_cursor = abs_cursor;
	  if (buf[0] >= ' ' && buf[0] < '\177') {  /* regular char */
	    if (++(cur_text->input.n) >= cur_text->input.buf) {
	      cur_text->input.buf += cur_text->input.size;
	      cur_text->input.value =
		(char *)realloc(cur_text->input.value,
				cur_text->input.buf);
	    }
	    dp = cur_text->input.value + cur_text->input.n;
	    sp = dp - 1;
	    ep = cur_text->input.value + rel_cursor;
	    for (; *dp = *sp, sp != ep; sp--, dp--);
	    *ep = buf[0];
	    rel_cursor++;
	    abs_cursor++;
	    if (abs_cursor >= cur_text->input.size) {
	      if (rel_cursor < cur_text->input.n)
		abs_cursor = cur_text->input.size - 1;
	      else
		abs_cursor = cur_text->input.size;
	      cur_text->input.left = rel_cursor - abs_cursor;
	    }
	    goto redraw_newcursor;
	  }
	  /* unrecognized key press, check for buttons */
	  for (i = 0; i < n_items; i++) {
	    item = items + i;
	    fprintf(fp_err, "Button[%d], keypress==%d\n", i,
		    item->button.keypress);
	    if (item->type == I_BUTTON && item->button.keypress == keypress) {
	      RedrawItem(item, 1);
	      sleep(1);  /* .5 seconds */
	      RedrawItem(item, 0);
	      DoCommand(item);
	      goto no_redraw;
	    }
	  }
	  break;
	}
      redraw_newcursor:
	{
	  int x, dy;
	  x = BOX_SPC + TEXT_SPC + FontWidth(xfs[f_input]) * old_cursor - 1;
	  dy = cur_text->header.size_y - 1;
	  XSetForeground(dpy, gc_button, colors[c_itemback]);
	  XDrawLine(dpy, cur_text->header.win, gc_button,
		    x, BOX_SPC, x, dy - BOX_SPC);
	}
      redraw:
	{
	  int len, x, dy;
	  len = cur_text->input.n - cur_text->input.left;
	  if (len > cur_text->input.size)
	    len = cur_text->input.size;
	  else
	    XDrawImageString(dpy, cur_text->header.win, gc_input,
			     BOX_SPC + TEXT_SPC +
			     FontWidth(xfs[f_input]) * len,
			     BOX_SPC + TEXT_SPC + xfs[f_input]->ascent,
			     cur_text->input.blanks,
			     cur_text->input.size - len);
	  XDrawImageString(dpy, cur_text->header.win, gc_input,
			   BOX_SPC + TEXT_SPC,
			   BOX_SPC + TEXT_SPC + xfs[f_input]->ascent,
			   cur_text->input.value + cur_text->input.left, len);
	  x = BOX_SPC + TEXT_SPC + FontWidth(xfs[f_input]) * abs_cursor - 1;
	  dy = cur_text->header.size_y - 1;
	  XDrawLine(dpy, cur_text->header.win, gc_input,
		    x, BOX_SPC, x, dy - BOX_SPC);
	}
      no_redraw:
	break;  /* end of case KeyPress */
      }  /* end of switch (event.type) */
      continue;
    }  /* end of if (event.xany.window == frame) */
    for (i = 0; i < n_items; i++) {
      item = items + i;
      if (event.xany.window == item->header.win) {
	switch (event.type) {
	case Expose:
	  RedrawItem(item, 0);
	  break;
	case ButtonPress:
	  if (item->type == I_INPUT) {
	    old_item = cur_text;
	    old_item->input.o_cursor = rel_cursor;
	    cur_text = item;
	    RedrawItem(old_item, 1);
	    abs_cursor = (event.xbutton.x - BOX_SPC -
			  TEXT_SPC + FontWidth(xfs[f_input]) / 2)
	      / FontWidth(xfs[f_input]);
	    if (abs_cursor < 0)
	      abs_cursor = 0;
	    if (abs_cursor > item->input.size)
	      abs_cursor = item->input.size;
	    rel_cursor = abs_cursor + item->input.left;
	    if (rel_cursor < 0)
	      rel_cursor = 0;
	    if (rel_cursor > item->input.n)
	      rel_cursor = item->input.n;
	    if (rel_cursor > 0 && rel_cursor == item->input.left)
	      item->input.left--;
	    if (rel_cursor < item->input.n &&
		rel_cursor == item->input.left + item->input.size)
	      item->input.left++;
	    abs_cursor = rel_cursor - item->input.left;
	    RedrawItem(item, 0);
	  }
	  if (item->type == I_CHOICE)
	    ToggleChoice(item);
	  if (item->type == I_BUTTON) {
	    RedrawItem(item, 1);
	    XGrabPointer(dpy, item->header.win, False, ButtonReleaseMask,
			 GrabModeAsync, GrabModeAsync,
			 None, None, CurrentTime);
	  }
	  break;
	case ButtonRelease:
	  RedrawItem(item, 0);
	  if (grab_server && server_grabbed) {
	    XGrabPointer(dpy, frame, True, 0, GrabModeAsync, GrabModeAsync,
			 None, None, CurrentTime);
	    XFlush(dpy);
	  } else {
	    XUngrabPointer(dpy, CurrentTime);
	    XFlush(dpy);
	  }
	  if (event.xbutton.x >= 0 &&
	      event.xbutton.x < item->header.size_x &&
	      event.xbutton.y >= 0 &&
	      event.xbutton.y < item->header.size_y) {
	    DoCommand(item);
	  }
	  break;
	}
      }
    }  /* end of for (i = 0 */
  }  /* while loop */
}

/* main event loop */
void MainLoop ()
{
  fd_set fds;

  while (1) {
    FD_ZERO(&fds);
    FD_SET(fd_in, &fds);
    FD_SET(fd_x, &fds);

    XFlush(dpy);
    if (select(32, &fds, NULL, NULL, NULL) > 0) {
      if (FD_ISSET(fd_in, &fds))
	ReadFvwm();
      if (FD_ISSET(fd_x, &fds))
	ReadXServer();
    }
  }
}


/* main procedure */
int main (int argc, char **argv)
{
  FILE *fdopen();
  int i;

  buf = (char *)malloc(N);  /* some kludge */

#ifdef DEBUG
  fd_err = open(".FvwmFormErrors", O_WRONLY | O_CREAT, 0777);
  fp_err = fdopen(fd_err, "w");
#else
  fd_err = open("/dev/null", O_WRONLY, 0);
  fp_err = fdopen(fd_err, "w");
#endif

  /* we get rid of the path from program name */
  prog_name = argv[0];
  i = strlen(prog_name);
  while (prog_name[--i] != '/' && i > 0);
  if (i > 0)
    prog_name = prog_name + (i + 1);
  fprintf(fp_err, "%s started...\n", prog_name);

  if (argc < 6) {
#ifndef DEBUG
    fprintf(fp_err, "%s must be started by Fvwm.\n", prog_name);
    exit(1);
#else
    fd_out = 1;
    fd_in = 0;
    ref = None;
#endif
  } else {
    if(argc==7)
      prog_name = argv[6];
    fd_out = atoi(argv[1]);
    fd_in = atoi(argv[2]);
    ref = strtol(argv[4], NULL, 16);
    if (ref == 0) ref = None;
#ifdef DEBUG
    fprintf(fp_err, "ref == %d\n", ref);
#endif
  }

  fd[0]=fd_out;
  fd[1]=fd_in;

  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(fp_err, "%s: can't open display.\n", prog_name);
    exit(1);
  }
  fd_x = XConnectionNumber(dpy);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  scr_depth = DefaultDepth(dpy, screen);
  d_cmap = DefaultColormap(dpy, screen);

  ReadConfig();

  GetColors();

  OpenWindows();

  MainLoop();

  return 0;
}


void DeadPipe(int nonsense)
{
  exit(0);
}
