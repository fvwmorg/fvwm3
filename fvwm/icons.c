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
 * This module is mostly all new
 * by Rob Nation
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm icon code
 *
 ***********************************************************************/

#include "config.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <X11/Intrinsic.h>
#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include <stdio.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "events.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "icons.h"
#include "borders.h"
#include "focus.h"
#include "colormaps.h"
#include "stack.h"
#include "virtual.h"
#include "decorations.h"
#include "module_interface.h"
#include "libs/Colorset.h"
#include "gnome.h"
#include "ewmh.h"
#include "geometry.h"

static int do_all_iconboxes(FvwmWindow *t, icon_boxes **icon_boxes_ptr);
static void GetBitmapFile(FvwmWindow *tmp_win);
#ifdef XPM
static void GetXPMFile(FvwmWindow *tmp_win);
#endif

/****************************************************************************
 *
 * Get the Icon for the icon window (also used by ewmh_icon)
 *
 ****************************************************************************/
void GetIcon(FvwmWindow *tmp_win, Bool no_icon_window)
{
  char icon_order[5];
  int i;

  /* First, see if it was specified in the .fvwmrc */
  if (ICON_OVERRIDE_MODE(tmp_win) == ICON_OVERRIDE)
  {
    /* try fvwm provided icons before application provided icons */
    icon_order[0] = 0;
    icon_order[1] = 1;
    icon_order[2] = 2;
    icon_order[3] = 3;
    icon_order[4] = 4;
ICON_DBG((stderr,"ciw: hint order: xpm bmp iwh iph '%s'\n", tmp_win->name));
  }
  else if (ICON_OVERRIDE_MODE(tmp_win) == NO_ACTIVE_ICON_OVERRIDE)
  {
    if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconPixmapHint) &&
	WAS_ICON_HINT_PROVIDED(tmp_win) == ICON_HINT_MULTIPLE)
    {
      /* use application provided icon window or pixmap first, then fvwm
       * provided icons. */
      icon_order[0] = 2;
      icon_order[1] = 3;
      icon_order[2] = 4;
      icon_order[3] = 0;
      icon_order[4] = 1;
ICON_DBG((stderr,"ciw: hint order: iwh iph xpm bmp '%s'\n", tmp_win->name));
    }
    else if (Scr.DefaultIcon && tmp_win->icon_bitmap_file == Scr.DefaultIcon)
    {
      /* use application provided icon window/pixmap first, then fvwm provided
       * default icon */
      icon_order[0] = 2;
      icon_order[1] = 3;
      icon_order[2] = 4;
      icon_order[3] = 0;
      icon_order[4] = 1;
ICON_DBG((stderr,"ciw: hint order: iwh iph xpm bmp '%s'\n", tmp_win->name));
    }
    else
    {
      /* use application provided icon window or ewmh icon first, then fvwm
       * provided icons and then application provided icon pixmap */
      icon_order[0] = 2;
      icon_order[1] = 3;
      icon_order[2] = 0;
      icon_order[3] = 1;
      icon_order[4] = 4;
ICON_DBG((stderr,"ciw: hint order: iwh xpm bmp iph '%s'\n", tmp_win->name));
    }
  }
  else
  {
    /* use application provided icon rather than fvwm provided icon */
    icon_order[0] = 2;
    icon_order[1] = 3;
    icon_order[2] = 4;
    icon_order[3] = 0;
    icon_order[4] = 1;
ICON_DBG((stderr,"ciw: hint order: iwh iph bmp xpm '%s'\n", tmp_win->name));
  }

  tmp_win->icon_p_height = 0;
  tmp_win->icon_p_width = 0;
  for (i = 0; i < 5 && !tmp_win->icon_p_height && !tmp_win->icon_p_width; i++)
  {
    switch (icon_order[i])
    {
    case 0:
#ifdef XPM
      /* Next, check for a color pixmap */
      if (tmp_win->icon_bitmap_file)
      {
	GetXPMFile(tmp_win);
      }
ICON_DBG((stderr,"ciw: xpm%s used '%s'\n", (tmp_win->icon_p_height)?"":" not",tmp_win->name));
#endif /* XPM */
      break;
    case 1:
      /* First, check for a monochrome bitmap */
      if (tmp_win->icon_bitmap_file)
      {
	GetBitmapFile(tmp_win);
      }
ICON_DBG((stderr,"ciw: bmp%s used '%s'\n", (tmp_win->icon_p_height)?"":" not",tmp_win->name));
      break;
    case 2:
      /* Next, See if the app supplies its own icon window */
      if (no_icon_window)
	break;
      if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconWindowHint))
      {
	GetIconWindow(tmp_win);
      }
ICON_DBG((stderr,"ciw: iwh%s used '%s'\n", (tmp_win->icon_p_height)?"":" not",tmp_win->name));
      break;
    case 3:
      /* try an ewmh icon */
      if (HAS_EWMH_WM_ICON_HINT(tmp_win) == EWMH_TRUE_ICON)
      {
	if (EWMH_SetIconFromWMIcon(tmp_win, NULL, 0, False))
	  SET_USE_EWMH_ICON(tmp_win, True);
      }
ICON_DBG((stderr,"ciw: inh%s used '%s'\n", (tmp_win->icon_p_height)?"":" not",tmp_win->name));
      break;
    case 4:
      /* Finally, try to get icon bitmap from the application */
      if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconPixmapHint))
      {
	GetIconBitmap(tmp_win);
      }
ICON_DBG((stderr,"ciw: iph%s used '%s'\n", (tmp_win->icon_p_height)?"":" not",tmp_win->name));
      break;
    default:
      /* can't happen */
      break;
    }
  }
}

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
void CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y)
{
  /* mask for create windows */
  unsigned long valuemask;
  /* attributes for create windows */
  XSetWindowAttributes attributes;
  XWindowChanges xwc;
  Window old_icon_pixmap_w;
  Window old_icon_w;
  Bool is_old_icon_shaped = IS_ICON_SHAPED(tmp_win);

  old_icon_w = tmp_win->icon_title_w;
  old_icon_pixmap_w = (IS_ICON_OURS(tmp_win)) ? tmp_win->icon_pixmap_w : None;
  if (!IS_ICON_OURS(tmp_win) && tmp_win->icon_pixmap_w)
    XUnmapWindow(dpy,tmp_win->icon_pixmap_w);
  SET_ICON_OURS(tmp_win, 1);
  SET_PIXMAP_OURS(tmp_win, 0);
  SET_ICON_SHAPED(tmp_win, 0);
  tmp_win->icon_pixmap_w = None;
  tmp_win->iconPixmap = None;
  tmp_win->iconDepth = 0;

  if (IS_ICON_SUPPRESSED(tmp_win))
    return;

  GetIcon(tmp_win, False);

  /* figure out the icon window size */
  if (!HAS_NO_ICON_TITLE(tmp_win))
  {
    tmp_win->icon_t_width =
      XTextWidth(tmp_win->icon_font.font, tmp_win->visible_icon_name,
		 strlen(tmp_win->visible_icon_name));
    tmp_win->icon_g.height = ICON_HEIGHT(tmp_win);
  }
  else
  {
    tmp_win->icon_t_width = 0;
    tmp_win->icon_g.height = 0;
  }

  /* make space for relief to be drawn outside the icon */
  /* this does not happen if fvwm is using a non-default visual (with
     private colormap) and the client has supplied a pixmap (not a bitmap) */
  if ((IS_ICON_OURS(tmp_win)) && (tmp_win->icon_p_height > 0)
      && (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win)))
  {
    tmp_win->icon_p_width += 2 * ICON_RELIEF_WIDTH;
    tmp_win->icon_p_height += 2 * ICON_RELIEF_WIDTH;
  }

  if (tmp_win->icon_p_width == 0)
  {
    tmp_win->icon_p_width = tmp_win->icon_t_width +
	    2 * (ICON_TITLE_TEXT_GAP_COLLAPSED + ICON_RELIEF_WIDTH);
    if (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win))
    {
      tmp_win->icon_p_width +=
	2 * (ICON_TITLE_TO_STICK_EXTRA_GAP + ICON_TITLE_STICK_MIN_WIDTH);
    }
  }
  tmp_win->icon_g.width = tmp_win->icon_p_width;

  tmp_win->icon_g.x = tmp_win->icon_xl_loc = def_x;
  tmp_win->icon_g.y = def_y;

  /* create the icon title window */
  valuemask = CWColormap | CWBorderPixel
    | CWBackPixel | CWCursor | CWEventMask;
  attributes.colormap = Pcmap;
  attributes.background_pixel = Scr.StdBack;
  attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
  attributes.border_pixel = 0;
  attributes.event_mask = XEVMASK_ICONW;
  if (HAS_NO_ICON_TITLE(tmp_win))
  {
    if (tmp_win->icon_title_w)
    {
      XDestroyWindow(dpy, tmp_win->icon_title_w);
      XDeleteContext(dpy, tmp_win->icon_title_w, FvwmContext);
    }
  }
  else
  {
    if (!tmp_win->icon_title_w)
    {
      tmp_win->icon_title_w =
	XCreateWindow(
	  dpy, Scr.Root, def_x, def_y + tmp_win->icon_p_height,
	  tmp_win->icon_g.width, tmp_win->icon_g.height, 0, Pdepth,
	  InputOutput, Pvisual, valuemask, &attributes);
    }
    else
    {
      XMoveResizeWindow(
	dpy, tmp_win->icon_title_w, def_x, def_y + tmp_win->icon_p_height,
	  tmp_win->icon_g.width, tmp_win->icon_g.height);
    }
  }
  if (Scr.DefaultColorset >= 0)
    SetWindowBackground(dpy, tmp_win->icon_title_w, tmp_win->icon_g.width,
			tmp_win->icon_g.height, &Colorset[Scr.DefaultColorset],
			Pdepth, Scr.StdGC, False);

  /* create a window to hold the picture */
  if (IS_ICON_OURS(tmp_win) && tmp_win->icon_p_width > 0 &&
      tmp_win->icon_p_height > 0)
  {
    /* use fvwm's visuals in these cases */
    if (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win))
    {
      if (!old_icon_pixmap_w)
      {
	tmp_win->icon_pixmap_w =
	  XCreateWindow(
	    dpy, Scr.Root, def_x, def_y, tmp_win->icon_p_width,
	    tmp_win->icon_p_height, 0, Pdepth, InputOutput, Pvisual, valuemask,
	    &attributes);
      }
      else
      {
	tmp_win->icon_pixmap_w = old_icon_pixmap_w;
	XMoveResizeWindow(
	  dpy, tmp_win->icon_pixmap_w, def_x, def_y, tmp_win->icon_p_width,
	  tmp_win->icon_p_height);
      }
      if (Scr.DefaultColorset >= 0)
	SetWindowBackground(dpy, tmp_win->icon_title_w, tmp_win->icon_p_width,
			    tmp_win->icon_p_height,
			    &Colorset[Scr.DefaultColorset], Pdepth, Scr.StdGC,
			    False);
    }
    else
    {
      /* client supplied icon pixmap and fvwm is using another visual */
      /* use it as the background pixmap, don't try to put relief on it
       * because fvwm will not have the correct colors
       * the Exceed server has problems maintaining the icon window, it usually
       * fails to refresh the icon leaving it black so ask for expose events */
      attributes.background_pixmap = tmp_win->iconPixmap;
      attributes.colormap = DefaultColormap(dpy, Scr.screen);
      valuemask &= ~CWBackPixel;
      valuemask |= CWBackPixmap;
      tmp_win->icon_pixmap_w =
	XCreateWindow(
	  dpy, Scr.Root, def_x, def_y, tmp_win->icon_p_width,
	  tmp_win->icon_p_height, 0, DefaultDepth(dpy, Scr.screen), InputOutput,
	  DefaultVisual(dpy, Scr.screen), valuemask, &attributes);
    }
  }
  else
  {
    /* client supplied icon window: select events on it */
    attributes.event_mask = XEVMASK_ICONPW;
    valuemask = CWEventMask;
    XChangeWindowAttributes(dpy, tmp_win->icon_pixmap_w, valuemask,&attributes);
  }
  if (old_icon_pixmap_w && old_icon_pixmap_w != tmp_win->icon_pixmap_w)
  {
    /* destroy the old window */
    XDestroyWindow(dpy, old_icon_pixmap_w);
    XDeleteContext(dpy, old_icon_pixmap_w, FvwmContext);
    is_old_icon_shaped = False;
  }

  if (FShapesSupported)
  {
    if (IS_ICON_SHAPED(tmp_win))
    {
      /* when fvwm is using the non-default visual client supplied icon pixmaps
       * are drawn in a window with no relief */
      int off;
      off = (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win)) ?
	ICON_RELIEF_WIDTH : 0;
      FShapeCombineMask(
	dpy, tmp_win->icon_pixmap_w, FShapeBounding, off, off,
	tmp_win->icon_maskPixmap, FShapeSet);
    }
    else if (is_old_icon_shaped && tmp_win->icon_pixmap_w == old_icon_pixmap_w)
    {
      /* remove the shape */
      XRectangle r;

      r.x = 0;
      r.y = 0;
      r.width = tmp_win->icon_p_width;
      r.height = tmp_win->icon_p_height;
      FShapeCombineRectangles(
	dpy, tmp_win->icon_pixmap_w, FShapeBounding, 0, 0, &r, 1, FShapeSet, 0);
    }
  }

  if (tmp_win->icon_title_w != None && tmp_win->icon_title_w != old_icon_w)
  {
    XSaveContext(dpy, tmp_win->icon_title_w, FvwmContext, (caddr_t)tmp_win);
    XDefineCursor(dpy, tmp_win->icon_title_w, Scr.FvwmCursors[CRS_DEFAULT]);
    GrabAllWindowKeysAndButtons(dpy, tmp_win->icon_title_w, Scr.AllBindings,
				C_ICON, GetUnusedModifiers(),
				Scr.FvwmCursors[CRS_DEFAULT], True);
    xwc.sibling = tmp_win->frame;
    xwc.stack_mode = Below;
    XConfigureWindow(dpy, tmp_win->icon_title_w, CWSibling|CWStackMode, &xwc);
  }
  if (tmp_win->icon_pixmap_w != None &&
      tmp_win->icon_pixmap_w != old_icon_pixmap_w)
  {
    XSaveContext(dpy, tmp_win->icon_pixmap_w, FvwmContext, (caddr_t)tmp_win);
    XDefineCursor(dpy, tmp_win->icon_pixmap_w, Scr.FvwmCursors[CRS_DEFAULT]);
    GrabAllWindowKeysAndButtons(dpy, tmp_win->icon_pixmap_w, Scr.AllBindings,
				C_ICON, GetUnusedModifiers(),
				Scr.FvwmCursors[CRS_DEFAULT], True);
    xwc.sibling = tmp_win->frame;
    xwc.stack_mode = Below;
    XConfigureWindow(dpy,tmp_win->icon_pixmap_w,CWSibling|CWStackMode,&xwc);
  }
  return;
}

/****************************************************************************
 *
 * Draws the icon window
 *
 ****************************************************************************/
void DrawIconWindow(FvwmWindow *tmp_win)
{
  GC Shadow;
  GC Relief;
  Pixel TextColor;
  Pixel BackColor;
  color_quad *draw_colors;
  int is_expanded = IS_ICON_ENTERED(tmp_win);

  if (IS_ICON_SUPPRESSED(tmp_win))
    return;

  if (tmp_win->icon_title_w != None)
    flush_expose(tmp_win->icon_title_w);
  if (tmp_win->icon_pixmap_w != None)
    flush_expose(tmp_win->icon_pixmap_w);

  if (Scr.Hilite == tmp_win)
    draw_colors = &(tmp_win->hicolors);
  else
    draw_colors = &(tmp_win->colors);
  if (Pdepth < 2 && Scr.Hilite != tmp_win)
  {
    Relief = Scr.StdReliefGC;
    Shadow = Scr.StdShadowGC;
  }
  else
  {
    if (Pdepth < 2 && Scr.Hilite == tmp_win)
      Relief = Scr.ScratchGC2;
    else
    {
      Globalgcv.foreground = draw_colors->hilight;
      Globalgcm = GCForeground;
      XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
      Relief = Scr.ScratchGC1;
    }
    Globalgcv.foreground = draw_colors->shadow;
    XChangeGC(dpy,Scr.ScratchGC2, Globalgcm, &Globalgcv);
    Shadow = Scr.ScratchGC2;
  }
  TextColor = draw_colors->fore;
  BackColor = draw_colors->back;

  if (tmp_win->icon_title_w != None)
  {
    int x_title;
    int x_title_min = 0;
    int w_title = tmp_win->icon_t_width;
    int x_title_w = tmp_win->icon_xl_loc;
    int w_title_w = tmp_win->icon_g.width;
    int x_stipple = ICON_RELIEF_WIDTH;
    int w_title_text_gap = 0;
    int w_stipple = 0;
    int is_sticky = (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win));
    int use_unexpanded_size = 1;

    if (is_expanded && tmp_win->icon_pixmap_w != None)
    {
      int sx;
      int sy;
      int sw;
      int sh;

      use_unexpanded_size = 0;
      w_title = tmp_win->icon_t_width;
      w_title_text_gap = ICON_TITLE_TEXT_GAP_EXPANDED;
      x_title_min = w_title_text_gap + ICON_RELIEF_WIDTH;
      if (is_sticky)
      {
	w_stipple = ICON_TITLE_STICK_MIN_WIDTH;
	x_title_min += w_stipple + ICON_TITLE_TO_STICK_EXTRA_GAP;
      }
      /* resize the icon name window */
      w_title_w = w_title + 2 * x_title_min;
      if (w_title_w <= tmp_win->icon_p_width)
      {
	/* the expanded title is smaller, so do not expand at all */
	is_expanded = 0;
	w_stipple = 0;
	use_unexpanded_size = 1;
      }
      else
      {
	x_title_w = tmp_win->icon_g.x - (w_title_w - tmp_win->icon_p_width)/2;
	FScreenGetScrRect(NULL, FSCREEN_CURRENT, &sx, &sy, &sw, &sh);
	/* start keep label on screen. dje 8/7/97 */
	if (x_title_w < sx) {  /* if new loc neg (off left edge) */
	  x_title_w = sx;      /* move to edge */
	} else {                         /* if not on left edge */
	  /* if (new loc + width) > screen width (off edge on right) */
	  if ((x_title_w + w_title_w) >
	      sx + sw) {      /* off right */
	    /* position up against right edge */
	    x_title_w = sx + sw - w_title_w;
	  }
	  /* end keep label on screen. dje 8/7/97 */
	}
      }
    }
    if (use_unexpanded_size)
    {
      w_title_text_gap = ICON_TITLE_TEXT_GAP_COLLAPSED;
      x_title_min = w_title_text_gap + ICON_RELIEF_WIDTH;
      /* resize the icon name window */
      w_title_w = tmp_win->icon_p_width;
      x_title_w = tmp_win->icon_g.x;
    }
    tmp_win->icon_g.width = w_title_w;
    tmp_win->icon_xl_loc = x_title_w;

    /* set up TitleGC for drawing the icon label */
    if (tmp_win->icon_font.font != None)
      NewFontAndColor(tmp_win->icon_font.font->fid, TextColor, BackColor);

    tmp_win->icon_g.height = ICON_HEIGHT(tmp_win);
    XMoveResizeWindow(
      dpy, tmp_win->icon_title_w, x_title_w,
      tmp_win->icon_g.y + tmp_win->icon_p_height, w_title_w,
      ICON_HEIGHT(tmp_win));
    XSetWindowBackground(dpy, tmp_win->icon_title_w, BackColor);
    XClearWindow(dpy,tmp_win->icon_title_w);

    /* text position */
    x_title = (w_title_w - w_title) / 2;
    if (x_title < x_title_min)
      x_title = x_title_min;
    if (is_sticky)
    {
      XRectangle r;

      if (w_stipple == 0)
      {
	w_stipple = ((w_title_w - 2 * (x_stipple + w_title_text_gap) -
		      w_title) + 1) / 2;
      }
      if (w_stipple < ICON_TITLE_STICK_MIN_WIDTH)
      {
	w_stipple = ICON_TITLE_STICK_MIN_WIDTH;
      }
      if (x_title < x_stipple + w_stipple + w_title_text_gap)
      {
	x_title = x_stipple + w_stipple + w_title_text_gap;
      }
      r.x = x_title;
      r.y = 0;
      r.width = w_title_w - 2 * x_title;
      if (r.width < 1)
	r.width = 1;
      r.height = ICON_HEIGHT(tmp_win);
      XSetClipRectangles(dpy, Scr.TitleGC, 0, 0, &r, 1, Unsorted);
    }
#ifdef I18N_MB
    XmbDrawString(dpy, tmp_win->icon_title_w, tmp_win->icon_font.fontset,
#else
    XDrawString(dpy, tmp_win->icon_title_w,
#endif
		Scr.TitleGC, x_title,
		tmp_win->icon_g.height - tmp_win->icon_font.height +
		tmp_win->icon_font.y + ICON_TITLE_VERT_TEXT_OFFSET,
		tmp_win->visible_icon_name, strlen(tmp_win->visible_icon_name));
    RelieveRectangle(
	    dpy, tmp_win->icon_title_w, 0, 0, w_title_w - 1,
	    ICON_HEIGHT(tmp_win) - 1, Relief, Shadow, ICON_RELIEF_WIDTH);
    if (is_sticky)
    {
      /* an odd number of lines every 4 pixels */
      int num = (ICON_HEIGHT(tmp_win) / ICON_TITLE_STICK_VERT_DIST / 2) * 2 - 1;
      int min = ICON_HEIGHT(tmp_win) / 2 - num * 2 + 1;
      int max = ICON_HEIGHT(tmp_win) / 2 + num * 2 -
	      ICON_TITLE_STICK_VERT_DIST + 1;
      int i;

      for(i = min; w_stipple > 0 && i <= max; i += ICON_TITLE_STICK_VERT_DIST)
      {
	RelieveRectangle(
	  dpy, tmp_win->icon_title_w, x_stipple, i, w_stipple - 1, 1, Shadow,
	  Relief, ICON_TITLE_STICK_HEIGHT);
	RelieveRectangle(
	  dpy, tmp_win->icon_title_w, w_title_w - x_stipple - w_stipple, i,
	  w_stipple - 1, 1, Shadow, Relief, ICON_TITLE_STICK_HEIGHT);
      }
      XSetClipMask(dpy, Scr.TitleGC, None);
    }
  }

  if(tmp_win->icon_pixmap_w != None)
  {
    XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_g.x,
		tmp_win->icon_g.y);
  }

  /* only relieve unshaped icons that share fvwm's visual */
  if ((tmp_win->iconPixmap != None) && !IS_ICON_SHAPED(tmp_win)
      && (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win)))
  {
    RelieveRectangle(dpy, tmp_win->icon_pixmap_w, 0, 0,
		     tmp_win->icon_p_width - 1, tmp_win->icon_p_height - 1,
		     Relief, Shadow, ICON_RELIEF_WIDTH);
  }

  /* need to locate the icon pixmap */
  if (tmp_win->iconPixmap != None)
  {
    if (tmp_win->iconDepth == 1)
    {
      /* it's a bitmap */
      XCopyPlane(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w,
		 Scr.TitleGC, 0, 0,
		 tmp_win->icon_p_width - 2 * ICON_RELIEF_WIDTH,
		 tmp_win->icon_p_height - 2 * ICON_RELIEF_WIDTH,
		 ICON_RELIEF_WIDTH, ICON_RELIEF_WIDTH, 1);
    }
    else
    {
      if (Pdefault || IS_PIXMAP_OURS(tmp_win))
      {
        /* it's a pixmap that need copying */
	XCopyArea(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w,
		  Scr.TitleGC, 0, 0,
		  tmp_win->icon_p_width - 2 * ICON_RELIEF_WIDTH,
		  tmp_win->icon_p_height - 2 * ICON_RELIEF_WIDTH,
		  ICON_RELIEF_WIDTH, ICON_RELIEF_WIDTH);
      }
      else
      {
        /* it's a client pixmap and fvwm is not using the root visual
         * The icon window has no 3d border so copy to (0,0)
         * install the root colormap temporarily to help the Exceed server */
        if (Scr.bo.InstallRootCmap)
          InstallRootColormap();
	XCopyArea(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w,
		  DefaultGC(dpy, Scr.screen), 0, 0, tmp_win->icon_p_width,
		  tmp_win->icon_p_height, 0, 0);
	if (Scr.bo.InstallRootCmap)
	  UninstallRootColormap();
      }
    }
  }

  if (is_expanded)
  {
    if (tmp_win->icon_title_w != None)
    {
      XRaiseWindow (dpy, tmp_win->icon_title_w);
      raisePanFrames ();
    }
  }
  else
  {
    XWindowChanges xwc;
    int mask;
    xwc.sibling = tmp_win->frame;
    xwc.stack_mode = Below;
    mask = CWSibling|CWStackMode;
    if (tmp_win->icon_title_w != None)
    {
      XConfigureWindow(dpy, tmp_win->icon_title_w, mask, &xwc);
    }
    if (tmp_win->icon_pixmap_w != None)
    {
      XConfigureWindow(dpy, tmp_win->icon_pixmap_w, mask, &xwc);
    }
  }
  /* wait for pending EnterNotify/LeaveNotify events to suppress race condition
   * w/ expanding/collapsing icon titles */
  XSync(dpy, 0);
}

/***********************************************************************
 *
 *  Procedure:
 *    ChangeIconPixmap - procedure change the icon pixmap or "pixmap"
 *      window. Called in events.c and ewmh_events.c
 *
 ************************************************************************/
void ChangeIconPixmap(FvwmWindow *tmp_win)
{
  if (!IS_ICONIFIED(tmp_win))
  {
ICON_DBG((stderr,"hpn: postpone icon change '%s'\n", tmp_win->name));
    /* update the icon later when application is iconified */
    SET_HAS_ICON_CHANGED(tmp_win, 1);
  }
  else if (IS_ICONIFIED(tmp_win))
  {
    ICON_DBG((stderr,"hpn: applying new icon '%s'\n", tmp_win->name));
    SET_ICONIFIED(tmp_win, 0);
    SET_ICON_UNMAPPED(tmp_win, 0);
    CreateIconWindow(tmp_win, tmp_win->icon_g.x,tmp_win->icon_g.y);
    BroadcastPacket(M_ICONIFY, 7,
		    tmp_win->w, tmp_win->frame,
		    (unsigned long)tmp_win,
		    tmp_win->icon_g.x, tmp_win->icon_g.y,
		    tmp_win->icon_g.width, tmp_win->icon_g.height);
    /* domivogt (15-Sep-1999): BroadcastConfig informs modules of the
     * configuration change including the iconified flag. So this
     * flag must be set here. I'm not sure if the two calls of the
     * SET_ICONIFIED macro after BroadcastConfig are necessary, but
     * since it's only minimal overhead I prefer to be on the safe
     * side. */
    SET_ICONIFIED(tmp_win, 1);
    BroadcastConfig(M_CONFIGURE_WINDOW, tmp_win);
    SET_ICONIFIED(tmp_win, 0);

    if (!IS_ICON_SUPPRESSED(tmp_win))
    {
      LowerWindow(tmp_win);
      AutoPlaceIcon(tmp_win);
      if(tmp_win->Desk == Scr.CurrentDesk)
      {
	if(tmp_win->icon_title_w)
	  XMapWindow(dpy, tmp_win->icon_title_w);
	if(tmp_win->icon_pixmap_w != None)
	  XMapWindow(dpy, tmp_win->icon_pixmap_w);
      }
    }
    SET_ICONIFIED(tmp_win, 1);
    DrawIconWindow(tmp_win);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	RedoIconName - procedure to re-position the icon window and name
 *
 ************************************************************************/
void RedoIconName(FvwmWindow *tmp_win)
{
  if(IS_ICON_SUPPRESSED(tmp_win))
    return;

  if (tmp_win->icon_title_w == (int)NULL)
    return;

  if (HAS_NO_ICON_TITLE(tmp_win))
  {
    tmp_win->icon_t_width = 0;
  }
  else
  {
    tmp_win->icon_t_width =
      XTextWidth(tmp_win->icon_font.font, tmp_win->visible_icon_name,
                 strlen(tmp_win->visible_icon_name));
    if (!HAS_NO_ICON_TITLE(tmp_win) && tmp_win->icon_pixmap_w == None)
    {
      /* icon has only a title, set the fake width of the icon window
       * appropriately */
      tmp_win->icon_p_width = tmp_win->icon_t_width +
	      2 * (ICON_TITLE_TEXT_GAP_COLLAPSED + ICON_RELIEF_WIDTH);
      if (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win))
      {
	tmp_win->icon_p_width +=
	  2 * (ICON_TITLE_TO_STICK_EXTRA_GAP + ICON_TITLE_STICK_MIN_WIDTH);
      }
      tmp_win->icon_g.width = tmp_win->icon_p_width;
    }

  }
  /* clear the icon window, and trigger a re-draw via an expose event */
  if (IS_ICONIFIED(tmp_win))
  {
    DrawIconWindow(tmp_win);
  }
  if (IS_ICONIFIED(tmp_win))
    XClearArea(dpy, tmp_win->icon_title_w, 0, 0, 0, 0, True);
  return;
}




/************************************************************************
 *
 *  Procedure:
 *	AutoPlace - Find a home for an icon
 *
 ************************************************************************/
void AutoPlaceIcon(FvwmWindow *t)
{
  int tw,th,tx,ty;
  int base_x, base_y;
  int width,height;
  FvwmWindow *test_window;
  Bool loc_ok;
  Bool loc_ok_wrong_screen;
  Bool loc_ok_wrong_screen2;
  int real_x=10, real_y=10;
  int new_x, new_y;
  Bool do_move_pixmap = False;
  Bool do_move_title = False;

  /* New! Put icon in same page as the center of the window */
  /* Not a good idea for StickyIcons. Neither for icons of windows that are
   * visible on the current page. */
  if (IS_ICON_STICKY(t) || IS_STICKY(t))
  {
    base_x = 0;
    base_y = 0;
    /*Also, if its a stickyWindow, put it on the current page! */
    new_x = t->frame_g.x % Scr.MyDisplayWidth;
    new_y = t->frame_g.y % Scr.MyDisplayHeight;
    if(new_x + t->frame_g.width <= 0)
      new_x += Scr.MyDisplayWidth;
    if(new_y + t->frame_g.height <= 0)
      new_y += Scr.MyDisplayHeight;
    SetupFrame(t, new_x, new_y, t->frame_g.width, t->frame_g.height, False);
    t->Desk = Scr.CurrentDesk;
  }
  else if (IsRectangleOnThisPage(&(t->frame_g), t->Desk))
  {
    base_x = 0;
    base_y = 0;
  }
  else
  {
    base_x=((t->frame_g.x+Scr.Vx+(t->frame_g.width>>1))/Scr.MyDisplayWidth)*
      Scr.MyDisplayWidth;
    base_y=((t->frame_g.y+Scr.Vy+(t->frame_g.height>>1))/
	    Scr.MyDisplayHeight)*Scr.MyDisplayHeight;
    /* limit icon position to desktop */
    if (base_x > Scr.VxMax)
      base_x = Scr.VxMax;
    if (base_x < 0)
      base_x = 0;
    if (base_y > Scr.VyMax)
      base_y = Scr.VyMax;
    if (base_y < 0)
      base_y = 0;
    base_x -= Scr.Vx;
    base_y -= Scr.Vy;
  }
  if(IS_ICON_MOVED(t))
  {
    /* just make sure the icon is on this page */
    t->icon_g.x = t->icon_g.x % Scr.MyDisplayWidth + base_x;
    t->icon_g.y = t->icon_g.y % Scr.MyDisplayHeight + base_y;
    if(t->icon_g.x < 0)
      t->icon_g.x += Scr.MyDisplayWidth;
    if(t->icon_g.y < 0)
      t->icon_g.y += Scr.MyDisplayHeight;
    t->icon_xl_loc = t->icon_g.x;
    do_move_pixmap = True;
    do_move_title = True;
  }
  else if (USE_ICON_POSITION_HINT(t) && t->wmhints &&
	   t->wmhints->flags & IconPositionHint)
  {
    t->icon_g.x = t->wmhints->icon_x;
    t->icon_g.y = t->wmhints->icon_y;
    t->icon_xl_loc = t->icon_g.x;
  }
  /* dje 10/12/97:
     Look thru chain of icon boxes assigned to window.
     Add logic for grids and fill direction.
  */
  else if (DO_IGNORE_ICON_BOXES(t))
  {
    int sx;
    int sy;
    int sw;
    int sh;
    fscreen_scr_arg fscr;

    t->icon_g.x = t->frame_g.x;
    if (HAS_BOTTOM_TITLE(t))
    {
      t->icon_g.y =
	t->frame_g.y + t->frame_g.height - t->icon_g.height - t->icon_p_height;
    }
    else
    {
      t->icon_g.y = t->frame_g.y;
    }
    fscr.xypos.x = t->icon_g.x + t->icon_g.width / 2;
    fscr.xypos.y = t->icon_g.y + t->icon_g.height / 2;
    FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &sx, &sy, &sw, &sh);
    if (t->icon_g.x < sx)
      t->icon_g.x = sx;
    else if (t->icon_g.x + t->icon_g.width > sx + sw)
      t->icon_g.x = sx + sw - t->icon_g.width;
    if (t->icon_g.y < sy)
      t->icon_g.y = sy;
    else if (t->icon_g.y + t->icon_g.height + t->icon_p_height > sy + sh)
      t->icon_g.y = sy + sh - t->icon_g.height - t->icon_p_height;
    do_move_pixmap = True;
    do_move_title = True;
  }
  else
  {
    /* A place to hold inner and outer loop variables. */
    typedef struct dimension_struct
    {
      int step;                         /* grid size (may be negative) */
      int start_at;                     /* starting edge */
      int real_start;                   /* on screen starting edge */
      int end_at;                       /* ending edge */
      int base;                         /* base for screen */
      int icon_dimension;               /* height or width */
      int nom_dimension;                /* nonminal height or width */
      int screen_dimension;             /* screen height or width */
      int screen_offset;                /* screen offset */
    } dimension;
    dimension dim[3];                   /* space for work, 1st, 2nd dimen */
    icon_boxes *icon_boxes_ptr;         /* current icon box */
    int i;                              /* index for inner/outer loop data */
    fscreen_scr_arg fscr;
    rectangle ref;

    /* Hopefully this makes the following more readable. */
#define ICONBOX_LFT icon_boxes_ptr->IconBox[0]
#define ICONBOX_TOP icon_boxes_ptr->IconBox[1]
#define ICONBOX_RGT icon_boxes_ptr->IconBox[2]
#define ICONBOX_BOT icon_boxes_ptr->IconBox[3]
#define BOT_FILL icon_boxes_ptr->IconFlags & ICONFILLBOT
#define RGT_FILL icon_boxes_ptr->IconFlags & ICONFILLRGT
#define HRZ_FILL icon_boxes_ptr->IconFlags & ICONFILLHRZ

    /* needed later */
    fscr.xypos.x = t->frame_g.x + (t->frame_g.width / 2) - base_x;
    fscr.xypos.y = t->frame_g.y + (t->frame_g.height / 2) - base_y;
    /* unnecessary copy of width */
    width = t->icon_p_width;
    /* total height */
    height = t->icon_g.height + t->icon_p_height;
    /* no slot found yet */
    loc_ok = False;
    loc_ok_wrong_screen = False;

    /* check all boxes in order */
    icon_boxes_ptr = NULL;              /* init */
    while(do_all_iconboxes(t, &icon_boxes_ptr))
    {
      if (loc_ok == True)
      {
	/* leave for loop */
        break;
      }
      /* get the screen dimensions for the icon box */
      if (icon_boxes_ptr->IconScreen == FSCREEN_CURRENT)
	fscr.mouse_ev = NULL;
      FScreenGetScrRect(
	&fscr,
        icon_boxes_ptr->IconScreen,
	&ref.x, &ref.y, &ref.width, &ref.height);
      dim[1].screen_offset = ref.y;
      dim[1].screen_dimension = ref.height;
      dim[2].screen_offset = ref.x;
      dim[2].screen_dimension = ref.width;
      /* y amount */
      dim[1].step = icon_boxes_ptr->IconGrid[1];
      /* init start from */
      dim[1].start_at = ICONBOX_TOP + dim[1].screen_offset;
      if (icon_boxes_ptr->IconSign[1] == '-') {
        dim[1].start_at += dim[1].screen_dimension;
      }
      /* init end at */
      dim[1].end_at = ICONBOX_BOT + dim[1].screen_offset;
      if (icon_boxes_ptr->IconSign[3] == '-') {
        dim[1].end_at += dim[1].screen_dimension;
      }
      /* save base */
      dim[1].base = base_y;
      /* save dimension */
      dim[1].icon_dimension = height;
      if (BOT_FILL)
      {
	/* fill from bottom */
	/* reverse step */
        dim[1].step = 0 - dim[1].step;
      } /* end fill from bottom */

      /* x amount */
      dim[2].step = icon_boxes_ptr->IconGrid[0];
      /* init start from */
      dim[2].start_at = ICONBOX_LFT + dim[2].screen_offset;
      if (icon_boxes_ptr->IconSign[0] == '-') {
        dim[2].start_at += dim[2].screen_dimension;
      }
      /* init end at */
      dim[2].end_at = ICONBOX_RGT + dim[2].screen_offset;
      if (icon_boxes_ptr->IconSign[2] == '-') {
        dim[2].end_at += dim[2].screen_dimension;
      }
      /* save base */
      dim[2].base   = base_x;
      /* save dimension */
      dim[2].icon_dimension = width;
      if (RGT_FILL)
      {
	/* fill from right */
	/* reverse step */
        dim[2].step = 0 - dim[2].step;
      } /* end fill from right */
      for (i=1;i<=2;i++)
      {
	/* for dimensions 1 and 2 */
        /* If the window is taller than the icon box, ignore the icon height
         * when figuring where to put it. Same goes for the width
         * This should permit reasonably graceful handling of big icons. */
        dim[i].nom_dimension = dim[i].icon_dimension;
        if (dim[i].icon_dimension >= dim[i].end_at - dim[i].start_at)
	{
          dim[i].nom_dimension = dim[i].end_at - dim[i].start_at - 1;
        }
        if (dim[i].step < 0)
	{
	  /* if moving backwards */
	  /* save */
          dim[0].start_at = dim[i].start_at;
	  /* swap one */
          dim[i].start_at = dim[i].end_at;
	  /* swap the other */
          dim[i].end_at = dim[0].start_at;
          dim[i].start_at -= dim[i].icon_dimension;
        } /* end moving backwards */
	/* adjust both to base */
        dim[i].start_at += dim[i].base;
        dim[i].end_at += dim[i].base;
      } /* end 2 dimensions */
      if (HRZ_FILL)
      {
	/* if hrz first */
	/* save */
        memcpy(&dim[0],&dim[1],sizeof(dimension));
	/* switch one */
        memcpy(&dim[1],&dim[2],sizeof(dimension));
	/* switch the other */
        memcpy(&dim[2],&dim[0],sizeof(dimension));
      } /* end horizontal dimension first */
      /* save for reseting inner loop */
      dim[0].start_at = dim[2].start_at;
      loc_ok_wrong_screen2 = False;
      while((dim[1].step < 0            /* filling reversed */
             ? (dim[1].start_at + dim[1].icon_dimension - dim[1].nom_dimension
                > dim[1].end_at)        /* check back edge */
             : (dim[1].start_at + dim[1].nom_dimension
                < dim[1].end_at))       /* check front edge */
            && (!loc_ok)
	    && (!loc_ok_wrong_screen2)) {  /* nothing found yet */
        dim[1].real_start = dim[1].start_at; /* init */
        if (dim[1].start_at + dim[1].icon_dimension >
	    dim[1].screen_offset + dim[1].screen_dimension - 2 + dim[1].base)
	{
	  /* off screen, move on screen */
          dim[1].real_start = dim[1].screen_offset + dim[1].screen_dimension
            - dim[1].icon_dimension + dim[1].base;
        } /* end off screen */
        if (dim[1].start_at < dim[1].screen_offset + dim[1].base)
	{
	  /* if off other edge, move on screen */
          dim[1].real_start = dim[1].screen_offset + dim[1].base;
        } /* end off other edge */
	/* reset inner loop */
        dim[2].start_at = dim[0].start_at;
        while((dim[2].step < 0            /* filling reversed */
               ? (dim[2].start_at + dim[2].icon_dimension-dim[2].nom_dimension
                  > dim[2].end_at)        /* check back edge */
               : (dim[2].start_at + dim[2].nom_dimension
                  < dim[2].end_at))       /* check front edge */
	      && (!loc_ok)
	      && (!loc_ok_wrong_screen2)) { /* nothing found yet */
          dim[2].real_start = dim[2].start_at; /* init */
          if (dim[2].start_at + dim[2].icon_dimension >
              dim[2].screen_offset + dim[2].screen_dimension - 2 + dim[2].base)
	  {
	    /* if off screen, move on screen */
            dim[2].real_start = dim[2].screen_offset + dim[2].screen_dimension
              - dim[2].icon_dimension + dim[2].base;
          } /* end off screen */
          if (dim[2].start_at < dim[2].screen_offset + dim[2].base)
	  {
	    /* if off other edge, move on screen */
	    dim[2].real_start = dim[2].screen_offset + dim[2].base;
          } /* end off other edge */

          if (HRZ_FILL)
	  {
	    /* hrz first */
	    /* unreverse them */
            real_x = dim[1].real_start;
            real_y = dim[2].real_start;
          }
	  else
	  {
	    /* reverse them */
            real_x = dim[2].real_start;
            real_y = dim[1].real_start;
          }

	  /* this may be a good location */
	  if (FScreenIsRectangleOnScreen(&fscr, FSCREEN_XYPOS, &ref))
	  {
	    loc_ok = True;
	  }
	  else
	  {
	    loc_ok_wrong_screen2 = True;
	  }
          test_window = Scr.FvwmRoot.next;
          while((test_window != (FvwmWindow *)0)
                &&(loc_ok == True || loc_ok_wrong_screen2))
	  {
	    /* test overlap */
            if(test_window->Desk == t->Desk)
	    {
              if((IS_ICONIFIED(test_window)) &&
                 (!IS_TRANSIENT(test_window) ||
		  !IS_ICONIFIED_BY_PARENT(test_window)) &&
                 (test_window->icon_title_w||test_window->icon_pixmap_w) &&
                 (test_window != t)) {
                tw=test_window->icon_p_width;
                th=test_window->icon_p_height+
                  test_window->icon_g.height;
                tx = test_window->icon_g.x;
                ty = test_window->icon_g.y;

                if((tx<(real_x+width+3))&&((tx+tw+3) > real_x)&&
                   (ty<(real_y+height+3))&&((ty+th + 3)>real_y))
		{
		  /* don't accept this location */
                  loc_ok = False;
                  loc_ok_wrong_screen2 = False;
                } /* end if icons overlap */
              } /* end if its an icon */
            } /* end if same desk */
            test_window = test_window->next;
          } /* end while icons that may overlap */
	  if (loc_ok_wrong_screen2)
	  {
	    loc_ok_wrong_screen = True;
	  }
	  /* Grid inner value & direction */
          dim[2].start_at += dim[2].step;
        } /* end while room inner dimension */
	/* Grid outer value & direction */
        dim[1].start_at += dim[1].step;
      } /* end while room outer dimension */
    } /* end for all icon boxes, or found space */
    if(!loc_ok && !loc_ok_wrong_screen)
      /* If icon never found a home just leave it */
      return;
    t->icon_g.x = real_x;
    t->icon_g.y = real_y;

    if(t->icon_pixmap_w)
      XMoveWindow(dpy,t->icon_pixmap_w,t->icon_g.x, t->icon_g.y);

    t->icon_g.width = t->icon_p_width;
    t->icon_xl_loc = t->icon_g.x;

    do_move_title = True;
    BroadcastPacket(M_ICON_LOCATION, 7,
                    t->w, t->frame,
                    (unsigned long)t,
                    t->icon_g.x, t->icon_g.y,
                    t->icon_p_width, t->icon_g.height+t->icon_p_height);
  }
  if (do_move_pixmap && t->icon_pixmap_w != None)
  {
    XMoveWindow(dpy, t->icon_pixmap_w, t->icon_g.x, t->icon_g.y);
  }
  if (do_move_title && t->icon_title_w != None)
  {
    XMoveResizeWindow(
      dpy, t->icon_title_w, t->icon_xl_loc, t->icon_g.y+t->icon_p_height,
      t->icon_g.width, ICON_HEIGHT(t));
  }
}

static icon_boxes *global_icon_box_ptr;
/* Find next icon box to try to place icon in.
   Goes thru chain that the window got thru style matching,
   then the global icon box.
   Create the global icon box on first call.
   Return code indicates when the boxes are used up.
   The boxes could only get completely used up when you fill the screen
   with them.
*/
static int
do_all_iconboxes(FvwmWindow *t, icon_boxes **icon_boxes_ptr)
{
  if (global_icon_box_ptr == 0) {       /* if first time */
    int sx;
    int sy;
    int sw;
    int sh;
    /* Right now, the global box is hard-coded, fills the primary screen,
     * uses an 80x80 grid, and fills top-bottom, left-right */
    FScreenGetScrRect(NULL, FSCREEN_PRIMARY, &sx, &sy, &sw, &sh);
    global_icon_box_ptr = calloc(1, sizeof(icon_boxes));
    global_icon_box_ptr->IconBox[0] = sx;
    global_icon_box_ptr->IconBox[1] = sy;
    global_icon_box_ptr->IconBox[2] = sx + sw;
    global_icon_box_ptr->IconBox[3] = sy + sh;
    global_icon_box_ptr->IconGrid[0] = 80;
    global_icon_box_ptr->IconGrid[1] = 80;
    global_icon_box_ptr->IconFlags = ICONFILLHRZ;
  }
  if (*icon_boxes_ptr == NULL) {        /* first time? */
    *icon_boxes_ptr = t->IconBoxes;     /* start at windows box */
    if (!*icon_boxes_ptr) {             /* if window has no box */
      *icon_boxes_ptr = global_icon_box_ptr; /* use global box */
    }
    return (1);                         /* use box */
  }

  /* Here its not the first call, we are either on the chain or at
     the global box */
  if (*icon_boxes_ptr == global_icon_box_ptr) { /* if the global box */
    return (0);                         /* completely out of boxes (unlikely) */
  }
  *icon_boxes_ptr = (*icon_boxes_ptr)->next; /* move to next one on chain */
  if (*icon_boxes_ptr) {                /* if there is a next one */
    return (1);                         /* return it */
  }
  *icon_boxes_ptr = global_icon_box_ptr; /* global box */
  return (1);                           /* use it */
}

/****************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 ****************************************************************************/
static void GetBitmapFile(FvwmWindow *tmp_win)
{
  char *path = NULL;
  int HotX,HotY;

  path = findImageFile(tmp_win->icon_bitmap_file, NULL, R_OK);

  if (path == NULL)
    return;
  if (XReadBitmapFile(dpy, Scr.Root, path,
		      (unsigned int *)&tmp_win->icon_p_width,
		      (unsigned int *)&tmp_win->icon_p_height,
		      &tmp_win->iconPixmap, &HotX, &HotY) == BitmapSuccess)
  {
    tmp_win->iconDepth = 1;
    SET_PIXMAP_OURS(tmp_win, 1);
  }
  else
  {
    tmp_win->icon_p_width = 0;
    tmp_win->icon_p_height = 0;
  }

  free(path);
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
#ifdef XPM
static void GetXPMFile(FvwmWindow *tmp_win)
{
  XpmAttributes xpm_attributes;
  char *path = NULL;
  XpmImage my_image;
  int rc;

  path = findImageFile(tmp_win->icon_bitmap_file, NULL, R_OK);
  if(path == NULL)return;

  xpm_attributes.visual = Pvisual;
  xpm_attributes.colormap = Pcmap;
  xpm_attributes.depth = Pdepth;
  xpm_attributes.closeness = 40000; /* Allow for "similar" colors */
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmCloseness
    | XpmVisual | XpmColormap | XpmDepth;

  rc = XpmReadFileToXpmImage(path, &my_image, NULL);
  if (rc != XpmSuccess)
  {
    fvwm_msg(ERR,"GetXPMFile","XpmReadFileToXpmImage failed, pixmap %s, rc %d",
	     path, rc);
    free(path);
    return;
  }
  free(path);
  color_reduce_pixmap(&my_image,Scr.ColorLimit);
  rc = XpmCreatePixmapFromXpmImage(dpy,Scr.NoFocusWin, &my_image,
                                   &tmp_win->iconPixmap,
                                   &tmp_win->icon_maskPixmap,
                                   &xpm_attributes);
  if (rc != XpmSuccess)
  {
    fvwm_msg(ERR,"GetXPMFile",
             "XpmCreatePixmapFromXpmImage failed, rc %d\n", rc);
    XpmFreeXpmImage(&my_image);
    return;
  }
  tmp_win->icon_p_width = my_image.width;
  tmp_win->icon_p_height = my_image.height;
  SET_PIXMAP_OURS(tmp_win, 1);
  tmp_win->iconDepth = Pdepth;

  if (FShapesSupported && tmp_win->icon_maskPixmap)
    SET_ICON_SHAPED(tmp_win, 1);

  XpmFreeXpmImage(&my_image);

}
#endif /* XPM */

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 ****************************************************************************/
void GetIconWindow(FvwmWindow *tmp_win)
{
  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  if (XGetGeometry(
	dpy, tmp_win->wmhints->icon_window, &JunkRoot, &JunkX, &JunkY,
	(unsigned int *)&tmp_win->icon_p_width,
	(unsigned int *)&tmp_win->icon_p_height, &JunkBW, &JunkDepth) == 0)
  {
    fvwm_msg(ERR,"GetIconWindow", "Window '%s' has a bad icon window!"
	     " Ignoring icon window.",
	     tmp_win->name);
    /* disable the icon window hint */
    tmp_win->wmhints->icon_window = None;
    tmp_win->wmhints->flags &= ~IconWindowHint;
    return;
  }
  tmp_win->icon_p_width += JunkBW<<1;
  tmp_win->icon_p_height += JunkBW<<1;
  /*
   * Now make the new window the icon window for this window,
   * and set it up to work as such (select for key presses
   * and button presses/releases, set up the contexts for it,
   * and define the cursor for it).
   */
  tmp_win->icon_pixmap_w = tmp_win->wmhints->icon_window;
  if (FShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      SET_ICON_SHAPED(tmp_win, 1);
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
  /* Make sure that the window is a child of the root window ! */
  /* Olwais screws this up, maybe others do too! */
  XReparentWindow(dpy, tmp_win->icon_pixmap_w, Scr.Root, 0,0);
  SET_ICON_OURS(tmp_win, 0);
}


/****************************************************************************
 *
 * Looks for an application supplied bitmap or pixmap
 *
 ****************************************************************************/
void GetIconBitmap(FvwmWindow *tmp_win)
{
  unsigned int width, height, depth;

  /* We are guaranteed that wmhints is non-null when calling this routine */
  if (!XGetGeometry(dpy, tmp_win->wmhints->icon_pixmap, &JunkRoot,
		    &JunkX, &JunkY, &width, &height, &JunkBW, &depth))
  {
    fvwm_msg(ERR,"GetIconBitmap", "Window '%s' has a bad icon pixmap!"
	     " Ignoring icon.", tmp_win->name);
    /* disable icon pixmap hint */
    tmp_win->wmhints->icon_pixmap = None;
    tmp_win->wmhints->flags &= ~IconPixmapHint;
    return;
  }

  /* sanity check the pixmap depth, it must be the same as the root or 1 */
  if ((depth != 1) && (depth != DefaultDepth(dpy,Scr.screen)))
  {
    fvwm_msg(ERR, "GetIconBitmap",
	     "Window '%s' has a bad icon bitmap depth %d (should be 1 or %d)!"
	     " Ignoring icon bitmap.",
	     tmp_win->name, depth, DefaultDepth(dpy,Scr.screen));
    /* disable icon pixmap hint */
    tmp_win->wmhints->icon_pixmap = None;
    tmp_win->wmhints->flags &= ~IconPixmapHint;
    return;
  }

  tmp_win->iconPixmap = tmp_win->wmhints->icon_pixmap;
  tmp_win->icon_p_width = width;
  tmp_win->icon_p_height = height;
  tmp_win->iconDepth = depth;

  if (FShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      SET_ICON_SHAPED(tmp_win, 1);
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }

  SET_PIXMAP_OURS(tmp_win, 0);
}



/***********************************************************************
 *
 *  Procedure:
 *	DeIconify a window
 *
 ***********************************************************************/
void DeIconify(FvwmWindow *tmp_win)
{
  FvwmWindow *t,*tmp;
  FvwmWindow *sf = get_focus_window();
  rectangle icon_rect;

  if (!tmp_win)
    return;
  if (IS_MAP_PENDING(tmp_win))
  {
    /* final state: de-iconified */
    SET_ICONIFY_AFTER_MAP(tmp_win, 0);
    return;
  }
  while (IS_ICONIFIED_BY_PARENT(tmp_win))
  {
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (t != tmp_win && tmp_win->transientfor == t->w)
	tmp_win = t;
    }
  }

  /* AS dje  RaiseWindow(tmp_win); */

  mark_transient_subtree(tmp_win, MARK_ALL_LAYERS, MARK_ALL, False, True);
  if (tmp_win == sf)
  {
    /* take away the focus before mapping */
    DeleteFocus(0);
  }
  /* now de-iconify transients */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (t == tmp_win || IS_IN_TRANSIENT_SUBTREE(t))
    {
      SET_IN_TRANSIENT_SUBTREE(t, 0);
      SET_MAPPED(t, 1);
      SET_ICONIFIED_BY_PARENT(t, 0);
      if(Scr.Hilite == t)
	DrawDecorations(t, DRAW_ALL, False, True, None, CLEAR_ALL);

      /* AS stuff starts here dje */
      if (t->icon_pixmap_w)
	XUnmapWindow(dpy, t->icon_pixmap_w);
      if (t->icon_title_w)
	XUnmapWindow(dpy, t->icon_title_w);
      XFlush(dpy);
      /* End AS */
      XMapWindow(dpy, t->w);
      if (t->Desk == Scr.CurrentDesk)
      {
	rectangle r;

	r.x = t->icon_g.x;
	r.y = t->icon_g.y;
	r.width = t->icon_g.width;
	r.height = t->icon_p_height+t->icon_g.height;
	/* update absoluthe geometry in case the icon was moved over a page
	 * boundary; the move code already takes care of keeping the frame
	 * geometry up to date */
	update_absolute_geometry(t);
	if (IsRectangleOnThisPage(&r, t->Desk) &&
	    !IsRectangleOnThisPage(&(t->frame_g), t->Desk))
	{
	  /* Make sure we keep it on screen when de-iconifying. */
	  t->frame_g.x -=
	    truncate_to_multiple(t->frame_g.x,Scr.MyDisplayWidth);
	  t->frame_g.y -=
	    truncate_to_multiple(t->frame_g.y,Scr.MyDisplayHeight);
	  XMoveWindow(dpy, t->frame, t->frame_g.x, t->frame_g.y);
          update_absolute_geometry(t);
          maximize_adjust_offset(t);
	}
      }
      /* domivogt (1-Mar-2000): The next block is a hack to prevent animation
       * if the window has an icon, but neither a pixmap nor a title. */
      if (HAS_NO_ICON_TITLE(t) && t->icon_pixmap_w == None)
      {
        t->icon_g.width = 0;
        t->icon_g.height = 0;
        t->icon_p_width = 0;
        t->icon_p_height = 0;
      }
      if (!EWMH_GetIconGeometry(t, &icon_rect))
      {
	icon_rect.x = t->icon_g.x;
	icon_rect.y = t->icon_g.y;
	icon_rect.width = t->icon_g.width;
	icon_rect.height = t->icon_g.height+t->icon_p_height;
      }
      if (t == tmp_win)
      {
	BroadcastPacket(M_DEICONIFY, 11,
			t->w, t->frame,
			(unsigned long)t,
			icon_rect.x, icon_rect.y,
			icon_rect.width, icon_rect.height,
			t->frame_g.x, t->frame_g.y,
			t->frame_g.width, t->frame_g.height);
      }
      else
      {
	BroadcastPacket(M_DEICONIFY, 7,
			t->w, t->frame,
			(unsigned long)t,
			t->icon_g.x, t->icon_g.y,
			t->icon_p_width,
			t->icon_p_height + t->icon_g.height);
      }
      if(t->Desk == Scr.CurrentDesk)
      {
	XMapWindow(dpy, t->frame);
	SET_MAP_PENDING(t, 1);
      }
      XMapWindow(dpy, t->Parent);
      SetMapStateProp(t, NormalState);
      SET_ICONIFIED(t, 0);
      SET_ICON_UNMAPPED(t, 0);
      SET_ICON_ENTERED(t, 0);
      /* Need to make sure the border is colored correctly,
       * in case it was stuck or unstuck while iconified. */
      tmp = Scr.Hilite;
      Scr.Hilite = t;
      DrawDecorations(t, DRAW_ALL, False, True, None, CLEAR_ALL);
      Scr.Hilite = tmp;
    }
  }

#if 1
  RaiseWindow(tmp_win); /* moved dje */
#endif
  if (sf)
  {
    focus_grab_buttons(sf, True);
  }
  if (sf == tmp_win)
  {
    /* update the focus to make sure the application knows its state */
    if (HAS_CLICK_FOCUS(tmp_win) || HAS_SLOPPY_FOCUS(tmp_win))
    {
      SetFocusWindow(tmp_win, 0);
    }
  }
  else if (HAS_CLICK_FOCUS(tmp_win))
  {
#if 0
    FocusOn(tmp_win, TRUE, "");
#else
    SetFocusWindow(tmp_win, 1);
#endif
  }
  GNOME_SetWinArea(tmp_win);

  return;
}


/****************************************************************************
 *
 * Iconifies the selected window
 *
 ****************************************************************************/
void Iconify(FvwmWindow *tmp_win, int def_x, int def_y)
{
  FvwmWindow *t;
  FvwmWindow *sf;
  XWindowAttributes winattrs = {0};
  unsigned long eventMask;
  rectangle icon_rect;

  if(!tmp_win)
    return;
  if (IS_MAP_PENDING(tmp_win))
  {
    /* final state: iconified */
    SET_ICONIFY_AFTER_MAP(tmp_win, 1);
    return;
  }
  if (!XGetWindowAttributes(dpy, tmp_win->w, &winattrs))
  {
    return;
  }
  eventMask = winattrs.your_event_mask;
#if 0
  if (tmp_win == Scr.Hilite && HAS_CLICK_FOCUS(tmp_win) && tmp_win->next)
  {
    SetFocusWindow(tmp_win->next, 1);
  }
#endif

  mark_transient_subtree(tmp_win, MARK_ALL_LAYERS, MARK_ALL, False, True);
  sf = get_focus_window();
  if (sf && IS_IN_TRANSIENT_SUBTREE(sf))
  {
    restore_focus_after_unmap(sf, True);
  }
  /* iconify transients first */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (t == tmp_win || IS_IN_TRANSIENT_SUBTREE(t))
    {
      SET_IN_TRANSIENT_SUBTREE(t, 0);
      SET_ICON_ENTERED(t, 0);
      /*
       * Prevent the receipt of an UnmapNotify, since that would
       * cause a transition to the Withdrawn state.
       */
      SET_MAPPED(t, 0);
      XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
      XUnmapWindow(dpy, t->w);
      XSelectInput(dpy, t->w, eventMask);
      XUnmapWindow(dpy, t->frame);
      t->DeIconifyDesk = t->Desk;
      if (t->icon_title_w)
	XUnmapWindow(dpy, t->icon_title_w);
      if (t->icon_pixmap_w)
	XUnmapWindow(dpy, t->icon_pixmap_w);

      SetMapStateProp(t, IconicState);
      DrawDecorations(t, DRAW_ALL, False, False, None, CLEAR_ALL);
      if (t == tmp_win && !IS_ICONIFIED_BY_PARENT(tmp_win))
      {
	SET_DEICONIFY_PENDING(t, 1);
      }
      else
      {
	SET_ICONIFIED(t, 1);
	SET_ICON_UNMAPPED(t, 1);
	SET_ICONIFIED_BY_PARENT(t, 1);

	BroadcastPacket(M_ICONIFY, 7,
			t->w, t->frame,
			(unsigned long)t,
			-10000, -10000,
			t->icon_g.width,
			t->icon_g.height+t->icon_p_height);
	BroadcastConfig(M_CONFIGURE_WINDOW,t);
      }
    } /* if */
  } /* for */

  /* necessary during a recapture */
  if (IS_ICONIFIED_BY_PARENT(tmp_win))
    return;

  if (tmp_win->icon_title_w == None || HAS_ICON_CHANGED(tmp_win))
  {
    if(IS_ICON_MOVED(tmp_win))
      CreateIconWindow(tmp_win,tmp_win->icon_g.x,tmp_win->icon_g.y);
    else
      CreateIconWindow(tmp_win, def_x, def_y);
    SET_HAS_ICON_CHANGED(tmp_win, 0);
  }

  /* if no pixmap we want icon width to change to text width every iconify */
  if (tmp_win->icon_title_w && !tmp_win->icon_pixmap_w)
  {
    if (HAS_NO_ICON_TITLE(tmp_win))
    {
      tmp_win->icon_t_width = 0;
      tmp_win->icon_p_width = 0;
      tmp_win->icon_g.width = 0;
    }
    else
    {
      tmp_win->icon_t_width =
        XTextWidth(tmp_win->icon_font.font,tmp_win->visible_icon_name,
                   strlen(tmp_win->visible_icon_name));
      tmp_win->icon_p_width = tmp_win->icon_t_width +
	      2 * (ICON_TITLE_TEXT_GAP_COLLAPSED + ICON_RELIEF_WIDTH);
      if (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win))
      {
	tmp_win->icon_p_width +=
	  2 * (ICON_TITLE_TO_STICK_EXTRA_GAP + ICON_TITLE_STICK_MIN_WIDTH);
      }
      tmp_win->icon_g.width = tmp_win->icon_p_width;
    }
  }

  /* this condition will be true unless we restore a window to
     iconified state from a saved session. */
  if (!(DO_START_ICONIC(tmp_win) && IS_ICON_MOVED(tmp_win)))
  {
    AutoPlaceIcon(tmp_win);
  }
  /* domivogt (1-Mar-2000): The next block is a hack to prevent animation if the
   * window has an icon, but neither a pixmap nor a title. */
  if (HAS_NO_ICON_TITLE(tmp_win) && tmp_win->icon_pixmap_w == None)
  {
    tmp_win->icon_g.width = 0;
    tmp_win->icon_g.height = 0;
    tmp_win->icon_p_width = 0;
    tmp_win->icon_p_height = 0;
  }
  SET_ICONIFIED(tmp_win, 1);
  SET_ICON_UNMAPPED(tmp_win, 0);
  if (!EWMH_GetIconGeometry(tmp_win, &icon_rect))
  {
    icon_rect.x = tmp_win->icon_g.x;
    icon_rect.y = tmp_win->icon_g.y;
    icon_rect.width = tmp_win->icon_g.width;
    icon_rect.height = tmp_win->icon_g.height+tmp_win->icon_p_height;
  }
  BroadcastPacket(M_ICONIFY, 11,
                  tmp_win->w, tmp_win->frame,
                  (unsigned long)tmp_win,
                  icon_rect.x,
                  icon_rect.y,
                  icon_rect.width,
                  icon_rect.height,
                  tmp_win->frame_g.x, /* next 4 added for Animate module */
                  tmp_win->frame_g.y,
                  tmp_win->frame_g.width,
                  tmp_win->frame_g.height);
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);

  if (!(DO_START_ICONIC(tmp_win) && IS_ICON_MOVED(tmp_win)))
  {
    LowerWindow(tmp_win);
  }
  if (IS_ICON_STICKY(tmp_win) || IS_STICKY(tmp_win))
  {
    tmp_win->Desk = Scr.CurrentDesk;
  }
  if (tmp_win->Desk == Scr.CurrentDesk)
  {
    if (tmp_win->icon_title_w != None)
      XMapWindow(dpy, tmp_win->icon_title_w);

    if(tmp_win->icon_pixmap_w != None)
      XMapWindow(dpy, tmp_win->icon_pixmap_w);
  }
#if 0
  if (HAS_CLICK_FOCUS(tmp_win) || HAS_SLOPPY_FOCUS(tmp_win))
  {
    if (tmp_win == get_focus_window())
    {
      if (HAS_CLICK_FOCUS(tmp_win) && tmp_win->next)
      {
	SetFocusWindow(tmp_win->next, 1);
      }
      else
      {
	DeleteFocus(1);
      }
    }
  }
#endif
  if ((sf = get_focus_window()))
  {
    focus_grab_buttons(sf, True);
  }

  return;
}



/****************************************************************************
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 ****************************************************************************/
void SetMapStateProp(FvwmWindow *tmp_win, int state)
{
  unsigned long data[2];		/* "suggested" by ICCCM version 1 */

  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_title_w;
/*  data[2] = (unsigned long) tmp_win->icon_pixmap_w;*/

  XChangeProperty(dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32,
		  PropModeReplace, (unsigned char *) data, 2);
  return;
}


void CMD_Iconify(F_CMD_ARGS)
{
  int toggle;
  int x;
  int y;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT, ButtonRelease))
    return;

  toggle = ParseToggleArgument(action, NULL, -1, 0);
  if (toggle == -1)
  {
    if (GetIntegerArguments(action, NULL, &toggle, 1) > 0)
    {
      if (toggle > 0)
	toggle = 1;
      else if (toggle < 0)
	toggle = 0;
      else
	toggle = -1;
    }
  }
  if (toggle == -1)
    toggle = (IS_ICONIFIED(tmp_win)) ? 0 : 1;

  if (IS_ICONIFIED(tmp_win))
  {
    if (toggle == 0)
    {
      DeIconify(tmp_win);
      EWMH_SetWMState(tmp_win, False);
    }
  }
  else
  {
    if (toggle == 1)
    {
      if (!is_function_allowed(F_ICONIFY, NULL, tmp_win, False, True))
      {
	XBell(dpy, 0);
	return;
      }
      x = 0;
      y = 0;
      GetLocationFromEventOrQuery(dpy, Scr.Root, eventp, &x, &y);
      Iconify(tmp_win, x, y);
      EWMH_SetWMState(tmp_win, False);
    }
  }
}
