/* -*-c-*- */
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

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Flocale.h"
#include <libs/gravity.h>
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
#include "frame.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

extern Window PressedW;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

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

typedef struct
{
	GC relief;
	GC shadow;
	GC transparent;
	GC tile;
} draw_border_gcs;

typedef struct
{
	int offset_tl;
	int offset_br;
	int thickness;
	int length;
	unsigned has_x_marks : 1;
	unsigned has_y_marks : 1;
} border_marks_descr;

typedef struct
{
	int w_dout;
	int w_hiout;
	int w_trout;
	int w_c;
	int w_trin;
	int w_shin;
	int w_din;
	int sum;
	int trim;
	unsigned is_flat : 1;
} border_relief_size_descr;

typedef struct
{
	rectangle sidebar_g;
	border_relief_size_descr relief;
	border_marks_descr marks;
	draw_border_gcs gcs;
} border_relief_descr;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static const char ulgc[] = { 1, 0, 0, 0x7f, 2, 1, 1 };
static const char brgc[] = { 1, 1, 2, 0x7f, 0, 0, 3 };

/* ---------------------------- exported variables (globals) ---------------- */

XGCValues Globalgcv;
unsigned long Globalgcm;

/* ---------------------------- local functions ----------------------------- */

/* used by several drawing functions */
static void ClipClear(Window win, XRectangle *rclip, Bool do_flush_expose)
{
	if (rclip)
	{
		XClearArea(
			dpy, win, rclip->x, rclip->y, rclip->width,
			rclip->height, False);
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
	Window win, GC ReliefGC, GC ShadowGC, Pixel fore_color,
	Pixel back_color, struct vector_coords *coords, int w, int h)
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
		XDrawLine(
			dpy, win,
			  (coords->use_fgbg & (1 << i))
			  ? (coords->line_style & (1 << i)) ?
			fore_GC : back_GC
			  : (coords->line_style & (1 << i)) ?
			ReliefGC : ShadowGC,
			w * coords->x[i-1]/100, h * coords->y[i-1]/100,
			w * coords->x[i]/100, h * coords->y[i]/100);
	}

	return;
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
  mwm_flags stateflags, int set_layer, int layer, int left1right0,
  XRectangle *rclip, Pixmap *pbutton_background_pixmap)
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
	(stateflags & MWM_DECOR_STICK && IS_STICKY(t)) ||
        (set_layer && (t->layer == layer))))
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
	{
		pm = CreateStretchPixmap(
			dpy, src->picture, src->width, src->height, src->depth,
			width, height, gc);
	}
	else
	{
		pm = CreateTiledPixmap(
			dpy, src->picture, src->width, src->height, width,
			height, src->depth, gc);
	}
	XCopyArea(dpy, pm, dest, gc, 0, 0, width, height, x_start, y_start);
	XFreePixmap(dpy, pm);

	return;
}

static int get_multipm_length(
	FvwmWindow *fw, Picture **pm, int part)
{
	if (pm[part] == NULL)
	{
		return 0;
	}
	else if (HAS_VERTICAL_TITLE(fw))
	{
		return pm[part]->height;
	}
	else
	{
		return pm[part]->width;
	}
}

/****************************************************************************
 *
 *  Redraws multi-pixmap titlebar (tril@igs.net)
 *
 ****************************************************************************/
#define SWAP_ARGS(f,a1,a2) (f)?(a2):(a1),(f)?(a1):(a2)
static void DrawMultiPixmapTitlebar(FvwmWindow *fw, DecorFace *df)
{
	Window title_win;
	GC gc;
	char *title;
	Picture **pm;
	short stretch_flags;
	int text_length, text_pos, text_offset;
	int before_space, after_space, under_offset, under_width;
	int size;
	FlocaleWinString fstr;
	rectangle tmp_g;
	Bool has_vt = HAS_VERTICAL_TITLE(fw);

	pm = df->u.multi_pixmaps;
	stretch_flags = df->u.multi_stretch_flags;
	gc = Scr.TitleGC;
	XSetClipMask(dpy, gc, None);
	title_win = FW_W_TITLE(fw);
	title = fw->visible_name;

	tmp_g.width = 0;
	tmp_g.height = 0;
	get_title_geometry(fw, &tmp_g);
	if (pm[TBP_MAIN])
	{
		RenderIntoWindow(
			gc, pm[TBP_MAIN], title_win, 0, 0, tmp_g.width,
			tmp_g.height, (stretch_flags & (1 << TBP_MAIN)));
	}
	else if (!title)
	{
		RenderIntoWindow(
			gc, pm[TBP_LEFT_MAIN], title_win, 0, 0, tmp_g.width,
			tmp_g.height, (stretch_flags & (1 << TBP_LEFT_MAIN)));
	}

	if (title)
	{
		int len = strlen(title);

		if (has_vt)
		{
			len = -len;
		}
		text_length = FlocaleTextWidth(fw->title_font, title, len);
		if (text_length > fw->title_length)
		{
			text_length = fw->title_length;
		}
		switch (TB_JUSTIFICATION(GetDecor(fw, titlebar)))
		{
		case JUST_LEFT:
			text_pos = TITLE_PADDING;
			text_pos += get_multipm_length(
				fw, pm, TBP_LEFT_OF_TEXT);
			text_pos += get_multipm_length(
				fw, pm, TBP_LEFT_END);
			if (text_pos > fw->title_length - text_length)
			{
				text_pos = fw->title_length - text_length;
			}
			break;
		case JUST_RIGHT:
			text_pos = fw->title_length - text_length -
				TITLE_PADDING;
			text_pos -= get_multipm_length(
				fw, pm, TBP_RIGHT_OF_TEXT);
			text_pos -= get_multipm_length(
				fw, pm, TBP_RIGHT_END);
			if (text_pos < 0)
			{
				text_pos = 0;
			}
			break;
		default:
			text_pos = (fw->title_length - text_length) / 2;
			break;
		}

		under_offset = text_pos;
		under_width = text_length;
		/* If there's title padding, put it *inside* the undertext
		 * area: */
		if (under_offset >= TITLE_PADDING)
		{
			under_offset -= TITLE_PADDING;
			under_width += TITLE_PADDING;
		}
		if (under_offset + under_width + TITLE_PADDING <=
		    fw->title_length)
		{
			under_width += TITLE_PADDING;
		}
		before_space = under_offset;
		after_space = fw->title_length - before_space -
			under_width;

		if (pm[TBP_LEFT_MAIN] && before_space > 0)
		{
			RenderIntoWindow(
				gc, pm[TBP_LEFT_MAIN], title_win, 0, 0,
				SWAP_ARGS(has_vt, before_space,
					  fw->title_thickness),
				(stretch_flags & (1 << TBP_LEFT_MAIN)));
		}
		if (pm[TBP_RIGHT_MAIN] && after_space > 0)
		{
			RenderIntoWindow(
				gc, pm[TBP_RIGHT_MAIN], title_win,
				SWAP_ARGS(has_vt, under_offset + under_width,
					  0),
				SWAP_ARGS(has_vt, after_space,
					  fw->title_thickness),
				(stretch_flags & (1 << TBP_RIGHT_MAIN)));
		}
		if (pm[TBP_UNDER_TEXT] && under_width > 0)
		{
			RenderIntoWindow(
				gc, pm[TBP_UNDER_TEXT], title_win,
				SWAP_ARGS(has_vt, under_offset, 0),
				SWAP_ARGS(has_vt, under_width,
					  fw->title_thickness),
				(stretch_flags & (1 << TBP_UNDER_TEXT)));
		}
		size = get_multipm_length(fw, pm, TBP_LEFT_OF_TEXT);
		if (size > 0 && size <= before_space)
		{

			XCopyArea(
				dpy, pm[TBP_LEFT_OF_TEXT]->picture, title_win,
				gc, 0, 0,
				SWAP_ARGS(has_vt, size,
					  fw->title_thickness),
				SWAP_ARGS(has_vt, under_offset - size, 0));
			before_space -= size;
		}
		size = get_multipm_length(fw, pm, TBP_RIGHT_OF_TEXT);
		if (size > 0 && size <= after_space)
		{
			XCopyArea(
				dpy, pm[TBP_RIGHT_OF_TEXT]->picture, title_win,
				gc, 0, 0,
				SWAP_ARGS(has_vt, size,
					  fw->title_thickness),
				SWAP_ARGS(has_vt, under_offset + under_width,
					  0));
			after_space -= size;
		}

		get_title_font_size_and_offset(
			fw, GET_TITLE_DIR(fw),
			GET_TITLE_TEXT_DIR_MODE(fw), &size, &text_offset);
		memset(&fstr, 0, sizeof(fstr));
		fstr.str = fw->visible_name;
		fstr.win = title_win;
		if (has_vt)
		{
			fstr.x = text_offset;
			fstr.y = text_pos;
		}
		else
		{
			fstr.y = text_offset;
			fstr.x = text_pos;
		}
		fstr.gc = gc;
		fstr.flags.text_direction = fw->title_text_dir;
		FlocaleDrawString(dpy, fw->title_font, &fstr, 0);
	}
	else
	{
		before_space = fw->title_length / 2;
		after_space = fw->title_length / 2;
	}

	size = get_multipm_length(fw, pm, TBP_LEFT_END);
	if (size > 0 && size <= before_space)
	{
		XCopyArea(
			dpy, pm[TBP_LEFT_END]->picture, title_win, gc, 0, 0,
			SWAP_ARGS(has_vt, size, fw->title_thickness),
			0, 0);
	}
	size = get_multipm_length(fw, pm, TBP_RIGHT_END);
	if (size > 0 && size <= after_space)
	{
		XCopyArea(
			dpy, pm[TBP_RIGHT_END]->picture, title_win, gc, 0, 0,
			SWAP_ARGS(has_vt, size, fw->title_thickness),
			fw->title_length - size, 0);
	}

	return;
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
				return (toggled) ?
					BS_ToggledActiveDown : BS_ActiveDown;
			}
			else
			{
				return (toggled) ?
					BS_ToggledActiveUp : BS_ActiveUp;
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
    if (FW_W_BUTTON(t, i) != None &&
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
	get_button_state(has_focus, toggled, FW_W_BUTTON(t, i));
      DecorFace *df = &TB_STATE(GetDecor(t, buttons[i]))[bs];
      if(flush_expose(FW_W_BUTTON(t, i)) || expose_win == FW_W_BUTTON(t, i) ||
	 expose_win == None || cd->flags.has_color_changed)
      {
	int is_inverted = (PressedW == FW_W_BUTTON(t, i));

	if (DFS_USE_BORDER_STYLE(df->style))
	{
	  XChangeWindowAttributes(
	    dpy, FW_W_BUTTON(t, i), cd->valuemask, &cd->attributes);
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
	    dpy, FW_W_BUTTON(t, i), cd->notex_valuemask, &cd->notex_attributes);
	  t->button_background_pixmap[i] = None;
	  pass_bg_pixmap = &t->button_background_pixmap[i];
	}
	XClearWindow(dpy, FW_W_BUTTON(t, i));
	if (DFS_USE_TITLE_STYLE(df->style))
	{
	  DecorFace *tsdf = &TB_STATE(GetDecor(t, titlebar))[bs];

	  is_lowest = True;
	  for (; tsdf; tsdf = tsdf->next)
	  {
	    DrawButton(
		    t, FW_W_BUTTON(t, i), t->title_thickness,
		    t->title_thickness, tsdf, cd->relief_gc, cd->shadow_gc,
		    cd->fore_color, cd->back_color, is_lowest,
		    TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])),
		    TB_FLAGS(t->decor->buttons[i]).has_layer,
		    TB_LAYER(t->decor->buttons[i]), left1right0, NULL,
		    pass_bg_pixmap);
	    is_lowest = False;
	  }
	}
	is_lowest = True;
	for (; df; df = df->next)
	{
	  DrawButton(
		  t, FW_W_BUTTON(t, i), t->title_thickness, t->title_thickness,
		  df, cd->relief_gc, cd->shadow_gc,
		  cd->fore_color, cd->back_color, is_lowest,
		  TB_MWM_DECOR_FLAGS(GetDecor(t, buttons[i])),
		  TB_FLAGS(t->decor->buttons[i]).has_layer,
		  TB_LAYER(t->decor->buttons[i]),
		  left1right0, NULL, pass_bg_pixmap);
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
	      dpy, FW_W_BUTTON(t, i), 0, 0, t->title_thickness - 1,
	      t->title_thickness - 1, (reverse ? cd->shadow_gc : cd->relief_gc),
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
  int offset;
  int length;
  int i;
  FlocaleWinString fstr;
  enum ButtonState title_state;
  DecorFaceStyle *tb_style;
  Pixmap *pass_bg_pixmap;
  GC rgc = cd->relief_gc;
  GC sgc = cd->shadow_gc;
  Bool reverse = False;
  Bool is_clipped = False;
  Bool has_vt;
  Bool toggled =
    (HAS_MWM_BUTTONS(t) &&
     ((TB_HAS_MWM_DECOR_MAXIMIZE(GetDecor(t, titlebar)) && IS_MAXIMIZED(t))||
      (TB_HAS_MWM_DECOR_SHADE(GetDecor(t, titlebar)) && IS_SHADED(t)) ||
      (TB_HAS_MWM_DECOR_STICK(GetDecor(t, titlebar)) && IS_STICKY(t))));

  if (PressedW == FW_W_TITLE(t))
  {
    GC tgc;

    tgc = sgc;
    sgc = rgc;
    rgc = tgc;
  }
#if 0
  flush_expose(FW_W_TITLE(t));
#endif

  if (t->visible_name != (char *)NULL)
  {
    length = FlocaleTextWidth(
      t->title_font, t->visible_name,
      (HAS_VERTICAL_TITLE(t)) ?
      -strlen(t->visible_name) : strlen(t->visible_name));
    if (length > t->title_length - 12)
      length = t->title_length - 4;
    if (length < 0)
      length = 0;
  }
  else
  {
    length = 0;
  }

  title_state = get_button_state(has_focus, toggled, FW_W_TITLE(t));
  tb_style = &TB_STATE(GetDecor(t, titlebar))[title_state].style;
  switch (TB_JUSTIFICATION(GetDecor(t, titlebar)))
  {
  case JUST_LEFT:
    offset = WINDOW_TITLE_TEXT_OFFSET;
    break;
  case JUST_RIGHT:
    offset = t->title_length - length - WINDOW_TITLE_TEXT_OFFSET;
    break;
  case JUST_CENTER:
  default:
    offset = (t->title_length - length) / 2;
    break;
  }

  NewFontAndColor(t->title_font, cd->fore_color, cd->back_color);

  /* the next bit tries to minimize redraw (veliaa@rpi.edu) */
  /* we need to check for UseBorderStyle for the titlebar */
  {
    DecorFace *df = (has_focus) ?
      &GetDecor(t,BorderStyle.active) : &GetDecor(t,BorderStyle.inactive);

    if (DFS_USE_BORDER_STYLE(*tb_style) &&
	DFS_FACE_TYPE(df->style) == TiledPixmapButton &&
	df->u.p)
    {
      XSetWindowBackgroundPixmap(dpy, FW_W_TITLE(t), df->u.p->picture);
      t->title_background_pixmap = df->u.p->picture;
      pass_bg_pixmap = NULL;
    }
    else
    {
      pass_bg_pixmap = &t->title_background_pixmap;
    }
  }
  ClipClear(FW_W_TITLE(t), rclip, False);
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
  memset(&fstr, 0, sizeof(fstr));
  fstr.str = t->visible_name;
  fstr.win = FW_W_TITLE(t);
  fstr.flags.text_direction = t->title_text_dir;
  if (HAS_VERTICAL_TITLE(t))
  {
    fstr.y = offset;
    fstr.x = t->title_text_offset + 1;
    has_vt = True;
  }
  else
  {
    fstr.x = offset;
    fstr.y = t->title_text_offset + 1;
    has_vt = False;
  }
  fstr.gc = Scr.TitleGC;
  if (Pdepth < 2)
  {
    XFillRectangle(
      dpy, FW_W_TITLE(t), ((PressedW == FW_W_TITLE(t)) ? sgc : rgc),
      offset - 2, 0, length+4, t->title_thickness);
    if(t->visible_name != (char *)NULL)
    {
      FlocaleDrawString(dpy, t->title_font, &fstr, 0);
    }
    /* for mono, we clear an area in the title bar where the window
     * title goes, so that its more legible. For color, no need */
    RelieveRectangle(
      dpy, FW_W_TITLE(t), 0, 0,
      SWAP_ARGS(has_vt, offset - 3, t->title_thickness - 1),
      rgc, sgc, cd->relief_width);
    RelieveRectangle(
      dpy, FW_W_TITLE(t),
      SWAP_ARGS(has_vt, offset + length + 2, 0),
      SWAP_ARGS(has_vt, t->title_length - length - offset - 3,
		t->title_thickness - 1),
      rgc, sgc, cd->relief_width);
    XDrawLine(
      dpy, FW_W_TITLE(t), sgc,
      SWAP_ARGS(has_vt, 0, offset + length + 1),
      SWAP_ARGS(has_vt, offset + length + 1, t->title_thickness));
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
    if (PressedW == FW_W_TITLE(t))
    {
      for (; df; df = df->next)
      {
	DrawButton(
	  t, FW_W_TITLE(t),
	  SWAP_ARGS(has_vt, t->title_length, t->title_thickness),
	  df, sgc, rgc, cd->fore_color, cd->back_color, is_lowest, 0, 0, 0, 1,
	  rclip, pass_bg_pixmap);
	is_lowest = False;
      }
    }
    else
    {
      for (; df; df = df->next)
      {
	DrawButton(
	  t, FW_W_TITLE(t),
	  SWAP_ARGS(has_vt, t->title_length, t->title_thickness),
	  df, rgc, sgc, cd->fore_color, cd->back_color, is_lowest, 0, 0, 0,
          1, rclip, pass_bg_pixmap);
	is_lowest = False;
      }
    }

#ifdef FANCY_TITLEBARS
    if (TB_STATE(GetDecor(t, titlebar))[title_state].style.face_type
        != MultiPixmap)
#endif
    {
      FlocaleDrawString(dpy, t->title_font, &fstr, 0);
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
	dpy, FW_W_TITLE(t), 0, 0,
	SWAP_ARGS(has_vt, t->title_length - 1, t->title_thickness - 1),
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
      (int)(t->title_thickness / WINDOW_TITLE_STICK_VERT_DIST / 2) * 2 - 1;
    int min = t->title_thickness / 2 - num * 2 + 1;
    int max =
            t->title_thickness / 2 + num * 2 - WINDOW_TITLE_STICK_VERT_DIST + 1;
    int left_x = WINDOW_TITLE_STICK_OFFSET;
    int left_w = offset - left_x - WINDOW_TITLE_TO_STICK_GAP;
    int right_x = offset + length + WINDOW_TITLE_TO_STICK_GAP - 1;
    int right_w = t->title_length - right_x - WINDOW_TITLE_STICK_OFFSET;

    if (left_w < WINDOW_TITLE_STICK_MIN_WIDTH)
    {
      left_x = 0;
      left_w = WINDOW_TITLE_STICK_MIN_WIDTH;
    }
    if (right_w < WINDOW_TITLE_STICK_MIN_WIDTH)
    {
      right_w = WINDOW_TITLE_STICK_MIN_WIDTH;
      right_x = t->title_length - WINDOW_TITLE_STICK_MIN_WIDTH - 1;
    }
    for (i = min; i <= max; i += WINDOW_TITLE_STICK_VERT_DIST)
    {
      if (left_w > 0)
      {
	RelieveRectangle(
          dpy, FW_W_TITLE(t),
	  SWAP_ARGS(has_vt, left_x, i),
	  SWAP_ARGS(has_vt, left_w, 1),
	  sgc, rgc, 1);
      }
      if (right_w > 0)
      {
	RelieveRectangle(
          dpy, FW_W_TITLE(t),
	  SWAP_ARGS(has_vt, right_x, i),
	  SWAP_ARGS(has_vt, right_w, 1),
	  sgc, rgc, 1);
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

static void get_common_decorations(
	common_decorations_type *cd, FvwmWindow *t,
	draw_window_parts draw_parts, Bool has_focus, int force,
	Bool is_border, Bool do_change_gcs)
{
	color_quad *draw_colors;

	if (has_focus)
	{
		/* are we using textured borders? */
		if (DFS_FACE_TYPE(
			    GetDecor(t, BorderStyle.active.style)) ==
		    TiledPixmapButton)
		{
			cd->texture_pixmap = GetDecor(
				t, BorderStyle.active.u.p->picture);
		}
		cd->back_pixmap = Scr.gray_pixmap;
		if (is_border)
		{
			draw_colors = &(t->border_hicolors);
		}
		else
		{
			draw_colors = &(t->hicolors);
		}
	}
	else
	{
		if (DFS_FACE_TYPE(GetDecor(t, BorderStyle.inactive.style)) ==
		    TiledPixmapButton)
		{
			cd->texture_pixmap = GetDecor(
				t, BorderStyle.inactive.u.p->picture);
		}
		if (IS_STICKY(t))
		{
			cd->back_pixmap = Scr.sticky_gray_pixmap;
		}
		else
		{
			cd->back_pixmap = Scr.light_gray_pixmap;
		}
		if (is_border)
		{
			draw_colors = &(t->border_colors);
		}
		else
		{
			draw_colors = &(t->colors);
		}
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
	}
	else
	{
		if (Pdepth < 2)
		{
			cd->attributes.background_pixmap = cd->back_pixmap;
			cd->valuemask = CWBackPixmap;
		}
		else
		{
			cd->attributes.background_pixel = cd->back_color;
			cd->valuemask = CWBackPixel;
		}
	}
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


	return;
}

static int border_context_to_parts(
	int context)
{
	if (context == C_FRAME || context == C_SIDEBAR ||
	    context == (C_FRAME | C_SIDEBAR))
	{
		return DRAW_FRAME;
	}
	else if (context == C_F_TOPLEFT)
	{
		return DRAW_BORDER_NW;
	}
	else if (context == C_F_TOPRIGHT)
	{
		return DRAW_BORDER_NE;
	}
	else if (context == C_F_BOTTOMLEFT)
	{
		return DRAW_BORDER_SW;
	}
	else if (context == C_F_BOTTOMRIGHT)
	{
		return DRAW_BORDER_SE;
	}
	else if (context == C_SB_LEFT)
	{
		return DRAW_BORDER_W;
	}
	else if (context == C_SB_RIGHT)
	{
		return DRAW_BORDER_E;
	}
	else if (context == C_SB_TOP)
	{
		return DRAW_BORDER_N;
	}
	else if (context == C_SB_BOTTOM)
	{
		return DRAW_BORDER_S;
	}
	else if (context == C_TITLE)
	{
		return DRAW_TITLE;
	}
	else if (context & (C_LALL | C_RALL))
	{
		return DRAW_BUTTONS;
	}

	return DRAW_NONE;
}

static int border_get_parts_and_pos_to_draw(
	common_decorations_type *cd, FvwmWindow *fw,
	draw_window_parts pressed_parts, draw_window_parts force_draw_parts,
	rectangle *old_g, rectangle *new_g, Bool do_hilight,
	border_relief_descr *br)
{
	draw_window_parts draw_parts;
	draw_window_parts parts_to_light;
	rectangle sidebar_g_old;
	DecorFaceStyle *borderstyle;
	Bool has_x_marks;
	Bool has_x_marks_old;
	Bool has_y_marks;
	Bool has_y_marks_old;

	draw_parts = 0;
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	frame_get_sidebar_geometry(
		fw, borderstyle,  new_g, &br->sidebar_g, &has_x_marks,
		&has_y_marks);
	if (has_x_marks == True)
	{
		draw_parts |= DRAW_X_HANDLES;
		br->marks.has_x_marks = 1;
	}
	else
	{
		br->marks.has_x_marks = 0;
	}
	if (has_y_marks == True)
	{
		draw_parts |= DRAW_Y_HANDLES;
		br->marks.has_y_marks = 1;
	}
	else
	{
		br->marks.has_y_marks = 0;
	}
	draw_parts |= (pressed_parts ^ fw->border_state.parts_inverted);
	parts_to_light = (do_hilight == True) ? DRAW_FRAME : DRAW_NONE;
	draw_parts |= (parts_to_light ^ fw->border_state.parts_lit);
	draw_parts |= (~(fw->border_state.parts_drawn) & DRAW_FRAME);
	draw_parts |= force_draw_parts;
	if (old_g == NULL)
	{
		old_g = &fw->frame_g;
	}
	if (draw_parts == DRAW_FRAME)
	{
		draw_parts |= DRAW_FRAME;
		return draw_parts;
	}
	frame_get_sidebar_geometry(
		fw, borderstyle, old_g, &sidebar_g_old, &has_x_marks_old,
		&has_y_marks_old);
	if (sidebar_g_old.x != br->sidebar_g.x ||
	    sidebar_g_old.y != br->sidebar_g.y)
	{
		draw_parts |= DRAW_FRAME;
		return draw_parts;
	}
	if (has_x_marks_old != has_x_marks ||
	    has_y_marks_old != has_y_marks)
	{
		/* handle marks are drawn into the corner handles */
		draw_parts |= DRAW_CORNERS;
	}
	if ((cd->valuemask & CWBackPixmap) &&
	    sidebar_g_old.width != br->sidebar_g.width)
	{
		/* update the pixmap origin */
		draw_parts |=
			DRAW_BORDER_NE |
			DRAW_BORDER_E |
			DRAW_BORDER_SE;
	}
	if ((cd->valuemask & CWBackPixmap) &&
	    sidebar_g_old.height != br->sidebar_g.height)
	{
		/* update the pixmap origin */
		draw_parts |=
			DRAW_BORDER_SW |
			DRAW_BORDER_S |
			DRAW_BORDER_SE;
	}

	return draw_parts;
}

static void border_get_border_gcs(
	draw_border_gcs *ret_gcs, common_decorations_type *cd, FvwmWindow *fw,
	Bool do_hilight)
{
	static GC transparent_gc = None;
	static GC tile_gc = None;
	DecorFaceStyle *borderstyle;
	Bool is_reversed = False;
	unsigned long valuemask;
	XGCValues xgcv;

	if (transparent_gc == None && HAS_BORDER(fw) && !HAS_MWM_BORDER(fw))
	{
		XGCValues xgcv;

		xgcv.function = GXnoop;
		xgcv.plane_mask = 0;
		transparent_gc = fvwmlib_XCreateGC(
			dpy, FW_W_FRAME(fw), GCFunction | GCPlaneMask, &xgcv);
	}
	ret_gcs->transparent = transparent_gc;
	if (tile_gc == None)
	{
		XGCValues xgcv;

		tile_gc = fvwmlib_XCreateGC(dpy, FW_W_FRAME(fw), 0, &xgcv);
	}
	ret_gcs->tile = tile_gc;
	/* setup the tile gc*/
	if ((cd->valuemask & CWBackPixmap))
	{
		/* the origin has to be set depending on the border part to
		 * draw. */
		xgcv.fill_style = FillTiled;
		xgcv.tile = cd->attributes.background_pixmap;
		valuemask = GCTile | GCFillStyle;
	}
	else
	{
		xgcv.fill_style = FillSolid;
		xgcv.foreground = cd->attributes.background_pixel;
		valuemask = GCForeground | GCFillStyle;
	}
	XChangeGC(dpy, tile_gc, valuemask, &xgcv);
	/* get the border style bits */
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	if (borderstyle->flags.button_relief == DFS_BUTTON_IS_SUNK)
	{
		is_reversed = True;
	}
	if (is_reversed)
	{
		ret_gcs->shadow = cd->relief_gc;
		ret_gcs->relief = cd->shadow_gc;
	}
	else
	{
		ret_gcs->relief = cd->relief_gc;
		ret_gcs->shadow = cd->shadow_gc;
	}

	return;
}

static void trim_border_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
{
	/* If the border is too thin to accomodate the standard look, we remove
	 * parts of the border so that at least one pixel of the original
	 * colour is visible. We make an exception for windows with a border
	 * width of 2, though. */
	if ((!IS_SHADED(fw) || HAS_TITLE(fw)) && fw->boundary_width == 2)
	{
		ret_size_descr->trim--;
	}
	if (ret_size_descr->trim < 0)
	{
		ret_size_descr->trim = 0;
	}
	for ( ; ret_size_descr->trim > 0; ret_size_descr->trim--)
	{
		if (ret_size_descr->w_hiout > 1)
		{
			ret_size_descr->w_hiout--;
		}
		else if (ret_size_descr->w_shin > 0)
		{
			ret_size_descr->w_shin--;
		}
		else if (ret_size_descr->w_hiout > 0)
		{
			ret_size_descr->w_hiout--;
		}
		else if (ret_size_descr->w_trout > 0)
		{
			ret_size_descr->w_trout = 0;
			ret_size_descr->w_trin = 0;
			ret_size_descr->w_din = 0;
			ret_size_descr->w_hiout = 1;
		}
		ret_size_descr->sum--;
	}
	ret_size_descr->w_c = fw->boundary_width - ret_size_descr->sum;

	return;
}

static void check_remove_inset(
	DecorFaceStyle *borderstyle, border_relief_size_descr *ret_size_descr)
{
	if (!DFS_HAS_NO_INSET(*borderstyle))
	{
		return;
	}
	ret_size_descr->w_shin = 0;
	ret_size_descr->sum--;
	ret_size_descr->trim--;
	if (ret_size_descr->w_trin)
	{
		ret_size_descr->w_trout = 0;
		ret_size_descr->w_trin = 0;
		ret_size_descr->w_din = 0;
		ret_size_descr->w_hiout = 1;
		ret_size_descr->sum -= 2;
		ret_size_descr->trim -= 2;
	}

	return;
}

static void border_fetch_mwm_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
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
	ret_size_descr->w_dout = 0;
	ret_size_descr->w_hiout = 2;
	ret_size_descr->w_trout = 0;
	ret_size_descr->w_trin = 0;
	ret_size_descr->w_shin = 1;
	ret_size_descr->w_din = 0;
	ret_size_descr->sum = 3;
	ret_size_descr->trim = ret_size_descr->sum - fw->boundary_width + 1;
	check_remove_inset(borderstyle, ret_size_descr);
	trim_border_layout(fw, borderstyle, ret_size_descr);

	return;
}

static void border_fetch_fvwm_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
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
	ret_size_descr->w_dout = 1;
	ret_size_descr->w_hiout = 1;
	ret_size_descr->w_trout = 1;
	ret_size_descr->w_trin = 1;
	ret_size_descr->w_shin = 1;
	ret_size_descr->w_din = 1;
	/* w_trout + w_trin counts only as one pixel of border because
	 * they let one pixel of the original colour shine through. */
	ret_size_descr->sum = 6;
	ret_size_descr->trim = ret_size_descr->sum - fw->boundary_width;
	check_remove_inset(borderstyle, ret_size_descr);
	trim_border_layout(fw, borderstyle, ret_size_descr);

	return;
}

static void border_get_border_relief_size_descr(
	border_relief_size_descr *ret_size_descr, FvwmWindow *fw,
	Bool do_hilight)
{
	DecorFaceStyle *borderstyle;

	if (is_window_border_minimal(fw))
	{
		/* the border is too small, only a background but no relief */
		ret_size_descr->is_flat = 1;
		return;
	}
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	if (borderstyle->flags.button_relief == DFS_BUTTON_IS_FLAT)
	{
		ret_size_descr->is_flat = 1;
		return;
	}
	ret_size_descr->is_flat = 0;
	/* get the relief layout */
	if (HAS_MWM_BORDER(fw))
	{
		border_fetch_mwm_layout(fw, borderstyle, ret_size_descr);
	}
	else
	{
		border_fetch_fvwm_layout(fw, borderstyle, ret_size_descr);
	}

	return;
}

static void border_get_border_marks_descr(
	common_decorations_type *cd, border_relief_descr *br, FvwmWindow *fw)
{
	int inset;

	/* get mark's length and thickness */
	inset = (br->relief.w_shin != 0 || br->relief.w_din != 0);
	br->marks.length = fw->boundary_width - br->relief.w_dout - inset;
	if (br->marks.length <= 0)
	{
		br->marks.has_x_marks = 0;
		br->marks.has_y_marks = 0;
		return;
	}
	br->marks.thickness = cd->relief_width;
	if (br->marks.thickness > br->marks.length)
	{
		br->marks.thickness = br->marks.length;
	}
	/* get offsets from outer side of window */
	br->marks.offset_tl = br->relief.w_dout;
	br->marks.offset_br =
		-br->relief.w_dout - br->marks.length - br->marks.offset_tl;

	return;
}


static void border_set_part_pixmap_back(
	common_decorations_type *cd, GC gc, rectangle *part_g, Pixmap dest_pix)
{
	if (cd->valuemask & CWBackPixmap)
	{
		XGCValues xgcv;

		/* calculate the tile offset */
		xgcv.ts_x_origin = -part_g->x;
		xgcv.ts_y_origin = -part_g->y;
		XChangeGC(
			dpy, gc, GCTileStipXOrigin | GCTileStipYOrigin, &xgcv);
	}
	XFillRectangle(dpy, dest_pix, gc, 0, 0, part_g->width, part_g->height);

	return;
}

inline static Pixmap border_create_part_pixmap(
	common_decorations_type *cd, rectangle *part_g)
{
	Pixmap p;

	p = XCreatePixmap(dpy, Scr.Root, part_g->width, part_g->height, Pdepth);

	return p;
}

static void border_draw_part_relief(
	border_relief_descr *br, rectangle *frame_g, rectangle *part_g,
	Pixmap dest_pix, Bool is_inverted)
{
	int i;
	int off_x = 0;
	int off_y = 0;
	int width = frame_g->width - 1;
	int height = frame_g->height - 1;
	int w[7];
	GC gc[4];

	w[0] = br->relief.w_dout;
	w[1] = br->relief.w_hiout;
	w[2] = br->relief.w_trout;
	w[3] = br->relief.w_c;
	w[4] = br->relief.w_trin;
	w[5] = br->relief.w_shin;
	w[6] = br->relief.w_din;
	gc[(is_inverted == True)] = br->gcs.relief;
	gc[!(is_inverted == True)] = br->gcs.shadow;
	gc[2] = br->gcs.transparent;
	gc[3] = br->gcs.shadow;

	off_x = -part_g->x;
	off_y = -part_g->y;
	width = frame_g->width - 1;
	height = frame_g->height - 1;
	for (i = 0; i < 7; i++)
	{
		if (ulgc[i] != 0x7f && w[i] > 0)
		{
			RelieveRectangle(
				dpy, dest_pix, off_x, off_y,
				width, height, gc[(int)ulgc[i]],
				gc[(int)brgc[i]], w[i]);
		}
		off_x += w[i];
		off_y += w[i];
		width -= 2 * w[i];
		height -= 2 * w[i];
	}

	return;
}

static void border_draw_x_mark(
	border_relief_descr *br, int x, int y, Pixmap dest_pix,
	Bool do_draw_shadow)
{
	int k;
	int length;
	GC gc;

	if (br->marks.has_x_marks == 0)
	{
		return;
	}
	x += br->marks.offset_tl;
	gc = (do_draw_shadow) ? br->gcs.shadow : br->gcs.relief;
	/* draw it */
	for (k = 0, length = br->marks.length - 1; k < br->marks.thickness;
	     k++, length--)
	{
		int x1;
		int x2;
		int y1;
		int y2;

		if (length < 0)
		{
			break;
		}
		if (do_draw_shadow)
		{
			x1 = x + k;
			y1 = y - 1 - k;
		}
		else
		{
			x1 = x;
			y1 = y + k;
		}
		x2 = x1 + length;
		y2 = y1;
		XDrawLine(dpy, dest_pix, gc, x1, y1, x2, y2);
	}

	return;
}

static void border_draw_y_mark(
	border_relief_descr *br, int x, int y, Pixmap dest_pix,
	Bool do_draw_shadow)
{
	int k;
	int length;
	GC gc;

	if (br->marks.has_y_marks == 0)
	{
		return;
	}
	y += br->marks.offset_tl;
	gc = (do_draw_shadow) ? br->gcs.shadow : br->gcs.relief;
	/* draw it */
	for (k = 0, length = br->marks.length; k < br->marks.thickness;
	     k++, length--)
	{
		int x1;
		int x2;
		int y1;
		int y2;

		if (length <= 0)
		{
			break;
		}
		if (do_draw_shadow)
		{
			x1 = x - 1 - k;
			y1 = y + k;
		}
		else
		{
			x1 = x + k;
			y1 = y;
		}
		x2 = x1;
		y2 = y1 + length - 1;
		XDrawLine(dpy, dest_pix, gc, x1, y1, x2, y2);
	}

	return;
}

static void border_draw_part_marks(
	border_relief_descr *br, rectangle *part_g, draw_window_parts part,
	Pixmap dest_pix)
{
	int l;
	int t;
	int w;
	int h;
	int o;

	l = br->sidebar_g.x;
	t = br->sidebar_g.y;
	w = part_g->width;
	h = part_g->height;
	o = br->marks.offset_br;
	switch (part)
	{
	case DRAW_BORDER_N:
		border_draw_y_mark(br, 0, 0, dest_pix, False);
		border_draw_y_mark(br, w, 0, dest_pix, True);
		break;
	case DRAW_BORDER_S:
		border_draw_y_mark(br, 0, h + o, dest_pix, False);
		border_draw_y_mark(br, w, h + o, dest_pix, True);
		break;
	case DRAW_BORDER_E:
		border_draw_x_mark(br, w + o, 0, dest_pix, False);
		border_draw_x_mark(br, w + o, h, dest_pix, True);
		break;
	case DRAW_BORDER_W:
		border_draw_x_mark(br, 0, 0, dest_pix, False);
		border_draw_x_mark(br, 0, h, dest_pix, True);
		break;
	case DRAW_BORDER_NW:
		border_draw_x_mark(br, 0, t, dest_pix, True);
		border_draw_y_mark(br, l, 0, dest_pix, True);
		break;
	case DRAW_BORDER_NE:
		border_draw_x_mark(br, l + o, t, dest_pix, True);
		border_draw_y_mark(br, 0, 0, dest_pix, False);
		break;
	case DRAW_BORDER_SW:
		border_draw_x_mark(br, 0, 0, dest_pix, False);
		border_draw_y_mark(br, l, t + o, dest_pix, True);
		break;
	case DRAW_BORDER_SE:
		border_draw_x_mark(br, l + o, 0, dest_pix, False);
		border_draw_y_mark(br, 0, t + o, dest_pix, False);
		break;
	default:
		return;
	}

	return;
}

inline static void border_set_part_background(
	Window w, Pixmap pix)
{
	XSetWindowAttributes xswa;

	xswa.background_pixmap = pix;
	XChangeWindowAttributes(dpy, w, CWBackPixmap, &xswa);

	return;
}

static void border_draw_one_part(
	common_decorations_type *cd, FvwmWindow *fw, rectangle *sidebar_g,
	rectangle *frame_g, border_relief_descr *br, draw_window_parts part,
	draw_window_parts draw_handles, Bool is_inverted, Bool do_hilight)
{
	rectangle part_g;
	Pixmap p;
	Window w;

	/* make a pixmap */
	border_get_part_geometry(fw, part, sidebar_g, &part_g, &w);
	p = border_create_part_pixmap(cd, &part_g);
	/* set the background tile */
	border_set_part_pixmap_back(cd, br->gcs.tile, &part_g, p);
	/* draw the relief over the background */
	if (!br->relief.is_flat)
	{
		border_draw_part_relief(br, frame_g, &part_g, p, is_inverted);
	}
	/* draw the handle marks */
	if (br->marks.has_x_marks || br->marks.has_y_marks)
	{
		border_draw_part_marks(br, &part_g, part, p);
	}
	/* apply the pixmap and destroy it */
	border_set_part_background(w, p);
	XFreePixmap(dpy, p);
	XClearWindow(dpy, w); XSync(dpy, 0); /*!!!*/

	return;
}

static void border_draw_all_parts(
        common_decorations_type *cd, FvwmWindow *fw, border_relief_descr *br,
	rectangle *frame_g, draw_window_parts draw_parts,
	draw_window_parts pressed_parts, Bool do_hilight)
{
	draw_window_parts part;
	draw_window_parts draw_handles;

	/* get the description of the drawing directives */
	border_get_border_relief_size_descr(&br->relief, fw, do_hilight);
	border_get_border_marks_descr(cd, br, fw);
	/* fetch the gcs used to draw the border */
	border_get_border_gcs(&br->gcs, cd, fw, do_hilight);
	/* draw everything in a big loop */
	draw_handles = (draw_parts & DRAW_HANDLES);
fprintf(stderr, "drawing border parts 0x%04x\n", draw_parts);
	for (part = DRAW_BORDER_N; (part & DRAW_FRAME); part <<= 1)
	{
		if (part & draw_parts)
		{
			border_draw_one_part(
				cd, fw, &br->sidebar_g, frame_g, br, part,
				draw_handles,
				(pressed_parts & part) ? True : False,
				do_hilight);
		}
	}
	/* update the border states */
	fw->border_state.parts_drawn |= draw_parts;
	if (do_hilight)
	{
		fw->border_state.parts_lit |= draw_parts;
	}
	fw->border_state.parts_inverted &= ~draw_parts;
	fw->border_state.parts_inverted |= (draw_parts & pressed_parts);

	return;
}

/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
static void border_draw_border_parts(
	common_decorations_type *cd, FvwmWindow *fw,
	draw_window_parts pressed_parts, draw_window_parts force_draw_parts,
	rectangle *old_g, rectangle *new_g, Bool do_hilight)
{
	border_relief_descr br;
	draw_window_parts draw_parts;

	if (!HAS_BORDER(fw))
	{
		/* just reset border states */
		fw->border_state.parts_drawn = 0;
		fw->border_state.parts_lit = 0;
		fw->border_state.parts_inverted = 0;
		return;
	}
	/* determine the parts to draw and the position to place them */
	if (HAS_DEPRESSABLE_BORDER(fw))
	{
		pressed_parts &= DRAW_FRAME;
	}
	else
	{
		pressed_parts = DRAW_NONE;
	}
	force_draw_parts &= DRAW_FRAME;
	memset(&br, 0, sizeof(br));
	draw_parts = border_get_parts_and_pos_to_draw(
		cd, fw, pressed_parts, force_draw_parts, old_g, new_g,
		do_hilight, &br);
	if ((draw_parts & DRAW_FRAME) != DRAW_NONE)
	{
		border_draw_all_parts(
			cd, fw, &br, new_g, draw_parts, pressed_parts,
			do_hilight);
	}

	return;
}

/* ---------------------------- interface functions ------------------------- */

void border_get_part_geometry(
	FvwmWindow *fw, draw_window_parts part, rectangle *sidebar_g,
	rectangle *ret_g, Window *ret_w)
{
	int bw;

	bw = fw->boundary_width;
	switch (part)
	{
	case DRAW_BORDER_N:
		ret_g->x = sidebar_g->x;
		ret_g->y = 0;
		*ret_w = FW_W_SIDE(fw, 0);
		break;
	case DRAW_BORDER_E:
		ret_g->x = 2 * sidebar_g->x + sidebar_g->width - bw;
		ret_g->y = sidebar_g->y;
		*ret_w = FW_W_SIDE(fw, 1);
		break;
	case DRAW_BORDER_S:
		ret_g->x = sidebar_g->x;
		ret_g->y = 2 * sidebar_g->y + sidebar_g->height - bw;
		*ret_w = FW_W_SIDE(fw, 2);
		break;
	case DRAW_BORDER_W:
		ret_g->x = 0;
		ret_g->y = sidebar_g->y;
		*ret_w = FW_W_SIDE(fw, 3);
		break;
	case DRAW_BORDER_NW:
		ret_g->x = 0;
		ret_g->y = 0;
		*ret_w = FW_W_CORNER(fw, 0);
		break;
	case DRAW_BORDER_NE:
		ret_g->x = sidebar_g->x + sidebar_g->width;
		ret_g->y = 0;
		*ret_w = FW_W_CORNER(fw, 1);
		break;
	case DRAW_BORDER_SW:
		ret_g->x = 0;
		ret_g->y = sidebar_g->y + sidebar_g->height;
		*ret_w = FW_W_CORNER(fw, 2);
		break;
	case DRAW_BORDER_SE:
		ret_g->x = sidebar_g->x + sidebar_g->width;;
		ret_g->y = sidebar_g->y + sidebar_g->height;
		*ret_w = FW_W_CORNER(fw, 3);
		break;
	default:
		return;
	}
	switch (part)
	{
	case DRAW_BORDER_N:
	case DRAW_BORDER_S:
		ret_g->width = sidebar_g->width;
		ret_g->height = bw;
		break;
	case DRAW_BORDER_E:
	case DRAW_BORDER_W:
		ret_g->width = bw;
		ret_g->height = sidebar_g->height;
		break;
	case DRAW_BORDER_NW:
	case DRAW_BORDER_NE:
	case DRAW_BORDER_SW:
	case DRAW_BORDER_SE:
		ret_g->width = sidebar_g->x;
		ret_g->height = sidebar_g->y;
		break;
	default:
		return;
	}

	return;
}

int get_button_number(int context)
{
	int i;

	for (i = 0; (C_L1 << i) & (C_LALL | C_RALL); i++)
	{
		if (context & (C_L1 << i))
		{
			return i;
		}
	}

	return -1;
}

void draw_clipped_decorations_with_geom(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, XRectangle *rclip, clear_window_parts clear_parts,
	rectangle *old_g, rectangle *new_g)
{
	common_decorations_type cd;
	Bool is_button_redraw_allowed = False;
	Bool is_title_redraw_allowed = False;
	Bool do_redraw_title = False;
	Bool do_redraw_buttons = False;
	Bool do_change_gcs = False;

	if (!t)
	{
		return;
	}
	memset(&cd, 0, sizeof(cd));
	if (force || expose_win == None)
	{
		is_button_redraw_allowed = True;
		is_title_redraw_allowed = True;
	}
	else if (expose_win == FW_W_TITLE(t))
	{
		is_title_redraw_allowed = True;
	}
	else if (expose_win != None)
	{
		is_button_redraw_allowed = True;
	}

	if (has_focus)
	{
		/* don't re-draw just for kicks */
		if(!force && Scr.Hilite == t)
		{
			is_button_redraw_allowed = False;
			is_title_redraw_allowed = False;
		}
		else
		{
			if (Scr.Hilite != t || force > 1)
			{
				cd.flags.has_color_changed = True;
			}
			if (Scr.Hilite != t && Scr.Hilite != NULL)
			{
				/* make sure that the previously highlighted
				 * window got unhighlighted */
				DrawDecorations(
					Scr.Hilite, DRAW_ALL, False, True, None,
					CLEAR_ALL);
			}
			Scr.Hilite = t;
		}
	}
	else
	{
		/* don't re-draw just for kicks */
		if (!force && Scr.Hilite != t)
		{
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
	if ((draw_parts & DRAW_TITLE) && HAS_TITLE(t) &&
	    is_title_redraw_allowed)
	{
		do_redraw_title = True;
		do_change_gcs = True;
	}
	if ((draw_parts & DRAW_BUTTONS) && HAS_TITLE(t) &&
	    is_button_redraw_allowed)
	{
		do_redraw_buttons = True;
		do_change_gcs = True;
	}
	get_common_decorations(
		&cd, t, draw_parts, has_focus, force, False, do_change_gcs);

	/* redraw */
	if (cd.flags.has_color_changed && HAS_TITLE(t))
	{
		if (!do_redraw_title || !rclip)
		{
			if (t->title_background_pixmap == None || force)
			{
				change_window_background(
					FW_W_TITLE(t), cd.notex_valuemask,
					&cd.notex_attributes);
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
	if (cd.flags.has_color_changed || (draw_parts & DRAW_FRAME))
	{
		draw_window_parts pressed_parts;
		draw_window_parts force_parts;
		int context;
		int i;

		get_common_decorations(
			&cd, t, draw_parts, has_focus, force, True, True);
		if ((clear_parts & CLEAR_FRAME) && !IS_WINDOW_BORDER_DRAWN(t))
		{
			/*!!!change_window_background(
			  t->decor_w, cd.valuemask, &cd.attributes);*/
			SET_WINDOW_BORDER_DRAWN(t, 1);
			rclip = NULL;
		}
		/*!!!*/
		context = frame_window_id_to_context(t, PressedW, &i);
		pressed_parts = border_context_to_parts(context);
		force_parts = (force) ? (draw_parts & DRAW_FRAME) : DRAW_NONE;



		border_draw_border_parts(
			&cd, t, pressed_parts, force_parts, old_g, new_g,
			has_focus);
	}

	/* Sync to make the change look fast! */
	XSync(dpy,0);

	return;
}

void draw_clipped_decorations(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, XRectangle *rclip, clear_window_parts clear_parts)
{
	draw_clipped_decorations_with_geom(
		t, draw_parts, has_focus, force, expose_win, rclip, clear_parts,
		NULL, &t->frame_g);

	return;
}

void DrawDecorations(
	FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
	Window expose_win, clear_window_parts clear_parts)
{
	draw_clipped_decorations(
		t, draw_parts, has_focus, force, expose_win, NULL, clear_parts);

	return;
}

/****************************************************************************
 *
 *  redraw the decoration when style change
 *
 ****************************************************************************/
void RedrawDecorations(FvwmWindow *fw)
{
	FvwmWindow *u = Scr.Hilite;

#if 0
	/*!!!*/
	if (IS_SHADED(fw))
	{
		XRaiseWindow(dpy, fw->decor_w);
	}
#endif
	/* domivogt (6-Jun-2000): Don't check if the window is visible here.
	 * If we do, some updates are not applied and when the window becomes
	 * visible again, the X Server may not redraw the window. */
	DrawDecorations(fw, DRAW_ALL, (Scr.Hilite == fw), 2, None, CLEAR_ALL);
	Scr.Hilite = u;

	return;
}

#if 0
void RedrawBorder(
	common_decorations_type *cd, FvwmWindow *t, Bool has_focus,
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
		tgc = fvwmlib_XCreateGC(
			dpy, FW_W_FRAME(t), GCFunction | GCPlaneMask, &xgcv);
	}
	/*
	 * draw the border; resize handles are InputOnly so draw in the decor_w
	 * and it will show through. The entire border is redrawn so flush all
	 * exposes up to this point.
	 */
#if 0
	flush_expose(t->decor_w);
#endif

	if (is_window_border_minimal(t))
	{
		/* just set the background pixmap */
		XSetWindowBackgroundPixmap(
			dpy, t->decor_w, cd->texture_pixmap);
		XClearWindow(dpy, t->decor_w);
		XSetWindowBackgroundPixmap(
		dpy, t->decor_w, cd->back_pixmap);
		XClearWindow(dpy, t->decor_w);

		return;
	}

	/*
	 * for color, make it the color of the decoration background
	 */

	/* get the border style bits */
	borderstyle = (do_hilight) ?
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
	 * remove any lines drawn in the border if hidden handles or noinset
	 * and if a handle was pressed
	 */

	if (PressedW == None &&
	    (DFS_HAS_NO_INSET(*borderstyle) ||
	     DFS_HAS_HIDDEN_HANDLES(*borderstyle)))
	{
		ClipClear(t->decor_w, NULL, False);
	}

	/*
	 * Nothing to do if flat
	 */

	if (borderstyle->flags.button_relief == DFS_BUTTON_IS_FLAT)
	{
		return;
	}

	/*
	 * draw the inside relief
	 */

	if (HAS_MWM_BORDER(t))
	{
		/* MWM borders look like this:
		 *
		 * HHCCCCS  from outside to inside on the left and top border
		 * SSCCCCH  from outside to inside on the bottom and right
		 *          border
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
		 * SSCCHHS  from outside to inside on the bottom and right
		 *          border
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
		/* w_trout + w_trin counts only as one pixel of border because
		 * they let one pixel of the original colour shine through. */
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
	/* If the border is too thin to accomodate the standard look, we remove
	 * parts of the border so that at least one pixel of the original
	 * colour is visible. We make an exception for windows with a border
	 * width of 2, though. */
	if ((!IS_SHADED(t) || HAS_TITLE(t)) && t->boundary_width == 2)
	{
		trim--;
	}
	if (trim < 0)
	{
		trim = 0;
	}
	for ( ; trim > 0; trim--)
	{
		if (w_hiout > 1)
		{
			w_hiout--;
		}
		else if (w_shin > 0)
		{
			w_shin--;
		}
		else if (w_hiout > 0)
		{
			w_hiout--;
		}
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
		t, rgc, sgc, tgc, sgc,
		w_dout, w_hiout, w_trout, w_c, w_trin, w_shin, w_din);

	/*
	 * draw the handle marks
	 */

	/* draw the handles as eight marks around the border */
	if (HAS_BORDER(t) && !DFS_HAS_HIDDEN_HANDLES(*borderstyle))
	{
		/* MWM border windows have thin 3d effects
		 * FvwmBorders have 3 pixels top/left, 2 bot/right so this makes
		 * calculating the length of the marks difficult, top and bottom
		 * marks for FvwmBorders are different if NoInset is specified
		 */
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
			{
				break;
			}
			/* hilite marks */
			i = 0;
			/* top left */
			marks[i].x1 = t->visual_corner_width + loff;
			marks[i].x2 = t->visual_corner_width + loff;
			marks[i].y1 = w_dout;
			marks[i].y2 = marks[i].y1 + length;
			i++;
			/* top right */
			marks[i].x1 = t->frame_g.width -
				t->visual_corner_width + loff;
			marks[i].x2 = t->frame_g.width -
				t->visual_corner_width + loff;
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
			marks[i].x1 = t->frame_g.width -
				t->visual_corner_width + loff;
			marks[i].x2 = t->frame_g.width -
				t->visual_corner_width + loff;
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
				marks[i].y1 = t->frame_g.height -
					t->visual_corner_width + loff;
				marks[i].y2 = t->frame_g.height -
					t->visual_corner_width + loff;
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
				marks[i].y1 = t->frame_g.height -
					t->visual_corner_width + loff;
				marks[i].y2 = t->frame_g.height -
					t->visual_corner_width + loff;
				i++;
			}
			XDrawSegments(dpy, t->decor_w, rgc, marks, i);

			/* shadow marks, reuse the array (XDrawSegments doesn't
			 * trash it) */
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

	/* a bit hacky to draw twice but you should see the code it replaces,
	 * never mind the esoterics, feel the thin-ness */
	if ((HAS_BORDER(t) || PressedW == t->decor_w ||
	     PressedW == FW_W_FRAME(t)) && HAS_DEPRESSABLE_BORDER(t))
	{
		XRectangle r;
		Bool is_pressed = False;

		if (PressedW == FW_W_SIDE(t, 0))
		{
			/* top */
			r.x = t->visual_corner_width;
			r.y = 1;
			r.width = t->frame_g.width - 2 * t->visual_corner_width;
			r.height = t->boundary_width - 1;
			is_pressed = True;
		}
		else if (PressedW == FW_W_SIDE(t, 1))
		{
			/* right */
			if (!IS_SHADED(t))
			{
				r.x = t->frame_g.width - t->boundary_width;
				r.y = t->visual_corner_width;
				r.width = t->boundary_width - 1;
				r.height = t->frame_g.height -
					2 * t->visual_corner_width;
				is_pressed = True;
			}
		}
		else if (PressedW == FW_W_SIDE(t, 2))
		{
			/* bottom */
			r.x = t->visual_corner_width;
			r.y = t->frame_g.height - t->boundary_width;
			r.width = t->frame_g.width - 2 * t->visual_corner_width;
			r.height = t->boundary_width - 1;
			is_pressed = True;
		}
		else if (PressedW == FW_W_SIDE(t, 3))
		{
			/* left */
			if (!IS_SHADED(t))
			{
				r.x = 1;
				r.y = t->visual_corner_width;
				r.width = t->boundary_width - 1;
				r.height = t->frame_g.height -
					2 * t->visual_corner_width;
				is_pressed = True;
			}
		}
		else if (PressedW == FW_W_CORNER(t, 0))
		{
			/* top left */
			r.x = 1;
			r.y = 1;
			r.width = t->visual_corner_width - 1;
			r.height = t->visual_corner_width - 1;
			is_pressed = True;
		}
		else if (PressedW == FW_W_CORNER(t, 1))
		{
			/* top right */
			r.x = t->frame_g.width - t->visual_corner_width;
			r.y = 1;
			r.width = t->visual_corner_width - 1;
			r.height = t->visual_corner_width - 1;
			is_pressed = True;
		}
		else if (PressedW == FW_W_CORNER(t, 2))
		{
			/* bottom left */
			r.x = 1;
			r.y = t->frame_g.height - t->visual_corner_width;
			r.width = t->visual_corner_width - 1;
			r.height = t->visual_corner_width - 1;
			is_pressed = True;
		}
		else if (PressedW == FW_W_CORNER(t, 3))
		{
			/* bottom right */
			r.x = t->frame_g.width - t->visual_corner_width;
			r.y = t->frame_g.height - t->visual_corner_width;
			r.width = t->visual_corner_width - 1;
			r.height = t->visual_corner_width - 1;
			is_pressed = True;
		}
		else if (PressedW == t->decor_w || PressedW == FW_W_FRAME(t))
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
			XSetClipRectangles(dpy, rgc, 0, 0, &r, 1, Unsorted);
			XSetClipRectangles(dpy, sgc, 0, 0, &r, 1, Unsorted);
			draw_frame_relief(
				t, sgc, rgc, tgc, sgc,
				w_dout, w_hiout, w_trout, w_c, w_trin,
				w_shin, w_din);
			is_clipped = True;
		}
	}
	if (is_clipped)
	{
		XSetClipMask(dpy, rgc, None);
		XSetClipMask(dpy, sgc, None);
	}

	return;
}
#endif

/* ---------------------------- builtin commands ---------------------------- */

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
			Scr.gs.use_active_down_buttons =
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
			Scr.gs.use_inactive_buttons =
				DEFAULT_USE_INACTIVE_BUTTONS;
			return;
		}
		first = False;
		if (StrEquals("activedown", token))
		{
			Scr.gs.use_active_down_buttons = ParseToggleArgument(
				action, &action,
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS, True);
		}
		else if (StrEquals("inactive", token))
		{
			Scr.gs.use_inactive_buttons = ParseToggleArgument(
				action, &action,
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS, True);
		}
		else
		{
			Scr.gs.use_active_down_buttons =
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
			Scr.gs.use_inactive_buttons =
				DEFAULT_USE_INACTIVE_BUTTONS;
			fvwm_msg(ERR, "cmd_button_state",
				 "unknown button state %s\n", token);
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
			{
				df = &decor->BorderStyle.active;
			}
			else
			{
				df = &decor->BorderStyle.inactive;
			}
			df->flags.has_changed = 1;
			while (isspace(*action))
				++action;
			if (*action != '(')
			{
				if (!*action)
				{
					fvwm_msg(
						ERR, "SetBorderStyle",
						"error in %s border"
						" specification", parm);
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
				fvwm_msg(
					ERR, "SetBorderStyle",
					"error in %s border specification",
					parm);
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
			if (ReadDecorFace(
				    prev, &decor->BorderStyle.active,-1,True))
			{
				ReadDecorFace(
					prev, &decor->BorderStyle.inactive, -1,
					False);
			}
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
				ReadDecorFace(
					prev, &decor->BorderStyle.inactive, -1,
					False);
				decor->BorderStyle.active.flags.has_changed = 1;
				decor->BorderStyle.inactive.flags.has_changed =
					1;
			}
			break;
		}
	}

	return;
}
