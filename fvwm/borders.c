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
 * fvwm window border drawing code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "libs/fvwmlib.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "borders.h"
#include "builtins.h"
#include "icons.h"
#include "module_interface.h"
#include "libs/Colorset.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

typedef struct
{
  int relief_width;
  GC relief_gc;
  GC shadow_gc;
  Pixel fore_color;
  Pixel back_color;
  Pixmap back_pixmap;
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  Pixmap texture_pixmap;
  XSetWindowAttributes notex_attributes;
  unsigned long notex_valuemask;

  struct
  {
    unsigned has_color_changed : 1;
  } flags;
} common_decorations_type;

XGCValues Globalgcv;
unsigned long Globalgcm;

extern Window PressedW;
extern FvwmDecor *cur_decor;

static void change_window_background(
  Window w, unsigned long valuemask, XSetWindowAttributes *attributes)
{
  XChangeWindowAttributes(dpy, w, valuemask, attributes);
  XClearWindow(dpy, w);
}

/****************************************************************************
 *
 *  Draws a little pattern within a window (more complex)
 *
 ****************************************************************************/
static void DrawLinePattern(
  Window win, GC ReliefGC, GC ShadowGC, struct vector_coords *coords, int w,
  int h)
{
  int i;
  for (i = 1; i < coords->num; ++i)
  {
      XDrawLine(dpy,win,
		(coords->line_style & (1 << i))  ? ReliefGC : ShadowGC,
		w * coords->x[i-1]/100,
		h * coords->y[i-1]/100,
		w * coords->x[i]/100,
		h * coords->y[i]/100);
  }
}

/****************************************************************************
 *
 *  Redraws buttons (veliaa@rpi.edu)
 *
 ****************************************************************************/
static void DrawButton(FvwmWindow *t, Window win, int w, int h,
		       DecorFace *df, GC ReliefGC, GC ShadowGC,
		       Boolean inverted, mwm_flags stateflags,
		       int left1right0)
{
  register DecorFaceType type = DFS_FACE_TYPE(df->style);
  Picture *p;
  int border = 0;
  int width, height, x, y;

  switch (type)
  {
    case SimpleButton:
      break;

    case SolidButton:
      XSetWindowBackground(dpy, win, df->u.back);
      flush_expose(win);
      XClearWindow(dpy,win);
      break;

    case VectorButton:
      if(HAS_MWM_BUTTONS(t) &&
	 ((stateflags & MWM_DECOR_MAXIMIZE && IS_MAXIMIZED(t)) ||
	  (stateflags & MWM_DECOR_SHADE && IS_SHADED(t)) ||
	  (stateflags & MWM_DECOR_STICK && IS_STICKY(t))))
	DrawLinePattern(win, ShadowGC, ReliefGC, &df->u.vector, w, h);
      else
	DrawLinePattern(win, ReliefGC, ShadowGC, &df->u.vector, w, h);
      break;

#ifdef MINI_ICONS
    case MiniIconButton:
    case PixmapButton:
      if (type == PixmapButton)
	p = df->u.p;
      else {
	if (!t->mini_icon)
	  break;
	p = t->mini_icon;
      }
#else
    case PixmapButton:
      p = df->u.p;
#endif /* MINI_ICONS */
      if (DFS_BUTTON_RELIEF(df->style) == DFS_BUTTON_IS_FLAT)
	border = 0;
      else
	border = HAS_MWM_BORDER(t) ? 1 : 2;
      width = w - border * 2; height = h - border * 2;

      x = border;
      if (DFS_H_JUSTIFICATION(df->style) == JUST_RIGHT)
	x += (int)(width - p->width);
      else if (DFS_H_JUSTIFICATION(df->style) == JUST_CENTER)
        /* round up for left buttons, down for right buttons */
	x += (int)(width - p->width + left1right0) / 2;

      y = border;
      if (DFS_V_JUSTIFICATION(df->style) == JUST_BOTTOM)
	  y += (int)(height - p->height);
      else if (DFS_V_JUSTIFICATION(df->style) == JUST_CENTER)
        /* round up */
	y += (int)(height - p->height + 1) / 2;

      if (x < border)
	x = border;
      if (y < border)
	y = border;
      if (width > p->width)
	width = p->width;
      if (height > p->height)
	height = p->height;
      if (width > w - x - border)
	width = w - x - border;
      if (height > h - y - border)
	height = h - y - border;

      XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
      XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
      XCopyArea(dpy, p->picture, win, Scr.TransMaskGC,
		0, 0, width, height, x, y);
      break;

    case TiledPixmapButton:
      XSetWindowBackgroundPixmap(dpy, win, df->u.p->picture);
      flush_expose(win);
      XClearWindow(dpy,win);
      break;

    case GradientButton:
    {
      unsigned int g_width;
      unsigned int g_height;

      flush_expose(win);
      XSetClipMask(dpy, Scr.TransMaskGC, None);
      /* find out the size the pixmap should be */
      CalculateGradientDimensions(
	dpy, win, df->u.grad.npixels, df->u.grad.gradient_type,
	&g_width, &g_height);
      /* draw the gradient directly into the window */
      CreateGradientPixmap(
	dpy, win, Scr.TransMaskGC, df->u.grad.gradient_type, g_width, g_height,
	df->u.grad.npixels, df->u.grad.pixels, win, 0, 0, w, h);
    }
    break;

    default:
      fvwm_msg(ERR,"DrawButton","unknown button type");
      break;
    }
}

/* rules to get button state */
static enum ButtonState get_button_state(
  Bool has_focus, Bool toggled, Window w)
{
  if (Scr.gs.use_active_down_buttons)
  {
    if (Scr.gs.use_inactive_buttons && !has_focus)
      return (toggled) ? ToggledInactive : Inactive;
    else
      return (PressedW == w)
	? (toggled ? ToggledActiveDown : ActiveDown)
	: (toggled ? ToggledActiveUp : ActiveUp);
  }
  else
  {
    if (Scr.gs.use_inactive_buttons && !has_focus)
    {
      return (toggled) ? ToggledInactive : Inactive;
    }
    else
    {
      return (toggled) ? ToggledActiveUp : ActiveUp;
    }
  }
}

static const char ulgc[] = { 1, 0, 0, -1, 2, 1, 1 };
static const char brgc[] = { 1, 1, 2, -1, 0, 0, 1 };

/* called twice by RedrawBorder */
static void draw_frame_relief(
  FvwmWindow *t, GC rgc, GC sgc, GC tgc, int w_dout, int w_hiout, int w_trout,
  int w_c, int w_trin, int w_shin, int w_din)
{
  int i;
  int offset = 0;
  int width = t->frame_g.width - 1;
  int height = t->frame_g.height - 1;
  int w[7];
  GC gc[3];

  w[0] = w_dout;
  w[1] = w_hiout;
  w[2] = w_trout;
  w[3] = w_c;
  w[4] = w_trin;
  w[5] = w_shin;
  w[6] = w_din;
  gc[0] = rgc;
  gc[1] = sgc;
  gc[2] = tgc;

  for (i = 0; i < 7; i++)
  {
    if (ulgc[i] != -1 && w[i] > 0)
    {
      RelieveRectangle(
	dpy, t->decor_w, offset, offset, width, height, gc[(int)ulgc[i]],
	gc[(int)brgc[i]], w[i]);
    }
    offset += w[i];
    width -= 2 * w[i];
    height -= 2 * w[i];
  }
}

/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
static void RedrawBorder(
  common_decorations_type *cd, FvwmWindow *t, Bool has_focus, int force,
  Window expose_win)
{
  static GC tgc = None;
  int i;
  GC rgc;
  GC sgc;
  DecorFaceStyle *borderstyle;
  Bool is_reversed = False;
  int w_dout;
  int w_hiout;
  int w_trout;
  int w_c;
  int w_trin;
  int w_shin;
  int w_din;
  int sum;
  int trim;

  if (tgc == None && !HAS_MWM_BORDER(t))
  {
    XGCValues xgcv;

    xgcv.function = GXnoop;
    xgcv.plane_mask = 0;
    tgc = XCreateGC(dpy, t->frame, GCFunction | GCPlaneMask, &xgcv);
  }
  /*
   * draw the border; resize handles are InputOnly so draw in the decor_w and
   * it will show through. The entire border is redrawn so flush all exposes
   * up to this point.
   */
  flush_expose(t->decor_w);

  if (t->boundary_width < 2)
  {
    /*
     * for mono - put a black border on
     */
    if (Pdepth <2)
    {
      XSetWindowBackgroundPixmap(dpy, t->decor_w, cd->back_pixmap);
      XClearWindow(dpy, t->decor_w);
    }
    else
    {
      if (cd->texture_pixmap != None)
	XSetWindowBackgroundPixmap(dpy, t->decor_w, cd->texture_pixmap);
      XClearWindow(dpy, t->decor_w);
    }
    return;
  }

  /*
   * for color, make it the color of the decoration background
   */

  /* get the border style bits */
  borderstyle = (has_focus) ?
    &GetDecor(t, BorderStyle.active.style) :
    &GetDecor(t, BorderStyle.inactive.style);

  is_reversed ^= (borderstyle->flags.button_relief == DFS_BUTTON_IS_SUNK);
  if (is_reversed)
  {
    sgc = cd->relief_gc;
    rgc = cd->shadow_gc;
  }
  else
  {
    rgc = cd->relief_gc;
    sgc = cd->shadow_gc;
  }

  /*
   * remove any lines drawn in the border if hidden handles or noinset and if
   * a handle was pressed
   */

  if (PressedW == None &&
      (DFS_HAS_NO_INSET(*borderstyle) ||
       DFS_HAS_HIDDEN_HANDLES(*borderstyle)))
    XClearWindow(dpy, t->decor_w);

  /*
   * draw the inside relief
   */

  if (borderstyle->flags.button_relief == DFS_BUTTON_IS_FLAT)
    return;

  /*
   * draw the inside relief
   */

  if (HAS_MWM_BORDER(t))
  {
    /* MWM borders look like this:
     *
     * HHCCCCS  from outside to inside on the left and top border
     * SSCCCCH  from outside to inside on the bottom and right border
     * |||||||
     * |||||||__ w_shin       (inner shadow area)
     * ||||||___ w_c          (transparent area)
     * |||||____ w_c          (transparent area)
     * ||||_____ w_c          (transparent area)
     * |||______ w_c          (transparent area)
     * ||_______ w_hiout      (outer hilight area)
     * |________ w_hiout      (outer hilight area)
     *
     *
     * C = original colour
     * H = hilight
     * S = shadow
     */
    w_dout = 0;
    w_hiout = 2;
    w_trout = 0;
    w_trin = 0;
    w_shin = 1;
    w_din = 0;
    sum = 3;
    trim = sum - t->boundary_width + 1;
  }
  else
  {
    /* Fvwm borders look like this:
     *
     * SHHCCSS  from outside to inside on the left and top border
     * SSCCHHS  from outside to inside on the bottom and right border
     * |||||||
     * |||||||__ w_din        (inner dark area)
     * ||||||___ w_shin       (inner shadow area)
     * |||||____ w_trin       (inner transparent/shadow area)
     * ||||_____ w_c          (transparent area)
     * |||______ w_trout      (outer transparent/hilight area)
     * ||_______ w_hiout      (outer hilight area)
     * |________ w_dout       (outer dark area)
     *
     * C = original colour
     * H = hilight
     * S = shadow
     *
     * reduced to 5 pixels it looks like this:
     *
     * SHHCS
     * SSCHS
     * |||||
     * |||||__ w_din        (inner dark area)
     * ||||___ w_trin       (inner transparent/shadow area)
     * |||____ w_trout      (outer transparent/hilight area)
     * ||_____ w_hiout      (outer hilight area)
     * |______ w_dout       (outer dark area)
     */
    w_dout = 1;
    w_hiout = 1;
    w_trout = 1;
    w_trin = 1;
    w_shin = 1;
    w_din = 1;
    /* w_trout + w_trin counts only as one pixel of border because they let one
     * pixel of the original colour shine through. */
    sum = 6;
    trim = sum - t->boundary_width;
  }
  if (DFS_HAS_NO_INSET(*borderstyle))
  {
    w_shin = 0;
    sum--;
    trim--;
  }
  /* If the border is too thin to accomodate the standard look, we remove parts
   * of the border so that at least one pixel of the original colour is
   * visible. We make an exception for windows with a border width of 2,
   * though. */
  if ((!IS_SHADED(t) || HAS_TITLE(t)) && t->boundary_width == 2)
    trim--;
  if (trim < 0)
    trim = 0;
  for ( ; trim > 0; trim--)
  {
    if (w_hiout > 1)
      w_hiout--;
    else if (w_shin > 0)
      w_shin--;
    else if (w_hiout > 0)
      w_hiout--;
    else if (w_trout > 0)
    {
      w_trout = 0;
      w_trin = 0;
      w_din = 0;
      w_hiout = 1;
    }
    sum--;
  }
  w_c = t->boundary_width - sum;
  draw_frame_relief(
    t, rgc, sgc, tgc, w_dout, w_hiout, w_trout, w_c, w_trin, w_shin, w_din);

  /*
   * draw the handle marks
   */

  /* draw the handles as eight marks around the border */
  if (HAS_BORDER(t) && (t->boundary_width > 1) &&
      !DFS_HAS_HIDDEN_HANDLES(*borderstyle))
  {
    /* MWM border windows have thin 3d effects
     * FvwmBorders have 3 pixels top/left, 2 bot/right so this makes
     * calculating the length of the marks difficult, top and bottom
     * marks for FvwmBorders are different if NoInset is specified */
    int tlength = t->boundary_width - 1;
    int blength = tlength;
    /* offset from bottom and right edge */
    int badjust = 3 - cd->relief_width;
    XSegment marks[8];
    int j;

    /* NoInset FvwmBorder windows need special treatment */
    if ((DFS_HAS_NO_INSET(*borderstyle)) && (cd->relief_width == 2))
      blength++;

    /*
     * draw the relief
     */

    for (j = 0; j < cd->relief_width; )
    {
      /* j is incremented below */
      /* shorten marks for beveled effect */
      tlength -= 2 * j;
      blength -= 2 * j;
      badjust += j;

      /* hilite marks */
      i = 0;

      /* top left */
      marks[i].x2 = marks[i].x1 = t->corner_width + j;
      marks[i].y2 = (marks[i].y1 = 1 + j) + tlength;
      i++;
      /* top right */
      marks[i].x2 = marks[i].x1 = t->frame_g.width - t->corner_width + j;
      marks[i].y2 = (marks[i].y1 = 1 + j) + tlength;
      i++;
      /* bot left */
      marks[i].x2 = marks[i].x1 = t->corner_width + j;
      marks[i].y2 = (marks[i].y1 = t->frame_g.height-badjust)-blength;
      i++;
      /* bot right */
      marks[i].x2 = marks[i].x1 = t->frame_g.width - t->corner_width+j;
      marks[i].y2 = (marks[i].y1 = t->frame_g.height-badjust)-blength;
      i++;

      if (!IS_SHADED(t))
      {
	/* left top */
	marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
	marks[i].y2 = marks[i].y1 = t->corner_width + j;
	i++;
	/* left bot */
	marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
	marks[i].y2 = marks[i].y1 = t->frame_g.height-t->corner_width+j;
	i++;
	/* right top */
	marks[i].x2 = (marks[i].x1 = t->frame_g.width-badjust) - blength;
	marks[i].y2 = marks[i].y1 = t->corner_width + j;
	i++;
	/* right bot */
	marks[i].x2 = (marks[i].x1 = t->frame_g.width-badjust) - blength;
	marks[i].y2 = marks[i].y1 = t->frame_g.height-t->corner_width +j;
	i++;
      }

      XDrawSegments(dpy, t->decor_w, rgc, marks, i);

      /* shadow marks, reuse the array assuming XDraw doesn't trash it */
      i = 0;
      /* yuck, but j goes 0->1 in the first pass, 1->3 in 2nd */
      j += j + 1;
      /* top left */
      marks[i].x2 = (marks[i].x1 -= j);
      i++;
      /* top right */
      marks[i].x2 = (marks[i].x1 -= j);
      i++;
      /* bot left */
      marks[i].x2 = (marks[i].x1 -= j);
      i++;
      /* bot right */
      marks[i].x2 = (marks[i].x1 -= j);
      i++;
      if (!IS_SHADED(t))
      {
	/* left top */
	marks[i].y2 = (marks[i].y1 -= j);
	i++;
	/* left bot */
	marks[i].y2 = (marks[i].y1 -= j);
	i++;
	/* right top */
	marks[i].y2 = (marks[i].y1 -= j);
	i++;
	/* right bot */
	marks[i].y2 = (marks[i].y1 -= j);
	i++;
      }
      XDrawSegments(dpy, t->decor_w, sgc, marks, i);
    }
  }

  /*
   * now draw the pressed in part on top if we have depressable borders
   */

  /* a bit hacky to draw twice but you should see the code it replaces, never
   * mind the esoterics, feel the thin-ness */
  if ((HAS_BORDER(t) || PressedW == t->decor_w || PressedW == t->frame) &&
      HAS_DEPRESSABLE_BORDER(t))
  {
    XRectangle r;
    Bool is_pressed = False;

    if (PressedW == t->sides[0])
    {
      /* top */
      r.x = t->corner_width;
      r.y = 1;
      r.width = t->frame_g.width - 2 * t->corner_width;
      r.height = t->boundary_width - 1;
      is_pressed = True;
    }

    else if (PressedW == t->sides[1])
    {
      /* right */
      r.x = t->frame_g.width - t->boundary_width;
      r.y = t->corner_width;
      r.width = t->boundary_width - 1;
      r.height = t->frame_g.height - 2 * t->corner_width;
      is_pressed = True;
    }
    else if (PressedW == t->sides[2])
    {
      /* bottom */
      r.x = t->corner_width;
      r.y = t->frame_g.height - t->boundary_width;
      r.width = t->frame_g.width - 2 * t->corner_width;
      r.height = t->boundary_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->sides[3])
    {
      /* left */
      r.x = 1;
      r.y = t->corner_width;
      r.width = t->boundary_width - 1;
      r.height = t->frame_g.height - 2 * t->corner_width;
      is_pressed = True;
    }
    else if (PressedW == t->corners[0])
    {
      /* top left */
      r.x = 1;
      r.y = 1;
      r.width = t->corner_width - 1;
      r.height = t->corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[1])
    {
      /* top right */
      r.x = t->frame_g.width - t->corner_width;
      r.y = 1;
      r.width = t->corner_width - 1;
      r.height = t->corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[2])
    {
      /* bottom left */
      r.x = 1;
      r.y = t->frame_g.height - t->corner_width;
      r.width = t->corner_width - 1;
      r.height = t->corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[3])
    {
      /* bottom right */
      r.x = t->frame_g.width - t->corner_width;
      r.y = t->frame_g.height - t->corner_width;
      r.width = t->corner_width - 1;
      r.height = t->corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->decor_w || PressedW == t->frame)
    {
      /* whole border */
      r.x = 1;
      r.y = 1;
      r.width = t->frame_g.width - 2;
      r.height = t->frame_g.height - 2;
      is_pressed = True;
    }

    if (is_pressed == True)
    {
      draw_frame_relief(
	t, sgc, rgc, tgc, w_dout, w_hiout, w_trout, w_c, w_trin, w_shin, w_din);
      XSetClipMask(dpy, sgc, None);
      XSetClipMask(dpy, rgc, None);
    }
  }
}

/****************************************************************************
 *
 * Redraws the window buttons
 *
 ****************************************************************************/
static void RedrawButtons(
  common_decorations_type *cd, FvwmWindow *t, Bool has_focus, int force,
  Window expose_win)
{
  int i;

  /*
   * draw buttons
   */

  for (i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    if (HAS_BUTTON(t, i))
    {
      mwm_flags stateflags =
	TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i]));
      Bool toggled =
	(HAS_MWM_BUTTONS(t) &&
	 ((stateflags & MWM_DECOR_MAXIMIZE && IS_MAXIMIZED(t)) ||
	  (stateflags & MWM_DECOR_SHADE && IS_SHADED(t)) ||
	  (stateflags & MWM_DECOR_STICK && IS_STICKY(t))));
      enum ButtonState bs =
	get_button_state(has_focus, toggled, t->button_w[i]);
      DecorFace *df = &TB_STATE(GetDecor(t, buttons[i]))[bs];
      if(flush_expose(t->button_w[i]) || expose_win == t->button_w[i] ||
	 expose_win == None || cd->flags.has_color_changed)
      {
	int inverted = PressedW == t->button_w[i];
	if (DFS_USE_BORDER_STYLE(df->style))
	{
	  XChangeWindowAttributes(
	    dpy, t->button_w[i], cd->valuemask, &cd->attributes);
	}
	else
	{
	  XChangeWindowAttributes(
	    dpy, t->button_w[i], cd->notex_valuemask, &cd->notex_attributes);
	}
	XClearWindow(dpy, t->button_w[i]);
	if (DFS_USE_TITLE_STYLE(df->style))
	{
	  DecorFace *tsdf = &TB_STATE(GetDecor(t, titlebar))[bs];
#ifdef MULTISTYLE
	  for (; tsdf; tsdf = tsdf->next)
#endif
	  {
	    DrawButton(t, t->button_w[i], t->title_g.height, t->title_g.height,
		       tsdf, cd->relief_gc, cd->shadow_gc, inverted,
		       TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])), 1);
	  }
	}
#ifdef MULTISTYLE
	for (; df; df = df->next)
#endif
	{
	  DrawButton(t, t->button_w[i], t->title_g.height, t->title_g.height,
		     df, cd->relief_gc, cd->shadow_gc, inverted,
		     TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])), 1);
	}

	{
	  Bool reverse = inverted;

	  switch (DFS_BUTTON_RELIEF(
	    TB_STATE(GetDecor(t, buttons[i]))[bs].style))
	  {
	  case DFS_BUTTON_IS_SUNK:
	    reverse = !inverted;
	  case DFS_BUTTON_IS_UP:
	    RelieveRectangle(
	      dpy, t->button_w[i], 0, 0, t->title_g.height - 1,
	      t->title_g.height - 1, (reverse ? cd->shadow_gc : cd->relief_gc),
	      (reverse ? cd->relief_gc : cd->shadow_gc), cd->relief_width);
	    break;
	  default:
	    /* flat */
	    break;
	  }
	}
      }
    } /* if (HAS_BUTTON(i))*/
  } /* for */
}


/****************************************************************************
 *
 *  Redraws just the title bar
 *
 ****************************************************************************/
void RedrawTitle(common_decorations_type *cd, FvwmWindow *t, Bool has_focus)
{
  int hor_off;
  int w;
  int i;
  enum ButtonState title_state;
  DecorFaceStyle *tb_style;
  GC rgc = cd->relief_gc;
  GC sgc = cd->shadow_gc;
  Bool reverse = False;
  Bool toggled =
    (HAS_MWM_BUTTONS(t) &&
     ((TB_HAS_MWM_DECOR_MAXIMIZE(GetDecor(t, titlebar)) && IS_MAXIMIZED(t))||
      (TB_HAS_MWM_DECOR_SHADE(GetDecor(t, titlebar)) && IS_SHADED(t)) ||
      (TB_HAS_MWM_DECOR_STICK(GetDecor(t, titlebar)) && IS_STICKY(t))));

  if (PressedW == t->title_w)
  {
    GC tgc;

    tgc = sgc;
    sgc = rgc;
    rgc = tgc;
  }
  flush_expose(t->title_w);

  if (t->name != (char *)NULL)
  {
#ifdef I18N_MB /* cannot use macro here, rewriting... */
    w = XmbTextEscapement(t->title_font.fontset,t->name, strlen(t->name));
#else
    w = XTextWidth(t->title_font.font, t->name, strlen(t->name));
#endif
    if (w > t->title_g.width - 12)
      w = t->title_g.width - 4;
    if (w < 0)
      w = 0;
  }
  else
  {
    w = 0;
  }

  title_state = get_button_state(has_focus, toggled, t->title_w);
  tb_style = &TB_STATE(GetDecor(t, titlebar))[title_state].style;
  switch (TB_JUSTIFICATION(GetDecor(t, titlebar)))
  {
  case JUST_LEFT:
    hor_off = 10;
    break;
  case JUST_RIGHT:
    hor_off = t->title_g.width - w - 10;
    break;
  case JUST_CENTER:
  default:
    hor_off = (t->title_g.width - w) / 2;
    break;
  }

  NewFontAndColor(
    t->title_font.font->fid, cd->fore_color, cd->back_color);

  /* the next bit tries to minimize redraw (veliaa@rpi.edu) */
  /* we need to check for UseBorderStyle for the titlebar */
  {
    DecorFace *df = (has_focus) ?
      &GetDecor(t,BorderStyle.active) : &GetDecor(t,BorderStyle.inactive);

    if (DFS_USE_BORDER_STYLE(*tb_style) &&
	DFS_FACE_TYPE(df->style) == TiledPixmapButton)
    {
      XSetWindowBackgroundPixmap(dpy, t->title_w, df->u.p->picture);
    }
  }
  XClearWindow(dpy, t->title_w);

  /*
   * draw title
   */

  if (Pdepth < 2)
  {
    /* for mono, we clear an area in the title bar where the window
     * title goes, so that its more legible. For color, no need */
    RelieveRectangle(
      dpy, t->title_w, 0, 0, hor_off - 3, t->title_g.height - 1, rgc, sgc,
      cd->relief_width);
    RelieveRectangle(
      dpy, t->title_w, hor_off + w + 2, 0, t->title_g.width - w - hor_off - 3,
      t->title_g.height - 1, rgc, sgc, cd->relief_width);
    XFillRectangle(
      dpy, t->title_w, ((PressedW == t->title_w) ? sgc : rgc), hor_off - 2, 0,
      w+4,t->title_g.height);
    XDrawLine(
      dpy, t->title_w, sgc, hor_off + w + 1, 0, hor_off + w + 1,
      t->title_g.height);
    if(t->name != (char *)NULL)
#ifdef I18N_MB
      XmbDrawString(dpy, t->title_w, GetDecor(t,WindowFont.fontset),
		    Scr.TitleGC, hor_off,
		    t->title_font.y + 1, t->name, strlen(t->name));
#else
      XDrawString(dpy, t->title_w, Scr.TitleGC, hor_off,
                  t->title_font.y + 1, t->name, strlen(t->name));
#endif
  }
  else
  {
    DecorFace *df = &TB_STATE(GetDecor(t, titlebar))[title_state];
    /* draw compound titlebar (veliaa@rpi.edu) */
    if (PressedW == t->title_w)
    {
#ifdef MULTISTYLE
      for (; df; df = df->next)
#endif
      {
	DrawButton(
	  t, t->title_w, t->title_g.width, t->title_g.height, df, sgc,
	  rgc, True, 0, 1);
      }
    }
    else
    {
#ifdef MULTISTYLE
      for (; df; df = df->next)
#endif
      {
	DrawButton(
	  t, t->title_w, t->title_g.width, t->title_g.height, df, rgc,
	  sgc, False, 0, 1);
      }
    }

    /*
     * draw title relief
     */

    switch (DFS_BUTTON_RELIEF(*tb_style))
    {
    case DFS_BUTTON_IS_SUNK:
      reverse = 1;
    case DFS_BUTTON_IS_UP:
      RelieveRectangle(
	dpy, t->title_w, 0, 0, t->title_g.width - 1, t->title_g.height - 1,
	(reverse) ? sgc : rgc, (reverse) ? rgc : sgc, cd->relief_width);
      break;
    default:
      /* flat */
      break;
    }

#ifdef I18N_MB
    if(t->name != (char *)NULL)
      XmbDrawString(dpy, t->title_w, GetDecor(t, WindowFont.fontset),
		    Scr.TitleGC, hor_off,
		    t->title_font.y + 1, t->name, strlen(t->name));
#else
    if(t->name != (char *)NULL)
      XDrawString(dpy, t->title_w, Scr.TitleGC, hor_off,
		  t->title_font.y + 1, t->name, strlen(t->name));
#endif
  }

  /*
   * draw the 'sticky' lines
   */

  if (IS_STICKY(t) || Scr.go.StipledTitles)
  {
    /* an odd number of lines every 4 pixels */
    int num = (int)(t->title_g.height / 8) * 2 - 1;
    int min = t->title_g.height / 2 - num * 2 + 1;
    int max = t->title_g.height / 2 + num * 2 - 3;

    for(i = min; i <= max; i += 4)
    {
      RelieveRectangle(dpy, t->title_w, 4, i, hor_off - 10, 1, sgc, rgc, 1);
      RelieveRectangle(
	dpy, t->title_w, hor_off + w + 6, i,
	t->title_g.width - hor_off - w - 11, 1, sgc, rgc, 1);
    }
  }
}


void SetupTitleBar(FvwmWindow *tmp_win, int w, int h)
{
  XWindowChanges xwc;
  unsigned long xwcm;
  int i;

  xwcm = CWWidth | CWX | CWY | CWHeight;
  tmp_win->title_g.x = tmp_win->boundary_width+
    (Scr.nr_left_buttons)*tmp_win->title_g.height;
  if(tmp_win->title_g.x >=  w - tmp_win->boundary_width)
    tmp_win->title_g.x = -10;
  if (HAS_BOTTOM_TITLE(tmp_win))
  {
    tmp_win->title_g.y =
      h - tmp_win->boundary_width - tmp_win->title_g.height;
  }
  else
  {
    tmp_win->title_g.y = tmp_win->boundary_width;
  }

  xwc.width = tmp_win->title_g.width;
  xwc.height = tmp_win->title_g.height;
  xwc.x = tmp_win->title_g.x;
  xwc.y = tmp_win->title_g.y;
  XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);

  xwcm = CWX | CWY | CWHeight | CWWidth;
  xwc.height = tmp_win->title_g.height;
  xwc.width = tmp_win->title_g.height;
  xwc.y = tmp_win->title_g.y;

  /* left */
  xwc.x = tmp_win->boundary_width;
  for(i = 0; i < Scr.nr_left_buttons; i++)
  {
    if(tmp_win->button_w[i] != None)
    {
      if(xwc.x + tmp_win->title_g.height < w-tmp_win->boundary_width)
      {
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
	xwc.x += tmp_win->title_g.height;
      }
      else
      {
	xwc.x = -tmp_win->title_g.height;
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
      }
    }
  }
  /* right */
  xwc.x = w - tmp_win->boundary_width;
  for(i = NR_LEFT_BUTTONS ; i < Scr.nr_right_buttons + NR_LEFT_BUTTONS; i++)
  {
    if(tmp_win->button_w[i] != None)
    {
      xwc.x -= tmp_win->title_g.height;
      if (xwc.x > tmp_win->boundary_width)
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
      else
      {
	xwc.x = -tmp_win->title_g.height;
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
      }
    }
  }
}


static void get_common_decorations(
  common_decorations_type *cd, FvwmWindow *t, draw_window_parts draw_parts,
  Bool has_focus, int force, Window expose_win)
{
  color_quad *draw_colors;

  if (has_focus)
  {
    /* are we using textured borders? */
    if (DFS_FACE_TYPE(
      GetDecor(t, BorderStyle.active.style)) == TiledPixmapButton)
    {
      cd->texture_pixmap = GetDecor(t, BorderStyle.active.u.p->picture);
    }
    cd->back_pixmap= Scr.gray_pixmap;
    draw_colors = &(t->hicolors);
  }
  else
  {
    if (DFS_FACE_TYPE(GetDecor(t, BorderStyle.inactive.style)) ==
	TiledPixmapButton)
    {
      cd->texture_pixmap = GetDecor(t, BorderStyle.inactive.u.p->picture);
    }
    if(IS_STICKY(t))
      cd->back_pixmap = Scr.sticky_gray_pixmap;
    else
    cd->back_pixmap = Scr.light_gray_pixmap;
    draw_colors = &(t->colors);
  }
  cd->fore_color = draw_colors->fore;
  cd->back_color = draw_colors->back;
  Globalgcv.foreground = draw_colors->hilight;
  Globalgcm = GCForeground;
  XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
  cd->relief_gc = Scr.ScratchGC1;
  Globalgcv.foreground = draw_colors->shadow;
  XChangeGC(dpy, Scr.ScratchGC2, Globalgcm, &Globalgcv);
  cd->shadow_gc = Scr.ScratchGC2;

  /* MWMBorder style means thin 3d effects */
  cd->relief_width = (HAS_MWM_BORDER(t) ? 1 : 2);

  if (cd->texture_pixmap)
  {
    cd->attributes.background_pixmap = cd->texture_pixmap;
    cd->valuemask = CWBackPixmap;
    if (Pdepth < 2)
    {
      cd->notex_attributes.background_pixmap = cd->back_pixmap;
      cd->notex_valuemask = CWBackPixmap;
    }
    else
    {
      cd->notex_attributes.background_pixel = cd->back_color;
      cd->notex_valuemask = CWBackPixel;
    }
  }
  else
  {
    if (Pdepth < 2)
    {
      cd->attributes.background_pixmap = cd->back_pixmap;
      cd->valuemask = CWBackPixmap;
      cd->notex_attributes.background_pixmap = cd->back_pixmap;
      cd->notex_valuemask = CWBackPixmap;
    }
    else
    {
      cd->attributes.background_pixel = cd->back_color;
      cd->valuemask = CWBackPixel;
      cd->notex_attributes.background_pixel = cd->back_color;
      cd->notex_valuemask = CWBackPixel;
    }
  }
}

void DrawDecorations(
  FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
  Window expose_win)
{
  common_decorations_type cd;
  Bool is_frame_redraw_allowed = False;
  Bool is_button_redraw_allowed = False;
  Bool is_title_redraw_allowed = False;

  if (!t)
    return;
  memset(&cd, 0, sizeof(cd));
  if (HAS_NEVER_FOCUS(t))
    has_focus = False;
  if (force || expose_win == None)
  {
    is_frame_redraw_allowed = True;
    is_button_redraw_allowed = True;
    is_title_redraw_allowed = True;
  }
  else if (expose_win == t->title_w)
    is_title_redraw_allowed = True;
  else if (expose_win == t->frame || expose_win == t->decor_w ||
           (HAS_BORDER(t) &&
            (expose_win == t->sides[0] || expose_win == t->corners[0] ||
             expose_win == t->sides[1] || expose_win == t->corners[1] ||
             expose_win == t->sides[2] || expose_win == t->corners[2] ||
             expose_win == t->sides[3] || expose_win == t->corners[3])))
  {
    is_frame_redraw_allowed = True;
  }
  else if (expose_win != None)
    is_button_redraw_allowed = True;

  if (has_focus)
  {
    /* don't re-draw just for kicks */
    if(!force && Scr.Hilite == t)
    {
      is_frame_redraw_allowed = False;
      is_button_redraw_allowed = False;
      is_title_redraw_allowed = False;
    }
    else
    {
      if (Scr.Hilite != t || force > 1)
	cd.flags.has_color_changed = True;
      if (Scr.Hilite != t && Scr.Hilite != NULL)
      {
	/* make sure that the previously highlighted window got
	 * unhighlighted */
	DrawDecorations(Scr.Hilite, DRAW_ALL, False, False, None);
      }
      Scr.Hilite = t;
    }
  }
  else
  {
    /* don't re-draw just for kicks */
    if (!force && Scr.Hilite != t)
    {
      is_frame_redraw_allowed = False;
      is_button_redraw_allowed = False;
      is_title_redraw_allowed = False;
    }
    else if (Scr.Hilite == t || force > 1)
    {
      cd.flags.has_color_changed = True;
      Scr.Hilite = NULL;
    }
  }
  if(IS_ICONIFIED(t))
  {
    DrawIconWindow(t);
    return;
  }

  get_common_decorations(
    &cd, t, draw_parts, has_focus, force, expose_win);

  if (cd.flags.has_color_changed)
  {
    change_window_background(t->decor_w, cd.valuemask, &cd.attributes);
    if (HAS_TITLE(t))
    {
      change_window_background(
	t->title_w, cd.notex_valuemask, &cd.notex_attributes);
    }
  }

  if ((draw_parts & DRAW_FRAME) && is_frame_redraw_allowed)
    RedrawBorder(&cd, t, has_focus, force, expose_win);
  if ((draw_parts & DRAW_BUTTONS) && HAS_TITLE(t) && is_button_redraw_allowed)
    RedrawButtons(&cd, t, has_focus, force, expose_win);
  if ((draw_parts & DRAW_TITLE) && HAS_TITLE(t) && is_title_redraw_allowed)
    RedrawTitle(&cd, t, has_focus);

  /* Sync to make the change look fast! */
  XSync(dpy,0);
}


/***********************************************************************
 *
 *  Procedure:
 *      SetupFrame - set window sizes
 *
 *  Inputs:
 *      tmp_win - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/

void SetupFrame(
  FvwmWindow *tmp_win, int x, int y, int w, int h, Bool sendEvent)
{
  XEvent client_event;
  XWindowChanges xwc;
  unsigned long xwcm;
  int i;
  Bool is_resized = False;
  Bool is_moved = False;
  int xwidth;
  int ywidth;
  int left;
  int right;
  int shaded = IS_SHADED(tmp_win);

  int xmove_sign;
  int ymove_sign;
  int xsize_sign;
  int ysize_sign;
  int i0 = 0;
  int i1 = 0;
  int id = 0;
  int dx;
  int dy;
  int px;
  int py;
  int decor_gravity;
  Bool is_order_reversed = False;

#ifdef FVWM_DEBUG_MSGS
  fvwm_msg(DBG,"SetupFrame",
           "Routine Entered (x == %d, y == %d, w == %d, h == %d)",
           x, y, w, h);
#endif

  if (w != tmp_win->frame_g.width || h != tmp_win->frame_g.height)
    is_resized = True;
  if (x != tmp_win->frame_g.x || y != tmp_win->frame_g.y)
    is_moved = True;

  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if (is_moved && !is_resized)
    sendEvent = True;

  /* optimization code for opaque resize */
#define SIGN(x) (((x) < 0) ? -1 : !!(x))
  xmove_sign = SIGN(x - tmp_win->frame_g.x);
  ymove_sign = SIGN(y - tmp_win->frame_g.y);
  xsize_sign = SIGN(w - tmp_win->frame_g.width);
  ysize_sign = SIGN(h - tmp_win->frame_g.height);
#undef SIGN(x)

  if (HAS_BOTTOM_TITLE(tmp_win))
  {
    decor_gravity = SouthEastGravity;
  }
  else
  {
    decor_gravity = NorthWestGravity;
  }

  if (xsize_sign != 0 || ysize_sign != 0)
  {
    if (ysize_sign == 0 || xsize_sign > 0)
      is_order_reversed = (xsize_sign >= 0);
    else
      is_order_reversed = (ysize_sign >= 0);
  }
  else
  {
    is_order_reversed = False;
  }

  /*
   * Set up the title geometry first
   */

  if (is_resized)
  {
    left = tmp_win->nr_left_buttons;
    right = tmp_win->nr_right_buttons;
    tmp_win->title_g.width = w - (left + right) * tmp_win->title_g.height
                           - 2 * tmp_win->boundary_width;
    if(tmp_win->title_g.width < 1)
      tmp_win->title_g.width = 1;
  }

  /*
   * Now set up the client, the parent and the frame window
   */

  set_decor_gravity(
    tmp_win, decor_gravity, NorthWestGravity, NorthWestGravity);
  tmp_win->attr.width = w - 2 * tmp_win->boundary_width;
  tmp_win->attr.height =
    h - tmp_win->title_g.height - 2*tmp_win->boundary_width;
  if (tmp_win->attr.height <= 1 || tmp_win->attr.width <= 1)
    is_order_reversed = 0;
  px = tmp_win->boundary_width;
  if (HAS_BOTTOM_TITLE(tmp_win))
    py = tmp_win->boundary_width;
  else
    py = tmp_win->title_g.height + tmp_win->boundary_width;
  dx = 0;
  dy = 0;
  if (is_order_reversed && decor_gravity == SouthEastGravity)
  {
    dx = tmp_win->frame_g.width - w;
    dy = tmp_win->frame_g.height - h;
  }

  if (is_order_reversed)
  {
    i0 = 3;
    i1 = -1;
    id = -1;
  }
  else
  {
    i0 = 0;
    i1 = 4;
    id = 1;
  }
  for (i = i0; i != i1; i += id)
  {
    switch(i)
    {
    case 0:
      /*
       * fix up frame and assign size/location values in tmp_win
       */
      XMoveResizeWindow(dpy, tmp_win->frame, x, y, w, h);
      break;
    case 1:
      if (!shaded)
      {
	XMoveResizeWindow(
	dpy, tmp_win->Parent, px, py, max(tmp_win->attr.width, 1),
	max(tmp_win->attr.height, 1));
      }
      break;
    case 2:
      if (!shaded)
      {
	XResizeWindow(
	  dpy, tmp_win->w, max(tmp_win->attr.width, 1),
	  max(tmp_win->attr.height, 1));
	/* This reduces flickering */
	XSync(dpy, 0);
      }
      break;
    case 3:
      XMoveResizeWindow(dpy, tmp_win->decor_w, dx, dy, w, h);
      break;
    }
  }
  set_decor_gravity(
    tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
  XMoveWindow(dpy, tmp_win->Parent, px, py);
  XMoveWindow(dpy, tmp_win->decor_w, 0, 0);
  tmp_win->frame_g.x = x;
  tmp_win->frame_g.y = y;
  tmp_win->frame_g.width = w;
  tmp_win->frame_g.height = h;

  /*
   * Set up the decoration windows
   */

  if (is_resized)
  {
    if (HAS_TITLE(tmp_win))
    {
      SetupTitleBar(tmp_win, w, h);
    }

    if(HAS_BORDER(tmp_win))
    {
      int add;

      if(w < 2*tmp_win->corner_width)
        tmp_win->corner_width = w/3;
      if((h < 2*tmp_win->corner_width) && (!shaded))
        tmp_win->corner_width = h/3;
      xwidth = w - 2*tmp_win->corner_width;
      ywidth = h - 2*tmp_win->corner_width;
      xwcm = CWWidth | CWHeight | CWX | CWY;
      if(xwidth<2)
        xwidth = 2;
      if(ywidth<2)
        ywidth = 2;

      if (IS_SHADED(tmp_win))
	add = tmp_win->corner_width / 3;
      else
	add = 0;
      for(i = 0; i < 4; i++)
      {
        if(i==0)
        {
          xwc.x = tmp_win->corner_width;
          xwc.y = 0;
          xwc.height = tmp_win->boundary_width;
          xwc.width = xwidth;
        }
        else if (i==1)
        {
	  xwc.x = w - tmp_win->boundary_width;
          xwc.y = tmp_win->corner_width;
	  if (IS_SHADED(tmp_win))
	  {
	    xwc.y /= 3;
	    xwc.height = add + 2;
	  }
	  else
	  {
	    xwc.height = ywidth;
	  }
          xwc.width = tmp_win->boundary_width;

        }
        else if(i==2)
        {
          xwc.x = tmp_win->corner_width;
          xwc.y = h - tmp_win->boundary_width;
          xwc.height = tmp_win->boundary_width;
          xwc.width = xwidth;
        }
        else
        {
	  xwc.x = 0;
          xwc.y = tmp_win->corner_width;
	  if (IS_SHADED(tmp_win))
	  {
	    xwc.y /= 3;
	    xwc.height = add + 2;
	  }
	  else
	  {
	    xwc.height = ywidth;
	  }
          xwc.width = tmp_win->boundary_width;
        }
	XConfigureWindow(dpy, tmp_win->sides[i], xwcm, &xwc);
      }

      xwcm = CWX|CWY|CWWidth|CWHeight;
      xwc.width = tmp_win->corner_width;
      xwc.height = tmp_win->corner_width;
      for(i = 0; i < 4; i++)
      {
        if (i & 0x1)
          xwc.x = w - tmp_win->corner_width;
        else
          xwc.x = 0;

        if (i & 0x2)
          xwc.y = h - tmp_win->corner_width + add;
        else
          xwc.y = -add;

	XConfigureWindow(dpy, tmp_win->corners[i], xwcm, &xwc);
      }

    }
  }


  /*
   * Set up window shape
   */

#ifdef SHAPE
  if (ShapesSupported)
  {
    if (is_resized && tmp_win->wShaped)
    {
      SetShape(tmp_win, w);
    }
  }
#endif /* SHAPE */

  /*
   * Send ConfigureNotify
   */

  XSync(dpy,0);
  /* must not send events to shaded windows because this might cause them to
   * look at their current geometry */
  if (sendEvent && !shaded)
  {
    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = tmp_win->w;
    client_event.xconfigure.window = tmp_win->w;

    client_event.xconfigure.x = x + tmp_win->boundary_width;
    client_event.xconfigure.y =
      y + ((HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height) +
      tmp_win->boundary_width;
    client_event.xconfigure.width = w-2*tmp_win->boundary_width;
    client_event.xconfigure.height =h-2*tmp_win->boundary_width -
      tmp_win->title_g.height;

    client_event.xconfigure.border_width = 0;
    /* Real ConfigureNotify events say we're above title window, so ... */
    /* what if we don't have a title ????? */
    client_event.xconfigure.above = tmp_win->frame;
    client_event.xconfigure.override_redirect = False;
    XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
#ifdef FVWM_DEBUG_MSGS
    fvwm_msg(DBG,"SetupFrame","Sent ConfigureNotify (w == %d, h == %d)",
             client_event.xconfigure.width,client_event.xconfigure.height);
#endif
#if 1
    /* This is for buggy tk, which waits for the real ConfigureNotify
       on frame instead of the synthetic one on w. The geometry data
       in the event will not be correct for the frame, but tk doesn't
       look at that data anyway. */
    client_event.xconfigure.event = tmp_win->frame;
    client_event.xconfigure.window = tmp_win->frame;

    XSendEvent(dpy, tmp_win->frame, False,StructureNotifyMask,&client_event);
#endif
  }

  XSync(dpy,0);
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
#if 0
fprintf(stderr,"sf: window '%s'\n", tmp_win->name);
fprintf(stderr,"    frame:  %5d %5d, %4d x %4d\n", tmp_win->frame_g.x, tmp_win->frame_g.y, tmp_win->frame_g.width, tmp_win->frame_g.height);
fprintf(stderr,"    normal: %5d %5d, %4d x %4d\n", tmp_win->normal_g.x, tmp_win->normal_g.y, tmp_win->normal_g.width, tmp_win->normal_g.height);
if (IS_MAXIMIZED(tmp_win))
fprintf(stderr,"    max:    %5d %5d, %4d x %4d, %5d %5d\n", tmp_win->max_g.x, tmp_win->max_g.y, tmp_win->max_g.width, tmp_win->max_g.height, tmp_win->max_offset.x, tmp_win->max_offset.y);
#endif
}

void ForceSetupFrame(
  FvwmWindow *tmp_win, int x, int y, int w, int h, Bool sendEvent)
{
  tmp_win->frame_g.x = x + 1;
  tmp_win->frame_g.y = y + 1;
  tmp_win->frame_g.width = 0;
  tmp_win->frame_g.height = 0;
  SetupFrame(tmp_win, x, y, w, h, sendEvent);
}

void set_decor_gravity(
  FvwmWindow *tmp_win, int gravity, int parent_gravity, int client_gravity)
{
  int valuemask = CWWinGravity;
  XSetWindowAttributes xcwa;
  int i;

  xcwa.win_gravity = client_gravity;
  XChangeWindowAttributes(dpy, tmp_win->w, valuemask, &xcwa);
  xcwa.win_gravity = parent_gravity;
  XChangeWindowAttributes(dpy, tmp_win->Parent, valuemask, &xcwa);
  xcwa.win_gravity = gravity;
  XChangeWindowAttributes(dpy, tmp_win->decor_w, valuemask, &xcwa);
  if (HAS_TITLE(tmp_win))
  {
    int left_gravity;
    int right_gravity;

    if (gravity == StaticGravity || gravity == ForgetGravity)
    {
      left_gravity = gravity;
      right_gravity = gravity;
    }
    else
    {
      /* West...Gravity */
      left_gravity = gravity - ((gravity - 1) % 3);
      /* East...Gravity */
      right_gravity = left_gravity + 2;
    }
    xcwa.win_gravity = left_gravity;
    XChangeWindowAttributes(dpy, tmp_win->title_w, valuemask, &xcwa);
    for (i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
      if (i == NR_LEFT_BUTTONS)
	xcwa.win_gravity = right_gravity;
      if (tmp_win->button_w[i])
	XChangeWindowAttributes(dpy, tmp_win->button_w[i], valuemask, &xcwa);
    }
  }
}


/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void SetShape(FvwmWindow *tmp_win, int w)
{
#ifdef SHAPE
  if (ShapesSupported)
  {
    if (tmp_win->wShaped)
    {
      XRectangle rect;

      XShapeCombineShape(
	dpy, tmp_win->frame, ShapeBounding, tmp_win->boundary_width,
	((HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height) +
	tmp_win->boundary_width, tmp_win->w, ShapeBounding, ShapeSet);
      if (tmp_win->title_w)
	{
	  /* windows w/ titles */
	  rect.x = tmp_win->boundary_width;
	  rect.y = tmp_win->title_g.y;
	  rect.width = w - 2*tmp_win->boundary_width;
	  rect.height = tmp_win->title_g.height;


	  XShapeCombineRectangles(dpy,tmp_win->frame,ShapeBounding,
				  0,0,&rect,1,ShapeUnion,Unsorted);
	}
    }
    else
      XShapeCombineMask (dpy, tmp_win->frame, ShapeBounding,
			 0, 0, None, ShapeSet);
  }
#endif
}

/****************************************************************************
 *
 *  Sets the allowed button states
 *
 ****************************************************************************/
void cmd_button_state(F_CMD_ARGS)
{
  char *token;

  while ((token = PeekToken(action, &action)))
  {
    static char first = True;
    if (!token && first)
    {
      Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
      Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
      return;
    }
    first = False;
    if (StrEquals("activedown", token))
    {
      Scr.gs.use_active_down_buttons =
	ParseToggleArgument(action, &action, DEFAULT_USE_ACTIVE_DOWN_BUTTONS,
			    True);
    }
    else if (StrEquals("inactive", token))
    {
      Scr.gs.use_inactive_buttons =
	ParseToggleArgument(action, &action, DEFAULT_USE_ACTIVE_DOWN_BUTTONS,
			    True);
    }
    else
    {
      Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
      Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
      fvwm_msg(ERR, "cmd_button_state", "unknown button state %s\n", token);
      return;
    }
  }
  return;
}


/****************************************************************************
 *
 *  Sets the border style (veliaa@rpi.edu)
 *
 ****************************************************************************/
void SetBorderStyle(F_CMD_ARGS)
{
  char *parm;
  char *prev;
#ifdef USEDECOR
  FvwmDecor *decor = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
  FvwmDecor *decor = &Scr.DefaultDecor;
#endif

  Scr.flags.do_need_window_update = 1;
  decor->flags.has_changed = 1;

  for (prev = action; (parm = PeekToken(action, &action)); prev = action)
  {
    if (StrEquals(parm, "active") || StrEquals(parm, "inactive"))
    {
      int len;
      char *end, *tmp;
      DecorFace tmpdf, *df;
      memset(&tmpdf.style, 0, sizeof(tmpdf.style));
      DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
#ifdef MULTISTYLE
      tmpdf.next = NULL;
#endif
#ifdef MINI_ICONS
      tmpdf.u.p = NULL;
#endif
      if (StrEquals(parm,"active"))
	df = &decor->BorderStyle.active;
      else
	df = &decor->BorderStyle.inactive;
      df->flags.has_changed = 1;
      while (isspace(*action))
	++action;
      if (*action != '(')
      {
	if (!*action)
	{
	  fvwm_msg(ERR,"SetBorderStyle",
		   "error in %s border specification", parm);
	  return;
	}
	while (isspace(*action))
	  ++action;
	if (ReadDecorFace(action, &tmpdf,-1,True))
	{
	  FreeDecorFace(dpy, df);
	  *df = tmpdf;
	}
	break;
      }
      end = strchr(++action, ')');
      if (!end)
      {
	fvwm_msg(ERR,"SetBorderStyle",
		 "error in %s border specification", parm);
	return;
      }
      len = end - action + 1;
      tmp = safemalloc(len);
      strncpy(tmp, action, len - 1);
      tmp[len - 1] = 0;
      ReadDecorFace(tmp, df,-1,True);
      free(tmp);
      action = end + 1;
    }
    else if (strcmp(parm,"--")==0)
    {
      if (ReadDecorFace(prev, &decor->BorderStyle.active,-1,True))
	ReadDecorFace(prev, &decor->BorderStyle.inactive,-1,False);
      decor->BorderStyle.active.flags.has_changed = 1;
      decor->BorderStyle.inactive.flags.has_changed = 1;
      break;
    }
    else
    {
      DecorFace tmpdf;
      memset(&tmpdf.style, 0, sizeof(tmpdf.style));
      DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
#ifdef MULTISTYLE
      tmpdf.next = NULL;
#endif
#ifdef MINI_ICONS
      tmpdf.u.p = NULL;
#endif
      if (ReadDecorFace(prev, &tmpdf,-1,True))
      {
	FreeDecorFace(dpy,&decor->BorderStyle.active);
	decor->BorderStyle.active = tmpdf;
	ReadDecorFace(prev, &decor->BorderStyle.inactive,-1,False);
	decor->BorderStyle.active.flags.has_changed = 1;
	decor->BorderStyle.inactive.flags.has_changed = 1;
      }
      break;
    }
  }
}
