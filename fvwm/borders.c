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

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "fvwm.h"
#include "externs.h"
#include "events.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "geometry.h"
#include "borders.h"
#include "builtins.h"
#include "icons.h"
#include "module_interface.h"
#include "libs/Colorset.h"
#include "add_window.h"

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

/* used by several drawing functions */
static void ClipClear(Window win, XRectangle *rclip, Bool do_flush_expose)
{
  if (rclip)
  {
    XClearArea(
      dpy, win, rclip->x, rclip->y, rclip->width, rclip->height, False);
  }
  else
  {
    if (do_flush_expose)
    {
      flush_expose(win);
    }
    XClearWindow(dpy, win);
  }

  return;
}

/***********************************************************************
 * change by KitS@bartley.demon.co.uk to correct popups off title buttons
 *
 *  Procedure:
 *ButtonPosition - find the actual position of the button
 *                 since some buttons may be disabled
 *
 *  Returned Value:
 *The button count from left or right taking in to account
 *that some buttons may not be enabled for this window
 *
 *  Inputs:
 *      context - context as per the global Context
 *      t       - the window (FvwmWindow) to test against
 *
 ***********************************************************************/
int ButtonPosition(int context, FvwmWindow *t)
{
  int i = 0;
  int buttons = -1;
  int end = Scr.nr_left_buttons;

  if (context & C_RALL)
  {
    i  = 1;
    end = Scr.nr_right_buttons;
  }
  {
    for (; i / 2 < end; i += 2)
    {
      if (t->button_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1 << i) * C_L1) & context)
	return buttons;
    }
  }
  /* you never know... */
  return 0;
}

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
  Window win, GC ReliefGC, GC ShadowGC, Pixel fore_color, Pixel back_color,
  struct vector_coords *coords, int w, int h)
{
  int i;

  /* It is rare, so evaluate this only when needed */
  GC fore_GC = NULL, back_GC = NULL;
  if (coords->use_fgbg)
  {
    Globalgcv.foreground = fore_color;
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC3, Globalgcm, &Globalgcv);
    fore_GC = Scr.ScratchGC3;
    Globalgcv.foreground = back_color;
    XChangeGC(dpy, Scr.ScratchGC4, Globalgcm, &Globalgcv);
    back_GC = Scr.ScratchGC4;
  }

  for (i = 1; i < coords->num; ++i)
  {
      XDrawLine(dpy,win,
		(coords->use_fgbg & (1 << i))
		  ? (coords->line_style & (1 << i)) ? fore_GC : back_GC
		  : (coords->line_style & (1 << i)) ? ReliefGC : ShadowGC,
		w * coords->x[i-1]/100,
		h * coords->y[i-1]/100,
		w * coords->x[i]/100,
		h * coords->y[i]/100);
  }
}

/* used by DrawButton() */
static void clip_button_pixmap(
  int *ox, int *oy, int *cw, int *ch, int*cx, int *cy, XRectangle *rclip)
{
  if (rclip)
  {
    if (rclip->x > *cx)
    {
      *ox = (rclip->x - *cx);
      *cx += *ox;
      *cw -= *ox;
    }
    if (*cx + *cw > rclip->x + rclip->width)
    {
      *cw = rclip->x - *cx + rclip->width;
    }
    if (rclip->y > *cy)
    {
      *oy = (rclip->y - *cy);
      *cy += *oy;
      *ch -= *oy;
    }
    if (*cy + *ch > rclip->y + rclip->height)
    {
      *ch = rclip->y - *cy + rclip->height;
    }
  }

  return;
}

/****************************************************************************
 *
 *  Redraws buttons (veliaa@rpi.edu)
 *
 ****************************************************************************/
static void DrawButton(
  FvwmWindow *t, Window win, int w, int h, DecorFace *df, GC ReliefGC,
  GC ShadowGC, Pixel fore_color, Pixel back_color, Bool is_lowest,
  mwm_flags stateflags, int left1right0, XRectangle *rclip,
  Pixmap *pbutton_background_pixmap)
{
  register DecorFaceType type = DFS_FACE_TYPE(df->style);
  Picture *p;
  int border = 0;
  int x;
  int y;
  int width;
  int height;

  /* Note: it is assumed that ReliefGC and ShadowGC are already clipped with
   * the rclip rectangle when this function is called. */
  switch (type)
  {
  case SimpleButton:
    break;

  case SolidButton:
    if (pbutton_background_pixmap)
    {
      XSetWindowBackground(dpy, win, df->u.back);
      *pbutton_background_pixmap = None;
    }
    ClipClear(win, rclip, True);
    break;

  case VectorButton:
  case DefaultVectorButton:
    if(HAS_MWM_BUTTONS(t) &&
       ((stateflags & MWM_DECOR_MAXIMIZE && IS_MAXIMIZED(t)) ||
	(stateflags & MWM_DECOR_SHADE && IS_SHADED(t)) ||
	(stateflags & MWM_DECOR_STICK && IS_STICKY(t))))
    {
      DrawLinePattern(win, ShadowGC, ReliefGC, fore_color, back_color,
        &df->u.vector, w, h);
    }
    else
    {
      DrawLinePattern(win, ReliefGC, ShadowGC, fore_color, back_color,
        &df->u.vector, w, h);
    }
    break;

#ifdef MINI_ICONS
  case MiniIconButton:
  case PixmapButton:
  case TiledPixmapButton:
#ifdef FANCY_TITLEBARS
  case MultiPixmap:       /* in case of UseTitleStyle */
#endif
    if (type == PixmapButton || type == TiledPixmapButton)
    {
      p = df->u.p;
    }
#ifdef FANCY_TITLEBARS
    else if (type == MultiPixmap) {
      if (left1right0 && df->u.multi_pixmaps[TBP_LEFT_BUTTONS])
        p = df->u.multi_pixmaps[TBP_LEFT_BUTTONS];
      else if (!left1right0 && df->u.multi_pixmaps[TBP_RIGHT_BUTTONS])
        p = df->u.multi_pixmaps[TBP_RIGHT_BUTTONS];
      else if (df->u.multi_pixmaps[TBP_BUTTONS])
        p = df->u.multi_pixmaps[TBP_BUTTONS];
      else if (left1right0 && df->u.multi_pixmaps[TBP_LEFT_MAIN])
        p = df->u.multi_pixmaps[TBP_LEFT_MAIN];
      else if (!left1right0 && df->u.multi_pixmaps[TBP_RIGHT_MAIN])
        p = df->u.multi_pixmaps[TBP_RIGHT_MAIN];
      else
        p = df->u.multi_pixmaps[TBP_MAIN];
    }
#endif
    else
    {
      if (!t->mini_icon)
	break;
      p = t->mini_icon;
    }
#else
  case PixmapButton:
  case TiledPixmapButton:
    p = df->u.p;
#endif /* MINI_ICONS */
    if (DFS_BUTTON_RELIEF(df->style) == DFS_BUTTON_IS_FLAT)
      border = 0;
    else
      border = HAS_MWM_BORDER(t) ? 1 : 2;
    x = border;
    y = border;
    width = w - border * 2;
    height = h - border * 2;

    if (is_lowest && !p->mask && pbutton_background_pixmap)
    {
      if (type == TiledPixmapButton ||
#ifdef FANCY_TITLEBARS
          type == MultiPixmap ||
#endif
	  (p && p->width >= width && p->height >= height))
      {
	/* better performance: set a background pixmap if possible, i.e. if the
	 * decoration is the lowest one in the button and it has no transparent
	 * parts and it is tiled or bigger than the button */
	if (*pbutton_background_pixmap != p->picture)
	{
	  *pbutton_background_pixmap = p->picture;
	  XSetWindowBackgroundPixmap(dpy, win, p->picture);
	  ClipClear(win, rclip, True);
	}
	else
	{
	  ClipClear(win, rclip, True);
	}
	break;
      }
    }
    else if (pbutton_background_pixmap && *pbutton_background_pixmap != None)
    {
      XSetWindowBackgroundPixmap(dpy, win, None);
      if (pbutton_background_pixmap)
      {
	*pbutton_background_pixmap = None;
	flush_expose(win);
	XClearWindow(dpy, win);
      }
    }
    if (type != TiledPixmapButton
#ifdef FANCY_TITLEBARS
        && type != MultiPixmap
#endif
       )
    {
      if (DFS_H_JUSTIFICATION(df->style) == JUST_RIGHT)
	x += (int)(width - p->width);
      else if (DFS_H_JUSTIFICATION(df->style) == JUST_CENTER)
	/* round up for left buttons, down for right buttons */
	x += (int)(width - p->width + left1right0) / 2;
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
    }

    XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
    XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
    if (type != TiledPixmapButton
#ifdef FANCY_TITLEBARS
        && type != MultiPixmap
#endif
       )
    {
      int cx = x;
      int cy = y;
      int cw = width;
      int ch = height;
      int ox = 0;
      int oy = 0;

      clip_button_pixmap(&ox, &oy, &cw, &ch, &cx, &cy, rclip);
      if (cw > 0 && ch > 0)
      {
	XCopyArea(
	  dpy, p->picture, win, Scr.TransMaskGC, ox, oy, cw, ch, cx, cy);
      }
    }
    else
    {
      int xi;
      int yi;

      for (yi = border; yi < height; yi += p->height)
      {
	for (xi = border; xi < width; xi += p->width)
	{
	  int lw = width - xi;
	  int lh = height - yi;
	  int cx;
	  int cy;
	  int cw;
	  int ch;
	  int ox;
	  int oy;

	  if (lw > p->width)
	    lw = p->width;
	  if (lh > p->height)
	    lh = p->height;
	  if (rclip)
	  {
	    if (xi + lw <= rclip->x || xi >= rclip->x + rclip->width ||
		yi + lh <= rclip->y || yi >= rclip->y + rclip->height)
	    {
	      continue;
	    }
	  }
	  cx = xi;
	  cy = yi;
	  cw = lw;
	  ch = lh;
	  ox = 0;
	  oy = 0;
	  clip_button_pixmap(&ox, &oy, &cw, &ch, &cx, &cy, rclip);
	  if (cw > 0 && ch > 0)
	  {
	    XSetClipOrigin(dpy, Scr.TransMaskGC, xi, yi);
	    XCopyArea(dpy, p->picture, win, Scr.TransMaskGC,
		      ox, oy, cw, ch, cx, cy);
	  }
	}
      }
    }
    XSetClipMask(dpy, Scr.TransMaskGC, None);
    break;

  case GradientButton:
  {
    unsigned int g_width;
    unsigned int g_height;

    if (!rclip)
    {
      flush_expose(win);
    }
    XSetClipMask(dpy, Scr.TransMaskGC, None);
    /* find out the size the pixmap should be */
    CalculateGradientDimensions(
      dpy, win, df->u.grad.npixels, df->u.grad.gradient_type,
      &g_width, &g_height);
    /* draw the gradient directly into the window */
    CreateGradientPixmap(
      dpy, win, Scr.TransMaskGC, df->u.grad.gradient_type, g_width, g_height,
      df->u.grad.npixels, df->u.grad.pixels, win, 0, 0, w, h, rclip);
  }
  break;

  default:
    fvwm_msg(ERR, "DrawButton", "unknown button type");
    break;
  }

  return;
}

#ifdef FANCY_TITLEBARS

/****************************************************************************
 *
 *  Tile or stretch src into dest, starting at the given location and
 *  continuing for the given width and height. This is a utility function used
 *  by DrawMultiPixmapTitlebar. (tril@igs.net)
 *
 ****************************************************************************/
static void RenderIntoWindow(GC gc, Picture *src, Window dest, int x_start,
                             int y_start, int width, int height, Bool stretch)
{
  Pixmap pm;
  if (stretch)
    pm = CreateStretchPixmap(dpy, src->picture, src->width, src->height,
                             src->depth, width, height, gc);
  else
    pm = CreateTiledPixmap(dpy, src->picture, src->width, src->height,
                           width, height, src->depth, gc);
  XCopyArea(dpy, pm, dest, gc, 0, 0, width, height, x_start, y_start);
  XFreePixmap(dpy, pm);
}

/****************************************************************************
 *
 *  Redraws multi-pixmap titlebar (tril@igs.net)
 *
 ****************************************************************************/
static void DrawMultiPixmapTitlebar(FvwmWindow *window, DecorFace *df)
{
  Window       title_win;
  GC           gc;
  char        *title;
  Picture    **pm;
  short        stretch_flags;
  int          title_width, title_height, text_width, text_offset, text_y_pos;
  int          left_space, right_space, under_offset, under_width;

  pm = df->u.multi_pixmaps;
  stretch_flags = df->u.multi_stretch_flags;
  gc = Scr.TitleGC;
  XSetClipMask(dpy, gc, None);
  title_win = window->title_w;
  title = window->visible_name;
  title_width = window->title_g.width;
  title_height = window->title_g.height;

  if (pm[TBP_MAIN])
    RenderIntoWindow(gc, pm[TBP_MAIN], title_win, 0, 0, title_width,
                     title_height, (stretch_flags & (1 << TBP_MAIN)));
  else if (!title)
    RenderIntoWindow(gc, pm[TBP_LEFT_MAIN], title_win, 0, 0, title_width,
                     title_height, (stretch_flags & (1 << TBP_LEFT_MAIN)));

  if (title) {
#ifdef I18N_MB
    text_width = XmbTextEscapement(window->title_font.fontset, title,
                                   strlen(title));
#else
    text_width = XTextWidth(window->title_font.font, title, strlen(title));
#endif
    if (text_width > title_width)
      text_width = title_width;
    switch (TB_JUSTIFICATION(GetDecor(window, titlebar))) {
    case JUST_LEFT:
      text_offset = TITLE_PADDING;
      if (pm[TBP_LEFT_OF_TEXT])
        text_offset += pm[TBP_LEFT_OF_TEXT]->width;
      if (pm[TBP_LEFT_END])
        text_offset += pm[TBP_LEFT_END]->width;
      if (text_offset > title_width - text_width)
        text_offset = title_width - text_width;
      break;
    case JUST_RIGHT:
      text_offset = title_width - text_width - TITLE_PADDING;
      if (pm[TBP_RIGHT_OF_TEXT])
        text_offset -= pm[TBP_RIGHT_OF_TEXT]->width;
      if (pm[TBP_RIGHT_END])
        text_offset -= pm[TBP_RIGHT_END]->width;
      if (text_offset < 0)
        text_offset = 0;
      break;
    default:
      text_offset = (title_width - text_width) / 2;
      break;
    }
    under_offset = text_offset;
    under_width = text_width;
    /* If there's title padding, put it *inside* the undertext area: */
    if (under_offset >= TITLE_PADDING) {
      under_offset -= TITLE_PADDING;
      under_width += TITLE_PADDING;
    }
    if (under_offset + under_width + TITLE_PADDING <= title_width)
      under_width += TITLE_PADDING;
    left_space = under_offset;
    right_space = title_width - left_space - under_width;

    if (pm[TBP_LEFT_MAIN] && left_space > 0)
      RenderIntoWindow(gc, pm[TBP_LEFT_MAIN], title_win, 0, 0, left_space,
                       title_height, (stretch_flags & (1 << TBP_LEFT_MAIN)));
    if (pm[TBP_RIGHT_MAIN] && right_space > 0)
      RenderIntoWindow(gc, pm[TBP_RIGHT_MAIN], title_win,
                       under_offset + under_width, 0, right_space,
                       title_height, (stretch_flags & (1 << TBP_RIGHT_MAIN)));
    if (pm[TBP_UNDER_TEXT] && under_width > 0)
      RenderIntoWindow(gc, pm[TBP_UNDER_TEXT], title_win, under_offset, 0,
                       under_width, title_height,
                       (stretch_flags & (1 << TBP_UNDER_TEXT)));
    if (pm[TBP_LEFT_OF_TEXT] &&
        pm[TBP_LEFT_OF_TEXT]->width <= left_space) {
      XCopyArea(dpy, pm[TBP_LEFT_OF_TEXT]->picture, title_win, gc, 0, 0,
                pm[TBP_LEFT_OF_TEXT]->width, title_height,
                under_offset - pm[TBP_LEFT_OF_TEXT]->width, 0);
      left_space -= pm[TBP_LEFT_OF_TEXT]->width;
    }
    if (pm[TBP_RIGHT_OF_TEXT] &&
        pm[TBP_RIGHT_OF_TEXT]->width <= right_space) {
      XCopyArea(dpy, pm[TBP_RIGHT_OF_TEXT]->picture, title_win, gc, 0, 0,
                pm[TBP_RIGHT_OF_TEXT]->width, title_height,
                under_offset + under_width, 0);
      right_space -= pm[TBP_RIGHT_OF_TEXT]->width;
    }

    text_y_pos = title_height
                 - (title_height - window->title_font.font->ascent) / 2
                 - 1;
    if (title_height > 19)
      text_y_pos--;
    if (text_y_pos < 0)
      text_y_pos = 0;
#ifdef I18N_MB
    XmbDrawString(dpy, title_win, window->title_font.fontset,
                  gc, text_offset, text_y_pos, title, strlen(title));
#else
    XDrawString(dpy, title_win, gc, text_offset, text_y_pos, title,
                strlen(title));
#endif
  }
  else
    left_space = right_space = title_width;

  if (pm[TBP_LEFT_END] && pm[TBP_LEFT_END]->width <= left_space)
    XCopyArea(dpy, pm[TBP_LEFT_END]->picture, title_win, gc, 0, 0,
              pm[TBP_LEFT_END]->width, title_height, 0, 0);
  if (pm[TBP_RIGHT_END] && pm[TBP_RIGHT_END]->width <= right_space)
    XCopyArea(dpy, pm[TBP_RIGHT_END]->picture, title_win, gc, 0, 0,
              pm[TBP_RIGHT_END]->width, title_height,
              title_width - pm[TBP_RIGHT_END]->width, 0);
}

#endif /* FANCY_TITLEBARS */

/* rules to get button state */
static enum ButtonState get_button_state(
  Bool has_focus, Bool toggled, Window w)
{
  if (Scr.gs.use_active_down_buttons)
  {
    if (Scr.gs.use_inactive_buttons && !has_focus)
    {
      return (toggled) ? BS_ToggledInactive : BS_Inactive;
    }
    else
    {
      if (PressedW == w)
      {
	return (toggled) ? BS_ToggledActiveDown : BS_ActiveDown;
      }
      else
      {
	return (toggled) ? BS_ToggledActiveUp : BS_ActiveUp;
      }
    }
  }
  else
  {
    if (Scr.gs.use_inactive_buttons && !has_focus)
    {
      return (toggled) ? BS_ToggledInactive : BS_Inactive;
    }
    else
    {
      return (toggled) ? BS_ToggledActiveUp : BS_ActiveUp;
    }
  }
}

static const char ulgc[] = { 1, 0, 0, 0x7f, 2, 1, 1 };
static const char brgc[] = { 1, 1, 2, 0x7f, 0, 0, 3 };

/* called twice by RedrawBorder */
static void draw_frame_relief(
  FvwmWindow *t, GC rgc, GC sgc, GC tgc, GC dgc, int w_dout, int w_hiout,
  int w_trout, int w_c, int w_trin, int w_shin, int w_din)
{
  int i;
  int offset = 0;
  int width = t->frame_g.width - 1;
  int height = t->frame_g.height - 1;
  int w[7];
  GC gc[4];

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
  gc[3] = dgc;

  for (i = 0; i < 7; i++)
  {
    if (ulgc[i] != 0x7f && w[i] > 0)
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
  Window expose_win, XRectangle *rclip)
{
  static GC tgc = None;
  int i;
  GC rgc;
  GC sgc;
  DecorFaceStyle *borderstyle;
  Bool is_reversed = False;
  Bool is_clipped = False;
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
    tgc = fvwmlib_XCreateGC(dpy, t->frame, GCFunction | GCPlaneMask, &xgcv);
  }
  /*
   * draw the border; resize handles are InputOnly so draw in the decor_w and
   * it will show through. The entire border is redrawn so flush all exposes
   * up to this point.
   */
#if 0
  flush_expose(t->decor_w);
#endif

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
  {
    ClipClear(t->decor_w, rclip, False);
  }

  /*
   * Nothing to do if flat
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
    if (w_trin)
    {
      w_trout = 0;
      w_trin = 0;
      w_din = 0;
      w_hiout = 1;
      sum -= 2;
      trim -= 2;
    }
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
  if (rclip)
  {
    XSetClipRectangles(dpy, rgc, 0, 0, rclip, 1, Unsorted);
    XSetClipRectangles(dpy, sgc, 0, 0, rclip, 1, Unsorted);
    is_clipped = True;
  }
  draw_frame_relief(
    t, rgc, sgc, tgc, sgc,
    w_dout, w_hiout, w_trout, w_c, w_trin, w_shin, w_din);

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
    int inset = (w_shin || w_din);
    XSegment marks[8];
    int k;

    /*
     * draw the relief
     */
    for (k = 0; k < cd->relief_width; k++)
    {
      int loff = k;
      int boff = k + w_dout + 1;
      int length = t->boundary_width - boff - inset;

      if (length < 0)
	break;
      /* hilite marks */
      i = 0;
      /* top left */
      marks[i].x1 = t->visual_corner_width + loff;
      marks[i].x2 = t->visual_corner_width + loff;
      marks[i].y1 = w_dout;
      marks[i].y2 = marks[i].y1 + length;
      i++;
      /* top right */
      marks[i].x1 = t->frame_g.width - t->visual_corner_width + loff;
      marks[i].x2 = t->frame_g.width - t->visual_corner_width + loff;
      marks[i].y1 = w_dout;
      marks[i].y2 = marks[i].y1 + length;
      i++;
      /* bot left */
      marks[i].x1 = t->visual_corner_width + loff;
      marks[i].x2 = t->visual_corner_width + loff;
      marks[i].y1 = t->frame_g.height - boff;
      marks[i].y2 = marks[i].y1 - length;
      i++;
      /* bot right */
      marks[i].x1 = t->frame_g.width - t->visual_corner_width + loff;
      marks[i].x2 = t->frame_g.width - t->visual_corner_width + loff;
      marks[i].y1 = t->frame_g.height - boff;
      marks[i].y2 = marks[i].y1 - length;
      i++;
      if (!IS_SHADED(t))
      {
	/* left top */
	marks[i].x1 = w_dout;
	marks[i].x2 = marks[i].x1 + length;
	marks[i].y1 = t->visual_corner_width + loff;
	marks[i].y2 = t->visual_corner_width + loff;
	i++;
	/* left bot */
	marks[i].x1 = w_dout;
	marks[i].x2 = marks[i].x1 + length;
	marks[i].y1 = t->frame_g.height-t->visual_corner_width + loff;
	marks[i].y2 = t->frame_g.height-t->visual_corner_width + loff;
	i++;
	/* right top */
	marks[i].x1 = t->frame_g.width - boff;
	marks[i].x2 = marks[i].x1 - length;
	marks[i].y1 = t->visual_corner_width + loff;
	marks[i].y2 = t->visual_corner_width + loff;
	i++;
	/* right bot */
	marks[i].x1 = t->frame_g.width - boff;
	marks[i].x2 = marks[i].x1 - length;
	marks[i].y1 = t->frame_g.height-t->visual_corner_width + loff;
	marks[i].y2 = t->frame_g.height-t->visual_corner_width + loff;
	i++;
      }
      XDrawSegments(dpy, t->decor_w, rgc, marks, i);

      /* shadow marks, reuse the array (XDrawSegments doesn't trash it) */
      i = 0;
      loff = 1 + k + k;
      /* top left */
      marks[i].x1 -= loff;
      marks[i].x2 -= loff;
      marks[i].y1 += k;
      marks[i].y2 += k;
      i++;
      /* top right */
      marks[i].x1 -= loff;
      marks[i].x2 -= loff;
      marks[i].y1 += k;
      marks[i].y2 += k;
      i++;
      /* bot left */
      marks[i].x1 -= loff;
      marks[i].x2 -= loff;
      marks[i].y1 += k;
      marks[i].y2 += k;
      i++;
      /* bot right */
      marks[i].x1 -= loff;
      marks[i].x2 -= loff;
      marks[i].y1 += k;
      marks[i].y2 += k;
      i++;
      if (!IS_SHADED(t))
      {
	/* left top */
	marks[i].x1 += k;
	marks[i].x2 += k;
	marks[i].y1 -= loff;
	marks[i].y2 -= loff;
	i++;
	/* left bot */
	marks[i].x1 += k;
	marks[i].x2 += k;
	marks[i].y1 -= loff;
	marks[i].y2 -= loff;
	i++;
	/* right top */
	marks[i].x1 += k;
	marks[i].x2 += k;
	marks[i].y1 -= loff;
	marks[i].y2 -= loff;
	i++;
	/* right bot */
	marks[i].x1 += k;
	marks[i].x2 += k;
	marks[i].y1 -= loff;
	marks[i].y2 -= loff;
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
      r.x = t->visual_corner_width;
      r.y = 1;
      r.width = t->frame_g.width - 2 * t->visual_corner_width;
      r.height = t->boundary_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->sides[1])
    {
      /* right */
      if (!IS_SHADED(t))
      {
	r.x = t->frame_g.width - t->boundary_width;
	r.y = t->visual_corner_width;
	r.width = t->boundary_width - 1;
	r.height = t->frame_g.height - 2 * t->visual_corner_width;
	is_pressed = True;
      }
    }
    else if (PressedW == t->sides[2])
    {
      /* bottom */
      r.x = t->visual_corner_width;
      r.y = t->frame_g.height - t->boundary_width;
      r.width = t->frame_g.width - 2 * t->visual_corner_width;
      r.height = t->boundary_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->sides[3])
    {
      /* left */
      if (!IS_SHADED(t))
      {
	r.x = 1;
	r.y = t->visual_corner_width;
	r.width = t->boundary_width - 1;
	r.height = t->frame_g.height - 2 * t->visual_corner_width;
	is_pressed = True;
      }
    }
    else if (PressedW == t->corners[0])
    {
      /* top left */
      r.x = 1;
      r.y = 1;
      r.width = t->visual_corner_width - 1;
      r.height = t->visual_corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[1])
    {
      /* top right */
      r.x = t->frame_g.width - t->visual_corner_width;
      r.y = 1;
      r.width = t->visual_corner_width - 1;
      r.height = t->visual_corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[2])
    {
      /* bottom left */
      r.x = 1;
      r.y = t->frame_g.height - t->visual_corner_width;
      r.width = t->visual_corner_width - 1;
      r.height = t->visual_corner_width - 1;
      is_pressed = True;
    }
    else if (PressedW == t->corners[3])
    {
      /* bottom right */
      r.x = t->frame_g.width - t->visual_corner_width;
      r.y = t->frame_g.height - t->visual_corner_width;
      r.width = t->visual_corner_width - 1;
      r.height = t->visual_corner_width - 1;
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
      Bool is_not_empty = True;

      if (rclip)
      {
	is_not_empty = intersect_xrectangles(&r, rclip);
      }
      if (is_not_empty)
      {
	XSetClipRectangles(dpy, rgc, 0, 0, &r, 1, Unsorted);
	XSetClipRectangles(dpy, sgc, 0, 0, &r, 1, Unsorted);
	draw_frame_relief(
	  t, sgc, rgc, tgc, sgc,
	  w_dout, w_hiout, w_trout, w_c, w_trin, w_shin, w_din);
	is_clipped = True;
      }
    }
  }
  if (is_clipped)
  {
    XSetClipMask(dpy, rgc, None);
    XSetClipMask(dpy, sgc, None);
  }

  return;
}

/****************************************************************************
 *
 * Redraws the window buttons
 *
 ****************************************************************************/
static void RedrawButtons(
  common_decorations_type *cd, FvwmWindow *t, Bool has_focus, int force,
  Window expose_win, XRectangle *rclip)
{
  int i;
  int left1right0;
  Bool is_lowest = True;
  Pixmap *pass_bg_pixmap;

  /* Note: the rclip rectangle is not used in this function. Buttons are usually
   * so small that it makes not much sense to limit drawing to a clip rectangle.
   */

  /*
   * draw buttons
   */

  for (i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    left1right0 = !(i&1);
    if (t->button_w[i] != None &&
        ((!left1right0 && i/2 < Scr.nr_right_buttons) ||
         (left1right0  && i/2 < Scr.nr_left_buttons)))
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
	int is_inverted = (PressedW == t->button_w[i]);

	if (DFS_USE_BORDER_STYLE(df->style))
	{
	  XChangeWindowAttributes(
	    dpy, t->button_w[i], cd->valuemask, &cd->attributes);
	  if (df->u.p)
	  {
	    t->button_background_pixmap[i] = df->u.p->picture;
	  }
	  if (df->u.p && df->u.p->picture)
	  {
	    pass_bg_pixmap = NULL;
	  }
	  else
	  {
	    pass_bg_pixmap = &t->button_background_pixmap[i];
	  }
	}
	else
	{
	  XChangeWindowAttributes(
	    dpy, t->button_w[i], cd->notex_valuemask, &cd->notex_attributes);
	  t->button_background_pixmap[i] = None;
	  pass_bg_pixmap = &t->button_background_pixmap[i];
	}
	XClearWindow(dpy, t->button_w[i]);
	if (DFS_USE_TITLE_STYLE(df->style))
	{
	  DecorFace *tsdf = &TB_STATE(GetDecor(t, titlebar))[bs];

	  is_lowest = True;
	  for (; tsdf; tsdf = tsdf->next)
	  {
	    DrawButton(t, t->button_w[i], t->title_g.height, t->title_g.height,
		       tsdf, cd->relief_gc, cd->shadow_gc,
		       cd->fore_color, cd->back_color, is_lowest,
		       TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])),
                       left1right0, NULL, pass_bg_pixmap);
	    is_lowest = False;
	  }
	}
	is_lowest = True;
	for (; df; df = df->next)
	{
	  DrawButton(t, t->button_w[i], t->title_g.height, t->title_g.height,
		     df, cd->relief_gc, cd->shadow_gc,
		     cd->fore_color, cd->back_color, is_lowest,
		     TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])), left1right0,
                     NULL, pass_bg_pixmap);
	  is_lowest = False;
	}

	{
	  Bool reverse = is_inverted;

	  switch (DFS_BUTTON_RELIEF(
	    TB_STATE(GetDecor(t, buttons[i]))[bs].style))
	  {
	  case DFS_BUTTON_IS_SUNK:
	    reverse = !is_inverted;
	  case DFS_BUTTON_IS_UP:
	    RelieveRectangle2(
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
static void RedrawTitle(
  common_decorations_type *cd, FvwmWindow *t, Bool has_focus, XRectangle *rclip)
{
  int hor_off;
  int w;
  int i;
  enum ButtonState title_state;
  DecorFaceStyle *tb_style;
  Pixmap *pass_bg_pixmap;
  GC rgc = cd->relief_gc;
  GC sgc = cd->shadow_gc;
  Bool reverse = False;
  Bool is_clipped = False;
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
#if 0
  flush_expose(t->title_w);
#endif

  if (t->visible_name != (char *)NULL)
  {
#ifdef I18N_MB /* cannot use macro here, rewriting... */
    w = XmbTextEscapement(t->title_font.fontset, t->visible_name,
			  strlen(t->visible_name));
#else
    w = XTextWidth(t->title_font.font, t->visible_name, strlen(t->visible_name));
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
    hor_off = WINDOW_TITLE_TEXT_OFFSET;
    break;
  case JUST_RIGHT:
    hor_off = t->title_g.width - w - WINDOW_TITLE_TEXT_OFFSET;
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
	DFS_FACE_TYPE(df->style) == TiledPixmapButton &&
	df->u.p)
    {
      XSetWindowBackgroundPixmap(dpy, t->title_w, df->u.p->picture);
      t->title_background_pixmap = df->u.p->picture;
      pass_bg_pixmap = NULL;
    }
    else
    {
      pass_bg_pixmap = &t->title_background_pixmap;
    }
  }
  ClipClear(t->title_w, rclip, False);
  if (rclip)
  {
    XSetClipRectangles(dpy, rgc, 0, 0, rclip, 1, Unsorted);
    XSetClipRectangles(dpy, sgc, 0, 0, rclip, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TitleGC, 0, 0, rclip, 1, Unsorted);
    is_clipped = True;
  }

  /*
   * draw title
   */

  if (Pdepth < 2)
  {
    XFillRectangle(
      dpy, t->title_w, ((PressedW == t->title_w) ? sgc : rgc), hor_off - 2, 0,
      w+4,t->title_g.height);
    if(t->visible_name != (char *)NULL)
#ifdef I18N_MB
      XmbDrawString(dpy, t->title_w, t->title_font.fontset,
		    Scr.TitleGC, hor_off,
		    t->title_font.y + 1,
		    t->visible_name, strlen(t->visible_name));
#else
      XDrawString(dpy, t->title_w, Scr.TitleGC, hor_off,
                  t->title_font.y + 1, t->visible_name, strlen(t->visible_name));
#endif
    /* for mono, we clear an area in the title bar where the window
     * title goes, so that its more legible. For color, no need */
    RelieveRectangle(
      dpy, t->title_w, 0, 0, hor_off - 3, t->title_g.height - 1, rgc, sgc,
      cd->relief_width);
    RelieveRectangle(
      dpy, t->title_w, hor_off + w + 2, 0, t->title_g.width - w - hor_off - 3,
      t->title_g.height - 1, rgc, sgc, cd->relief_width);
    XDrawLine(
      dpy, t->title_w, sgc, hor_off + w + 1, 0, hor_off + w + 1,
      t->title_g.height);
  }
  else
  {
    DecorFace *df = &TB_STATE(GetDecor(t, titlebar))[title_state];
    /* draw compound titlebar (veliaa@rpi.edu) */
    Bool is_lowest = True;

#ifdef FANCY_TITLEBARS
    if (df->style.face_type == MultiPixmap)
      DrawMultiPixmapTitlebar(t, df);
    else
#endif
    if (PressedW == t->title_w)
    {
      for (; df; df = df->next)
      {
	DrawButton(
	  t, t->title_w, t->title_g.width, t->title_g.height, df, sgc,
	  rgc, cd->fore_color, cd->back_color, is_lowest, 0, 1, rclip,
          pass_bg_pixmap);
	is_lowest = False;
      }
    }
    else
    {
      for (; df; df = df->next)
      {
	DrawButton(
	  t, t->title_w, t->title_g.width, t->title_g.height, df, rgc,
	  sgc, cd->fore_color, cd->back_color, is_lowest, 0, 1, rclip,
          pass_bg_pixmap);
	is_lowest = False;
      }
    }

#ifdef FANCY_TITLEBARS
    if (TB_STATE(GetDecor(t, titlebar))[title_state].style.face_type
        != MultiPixmap)
#endif
    {
#ifdef I18N_MB
    if(t->visible_name != (char *)NULL)
      XmbDrawString(dpy, t->title_w, t->title_font.fontset,
		    Scr.TitleGC, hor_off,
		    t->title_font.y + 1,
		    t->visible_name, strlen(t->visible_name));
#else
    if(t->visible_name != (char *)NULL)
      XDrawString(dpy, t->title_w, Scr.TitleGC, hor_off,
		  t->title_font.y + 1, t->visible_name, strlen(t->visible_name));
#endif
    }

    /*
     * draw title relief
     */
    switch (DFS_BUTTON_RELIEF(*tb_style))
    {
    case DFS_BUTTON_IS_SUNK:
      reverse = 1;
    case DFS_BUTTON_IS_UP:
      RelieveRectangle2(
	dpy, t->title_w, 0, 0, t->title_g.width - 1, t->title_g.height - 1,
	(reverse) ? sgc : rgc, (reverse) ? rgc : sgc, cd->relief_width);
      break;
    default:
      /* flat */
      break;
    }
  }

  /*
   * draw the 'sticky' lines
   */

  if (IS_STICKY(t) || HAS_STIPPLED_TITLE(t))
  {
    /* an odd number of lines every WINDOW_TITLE_STICK_VERT_DIST pixels */
    int num =
      (int)(t->title_g.height / WINDOW_TITLE_STICK_VERT_DIST / 2) * 2 - 1;
    int min = t->title_g.height / 2 - num * 2 + 1;
    int max =
            t->title_g.height / 2 + num * 2 - WINDOW_TITLE_STICK_VERT_DIST + 1;
    int left_x = WINDOW_TITLE_STICK_OFFSET;
    int left_w = hor_off - left_x - WINDOW_TITLE_TO_STICK_GAP;
    int right_x = hor_off + w + WINDOW_TITLE_TO_STICK_GAP - 1;
    int right_w = t->title_g.width - right_x - WINDOW_TITLE_STICK_OFFSET;

    if (left_w < WINDOW_TITLE_STICK_MIN_WIDTH)
    {
      left_x = 0;
      left_w = WINDOW_TITLE_STICK_MIN_WIDTH;
    }
    if (right_w < WINDOW_TITLE_STICK_MIN_WIDTH)
    {
      right_w = WINDOW_TITLE_STICK_MIN_WIDTH;
      right_x = t->title_g.width - WINDOW_TITLE_STICK_MIN_WIDTH - 1;
    }
    for (i = min; i <= max; i += WINDOW_TITLE_STICK_VERT_DIST)
    {
      if (left_w > 0)
      {
	RelieveRectangle(
          dpy, t->title_w, left_x, i, left_w, 1, sgc, rgc, 1);
      }
      if (right_w > 0)
      {
	RelieveRectangle(
          dpy, t->title_w, right_x, i, right_w, 1, sgc, rgc, 1);
      }
    }
  }

  if (is_clipped)
  {
    XSetClipMask(dpy, rgc, None);
    XSetClipMask(dpy, sgc, None);
    XSetClipMask(dpy, Scr.TitleGC, None);
  }

  return;
}


void SetupTitleBar(FvwmWindow *tmp_win, int w, int h)
{
  XWindowChanges xwc;
  unsigned long xwcm;
  int i;
  int buttons = 0;
  int ww = tmp_win->frame_g.width - 2 * tmp_win->boundary_width;
  int rest = 0;
  int bw;

  if (HAS_BOTTOM_TITLE(tmp_win))
  {
    tmp_win->title_g.y =
      h - tmp_win->boundary_width - tmp_win->title_g.height;
    tmp_win->title_top_height = 0;
  }
  else
  {
    tmp_win->title_g.y = tmp_win->boundary_width;
    tmp_win->title_top_height = tmp_win->title_g.height;
  }
  tmp_win->title_g.x = tmp_win->boundary_width;

  xwcm = CWX | CWY | CWHeight | CWWidth;
  xwc.y = tmp_win->title_g.y;
  xwc.width = tmp_win->title_g.height;
  xwc.height = tmp_win->title_g.height;
  for (i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    if (tmp_win->button_w[i])
      buttons++;
  }
  ww = tmp_win->frame_g.width - 2 * tmp_win->boundary_width -
    tmp_win->title_g.width;
  if (ww < buttons * xwc.width)
  {
    xwc.width = ww / buttons;
    if (xwc.width < 1)
      xwc.width = 1;
    if (xwc.width > tmp_win->title_g.height)
      xwc.width = tmp_win->title_g.height;
    rest = ww - buttons * xwc.width;
    if (rest > 0)
      xwc.width++;
  }
  /* left */
  xwc.x = tmp_win->boundary_width;
  for (i = 0; i < NUMBER_OF_BUTTONS; i += 2)
  {
    if (tmp_win->button_w[i] != None)
    {
      if (xwc.x + xwc.width < w - tmp_win->boundary_width)
      {
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
	xwc.x += xwc.width;
        tmp_win->title_g.x += xwc.width;
      }
      else
      {
	xwc.x = -tmp_win->title_g.height;
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
      }
      rest--;
      if (rest == 0)
      {
        xwc.width--;
      }
    }
  }
  bw = xwc.width;

  /* title */
  if (tmp_win->title_g.width <= 0 || ww < 0)
    tmp_win->title_g.x = -10;
  xwc.x = tmp_win->title_g.x;
  xwc.width = tmp_win->title_g.width;
  XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);

  /* right */
  xwc.width = bw;
  xwc.x = w - tmp_win->boundary_width - xwc.width;
  for (i = 1 ; i < NUMBER_OF_BUTTONS; i += 2)
  {
    if (tmp_win->button_w[i] != None)
    {
      if (xwc.x > tmp_win->boundary_width)
      {
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
	xwc.x -= xwc.width;
      }
      else
      {
	xwc.x = -tmp_win->title_g.height;
	XConfigureWindow(dpy, tmp_win->button_w[i], xwcm, &xwc);
      }
      rest--;
      if (rest == 0)
      {
        xwc.width--;
        xwc.x++;
      }
    }
  }

  return;
}


static void get_common_decorations(
  common_decorations_type *cd, FvwmWindow *t, draw_window_parts draw_parts,
  Bool has_focus, int force, Window expose_win, Bool is_border,
  Bool do_change_gcs)
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
    if (is_border)
      draw_colors = &(t->border_hicolors);
    else
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
    if (is_border)
      draw_colors = &(t->border_colors);
    else
      draw_colors = &(t->colors);
  }
  cd->fore_color = draw_colors->fore;
  cd->back_color = draw_colors->back;
  if (do_change_gcs)
  {
    Globalgcv.foreground = draw_colors->hilight;
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    cd->relief_gc = Scr.ScratchGC1;
    Globalgcv.foreground = draw_colors->shadow;
    XChangeGC(dpy, Scr.ScratchGC2, Globalgcm, &Globalgcv);
    cd->shadow_gc = Scr.ScratchGC2;
  }

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

void draw_clipped_decorations(
  FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
  Window expose_win, XRectangle *rclip, clear_window_parts clear_parts)
{
  common_decorations_type cd;
  Bool is_frame_redraw_allowed = False;
  Bool is_button_redraw_allowed = False;
  Bool is_title_redraw_allowed = False;
  Bool do_redraw_title = False;
  Bool do_redraw_buttons = False;
  Bool do_change_gcs = False;

  if (!t)
    return;
  memset(&cd, 0, sizeof(cd));
  if (force || expose_win == None)
  {
    is_frame_redraw_allowed = True;
    is_button_redraw_allowed = True;
    is_title_redraw_allowed = True;
  }
  else if (expose_win == t->title_w)
    is_title_redraw_allowed = True;
  else if (expose_win == t->frame || expose_win == t->decor_w)
    /* sides and corners are InputOnly and incapable of triggering Expose events
     */
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
	DrawDecorations(Scr.Hilite, DRAW_ALL, False, True, None, CLEAR_ALL);
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

  /* calculate some values and flags */
  if ((draw_parts & DRAW_TITLE) && HAS_TITLE(t) && is_title_redraw_allowed)
  {
    do_redraw_title = True;
    do_change_gcs = True;
  }
  if ((draw_parts & DRAW_BUTTONS) && HAS_TITLE(t) && is_button_redraw_allowed)
  {
    do_redraw_buttons = True;
    do_change_gcs = True;
  }
  get_common_decorations(
    &cd, t, draw_parts, has_focus, force, expose_win, False, do_change_gcs);

  /* redraw */
  if (cd.flags.has_color_changed && HAS_TITLE(t))
  {
    if (!do_redraw_title || !rclip)
    {
      if (t->title_background_pixmap == None || force)
      {
	change_window_background(
	  t->title_w, cd.notex_valuemask, &cd.notex_attributes);
	t->title_background_pixmap = None;
      }
    }
  }
  if (do_redraw_buttons)
  {
    RedrawButtons(&cd, t, has_focus, force, expose_win, rclip);
  }
  if (do_redraw_title)
  {
    RedrawTitle(&cd, t, has_focus, rclip);
  }
  if (cd.flags.has_color_changed ||
      ((draw_parts & DRAW_FRAME) && (is_frame_redraw_allowed)))
  {
    get_common_decorations(
      &cd, t, draw_parts, has_focus, force, expose_win, True, True);
    if ((clear_parts & CLEAR_FRAME) && (!rclip || !IS_WINDOW_BORDER_DRAWN(t)))
    {
      change_window_background(t->decor_w, cd.valuemask, &cd.attributes);
      SET_WINDOW_BORDER_DRAWN(t, 1);
      rclip = NULL;
    }
    RedrawBorder(&cd, t, has_focus, force, expose_win, rclip);
  }

  /* Sync to make the change look fast! */
  XSync(dpy,0);
}

void DrawDecorations(
  FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
  Window expose_win, clear_window_parts clear_parts)
{
  draw_clipped_decorations(
    t, draw_parts, has_focus, force, expose_win, NULL, clear_parts);

  return;
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

  if (w < 1)
  {
    w = 1;
  }
  if (h < 1)
  {
    h = 1;
  }
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
#undef SIGN

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
  py = tmp_win->boundary_width + tmp_win->title_top_height;
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
      XMoveResizeWindow(dpy, tmp_win->frame, x, y, w, h);
      break;
    case 1:
      if (!shaded)
      {
	XMoveResizeWindow(
	dpy, tmp_win->Parent, px, py, max(tmp_win->attr.width, 1),
	max(tmp_win->attr.height, 1));
      }
      else
      {
        XMoveWindow(dpy, tmp_win->Parent, px, py);
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
      else if (HAS_BOTTOM_TITLE(tmp_win))
      {
	rectangle big_g;

	big_g = (IS_MAXIMIZED(tmp_win)) ? tmp_win->max_g : tmp_win->normal_g;
	get_relative_geometry(&big_g, &big_g);
	XMoveWindow(dpy, tmp_win->w, 0, 1 - big_g.height +
		    2 * tmp_win->boundary_width + tmp_win->title_g.height);
      }
      else
      {
	XMoveWindow(dpy, tmp_win->w, 0, 0);
      }

      break;
    case 3:
      XMoveResizeWindow(dpy, tmp_win->decor_w, dx, dy, w, h);
      break;
    }
  }
  set_decor_gravity(
    tmp_win, NorthWestGravity, NorthWestGravity, NorthWestGravity);
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

      tmp_win->visual_corner_width = tmp_win->corner_width;
      if(w < 2*tmp_win->corner_width)
        tmp_win->visual_corner_width = w / 3;
      if (h < 2*tmp_win->corner_width && !shaded)
        tmp_win->visual_corner_width = h / 3;
      xwidth = w - 2 * tmp_win->visual_corner_width;
      ywidth = h - 2 * tmp_win->visual_corner_width;
      xwcm = CWWidth | CWHeight | CWX | CWY;
      if(xwidth<2)
        xwidth = 2;
      if(ywidth<2)
        ywidth = 2;

      if (IS_SHADED(tmp_win))
	add = tmp_win->visual_corner_width / 3;
      else
	add = 0;
      for(i = 0; i < 4; i++)
      {
        if(i==0)
        {
          xwc.x = tmp_win->visual_corner_width;
          xwc.y = 0;
          xwc.height = tmp_win->boundary_width;
          xwc.width = xwidth;
        }
        else if (i==1)
        {
	  xwc.x = w - tmp_win->boundary_width;
          xwc.y = tmp_win->visual_corner_width;
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
          xwc.x = tmp_win->visual_corner_width;
          xwc.y = h - tmp_win->boundary_width;
          xwc.height = tmp_win->boundary_width;
          xwc.width = xwidth;
        }
        else
        {
	  xwc.x = 0;
          xwc.y = tmp_win->visual_corner_width;
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
      xwc.width = tmp_win->visual_corner_width;
      xwc.height = tmp_win->visual_corner_width;
      for(i = 0; i < 4; i++)
      {
        if (i & 0x1)
          xwc.x = w - tmp_win->visual_corner_width;
        else
          xwc.x = 0;

        if (i & 0x2)
          xwc.y = h - tmp_win->visual_corner_width + add;
        else
          xwc.y = -add;

	XConfigureWindow(dpy, tmp_win->corners[i], xwcm, &xwc);
      }
    }
  }


  /*
   * Set up window shape
   */

  if (FShapesSupported && tmp_win->wShaped && is_resized)
  {
    SetShape(tmp_win, w);
  }

  /*
   * Send ConfigureNotify
   */

  XSync(dpy,0);
  /* must not send events to shaded windows because this might cause them to
   * look at their current geometry */
  if (sendEvent && !shaded)
  {
    SendConfigureNotify(tmp_win, x, y, w, h, 0, True);
#ifdef FVWM_DEBUG_MSGS
    fvwm_msg(DBG,"SetupFrame","Sent ConfigureNotify (w == %d, h == %d)",
             w,h);
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
  if (FShapesSupported)
  {
    if (tmp_win->wShaped)
    {
      XRectangle rect;

      FShapeCombineShape(
	dpy, tmp_win->frame, FShapeBounding, tmp_win->boundary_width,
	tmp_win->title_top_height + tmp_win->boundary_width, tmp_win->w,
	FShapeBounding, FShapeSet);
      if (tmp_win->title_w)
      {
	/* windows w/ titles */
	rect.x = tmp_win->boundary_width;
	rect.y = tmp_win->title_g.y;
	rect.width = w - 2*tmp_win->boundary_width;
	rect.height = tmp_win->title_g.height;

	FShapeCombineRectangles(
	  dpy, tmp_win->frame, FShapeBounding, 0, 0, &rect, 1, FShapeUnion,
	  Unsorted);
      }
    }
    else
    {
      /* unset shape */
      FShapeCombineMask(
	dpy, tmp_win->frame, FShapeBounding, 0, 0, None, FShapeSet);
    }
  }
}

/****************************************************************************
 *
 *  Sets the allowed button states
 *
 ****************************************************************************/
void CMD_ButtonState(F_CMD_ARGS)
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
void CMD_BorderStyle(F_CMD_ARGS)
{
  char *parm;
  char *prev;
#ifdef USEDECOR
  FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;
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
      tmpdf.next = NULL;
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
      tmpdf.next = NULL;
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

/****************************************************************************
 *
 *  redraw the decoration when style change
 *
 ****************************************************************************/
void RedrawDecorations(FvwmWindow *tmp_win)
{
  FvwmWindow *u = Scr.Hilite;

  if (IS_SHADED(tmp_win))
    XRaiseWindow(dpy, tmp_win->decor_w);
  /* domivogt (6-Jun-2000): Don't check if the window is visible here.  If we
   * do, some updates are not applied and when the window becomes visible
   * again, the X Server may not redraw the window. */
  DrawDecorations(
    tmp_win, DRAW_ALL, (Scr.Hilite == tmp_win), 2, None, CLEAR_ALL);
  Scr.Hilite = u;
}
