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

#include <stdlib.h>
#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "FvwmIconMan.h"
#include "x.h"
#include "xmanager.h"
#include "libs/PictureGraphics.h"

extern FlocaleWinString *FwinString;

/* button dirty bits: */
#define ICON_STATE_CHANGED  1
#define STATE_CHANGED       2
#define PICTURE_CHANGED     4
#define WINDOW_CHANGED      8
#define STRING_CHANGED      16
#define REDRAW_BUTTON       32
#define GEOMETRY_CHANGED    64

/* manager dirty bits: */
/*      GEOMETRY_CHANGED    64 same as with button */
#define MAPPING_CHANGED     2
#define SHAPE_CHANGED       4
#define REDRAW_MANAGER      8
#define REDRAW_BG           16

/* ButtonArray dirty bits: */
#define NUM_BUTTONS_CHANGED 1
#define NUM_WINDOWS_CHANGED 2

#define ALL_CHANGED         0x7f /* high bit is special */

typedef struct {
  int button_x, button_y, button_h, button_w; /* dim's of the whole button */
  int icon_x, icon_y, icon_h, icon_w;         /* what denotes icon state */
  int text_x, text_y, text_h, text_w;         /* text field */
  int text_base;                              /* text baseline */
} ButtonGeometry;

static void print_button_info (Button *b);
static void insert_windows_button (WinData *win);

/***************************************************************************/
/* Utility leaf functions                                                  */
/***************************************************************************/

static int selected_button_in_man (WinManager *man)
{
  assert (man);
  ConsoleDebug (X11, "selected_button_in_man: %p\n",
		globals.select_win);
  if (globals.select_win && globals.select_win->button &&
    globals.select_win->manager == man) {
    return globals.select_win->button->index;
  }
  return -1;
}

static void ClipRectangle (WinManager *man, int context,
			   int x, int y, int w, int h)
{
  XRectangle r;

  r.x = x;
  r.y = y;
  r.width = w;
  r.height = h;

  XSetClipRectangles(theDisplay, man->hiContext[context], 0, 0, &r, 1,
		     YXBanded);
}

static int num_visible_rows (int n, int cols)
{
  return (n - 1) / cols + 1;
}

static int first_row_len (int n, int cols)
{
  int ret = n % cols;
  if (ret == 0)
    ret = cols;
  return ret;
}

static int index_to_box (WinManager *man, int index)
{
  int first_len, n, cols;

  if (man->geometry.dir & GROW_DOWN) {
    return index;
  }
  else {
    n = man->buttons.num_windows;
    cols = man->geometry.cols;
    first_len = first_row_len (n, cols);
    if (index >= first_len)
      index += cols - first_len;
    index += (man->geometry.rows - num_visible_rows (n, cols)) * cols;
    return index;
  }
}

static int box_to_index (WinManager *man, int box)
{
  int first_len, n, cols;

  if (man->geometry.dir & GROW_DOWN) {
    return box;
  }
  else {
    n = man->buttons.num_windows;
    cols = man->geometry.cols;
    first_len = first_row_len (n, cols);

    box -= (man->geometry.rows - num_visible_rows (n, cols)) * cols;
    if (!((box >= 0 && box < first_len) || box >= cols))
      return -1;
    if (box >= first_len)
      box -= cols - first_len;
    return box;
  }
}

static int index_to_row (WinManager *man, int index)
{
  int row;

  row = index_to_box (man, index) / man->geometry.cols;

  return row;
}

static int index_to_col (WinManager *man, int index)
{
  int col;

  col = index_to_box (man, index) % man->geometry.cols;

  return col;
}

static int rects_equal (XRectangle *x, XRectangle *y)
{
  return (x->x == y->x) && (x->y == y->y) && (x->width == y->width) &&
    (x->height == y->height);
}

static int top_y_coord (WinManager *man)
{
  if (man->buttons.num_windows > 0 && (man->geometry.dir & GROW_UP)) {
    return index_to_row (man, 0) * man->geometry.boxheight;
  }
  else {
    return 0;
  }
}

static ManGeometry *figure_geometry (WinManager *man)
{
  /* Given the number of wins in icon_list and width x height, compute
     new geometry. */
  /* if GROW_FIXED is set, don't change window geometry */

  static ManGeometry ret;
  ManGeometry *g = &man->geometry;
  int n = man->buttons.num_windows;

  ret = *g;

  ConsoleDebug (X11, "figure_geometry: %s: %d, %d %d %d %d\n",
		man->titlename, n,
		ret.width, ret.height, ret.cols, ret.rows);

  if (n == 0) {
    n = 1;
  }

  if (man->geometry.dir & GROW_FIXED) {
    ret.cols = num_visible_rows (n, g->rows);
    ret.boxwidth = ret.width / ret.cols;
    if (ret.boxwidth < 1)
      ret.boxwidth = 1;
  }
  else {
    if (man->geometry.dir & GROW_VERT) {
      if (g->cols) {
	ret.rows = num_visible_rows (n, g->cols);
      }
      else {
	ConsoleMessage ("Internal error in figure_geometry\n");
	ret.rows = 1;
      }
      ret.height = ret.rows * g->boxheight;
      ret.width  = ret.cols * g->boxwidth;
    }
    else {
      /* need to set resize inc  */
      if (g->rows) {
	ret.cols = num_visible_rows (n, g->rows);
      }
      else {
	ConsoleMessage ("Internal error in figure_geometry\n");
	ret.cols = 1;
      }
      ret.height = ret.rows * g->boxheight;
      ret.width = ret.cols * g->boxwidth;
    }
  }

  ConsoleDebug (X11, "figure_geometry: %d %d %d %d %d\n",
		n, ret.width, ret.height, ret.cols, ret.rows);

  return &ret;
}

static ManGeometry *query_geometry (WinManager *man)
{
  XWindowAttributes frame_attr, win_attr;
  int off_x, off_y;
  static ManGeometry g;

  assert (man->window_mapped);

  off_x = 0;
  off_y = 0;
  man->theFrame = find_frame_window (man->theWindow, &off_x, &off_y);
  if (XGetWindowAttributes (theDisplay, man->theFrame, &frame_attr))
  {
    g.x = frame_attr.x + off_x + frame_attr.border_width;
    g.y = frame_attr.y + off_y + frame_attr.border_width;
  }
  else
  {
    g.x = off_x;
    g.y = off_y;
    fprintf(stderr, "%s: query_geometry: failed to get frame attributes.\n",
	    MyName);
  }
  if (XGetWindowAttributes (theDisplay, man->theWindow, &win_attr))
  {
    g.width = win_attr.width;
    g.height = win_attr.height;
  }
  else
  {
    g.width = 1;
    g.height = 1;
    fprintf(stderr, "%s: query_geometry: failed to get window attributes.\n",
	    MyName);
  }

  return &g;
}

static void fix_manager_size (WinManager *man, int w, int h)
{
  XSizeHints size;
  long mask;

  if (man->geometry.dir & GROW_FIXED)
    return;

  XGetWMNormalHints (theDisplay, man->theWindow, &size, &mask);
  size.min_width = w;
  size.max_width = w;
  size.min_height = h;
  size.max_height = h;
  XSetWMNormalHints (theDisplay, man->theWindow, &size);
}

/* Like XMoveResizeWindow(), but can move in arbitary directions */
static void resize_window (WinManager *man)
{
  ManGeometry *g;
  int x_changed, y_changed, dir;

  dir = man->geometry.dir;
  fix_manager_size (man, man->geometry.width, man->geometry.height);

  if ((dir & GROW_DOWN) && (dir & GROW_RIGHT)) {
    XResizeWindow (theDisplay, man->theWindow, man->geometry.width,
		   man->geometry.height);
  }
  else {
    MyXGrabServer (theDisplay);
    g = query_geometry (man);
    x_changed = y_changed = 0;
    if (dir & GROW_LEFT) {
      man->geometry.x = g->x + g->width - man->geometry.width;
      x_changed = 1;
    }
    else {
      man->geometry.x = g->x;
    }
    if (dir & GROW_UP) {
      man->geometry.y = g->y + g->height - man->geometry.height;
      y_changed = 1;
    }
    else {
      man->geometry.y = g->y;
    }

    ConsoleDebug (X11, "queried: y: %d, h: %d, queried: %d\n", g->y,
		  man->geometry.height, g->height);

    if (x_changed || y_changed) {
      XMoveResizeWindow (theDisplay, man->theWindow,
			 man->geometry.x,
			 man->geometry.y,
			 man->geometry.width, man->geometry.height);
    }
    else {
      XResizeWindow (theDisplay, man->theWindow, man->geometry.width,
		     man->geometry.height);
    }
    MyXUngrabServer (theDisplay);
  }
}

static char *make_display_string (WinData *win, char *format, int len)
{
#define MAX_DISPLAY_SIZE 1024
#define COPY(field)                                       \
  temp_p = win->field;                                    \
  if (temp_p)                                             \
    while (*temp_p && out_p - buf < len - 1) \
      *out_p++ = *temp_p++;                               \
  in_p++;

  static char buf[MAX_DISPLAY_SIZE];
  char *string, *in_p, *out_p, *temp_p;

  in_p = format;
  out_p = buf;

  if (len > MAX_DISPLAY_SIZE || len <= 0)
    len = MAX_DISPLAY_SIZE;

  while (*in_p && out_p - buf < len - 1) {
    if (*in_p == '%') {
      switch (*(++in_p)) {
      case 'i':
	COPY (visible_icon_name);
	break;

      case 't':
	COPY (visible_name);
	break;

      case 'r':
	COPY (resname);
	break;

      case 'c':
	COPY (classname);
	break;

      default:
	*out_p++ = *in_p++;
	break;
      }
    }
    else {
      *out_p++ = *in_p++;
    }
  }

  *out_p++ = '\0';

  string = buf;
  return string;

#undef COPY
#undef MAX_DISPLAY_SIZE
}

Button *button_above (WinManager *man, Button *b)
{
  int n = man->buttons.num_windows, cols = man->geometry.cols;
  int i = -1;

  if (b) {
    i = box_to_index (man, index_to_box (man, b->index) - cols);
  }
  if (i < 0 || i >= n)
    return b;
  else
    return man->buttons.buttons[i];
}

Button *button_below (WinManager *man, Button *b)
{
  int n = man->buttons.num_windows;
  int i = -1;

  if (b) {
    i = box_to_index (man, index_to_box (man, b->index) + man->geometry.cols);
  }
  if (i < 0 || i >= n)
    return b;
  else
    return man->buttons.buttons[i];
}

Button *button_right (WinManager *man, Button *b)
{
  int i = -1;

  if (index_to_col (man, b->index) < man->geometry.cols - 1) {
    i = box_to_index (man, index_to_box (man, b->index) + 1);
  }

  if (i < 0 || i >= man->buttons.num_windows)
    return b;
  else
    return man->buttons.buttons[i];
}

Button *button_left (WinManager *man, Button *b)
{
  int i = -1;
  if (index_to_col (man, b->index) > 0) {
    i = box_to_index (man, index_to_box (man, b->index) - 1);
  }

  if (i < 0 || i >= man->buttons.num_windows)
    return b;
  else
    return man->buttons.buttons[i];
}

Button *button_next (WinManager *man, Button *b)
{
  int i = b->index + 1;

  if (i >= 0 && i < man->buttons.num_windows)
    return man->buttons.buttons[i];
  else
    return b;
}

Button *button_prev (WinManager *man, Button *b)
{
  int i = b->index - 1;

  if (i >= 0 && i < man->buttons.num_windows)
    return man->buttons.buttons[i];
  else
    return b;
}

Button *xy_to_button (WinManager *man, int x, int y)
{
  int row = y / man->geometry.boxheight;
  int col = x / man->geometry.boxwidth;
  int box, index;

  if (x >= 0 && x <= man->geometry.width &&
      y >= 0 && y <= man->geometry.height) {
    box = row * man->geometry.cols + col;
    index = box_to_index (man, box);
    if (index >= 0 && index < man->buttons.num_windows)
      return man->buttons.buttons[index];
  }

  return NULL;
}

/***************************************************************************/
/* Routines which change dirtyable state                                   */
/***************************************************************************/

static void set_button_geometry (WinManager *man, Button *box)
{
  box->x = index_to_col (man, box->index) * man->geometry.boxwidth;
  box->y = index_to_row (man, box->index) * man->geometry.boxheight;
  box->w = man->geometry.boxwidth;
  box->h = man->geometry.boxheight;
  box->drawn_state.dirty_flags |= GEOMETRY_CHANGED;
}

static void clear_button (Button *b)
{
  assert (b);
  b->drawn_state.win = NULL;
  b->drawn_state.dirty_flags = REDRAW_BUTTON;
  b->drawn_state.display_string = NULL;
}

static void set_window_button (WinData *win, int index)
{
  Button *b;

  assert (win->manager && index < win->manager->buttons.num_buttons);

  b = win->manager->buttons.buttons[index];

  /* Can optimize here */
  if (FMiniIconsSupported)
  {
    b->drawn_state.pic = win->pic;
  }
  b->drawn_state.win = win;
  copy_string(&b->drawn_state.display_string, win->display_string);
  b->drawn_state.iconified = win->iconified;
  b->drawn_state.state = win->state;
  b->drawn_state.dirty_flags = ALL_CHANGED;

  win->button = b;
}

static void *Realloc (void *ptr, int size)
{
  if (ptr == NULL)
    return safemalloc (size);
  else
    return realloc (ptr, size);
}

static void set_num_buttons (ButtonArray *buttons, int n)
{
  int i;

  ConsoleDebug (X11, "set_num_buttons: %d %d\n", buttons->num_buttons, n);

  if (n > buttons->num_buttons) {
    buttons->dirty_flags |= NUM_BUTTONS_CHANGED;
    buttons->buttons = (Button **)Realloc (buttons->buttons,
					   n * sizeof (Button *));
    if (buttons->buttons == NULL) {
      ConsoleMessage ("Realloc failed! Bailing out\n");
      ShutMeDown(1);
    }

    for (i = buttons->num_buttons; i < n; i++) {
      buttons->buttons[i] = (Button *)safemalloc (sizeof (Button));
      memset(buttons->buttons[i], 0, sizeof(Button));
buttons->buttons[i]->drawn_state.display_string = NULL;
      buttons->buttons[i]->index = i;
    }

    buttons->dirty_flags |= NUM_BUTTONS_CHANGED;
    buttons->num_buttons = n;
  }
}

static void increase_num_windows (ButtonArray *buttons, int off)
{
  int n;

  if (off != 0) {
    buttons->num_windows += off;
    buttons->dirty_flags |= NUM_WINDOWS_CHANGED;

    if (buttons->num_windows > buttons->num_buttons) {
      n = buttons->num_windows + 10;
      set_num_buttons (buttons, n);
    }
  }
}

static void set_man_gravity_origin (WinManager *man)
{
  if (man->gravity == NorthWestGravity || man->gravity == NorthEastGravity)
    man->geometry.gravity_y = 0;
  else
    man->geometry.gravity_y = man->geometry.height;
  if (man->gravity == NorthWestGravity || man->gravity == SouthWestGravity)
    man->geometry.gravity_x = 0;
  else
    man->geometry.gravity_x = man->geometry.width;
}

static void set_man_geometry (WinManager *man, ManGeometry *new)
{
  int n;

  if (man->geometry.width != new->width ||
      man->geometry.height != new->height ||
      man->geometry.rows != new->rows ||
      man->geometry.cols != new->cols ||
      man->geometry.boxheight != new->boxheight ||
      man->geometry.boxwidth != new->boxwidth) {
    man->dirty_flags |= GEOMETRY_CHANGED;
  }

  man->geometry = *new;
  set_man_gravity_origin (man);
  n = man->geometry.rows * man->geometry.cols;
  if (man->buttons.num_buttons < n)
    set_num_buttons (&man->buttons, n + 10);
}

void set_manager_width (WinManager *man, int width)
{
  if (width != man->geometry.width) {
    ConsoleDebug (X11, "set_manager_width: %d -> %d, %d -> %d\n",
		  man->geometry.width, width, man->geometry.boxwidth,
		  width / man->geometry.cols);
    man->geometry.width = width;
    man->geometry.boxwidth = width / man->geometry.cols;
    if (man->geometry.boxwidth < 1)
      man->geometry.boxwidth = 1;
    man->dirty_flags |= GEOMETRY_CHANGED;
  }
}

void force_manager_redraw (WinManager *man)
{
  man->dirty_flags |= REDRAW_MANAGER;
  draw_manager (man);
}

void set_win_picture (WinData *win, Pixmap picture, Pixmap mask, Pixmap alpha,
		      unsigned int depth, unsigned int width,
		      unsigned int height)
{
  if (!FMiniIconsSupported)
  {
    return;
  }
  if (win->button)
    win->button->drawn_state.dirty_flags |= PICTURE_CHANGED;
  win->old_pic = win->pic;
  win->pic.picture = picture;
  win->pic.mask = mask;
  win->pic.alpha = alpha;
  win->pic.width = width;
  win->pic.height = height;
  win->pic.depth = depth;
}

void set_win_iconified (WinData *win, int iconified)
{
  /* This change has become necessary because with colorsets we don't know
   * the background colour of the button (gradient background). Thus the button
   * has to be redrawn completely, we can not just draw the square in the
   * background colour. */
   char string[256];

  if (win->button != NULL && (win->iconified != iconified) ) {
    /* we check the win->button width and height because they will only
     * be == zero on init, and if we didn't check we would get animations of
     * all iconified windows to the far left of whereever thae manager would
     * eventually be. */
    if (win->manager->AnimCommand && (win->manager->AnimCommand[0] != 0)
	&& IS_ICON_SUPPRESSED(win) && (win->button->w != 0)
	&& (win->button->h !=0) )
    {
      int abs_x, abs_y;
      Window junkw;

      XTranslateCoordinates(theDisplay, win->manager->theWindow, theRoot,
			    win->button->x, win->button->y, &abs_x, &abs_y,
			    &junkw);
      if (iconified)
      {
	sprintf(string, "%s %d %d %d %d %d %d %d %d",
		win->manager->AnimCommand,
		(int)win->x, (int)win->y, (int)win->width, (int)win->height,
		abs_x, abs_y, win->button->w, win->button->h);
      } else {
	sprintf(string, "%s %d %d %d %d %d %d %d %d",
		win->manager->AnimCommand,
		abs_x, abs_y, win->button->w, win->button->h,
		(int)win->x, (int)win->y, (int)win->width, (int)win->height);
      }
      SendText(Fvwm_fd, string, 0);

    }
    win->button->drawn_state.dirty_flags |= ICON_STATE_CHANGED;
  }
  win->iconified = iconified;
}

void set_win_state (WinData *win, int state)
{
  if (win->button && win->state != state)
    win->button->drawn_state.dirty_flags |= STATE_CHANGED;
  win->state = state;
}

void add_win_state (WinData *win, int flag)
{
  if (win->button && (win->state & flag) == 0)
    win->button->drawn_state.dirty_flags |= STATE_CHANGED;
  win->state |= flag;
  ConsoleDebug (X11, "add_win_state: %s 0x%x\n", win->titlename, flag);
}

void del_win_state (WinData *win, int flag)
{
  if (win->button && (win->state & flag))
    win->button->drawn_state.dirty_flags |= STATE_CHANGED;
  win->state &= ~flag;
  ConsoleDebug (X11, "del_win_state: %s 0x%x\n", win->titlename, flag);
}

void set_win_displaystring (WinData *win)
{
  WinManager *man = win->manager;
  int maxlen;

  if (!man || ((man->format_depend & CLASS_NAME) && !win->classname)
      || ((man->format_depend & ICON_NAME) && !win->visible_icon_name)
      || ((man->format_depend & TITLE_NAME) && !win->visible_name)
      || ((man->format_depend & RESOURCE_NAME) && !win->resname)) {
    return;
  }

  if (man->window_up) {
    assert (man->geometry.width && man->fontwidth);
    maxlen = man->geometry.width / man->fontwidth + 2 /* fudge factor */;
  }
  else {
    maxlen = 0;
  }
  copy_string(
    &win->display_string, make_display_string (win, man->formatstring, maxlen));
  if (win->button)
    win->button->drawn_state.dirty_flags |= STRING_CHANGED;
}

/* This function is here and not with the other static utility functions
   because it basically is the inverse of set_shape() */

static void clear_empty_region (WinManager *man)
{
  XRectangle rects[3];
  int num_rects = 0, n = man->buttons.num_windows, cols = man->geometry.cols;
  int rows = man->geometry.rows;
  int boxwidth = man->geometry.boxwidth;
  int boxheight = man->geometry.boxheight;

  if (man->shaped)
    return;

  rects[1].x = rects[1].y = rects[1].width = rects[1].height = 0;

  if (n == 0 || rows * cols == 0 /* just be to safe */)
  {
    rects[0].x = 0;
    rects[0].y = 0;
    rects[0].width = man->geometry.width;
    rects[0].height = man->geometry.height;
    num_rects = 1;
  }
  else if (man->geometry.dir & GROW_DOWN)
  {
    assert (cols);
    if (n % cols == 0)
    {
      rects[0].x = 0;
      rects[0].y = num_visible_rows (n, cols) * man->geometry.boxheight;
      rects[0].width = man->geometry.width;
      rects[0].height = man->geometry.height - rects[0].y;
      num_rects = 1;
    }
    else
    {
      rects[0].x = (n % cols) * man->geometry.boxwidth;
      rects[0].y = (num_visible_rows (n, cols) - 1) * man->geometry.boxheight;
      rects[0].width = man->geometry.width - rects[0].x;
      rects[0].height = boxheight;
      rects[1].x = 0;
      rects[1].y = rects[0].y + rects[0].height;
      rects[1].width = man->geometry.width;
      rects[1].height = man->geometry.height - rects[0].y;
      num_rects = 2;
    }
  }
  else
  {
    assert (cols);
    /* for shaped windows, we won't see this part of the window */
    if (n % cols == 0)
    {
      rects[0].x = 0;
      rects[0].y = 0;
      rects[0].width = man->geometry.width;
      rects[0].height = top_y_coord (man);
      num_rects = 1;
    }
    else
    {
      rects[0].x = 0;
      rects[0].y = 0;
      rects[0].width = man->geometry.width;
      rects[0].height = top_y_coord (man);
      rects[1].x = (n % cols) * man->geometry.boxwidth;
      rects[1].y = rects[0].height;
      rects[1].width = man->geometry.width - rects[1].x;
      rects[1].height = boxheight;
      num_rects = 2;
    }
  }

  if (n != 0 && rows * cols != 0)
  {
    rects[num_rects].x = cols * boxwidth;
    rects[num_rects].y = 0;
    rects[num_rects].width = man->geometry.width - cols * boxwidth;
    rects[num_rects].height = man->geometry.height;
    num_rects++;
  }

  ConsoleDebug (X11, "Clearing: %d: (%d, %d, %d, %d) + (%d, %d, %d, %d)\n",
		num_rects,
		rects[0].x, rects[0].y, rects[0].width, rects[0].height,
		rects[1].x, rects[1].y, rects[1].width, rects[1].height);

  if (man->colorsets[DEFAULT] >= 0 &&
      Colorset[man->colorsets[DEFAULT]].pixmap == ParentRelative)
  {
    for(n=0; n < num_rects; n++)
    {
      XClearArea(
	theDisplay, man->theWindow, rects[n].x, rects[n].y, rects[n].width,
	rects[n].height, False);
    }
  }
  else
  {
    XFillRectangles (theDisplay, man->theWindow,
		     man->backContext[DEFAULT], rects, num_rects);
  }
}

void set_shape (WinManager *man)
{
  if (FShapesSupported)
  {
    int n;
    XRectangle rects[2];
    int cols = man->geometry.cols;

    if (man->shaped == 0)
      return;

    ConsoleDebug (X11, "in set_shape: %s\n", man->titlename);

    n = man->buttons.num_windows;
    if (n == 0)
    {
      man->shape.num_rects = 1;
      rects[0].x = -1;
      rects[0].y = -1;
      rects[0].width = 1;
      rects[0].height = 1;
      if (man->shape.num_rects != 0) {
	man->dirty_flags |= SHAPE_CHANGED;
      }
      man->shape.rects[0] = rects[0];
      return;
    }

    if (cols == 0 || n % cols == 0) {
      rects[0].x = 0;
      rects[0].y = top_y_coord (man);
      rects[0].width = man->geometry.width;
      rects[0].height = num_visible_rows (n, cols) * man->geometry.boxheight;
      if (man->shape.num_rects != 1 || !rects_equal (rects, man->shape.rects)) {
	man->dirty_flags |= SHAPE_CHANGED;
      }
      man->shape.num_rects = 1;
      man->shape.rects[0] = rects[0];
    }
    else {
      if (man->geometry.dir & GROW_DOWN) {
	rects[0].x = 0;
	rects[0].y = 0;
	rects[0].width = man->geometry.width;
	rects[0].height =
	  (num_visible_rows (n, cols) - 1) * man->geometry.boxheight;
	rects[1].x = 0;
	rects[1].y = rects[0].height;
	rects[1].width = (n % cols) * man->geometry.boxwidth;
	rects[1].height = man->geometry.boxheight;
      }
      else {
	rects[0].x = 0;
	rects[0].y = top_y_coord (man);
	rects[0].width = (n % cols) * man->geometry.boxwidth;
	rects[0].height = man->geometry.boxheight;
	rects[1].x = 0;
	rects[1].y = rects[0].y + rects[0].height;
	rects[1].width = man->geometry.width;
	rects[1].height = (num_visible_rows (n, cols) - 1) *
	  man->geometry.boxheight;
      }
      if (man->shape.num_rects != 2 ||
	  !rects_equal (rects, man->shape.rects) ||
	  !rects_equal (rects + 1, man->shape.rects + 1)) {
	man->dirty_flags |= SHAPE_CHANGED;
      }
      man->shape.num_rects = 2;
      man->shape.rects[0] = rects[0];
      man->shape.rects[1] = rects[1];
    }
  }
}

void set_manager_window_mapping (WinManager *man, int flag)
{
  if (flag != man->window_mapped) {
    man->window_mapped = flag;
    man->dirty_flags |= MAPPING_CHANGED;
  }
}

/***************************************************************************/
/* Major exported functions                                                */
/***************************************************************************/

void init_boxes (void)
{
}

void init_button_array (ButtonArray *array)
{
  memset(array, 0, sizeof(ButtonArray));
}

/* Pretty much like resize_manager, but used only to figure the
   correct size when creating the window */

void size_manager (WinManager *man)
{
  ManGeometry *new;
  int oldwidth, oldheight, w, h;

  new = figure_geometry (man);

  assert (new->width && new->height);

  w = new->width;
  h = new->height;

  oldwidth = man->geometry.width;
  oldheight = man->geometry.height;

  set_man_geometry (man, new);

  if (oldheight != h || oldwidth != w) {
    if (man->geometry.dir & GROW_UP)
      man->geometry.y -= h - oldheight;
    if (man->geometry.dir & GROW_LEFT)
      man->geometry.x -= w - oldwidth;
  }

  ConsoleDebug (X11, "size_manager %s: %d %d %d %d\n", man->titlename,
		man->geometry.x, man->geometry.y, man->geometry.width,
		man->geometry.height);
}

static void resize_manager (WinManager *man, int force)
{
  ManGeometry *new;
  int oldwidth, oldheight, oldrows, oldcols;
  int dir;
  int i;

  if (man->can_draw == 0)
    return;

  oldwidth = man->geometry.width;
  oldheight = man->geometry.height;
  oldrows = man->geometry.rows;
  oldcols = man->geometry.cols;
  dir = man->geometry.dir;

  if (dir & GROW_FIXED) {
    new = figure_geometry (man);
    set_man_geometry (man, new);
    set_shape (man);
    if (force || oldrows != new->rows || oldcols != new->cols ||
	oldwidth != new->width || oldheight != new->height) {
      man->dirty_flags |= GEOMETRY_CHANGED;
    }
  }
  else {
    new = figure_geometry (man);
    set_man_geometry (man, new);
    set_shape (man);
    if (force || oldrows != new->rows || oldcols != new->cols ||
	oldwidth != new->width || oldheight != new->height) {
      resize_window (man);
    }
  }

  if (oldwidth != new->width || oldheight != new->height) {
    for (i = 0; i < NUM_CONTEXTS; i++) {
      if (man->pixmap[i])
	XFreePixmap(theDisplay, man->pixmap[i]);
      if ((man->colorsets[i] >= 0) && Colorset[man->colorsets[i]].pixmap) {
	man->pixmap[i] = CreateBackgroundPixmap(theDisplay, man->theWindow,
		       man->geometry.width, man->geometry.height,
		       &Colorset[man->colorsets[i]],
		       Pdepth, man->backContext[i], False);
	XSetTile(theDisplay, man->backContext[i], man->pixmap[i]);
	XSetFillStyle(theDisplay, man->backContext[i], FillTiled);
	if (i == DEFAULT)
	{
	  XSetWindowBackgroundPixmap(theDisplay, man->theWindow,
				     man->pixmap[i]);
	}
      } else {
	man->pixmap[i] = None;
	XSetFillStyle(theDisplay, man->backContext[i], FillSolid);
      }
    }
  }
}

static int center_padding (int h1, int h2)
{
  return (h2 - h1) / 2;
}

static void get_title_geometry (WinManager *man, ButtonGeometry *g)
{
  int text_pad;
  assert (man);
  g->button_x = 0;
  g->button_y = 0;
  g->button_w = man->geometry.boxwidth;
  g->button_h = man->geometry.boxheight;
  g->text_x = g->button_x + g->button_h / 2;
  g->text_w = g->button_w - 4 - (g->text_x - g->button_x);
  if (g->text_w <= 0)
    g->text_w = 1;
  g->text_h = man->fontheight;
  text_pad = center_padding (man->fontheight, g->button_h);

  g->text_y = g->button_y + text_pad;
  g->text_base = g->text_y + man->FButtonFont->ascent;
}

static void get_button_geometry (WinManager *man, Button *button,
				 ButtonGeometry *g)
{
  int icon_pad, text_pad;
  WinData *win;

  assert (man);

  win = button->drawn_state.win;

  g->button_x = button->x;
  g->button_y = button->y;

  g->button_w = button->w;
  g->button_h = button->h;

/* [BV 16-Apr-97] Mini Icons work on black-and-white too */
  if (FMiniIconsSupported && man->draw_icons && win && win->pic.picture) {
    /* If no window, then icon_* aren't used, so doesn't matter what
       they are */
    g->icon_w = min (win->pic.width, g->button_h);
    g->icon_h = min (g->button_h - 4, win->pic.height);
    icon_pad  = center_padding (g->icon_h, g->button_h);
    g->icon_x = g->button_x + 4;
    g->icon_y = g->button_y + icon_pad;
  }
  else {
    g->icon_h = man->geometry.boxheight - 8;
    g->icon_w = g->icon_h;

    icon_pad = center_padding (g->icon_h, g->button_h);
    g->icon_x = g->button_x + icon_pad;
    g->icon_y = g->button_y + icon_pad;
  }

  g->text_x = g->icon_x + g->icon_w + 2;
  g->text_w = g->button_w - 4 - (g->text_x - g->button_x);
  if (g->text_w <= 0)
    g->text_w = 1;
  g->text_h = man->fontheight;

  text_pad = center_padding (man->fontheight, g->button_h);

  g->text_y = g->button_y + text_pad;
  g->text_base = g->text_y + man->FButtonFont->ascent;
}

static void draw_3d_icon (WinManager *man, int box, ButtonGeometry *g,
			  int iconified, Contexts contextId)
{
  if (iconified == 0)
    return;
  else
    RelieveRectangle (theDisplay, man->theWindow, g->icon_x, g->icon_y,
		      g->icon_w - 1, g->icon_h - 1,
		      man->reliefContext[contextId],
		      man->shadowContext[contextId], 2);
}


  /* this routine should only be called from draw_button() */
static void iconify_box (WinManager *man, WinData *win, int box,
			 ButtonGeometry *g, int iconified, Contexts contextId,
			 int button_already_cleared)
{

  if (!man->window_up)
    return;

/* [BV 16-Apr-97] Mini Icons work on black-and-white too */
  if (FMiniIconsSupported && man->draw_icons && win->pic.picture) {
    if (iconified == 0 && man->draw_icons != 2) {
      if (!button_already_cleared) {
	XFillRectangle (theDisplay, man->theWindow,
			man->backContext[contextId], g->icon_x, g->icon_y,
			g->icon_w, g->icon_h);
      }
    }
    else {
      PGraphicsCopyFvwmPicture(theDisplay, &win->pic, man->theWindow,
			       man->hiContext[contextId],
			       0, 0, g->icon_w, g->icon_h,
			       g->icon_x, g->icon_y);
    }
  }
  else {
    if (Pdepth > 2) {
      draw_3d_icon (man, box, g, iconified, contextId);
    }
    else {
      if (iconified == 0) {
	XFillArc (theDisplay, man->theWindow, man->backContext[contextId],
		  g->icon_x, g->icon_y, g->icon_w, g->icon_h, 0, 360 * 64);
      }
      else {
	XFillArc (theDisplay, man->theWindow, man->hiContext[contextId],
		  g->icon_x, g->icon_y, g->icon_w, g->icon_h, 0, 360 * 64);
      }
    }
  }
}

int change_windows_manager (WinData *win)
{
  WinManager *oldman;
  WinManager *newman;

  ConsoleDebug (X11, "change_windows_manager: %s\n", win->titlename);

  oldman = win->manager;
  newman = figure_win_manager (win, ALL_NAME);
  if (oldman && newman != oldman && win->button) {
    delete_windows_button (win);
  }
  win->manager = newman;
  set_win_displaystring (win);
  check_win_complete (win);
  check_in_window (win);
  ConsoleDebug (X11, "change_windows_manager: returning %d\n",
		newman != oldman);
  return (newman != oldman);
}

void check_in_window (WinData *win)
{
  int in_viewport;
  int is_state_selected;

  if (win->manager && win->complete) {
    is_state_selected = (!win->manager->showonlyiconic || win->iconified);
    in_viewport = win_in_viewport (win);
    if (win->manager->usewinlist && DO_SKIP_WINDOW_LIST(win))
      in_viewport = 0;
    if (win->button == NULL && in_viewport && is_state_selected) {
      insert_windows_button (win);
      if (win->manager->window_up == 0 && globals.got_window_list)
	create_manager_window (win->manager->index);
    }
    if (win->button && (!in_viewport || !is_state_selected)) {
      if (win->button->drawn_state.display_string)
	Free(win->button->drawn_state.display_string);
      delete_windows_button (win);
    }
  }
}

static void get_gcs (WinManager *man, int state, int iconified,
		     GC *context1, GC *context2)
{
  GC gc1, gc2;

  switch (man->buttonState[state]) {
  case BUTTON_FLAT:
    gc1 = man->flatContext[state];
    gc2 = man->flatContext[state];
    break;

  case BUTTON_UP:
  case BUTTON_EDGEUP:
    gc1 = man->reliefContext[state];
    gc2 = man->shadowContext[state];
    break;

  case BUTTON_DOWN:
  case BUTTON_EDGEDOWN:
    gc1 = man->shadowContext[state];
    gc2 = man->reliefContext[state];
    break;

  default:
    ConsoleMessage ("Internal error in get_gcs\n");
    return;
  }

  if (((man->rev == REVERSE_ICON) && iconified)
      || ((man->rev == REVERSE_NORMAL) && !iconified)) {
    *context1 = gc2;
    *context2 = gc1;
  } else {
    *context1 = gc1;
    *context2 = gc2;
  }
  return;
}

static void draw_relief (WinManager *man, int button_state, ButtonGeometry *g,
			 GC context1, GC context2)
{
  int state;
  state = man->buttonState[button_state];

  if (state == BUTTON_FLAT)
    return;
  if (state == BUTTON_EDGEUP || state == BUTTON_EDGEDOWN) {
    RelieveRectangle (theDisplay, man->theWindow, g->button_x, g->button_y,
		      g->button_w - 1, g->button_h - 1, context1, context2, 2);
    RelieveRectangle (theDisplay, man->theWindow, g->button_x+2, g->button_y+2,
		      g->button_w - 5, g->button_h - 5, context2, context1, 2);
  }
  else {
    RelieveRectangle (theDisplay, man->theWindow, g->button_x, g->button_y,
		      g->button_w - 1, g->button_h - 1, context1, context2, 2);
  }
}

static void draw_button (WinManager *man, int button, int force)
{
  Button *b;
  WinData *win;
  ButtonGeometry g, old_g;
  GC context1, context2;
  Contexts button_state;
  int cleared_button = 0, dirty;
  int draw_background = 0, draw_icon = 0, draw_string = 0, clear_old_pic = 0;

  assert (man);

  if (!man->window_up) {
    ConsoleMessage ("draw_button: manager not up yet\n");
    return;
  }

  b = man->buttons.buttons[button];
  win = b->drawn_state.win;
  dirty = b->drawn_state.dirty_flags;

  if (win && win->button != b) {
    ConsoleMessage ("Internal error in draw_button.\n");
    return;
  }

  if (!win) {
    return;
  }

  if (force || (dirty & REDRAW_BUTTON)) {
    ConsoleDebug (X11, "draw_button: %d forced\n", b->index);
    draw_background = 1;
    draw_icon = 1;
    draw_string = 1;
  }
  /* figure out what we have to draw */
  if (dirty) {
    ConsoleDebug (X11, "draw_button: %d dirty\n", b->index);
    if (win) {
      if (GEOMETRY_CHANGED) {
	ConsoleDebug (X11, "\tGeometry changed\n");
	/* Determine if geometry has changed relative to the
	   window gravity */
	if (b->w != b->drawn_state.w || b->h != b->drawn_state.h ||
	    b->x - man->geometry.gravity_x !=
	      b->drawn_state.x - man->drawn_geometry.gravity_x ||
	    b->y - man->geometry.gravity_y !=
	      b->drawn_state.y - man->drawn_geometry.gravity_y) {
	  draw_background = 1;
	  draw_icon = 1;
	  draw_string = 1;
	}
      }
      if (STATE_CHANGED) {
	ConsoleDebug (X11, "\tState changed\n");
	b->drawn_state.state = win->state;
	draw_background = 1;
	draw_icon = 1;
	draw_string = 1;
      }
      if (FMiniIconsSupported && PICTURE_CHANGED) {
	FvwmPicture tpic;

	ConsoleDebug (X11, "\tPicture changed\n");
	tpic = win->pic;
	win->pic = win->old_pic;
	get_button_geometry (man, b, &old_g);
	win->pic = tpic;
	b->drawn_state.pic = win->pic;
	draw_icon = 1;
	draw_string = 1;
	clear_old_pic = 1;
      }
      if (ICON_STATE_CHANGED && b->drawn_state.iconified != win->iconified) {
	ConsoleDebug (X11, "\tIcon changed\n");
	b->drawn_state.iconified = win->iconified;
	draw_icon = 1;
	draw_background = 1;
	draw_string = 1;
      }
      if (STRING_CHANGED) {
	ConsoleDebug (X11, "\tString changed: %s\n", win->display_string);
	copy_string(&b->drawn_state.display_string, win->display_string);
	assert (b->drawn_state.display_string);
	draw_icon = 1;
	draw_background = 1;
	draw_string = 1;
      }
    }
  }

  if (win && (draw_background || draw_icon || draw_string)) {
    get_button_geometry (man, b, &g);
    ConsoleDebug (X11, "\tgeometry: %d %d %d %d\n", g.button_x, g.button_y,
		  g.button_w, g.button_h);
    button_state = b->drawn_state.state;
    if (b->drawn_state.iconified && button_state == PLAIN_CONTEXT) {
      button_state = ICON_CONTEXT;
    }
    if (draw_background) {
      ConsoleDebug (X11, "\tDrawing background\n");
      if (man->colorsets[button_state] >= 0 &&
	  Colorset[man->colorsets[button_state]].pixmap == ParentRelative)
      {
	XClearArea(theDisplay, man->theWindow, g.button_x, g.button_y,
		   g.button_w, g.button_h, False);
      }
      else
      {
	XFillRectangle(theDisplay, man->theWindow,
		       man->backContext[button_state], g.button_x,
		       g.button_y, g.button_w, g.button_h);
      }
      cleared_button = 1;

      if (Pdepth > 2) {
	get_gcs (man, button_state, win->iconified, &context1, &context2);
	draw_relief (man, button_state, &g, context1, context2);
      }
      else if (button_state & SELECT_CONTEXT) {
	XDrawRectangle (theDisplay, man->theWindow,
			man->hiContext[button_state],
			g.button_x + 2, g.button_y + 1,
			g.button_w - 4, g.button_h - 2);
      }
    }
    if (clear_old_pic) {
      ConsoleDebug (X11, "\tClearing old picture\n");
      if (!cleared_button) {
	XFillRectangle (theDisplay, man->theWindow,
			man->backContext[PLAIN_CONTEXT],
			old_g.icon_x, old_g.icon_y,
			old_g.icon_w + 2, old_g.icon_h);
      }
    }
    if (draw_icon) {
      ConsoleDebug (X11, "\tDrawing icon\n");
      iconify_box (man, win, button, &g, win->iconified, button_state,
		   cleared_button);
    }
    if (draw_string) {
      ConsoleDebug (X11, "\tDrawing text: %s\n",
		    b->drawn_state.display_string);
      ClipRectangle (man, button_state, g.text_x, g.text_y, g.text_w,
		     g.text_h);
      if (!cleared_button) {
	XFillRectangle (theDisplay, man->theWindow,
			man->backContext[button_state],
			g.text_x, g.text_y, g.text_w, g.text_h);
      }
      FwinString->str =  b->drawn_state.display_string;
      FwinString->win = man->theWindow;
      FwinString->gc = man->hiContext[button_state];
      if (man->colorsets[button_state] >= 0)
      {
	      FwinString->colorset = &Colorset[man->colorsets[button_state]];
	      FwinString->flags.has_colorset = True;
      }
      else
      {
	      FwinString->flags.has_colorset = False;
      }
      FwinString->x = g.text_x;
      FwinString->y = g.text_base;
      FwinString->len = strlen(b->drawn_state.display_string);
      if (FftSupport)
      {
	if (man->FButtonFont->fftf.fftfont != NULL)
	{
	  while(
	    FwinString->len >= 0 &&
	    FlocaleTextWidth(man->FButtonFont, FwinString->str,
			     FwinString->len)
	    > g.text_w)
	  {
	    FwinString->len--;
	  }
	}
      }
      FlocaleDrawString(
	theDisplay, man->FButtonFont, FwinString, FWS_HAVE_LENGTH);
      XSetClipMask (theDisplay, man->hiContext[button_state], None);
    }
  }

  b->drawn_state.dirty_flags = 0;
  b->drawn_state.x = b->x;
  b->drawn_state.y = b->y;
  b->drawn_state.w = b->w;
  b->drawn_state.h = b->h;
  XFlush (theDisplay);
}

void draw_managers (void)
{
  int i;
  for (i = 0; i < globals.num_managers; i++)
    draw_manager (&globals.managers[i]);
}

static void draw_empty_manager (WinManager *man)
{
  GC context1, context2;
  int state = TITLE_CONTEXT;
  ButtonGeometry g;
  int len = strlen (man->titlename);

  ConsoleDebug (X11, "draw_empty_manager\n");
  clear_empty_region (man);
  get_title_geometry (man, &g);

  if (len > 0) {
    if (man->colorsets[state] >= 0
	&& Colorset[man->colorsets[state]].pixmap == ParentRelative) {
      XClearArea (theDisplay, man->theWindow, g.button_x, g.button_y,
		  g.button_w, g.button_h, False);
    } else {
      XFillRectangle (theDisplay, man->theWindow, man->backContext[state],
		      g.button_x, g.button_y, g.button_w, g.button_h);
    }
  }
  if (Pdepth > 2) {
    get_gcs (man, state, 0, &context1, &context2);
    draw_relief (man, state, &g, context1, context2);
  }
  ClipRectangle (man, state, g.text_x, g.text_y, g.text_w, g.text_h);
  FwinString->str = man->titlename;
  FwinString->win = man->theWindow;
  FwinString->gc = man->hiContext[state];
  if (man->colorsets[state] >= 0)
  {
	  FwinString->colorset = &Colorset[man->colorsets[state]];
	  FwinString->flags.has_colorset = True;
  }
  else
  {
	  FwinString->flags.has_colorset = False;
  }
  FwinString->x = g.text_x;
  FwinString->y = g.text_base;
  FlocaleDrawString (theDisplay, man->FButtonFont, FwinString, 0);
  XSetClipMask (theDisplay, man->hiContext[state], None);
}

void draw_manager (WinManager *man)
{
  int i, force_draw = 0, update_geometry = 0, redraw_all = 0;
  int shape_changed = 0;

  assert (man->buttons.num_buttons >= 0 && man->buttons.num_windows >= 0);

  if (!man->window_up)
    return;

  ConsoleDebug (X11, "Drawing Manager: %s\n", man->titlename);
  redraw_all = man->dirty_flags & REDRAW_MANAGER;

  if (redraw_all || (man->buttons.dirty_flags & NUM_WINDOWS_CHANGED)) {
    ConsoleDebug (X11, "\tresizing manager\n");
    resize_manager (man, redraw_all);
    update_geometry = 1;
    force_draw = 1;
  }

  if (redraw_all || (man->dirty_flags & MAPPING_CHANGED)) {
    force_draw = 1;
    ConsoleDebug (X11, "manager %s: mapping changed\n", man->titlename);
  }

  if (FShapesSupported && man->shaped)
  {
    if (redraw_all || (man->dirty_flags & SHAPE_CHANGED)) {
      FShapeCombineRectangles(
	theDisplay, man->theWindow, FShapeBounding, 0, 0, man->shape.rects,
	man->shape.num_rects, FShapeSet, Unsorted);
      FShapeCombineRectangles(
	theDisplay, man->theWindow, FShapeClip, 0, 0, man->shape.rects,
	man->shape.num_rects, FShapeSet, Unsorted);
      shape_changed = 1;
      update_geometry = 1;
    }
  }

  if (redraw_all || (man->dirty_flags & GEOMETRY_CHANGED)) {
    ConsoleDebug (X11, "\tredrawing all buttons\n");
    update_geometry = 1;
  }

  if (update_geometry) {
    for (i = 0; i < man->buttons.num_windows; i++)
      set_button_geometry (man, man->buttons.buttons[i]);
  }

  if (force_draw || update_geometry || (man->dirty_flags & REDRAW_BG)) {
    ConsoleDebug (X11, "\tredrawing background\n");
    clear_empty_region (man);
  }
  if (force_draw || update_geometry)
  {
    /* maybe not usefull but safe */
    if (man->colorsets[DEFAULT] >= 0 &&
	 Colorset[man->colorsets[DEFAULT]].pixmap == ParentRelative)
      force_draw = True;
  }

  man->dirty_flags = 0;
  man->buttons.dirty_flags = 0;
  man->buttons.drawn_num_buttons = man->buttons.num_buttons;
  man->buttons.drawn_num_windows = man->buttons.num_windows;

  if (man->buttons.num_windows == 0) {
    if (force_draw)
      draw_empty_manager (man);
  }
  else {
    /* I was having the problem where when the shape changed the manager
       wouldn't get redrawn. It appears we weren't getting the expose.
       How can I tell when I am going to reliably get an expose event? */

    if (1 || !shape_changed) {
      /* if shape changed, we'll catch it on the expose */
      for (i = 0; i < man->buttons.num_buttons; i++) {
	draw_button (man, i, force_draw);
      }
    }
  }
  man->drawn_geometry = man->geometry;
  XFlush (theDisplay);
}


static int compute_weight(WinData *win)
{
  WinManager *man;
  WeightedSort *sort;
  int i;

  man = win->manager;
  for (i = 0; i < man->weighted_sorts_len; i++) {
    sort = &man->weighted_sorts[i];
    if (sort->resname && !matchWildcards(sort->resname, win->resname)) {
      continue;
    }
    if (sort->classname && !matchWildcards(sort->classname, win->classname)) {
      continue;
    }
    if (sort->titlename && !matchWildcards(sort->titlename, win->titlename)) {
      continue;
    }
    if (sort->iconname && !matchWildcards(sort->iconname, win->iconname)) {
      continue;
    }
    return sort->weight;
  }
  return 0;
}

static int compare_windows(SortType type, WinData *a, WinData *b)
{
  int wa, wb;

  if (type == SortId) {
    return a->app_id - b->app_id;
  }
  else if (type == SortName) {
    return strcasecmp (a->display_string, b->display_string);
  }
  else if (type == SortNameCase) {
    return strcmp (a->display_string, b->display_string);
  }
  else if (type == SortWeighted) {
    wa = compute_weight(a);
    wb = compute_weight(b);
    if (wa != wb) {
      return wa - wb;
    }
    return strcmp(a->display_string, b->display_string);
  }
  else {
    ConsoleMessage ("Internal error in compare_windows\n");
    return 0;
  }
}

/* find_windows_spot: returns index of button to stick the window in.
 *                    checks win->button to see if it's already in manager.
 *                    if it isn't, then gives spot at which it should be,
 *                    if it were.
 */

static int find_windows_spot (WinData *win)
{
  WinManager *man = win->manager;
  int num_windows = man->buttons.num_windows;

  if (man->sort != SortNone) {
    int i, cur, start, finish, cmp_dir, correction;
    Button **bp;

    bp = man->buttons.buttons;
    if (win->button) {
      /* start search from our current location */
      cur = win->button->index;

      if (cur - 1 >= 0 &&
	  compare_windows (man->sort,
			   win, bp[cur - 1]->drawn_state.win) < 0) {
	start = cur - 1;
	finish = -1;
	cmp_dir = -1;
	correction = 1;
      }
      else if (cur < num_windows - 1 &&
	       compare_windows (man->sort,
				win, bp[cur + 1]->drawn_state.win) > 0) {
	start = cur + 1;
	finish = num_windows;
	cmp_dir = 1;
	correction = -1;
      }
      else {
	return cur;
      }
    }
    else {
      start = 0;
      finish = num_windows;
      cmp_dir = 1;
      correction = 0;
    }
    for (i = start; i != finish && bp[i]->drawn_state.win && cmp_dir *
	     compare_windows (man->sort, win, bp[i]->drawn_state.win) > 0;
	 i = i + cmp_dir)
      ;
    i += correction;
    ConsoleDebug (X11, "find_windows_spot: %s %d\n", win->display_string, i);
    return i;
  }
  else {
    if (win->button) {
      /* already have a perfectly fine spot */
      return win->button->index;
    }
    else {
      return num_windows;
    }
  }

  /* shouldn't get here */
  return -1;
}

static void move_window_buttons (WinManager *man, int start, int finish,
				 int offset)
{
  int n = man->buttons.num_buttons, i;
  Button **bp;

  ConsoleDebug (X11, "move_window_buttons: %s(%d): (%d, %d) + %d\n",
		man->titlename, n, start, finish, offset);

  if (finish >= n || finish + offset >= n || start < 0 || start + offset < 0) {
    ConsoleMessage ("Internal error in move_window_buttons\n");
    ConsoleMessage ("\tn = %d, start = %d, finish = %d, offset = %d\n",
		    n, start, finish, offset);
    return;
  }
  bp = man->buttons.buttons;
  if (offset > 0)
  {
    for (i = finish; i >= start; i--)
    {
      bp[i + offset]->drawn_state = bp[i]->drawn_state;
      bp[i + offset]->drawn_state.dirty_flags = ALL_CHANGED;
      if (bp[i + offset]->drawn_state.win)
	bp[i + offset]->drawn_state.win->button = bp[i + offset];
    }
    for (i = 0; i < offset; i++)
      clear_button(bp[start + i]);
  }
  else if (offset < 0)
  {
    for (i = start; i <= finish; i++)
    {
      bp[i + offset]->drawn_state = bp[i]->drawn_state;
      bp[i + offset]->drawn_state.dirty_flags = ALL_CHANGED;
      if (bp[i + offset]->drawn_state.win)
	bp[i + offset]->drawn_state.win->button = bp[i + offset];
    }
    for (i = 0; i > offset; i--)
      clear_button(bp[finish + i]);
  }
}

static void insert_windows_button (WinData *win)
{
  int spot;
  int selected_index = -1;
  WinManager *man = win->manager;
  ButtonArray *buttons;

  ConsoleDebug (X11, "insert_windows_button: %s\n", win->titlename);

  assert (man);
  selected_index = selected_button_in_man (man);

  if (win->button) {
    ConsoleDebug (X11, "insert_windows_button: POSSIBLE BUG: "
		  "already have a button\n");
    return;
  }

  if (!win || !win->complete || !man) {
    ConsoleMessage ("Internal error in insert_windows_button\n");
    ShutMeDown (1);
  }

  buttons = &man->buttons;

  spot = find_windows_spot (win);

  increase_num_windows (buttons, 1);
  move_window_buttons (man, spot, buttons->num_windows - 2, 1);

  set_window_button (win, spot);
  if (selected_index >= 0) {
    ConsoleDebug (X11, "insert_windows_button: selected_index = %d, moving\n",
		  selected_index);
    move_highlight (man, man->buttons.buttons[selected_index]);
  }
}

void delete_windows_button (WinData *win)
{
  int spot;
  int selected_index = -1;
  WinManager *man = (win->manager);
  ButtonArray *buttons;

  ConsoleDebug (X11, "delete_windows_button: %s\n", win->titlename);

  assert (man);

  buttons = &win->manager->buttons;

  assert (win->button);
  assert (buttons->buttons);

  selected_index = selected_button_in_man (man);
  ConsoleDebug (X11, "delete_windows_button: selected_index = %d\n",
		selected_index);

  spot = win->button->index;

  move_window_buttons (win->manager, spot + 1, buttons->num_windows - 1, -1);
  increase_num_windows (buttons, -1);
  win->button = NULL;
  if (globals.focus_win == win) {
    globals.focus_win = NULL;
  }
  if (selected_index >= 0) {
    ConsoleDebug (X11, "delete_windows_button: selected_index = %d, moving\n",
		  selected_index);
    move_highlight (man, man->buttons.buttons[selected_index]);
  }
  win->state = 0;
}

void resort_windows_button (WinData *win)
{
  int new_spot, cur_spot;
  int selected_index = -1;
  WinManager *man = win->manager;

  assert (win->button && man);

  ConsoleDebug (X11, "In resort_windows_button: %s\n", win->resname);

  selected_index = selected_button_in_man (man);

  new_spot = find_windows_spot (win);
  cur_spot = win->button->index;

  print_button_info (win->button);

  if (new_spot != cur_spot) {
    ConsoleDebug (X11, "resort_windows_button: win moves from %d to %d\n",
		    cur_spot, new_spot);
    if (new_spot < cur_spot) {
      move_window_buttons (man, new_spot, cur_spot - 1, +1);
    }
    else {
      move_window_buttons (man, cur_spot + 1, new_spot, -1);
    }
    set_window_button (win, new_spot);

    if (selected_index >= 0) {
      move_highlight (man, man->buttons.buttons[selected_index]);
    }
  }
}

void move_highlight (WinManager *man, Button *b)
{
  WinData *old;

  assert (man);

  ConsoleDebug (X11, "move_highlight\n");

  old = globals.select_win;

  if (old && old->button) {
    del_win_state (old, SELECT_CONTEXT);
    old->manager->select_button = NULL;
    draw_button (old->manager, old->button->index, 0);
  }
  if (b && b->drawn_state.win) {
    add_win_state (b->drawn_state.win, SELECT_CONTEXT);
    draw_button (man, b->index, 0);
    globals.select_win = b->drawn_state.win;
  }
  else {
    globals.select_win = NULL;
  }

  man->select_button = b;
}

void man_exposed (WinManager *man, XEvent *theEvent)
{
  int x1, y1, w1, h1;
  int x2, y2, w2, h2;
  int i;
  Button **bp;

  ConsoleDebug (X11, "manager: %s, got expose\n", man->titlename);

  x1 = theEvent->xexpose.x;
  y1 = theEvent->xexpose.y;
  w1 = theEvent->xexpose.width;
  h1 = theEvent->xexpose.height;

  w2 = man->geometry.boxwidth;
  h2 = man->geometry.boxheight;

  bp = man->buttons.buttons;

  /* Background must be redrawn */
  man->dirty_flags |= REDRAW_BG;

  if (FHaveShapeExtension)
  {
    /* There's some weird problem where if we change window shapes, we can't
       draw into buttons in the area NewShape intersect (not OldShape) until
       we get our Expose event. So, for now, just redraw everything when we
       get Expose events. This has the disadvantage of drawing buttons twice,
       but avoids having to match which expose event results from which shape
       change */
    if (man->buttons.num_windows) {
      for (i = 0; i < man->buttons.num_windows; i++) {
	bp[i]->drawn_state.dirty_flags |= REDRAW_BUTTON;
      }
    }
    else {
      draw_empty_manager (man);
    }

    return;
  }

  if (man->buttons.num_windows) {
    for (i = 0; i < man->buttons.num_windows; i++) {
      x2 = index_to_col (man, i) * w2;
      y2 = index_to_row (man, i) * h2;
      if (RECTANGLES_INTERSECT (x1, y1, w1, h1, x2, y2, w2, h2)) {
	bp[i]->drawn_state.dirty_flags |= REDRAW_BUTTON;
      }
    }
  }
  else {
    draw_empty_manager (man);
  }
}

/***************************************************************************/
/* Debugging routines                                                      */
/***************************************************************************/

void check_managers_consistency (void)
{
#ifdef PRINT_DEBUG
  int i, j;
  Button **b;

  for (i = 0; i < globals.num_managers; i++) {
    for (j = 0, b = globals.managers[i].buttons.buttons;
	 j < globals.managers[i].buttons.num_buttons; j++, b++) {
      if ((*b)->drawn_state.win && (*b)->drawn_state.win->button != *b) {
	ConsoleMessage ("manager %d, button %d is confused\n", i, j);
	abort ();
      }
      else if ((*b)->drawn_state.win &&
	       j >= globals.managers[i].buttons.num_windows) {
	ConsoleMessage ("manager %d: button %d has window and shouldn't\n",
			i, j);
	abort ();
      }
    }
  }
#endif
}

static void print_button_info (Button *b)
{
#ifdef PRINT_DEBUG
  ConsoleMessage ("button: %d\n", b->index);
  ConsoleMessage ("win: 0x%x\n", b->drawn_state.win);
  ConsoleMessage ("dirty: 0x%x\n", b->drawn_state.dirty_flags);
  if (b->drawn_state.win) {
    ConsoleMessage ("name: %s\n", b->drawn_state.display_string);
    ConsoleMessage ("iconified: %d state %d\n", b->drawn_state.iconified,
		    b->drawn_state.state);
    ConsoleMessage ("win->button: 0x%x\n", b->drawn_state.win->button);
  }
#endif
}

#ifdef PRINT_DEBUG
static void print_buttons (WinManager *man)
{
  int i;
  Button *b;

  ConsoleMessage ("Buttons for manager: %s\n", man->titlename);

  for (i = 0; i < man->buttons.num_buttons; i++) {
    b = man->buttons.buttons[i];
    ConsoleMessage ("Button: %d, index = %d\n", i, b->index);
    ConsoleMessage ("\tdirty flags: 0x%x\n", b->drawn_state.dirty_flags);
    ConsoleMessage ("\twin: 0x%x\n", b->drawn_state.win);
  }
}
#endif

