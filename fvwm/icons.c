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
#include <stdio.h>
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
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

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

static int do_all_iconboxes(FvwmWindow *t, icon_boxes **icon_boxes_ptr);
static void GetBitmapFile(FvwmWindow *tmp_win);
static void GetXPMFile(FvwmWindow *tmp_win);



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

  SET_ICON_OURS(tmp_win, 1);
  SET_PIXMAP_OURS(tmp_win, 0);
  SET_ICON_SHAPED(tmp_win, 0);
  tmp_win->icon_pixmap_w = None;
  tmp_win->iconPixmap = None;
  tmp_win->iconDepth = 0;

  if (IS_ICON_SUPPRESSED(tmp_win))
    return;

  /* First, see if it was specified in the .fvwmrc */
  tmp_win->icon_p_height = 0;
  tmp_win->icon_p_width = 0;

  /* First, check for a monochrome bitmap */
  if (tmp_win->icon_bitmap_file != NULL)
    GetBitmapFile(tmp_win);

#ifdef XPM
  /* Next, check for a color pixmap */
  if ((tmp_win->icon_bitmap_file != NULL) && (tmp_win->icon_p_height == 0)
      && (tmp_win->icon_p_width == 0))
    GetXPMFile(tmp_win);
#endif /* XPM */

  /* Next, See if the app supplies its own icon window */
  if ((tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0)
      && (tmp_win->wmhints) && (tmp_win->wmhints->flags & IconWindowHint))
    GetIconWindow(tmp_win);

  /* Finally, try to get icon bitmap from the application */
  if ((tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0)
      && (tmp_win->wmhints) && (tmp_win->wmhints->flags & IconPixmapHint))
    GetIconBitmap(tmp_win);

  /* figure out the icon window size */
  if (!(HAS_NO_ICON_TITLE(tmp_win))||(tmp_win->icon_p_height == 0))
  {
    tmp_win->icon_t_width =
      XTextWidth(tmp_win->icon_font.font, tmp_win->icon_name,
		 strlen(tmp_win->icon_name));
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
    tmp_win->icon_p_width += 4;
    tmp_win->icon_p_height += 4;
  }

  if (tmp_win->icon_p_width == 0)
    tmp_win->icon_p_width = tmp_win->icon_t_width + 6;
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
  attributes.event_mask = (ButtonPressMask | ButtonReleaseMask
			   | VisibilityChangeMask | ExposureMask | KeyPressMask
			   | EnterWindowMask | LeaveWindowMask
			   | FocusChangeMask );
  if (!(HAS_NO_ICON_TITLE(tmp_win)) || (tmp_win->icon_p_height == 0))
  {
    tmp_win->icon_w =
      XCreateWindow(
	dpy, Scr.Root, def_x, def_y + tmp_win->icon_p_height,
	tmp_win->icon_g.width, tmp_win->icon_g.height, 0, Pdepth,
	InputOutput, Pvisual, valuemask, &attributes);
  }
  if (Scr.DefaultColorset >= 0)
    SetWindowBackground(dpy, tmp_win->icon_w, tmp_win->icon_g.width,
			tmp_win->icon_g.height, &Colorset[Scr.DefaultColorset],
			Pdepth, Scr.StdGC, False);

  /* create a window to hold the picture */
  if((IS_ICON_OURS(tmp_win)) && (tmp_win->icon_p_width > 0)
     && (tmp_win->icon_p_height > 0))
  {
    /* use fvwm's visuals in these cases */
    if (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win))
    {
      tmp_win->icon_pixmap_w =
	XCreateWindow(
	  dpy, Scr.Root, def_x, def_y, tmp_win->icon_p_width,
	  tmp_win->icon_p_height, 0, Pdepth, InputOutput, Pvisual, valuemask,
	  &attributes);
      if (Scr.DefaultColorset >= 0)
	SetWindowBackground(dpy, tmp_win->icon_w, tmp_win->icon_p_width,
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
    attributes.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask
      | VisibilityChangeMask | FocusChangeMask
      | EnterWindowMask | LeaveWindowMask;
    valuemask = CWEventMask;
    XChangeWindowAttributes(dpy, tmp_win->icon_pixmap_w, valuemask,&attributes);
  }


#ifdef SHAPE
  if (ShapesSupported && IS_ICON_SHAPED(tmp_win))
  {
    /* when fvwm is using the non-default visual client supplied icon pixmaps
     * are drawn in a window with no relief */
    int off = (Pdefault || (tmp_win->iconDepth == 1) || IS_PIXMAP_OURS(tmp_win))
      ? 2 : 0;
    XShapeCombineMask(dpy, tmp_win->icon_pixmap_w, ShapeBounding, off, off,
		      tmp_win->icon_maskPixmap, ShapeSet);
  }
#endif

  if(tmp_win->icon_w != None)
  {
    XSaveContext(dpy, tmp_win->icon_w, FvwmContext, (caddr_t)tmp_win);
    XDefineCursor(dpy, tmp_win->icon_w, Scr.FvwmCursors[CRS_DEFAULT]);
    GrabAllWindowKeysAndButtons(dpy, tmp_win->icon_w, Scr.AllBindings,
				C_ICON, GetUnusedModifiers(),
				Scr.FvwmCursors[CRS_DEFAULT], True);

    xwc.sibling = tmp_win->frame;
    xwc.stack_mode = Below;
    XConfigureWindow(dpy, tmp_win->icon_w, CWSibling|CWStackMode, &xwc);
  }
  if(tmp_win->icon_pixmap_w != None)
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
  int x;
  color_quad *draw_colors;

  if(IS_ICON_SUPPRESSED(tmp_win))
    return;

  if(tmp_win->icon_w != None)
    flush_expose (tmp_win->icon_w);
  if(tmp_win->icon_pixmap_w != None)
    flush_expose (tmp_win->icon_pixmap_w);

  if(Scr.Hilite == tmp_win)
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

  if(tmp_win->icon_w != None)
  {
    if (IS_ICON_ENTERED(tmp_win))
    {
      /* resize the icon name window */
      tmp_win->icon_g.width = tmp_win->icon_t_width + 8;
      if (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win))
	tmp_win->icon_g.width += 10;
      if(tmp_win->icon_g.width < tmp_win->icon_p_width)
	tmp_win->icon_g.width = tmp_win->icon_p_width;
      tmp_win->icon_xl_loc = tmp_win->icon_g.x -
	(tmp_win->icon_g.width - tmp_win->icon_p_width)/2;
      /* start keep label on screen. dje 8/7/97 */
      if (tmp_win->icon_xl_loc < 0) {  /* if new loc neg (off left edge) */
	tmp_win->icon_xl_loc = 0;      /* move to edge */
      } else {                         /* if not on left edge */
	/* if (new loc + width) > screen width (off edge on right) */
	if ((tmp_win->icon_xl_loc + tmp_win->icon_g.width) >
	    Scr.MyDisplayWidth) {      /* off right */
	  /* position up against right edge */
	  tmp_win->icon_xl_loc = Scr.MyDisplayWidth-tmp_win->icon_g.width;
	}
	/* end keep label on screen. dje 8/7/97 */
      }
    }
    else
    {
      /* resize the icon name window */
      tmp_win->icon_g.width = tmp_win->icon_p_width;
      tmp_win->icon_xl_loc = tmp_win->icon_g.x;
    }
  }

  /* set up TitleGC for drawing the icon label */
  if (tmp_win->icon_font.font != None)
    NewFontAndColor(tmp_win->icon_font.font->fid, TextColor, BackColor);
  if(tmp_win->icon_w != None)
  {
    tmp_win->icon_g.height = ICON_HEIGHT(tmp_win);
    XMoveResizeWindow(dpy, tmp_win->icon_w, tmp_win->icon_xl_loc,
                      tmp_win->icon_g.y + tmp_win->icon_p_height,
                      tmp_win->icon_g.width,ICON_HEIGHT(tmp_win));
    XSetWindowBackground(dpy, tmp_win->icon_w, BackColor);
    XClearWindow(dpy,tmp_win->icon_w);

    /* text position */
    x = (tmp_win->icon_g.width - tmp_win->icon_t_width) / 2;
    if(x < 3)
      x = 3;
    if ((IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win)))
    {
      XRectangle r;

      if (x < 7)
	x = 7;
      r.x = 0;
      r.y = 0;
      r.width = tmp_win->icon_g.width - 7;
      r.height = ICON_HEIGHT(tmp_win);
      XSetClipRectangles(dpy, Scr.TitleGC, 0, 0, &r, 1, Unsorted);
    }
#ifdef I18N_MB
    XmbDrawString (dpy, tmp_win->icon_w, tmp_win->icon_font.fontset,
#else
    XDrawString (dpy, tmp_win->icon_w,
#endif
		 Scr.TitleGC, x,
		 tmp_win->icon_g.height - tmp_win->icon_font.height +
		 tmp_win->icon_font.y - 3, tmp_win->icon_name,
		 strlen(tmp_win->icon_name));
    RelieveRectangle(dpy, tmp_win->icon_w, 0, 0, tmp_win->icon_g.width - 1,
		     ICON_HEIGHT(tmp_win) - 1, Relief, Shadow, 2);
    if (IS_STICKY(tmp_win) || IS_ICON_STICKY(tmp_win))
    {
      /* an odd number of lines every 4 pixels */
      int num = (ICON_HEIGHT(tmp_win) / 8) * 2 - 1;
      int min = ICON_HEIGHT(tmp_win) / 2 - num * 2 + 1;
      int max = ICON_HEIGHT(tmp_win) / 2 + num * 2 - 3;
      int i;
      int stipple_w = x - 6;

      if (stipple_w < 3)
	stipple_w = 3;
      for(i = min; i <= max; i += 4)
      {
	RelieveRectangle(
	  dpy, tmp_win->icon_w, 2, i, stipple_w, 1, Shadow, Relief, 1);
	RelieveRectangle(
	  dpy, tmp_win->icon_w, tmp_win->icon_g.width - 3 - stipple_w, i,
	  stipple_w, 1, Shadow, Relief, 1);
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
    RelieveRectangle(dpy, tmp_win->icon_pixmap_w, 0, 0,
		     tmp_win->icon_p_width - 1, tmp_win->icon_p_height - 1,
		     Relief, Shadow, 2);

  /* need to locate the icon pixmap */
  if (tmp_win->iconPixmap != None)
  {
    if (tmp_win->iconDepth == 1)
    {
      /* it's a bitmap */
      XCopyPlane(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w,
		 Scr.TitleGC, 0, 0, tmp_win->icon_p_width - 4,
		 tmp_win->icon_p_height - 4, 2, 2, 1);
    }
    else
    {
      if (Pdefault || IS_PIXMAP_OURS(tmp_win))
      {
        /* it's a pixmap that need copying */
	XCopyArea(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w,
		  Scr.TitleGC, 0, 0, tmp_win->icon_p_width - 4,
		  tmp_win->icon_p_height - 4, 2, 2);
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

  if (IS_ICON_ENTERED(tmp_win))
  {
    if (tmp_win->icon_w != None)
    {
      XRaiseWindow (dpy, tmp_win->icon_w);
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
    if (tmp_win->icon_w != None)
    {
      XConfigureWindow(dpy, tmp_win->icon_w, mask, &xwc);
    }
    if (tmp_win->icon_pixmap_w != None)
    {
      XConfigureWindow(dpy, tmp_win->icon_pixmap_w, mask, &xwc);
    }
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

  if (tmp_win->icon_w == (int)NULL)
    return;

  tmp_win->icon_t_width = XTextWidth(tmp_win->icon_font.font,tmp_win->icon_name,
				     strlen(tmp_win->icon_name));
  /* clear the icon window, and trigger a re-draw via an expose event */
  if (IS_ICONIFIED(tmp_win))
    DrawIconWindow(tmp_win);
  if (IS_ICONIFIED(tmp_win))
    XClearArea(dpy, tmp_win->icon_w, 0, 0, 0, 0, True);
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
  int real_x=10, real_y=10;
  int new_x, new_y;

  /* New! Put icon in same page as the center of the window */
  /* Not a good idea for StickyIcons. Neither for icons of windows that are
   * visible on the current page. */
  if(IS_ICON_STICKY(t)||IS_STICKY(t))
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
    /* just make sure the icon is on this screen */
    t->icon_g.x = t->icon_g.x % Scr.MyDisplayWidth + base_x;
    t->icon_g.y = t->icon_g.y % Scr.MyDisplayHeight + base_y;
    if(t->icon_g.x < 0)
      t->icon_g.x += Scr.MyDisplayWidth;
    if(t->icon_g.y < 0)
      t->icon_g.y += Scr.MyDisplayHeight;
  }
  else if (t->wmhints && t->wmhints->flags & IconPositionHint)
  {
    t->icon_g.x = t->wmhints->icon_x;
    t->icon_g.y = t->wmhints->icon_y;
  }
  /* dje 10/12/97:
     Look thru chain of icon boxes assigned to window.
     Add logic for grids and fill direction.
  */
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
    } dimension;
    dimension dim[3];                   /* space for work, 1st, 2nd dimen */
    icon_boxes *icon_boxes_ptr;         /* current icon box */
    int i;                              /* index for inner/outer loop data */

    /* Hopefully this makes the following more readable. */
#define ICONBOX_LFT icon_boxes_ptr->IconBox[0]
#define ICONBOX_TOP icon_boxes_ptr->IconBox[1]
#define ICONBOX_RGT icon_boxes_ptr->IconBox[2]
#define ICONBOX_BOT icon_boxes_ptr->IconBox[3]
#define BOT_FILL icon_boxes_ptr->IconFlags & ICONFILLBOT
#define RGT_FILL icon_boxes_ptr->IconFlags & ICONFILLRGT
#define HRZ_FILL icon_boxes_ptr->IconFlags & ICONFILLHRZ

    /* unnecessary copy of width */
    width = t->icon_p_width;
    /* total height */
    height = t->icon_g.height + t->icon_p_height;
    /* no slot found yet */
    loc_ok = False;

    /* check all boxes in order */
    icon_boxes_ptr = NULL;              /* init */
    while(do_all_iconboxes(t, &icon_boxes_ptr))
    {
      if (loc_ok == True)
      {
	/* leave for loop */
        break;
      }
      /* y amount */
      dim[1].step = icon_boxes_ptr->IconGrid[1];
      /* init start from */
      dim[1].start_at = ICONBOX_TOP;
      /* init end at */
      dim[1].end_at = ICONBOX_BOT;
      /* save base */
      dim[1].base = base_y;
      /* save dimension */
      dim[1].icon_dimension = height;
      dim[1].screen_dimension = Scr.MyDisplayHeight;
      if (BOT_FILL)
      {
	/* fill from bottom */
	/* reverse step */
        dim[1].step = 0 - dim[1].step;
      } /* end fill from bottom */

      /* x amount */
      dim[2].step = icon_boxes_ptr->IconGrid[0];
      /* init start from */
      dim[2].start_at = ICONBOX_LFT;
      /* init end at */
      dim[2].end_at = ICONBOX_RGT;
      /* save base */
      dim[2].base   = base_x;
      /* save dimension */
      dim[2].icon_dimension = width;
      dim[2].screen_dimension = Scr.MyDisplayWidth;
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
      while((dim[1].step < 0            /* filling reversed */
             ? (dim[1].start_at + dim[1].icon_dimension - dim[1].nom_dimension
                > dim[1].end_at)        /* check back edge */
             : (dim[1].start_at + dim[1].nom_dimension
                < dim[1].end_at))       /* check front edge */
            && (!loc_ok)) {             /* nothing found yet */
        dim[1].real_start = dim[1].start_at; /* init */
        if (dim[1].start_at + dim[1].icon_dimension >
            dim[1].screen_dimension - 2 + dim[1].base)
	{
	  /* off screen, move on screen */
          dim[1].real_start = dim[1].screen_dimension
            - dim[1].icon_dimension + dim[1].base;
        } /* end off screen */
        if (dim[1].start_at < dim[1].base)
	{
	  /* if off other edge, move on screen */
          dim[1].real_start = dim[1].base;
        } /* end off other edge */
	/* reset inner loop */
        dim[2].start_at = dim[0].start_at;
        while((dim[2].step < 0            /* filling reversed */
               ? (dim[2].start_at + dim[2].icon_dimension-dim[2].nom_dimension
                  > dim[2].end_at)        /* check back edge */
               : (dim[2].start_at + dim[2].nom_dimension
                  < dim[2].end_at))       /* check front edge */
              && (!loc_ok)) {             /* nothing found yet */
          dim[2].real_start = dim[2].start_at; /* init */
          if (dim[2].start_at + dim[2].icon_dimension >
              dim[2].screen_dimension - 2 + dim[2].base)
	  {
	    /* if off screen, move on screen */
            dim[2].real_start = dim[2].screen_dimension
              - dim[2].icon_dimension + dim[2].base;
          } /* end off screen */
          if (dim[2].start_at < dim[2].base)
	  {
	    /* if off other edge, move on screen */
	    dim[2].real_start = dim[2].base;
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
          loc_ok = True;
          test_window = Scr.FvwmRoot.next;
          while((test_window != (FvwmWindow *)0)
                &&(loc_ok == True))
	  {
	    /* test overlap */
            if(test_window->Desk == t->Desk)
	    {
              if((IS_ICONIFIED(test_window)) &&
                 (!IS_TRANSIENT(test_window) ||
		  !IS_ICONIFIED_BY_PARENT(test_window)) &&
                 (test_window->icon_w||test_window->icon_pixmap_w) &&
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
                } /* end if icons overlap */
              } /* end if its an icon */
            } /* end if same desk */
            test_window = test_window->next;
          } /* end while icons that may overlap */
	  /* Grid inner value & direction */
          dim[2].start_at += dim[2].step;
        } /* end while room inner dimension */
	/* Grid outer value & direction */
        dim[1].start_at += dim[1].step;
      } /* end while room outer dimension */
    } /* end for all icon boxes, or found space */
    if(loc_ok == False)
      /* If icon never found a home just leave it */
      return;
    t->icon_g.x = real_x;
    t->icon_g.y = real_y;

    if(t->icon_pixmap_w)
      XMoveWindow(dpy,t->icon_pixmap_w,t->icon_g.x, t->icon_g.y);

    t->icon_g.width = t->icon_p_width;
    t->icon_xl_loc = t->icon_g.x;

    if (t->icon_w != None)
      XMoveResizeWindow(
	dpy, t->icon_w, t->icon_xl_loc, t->icon_g.y+t->icon_p_height,
	t->icon_g.width, ICON_HEIGHT(t));
    BroadcastPacket(M_ICON_LOCATION, 7,
                    t->w, t->frame,
                    (unsigned long)t,
                    t->icon_g.x, t->icon_g.y,
                    t->icon_p_width, t->icon_g.height+t->icon_p_height);
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
    /* Right now, the global box is hard-coded, fills the screen,
       uses an 80x80 grid, and fills top-bottom, left-right */
    global_icon_box_ptr = calloc(1, sizeof(icon_boxes));
    global_icon_box_ptr->IconBox[2] = Scr.MyDisplayHeight;
    global_icon_box_ptr->IconBox[3] = Scr.MyDisplayWidth;
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
static void GetXPMFile(FvwmWindow *tmp_win)
{
#ifdef XPM
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

#ifdef SHAPE
  if (ShapesSupported && tmp_win->icon_maskPixmap)
    SET_ICON_SHAPED(tmp_win, 1);
#endif

  XpmFreeXpmImage(&my_image);

#endif /* XPM */
}

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 ****************************************************************************/
void GetIconWindow(FvwmWindow *tmp_win)
{
  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  if(XGetGeometry(dpy,   tmp_win->wmhints->icon_window, &JunkRoot,
		  &JunkX, &JunkY,(unsigned int *)&tmp_win->icon_p_width,
		  (unsigned int *)&tmp_win->icon_p_height,
		  &JunkBW, &JunkDepth)==0)
  {
    fvwm_msg(ERR,"GetIconWindow","Help! Bad Icon Window!");
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
#ifdef SHAPE
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      SET_ICON_SHAPED(tmp_win, 1);
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
#endif
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
  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  XGetGeometry(dpy, tmp_win->wmhints->icon_pixmap, &JunkRoot, &JunkX, &JunkY,
	       &width, &height, &JunkBW, &depth);

  /* sanity check the pixmap depth, it must be the same as the root or 1 */
  if ((depth != 1) && (depth != DefaultDepth(dpy,Scr.screen)))
  {
    fvwm_msg(ERR, "GetIconBitmap", "Bad client icon pixmap depth %d", depth);
    return;
  }

  tmp_win->iconPixmap = tmp_win->wmhints->icon_pixmap;
  tmp_win->icon_p_width = width;
  tmp_win->icon_p_height = height;
  tmp_win->iconDepth = depth;

#ifdef SHAPE
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      SET_ICON_SHAPED(tmp_win, 1);
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
#endif

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

  if(!tmp_win)
    return;

  while (IS_ICONIFIED_BY_PARENT(tmp_win))
  {
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (t != tmp_win && tmp_win->transientfor == t->w)
	tmp_win = t;
    }
  }

  /* AS dje  RaiseWindow(tmp_win); */
  /* now de-iconify transients */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if ((t == tmp_win)||
	((IS_TRANSIENT(t)) && (t->transientfor == tmp_win->w)))
    {
      SET_MAPPED(t, 1);
      SET_ICONIFIED_BY_PARENT(t, 0);
      if(Scr.Hilite == t)
	DrawDecorations(t, DRAW_ALL, False, True, None);

      /* AS stuff starts here dje */
      if (t->icon_pixmap_w)
	XUnmapWindow(dpy, t->icon_pixmap_w);
      if (t->icon_w)
	XUnmapWindow(dpy, t->icon_w);
      XFlush(dpy);
      /* End AS */
      XMapWindow(dpy, t->w);
      if(t->Desk == Scr.CurrentDesk)
      {
	rectangle r;

	r.x = t->icon_g.x;
	r.y = t->icon_g.y;
	r.width = t->icon_g.width;
	r.height = t->icon_p_height+t->icon_g.height;
	if (IsRectangleOnThisPage(&r, t->Desk) &&
	    !IsRectangleOnThisPage(&(t->frame_g), t->Desk))
	{
	  /* Make sure we keep it on screen when de-iconifying. */
	  t->frame_g.x -=
	    truncate_to_multiple(t->frame_g.x,Scr.MyDisplayWidth);
	  t->frame_g.y -=
	    truncate_to_multiple(t->frame_g.y,Scr.MyDisplayHeight);
	  XMoveWindow(dpy, t->frame, t->frame_g.x, t->frame_g.y);
	}
      }
      if (t == tmp_win)
	BroadcastPacket(M_DEICONIFY, 11,
			t->w, t->frame,
			(unsigned long)t,
			t->icon_g.x, t->icon_g.y,
			t->icon_p_width, t->icon_p_height+t->icon_g.height,
			t->frame_g.x, t->frame_g.y,
			t->frame_g.width, t->frame_g.height);
      else
	BroadcastPacket(M_DEICONIFY, 7,
			t->w, t->frame,
			(unsigned long)t,
			t->icon_g.x, t->icon_g.y,
			t->icon_p_width,
			t->icon_p_height+t->icon_g.height);
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
      DrawDecorations(t, DRAW_ALL, False, True, None);
      Scr.Hilite = tmp;
    }
  }

#if 1
  RaiseWindow(tmp_win); /* moved dje */
#endif

  if(HAS_CLICK_FOCUS(tmp_win))
    FocusOn(tmp_win, TRUE, "");
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
  XWindowAttributes winattrs = {0};
  unsigned long eventMask;

  if(!tmp_win)
    return;
  XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
  eventMask = winattrs.your_event_mask;

  if((tmp_win == Scr.Hilite)&&(HAS_CLICK_FOCUS(tmp_win))&&(tmp_win->next))
  {
    SetFocus(tmp_win->next->w,tmp_win->next,1);
  }

  /* iconify transients first */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if ((t==tmp_win)||((IS_TRANSIENT(t)) && (t->transientfor == tmp_win->w)))
    {
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
      if (t->icon_w)
	XUnmapWindow(dpy, t->icon_w);
      if (t->icon_pixmap_w)
	XUnmapWindow(dpy, t->icon_pixmap_w);

      SetMapStateProp(t, IconicState);
      DrawDecorations(t, DRAW_ALL, False, False, None);
      if (t == tmp_win)
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

  if (tmp_win->icon_w == None)
  {
    if(IS_ICON_MOVED(tmp_win))
      CreateIconWindow(tmp_win,tmp_win->icon_g.x,tmp_win->icon_g.y);
    else
      CreateIconWindow(tmp_win, def_x, def_y);
  }

  /* if no pixmap we want icon width to change to text width every iconify */
  if( (tmp_win->icon_w != None) && (tmp_win->icon_pixmap_w == None) ) {
    tmp_win->icon_t_width =
      XTextWidth(tmp_win->icon_font.font,tmp_win->icon_name,
                 strlen(tmp_win->icon_name));
    tmp_win->icon_p_width = tmp_win->icon_t_width+6 + 4;
    tmp_win->icon_g.width = tmp_win->icon_p_width;
  }

  /* this condition will be true unless we restore a window to
     iconified state from a saved session. */
  if (!(DO_START_ICONIC(tmp_win) && IS_ICON_MOVED(tmp_win)))
  {
    AutoPlaceIcon(tmp_win);
  }

  SET_ICONIFIED(tmp_win, 1);
  SET_ICON_UNMAPPED(tmp_win, 0);
  BroadcastPacket(M_ICONIFY, 11,
                  tmp_win->w, tmp_win->frame,
                  (unsigned long)tmp_win,
                  tmp_win->icon_g.x,
                  tmp_win->icon_g.y,
                  tmp_win->icon_g.width,
                  tmp_win->icon_g.height+tmp_win->icon_p_height,
                  tmp_win->frame_g.x, /* next 4 added for Animate module */
                  tmp_win->frame_g.y,
                  tmp_win->frame_g.width,
                  tmp_win->frame_g.height);
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);

  if (!(DO_START_ICONIC(tmp_win) && IS_ICON_MOVED(tmp_win)))
  {
    LowerWindow(tmp_win);
  }

  if(tmp_win->Desk == Scr.CurrentDesk)
  {
    if (tmp_win->icon_w != None)
      XMapWindow(dpy, tmp_win->icon_w);

    if(tmp_win->icon_pixmap_w != None)
      XMapWindow(dpy, tmp_win->icon_pixmap_w);
  }
  if(HAS_CLICK_FOCUS(tmp_win) || HAS_SLOPPY_FOCUS(tmp_win))
  {
    if (tmp_win == Scr.Focus)
    {
      if(Scr.PreviousFocus == Scr.Focus)
	Scr.PreviousFocus = NULL;
      if(HAS_CLICK_FOCUS(tmp_win) && tmp_win->next)
	SetFocus(tmp_win->next->w, tmp_win->next,1);
      else
	SetFocus(Scr.NoFocusWin, NULL,1);
    }
  }
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
  data[1] = (unsigned long) tmp_win->icon_w;
/*  data[2] = (unsigned long) tmp_win->icon_pixmap_w;*/

  XChangeProperty(dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32,
		  PropModeReplace, (unsigned char *) data, 2);
  return;
}


void iconify_function(F_CMD_ARGS)
{
  int toggle;

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
      DeIconify(tmp_win);
  }
  else
  {
    if (toggle == 1)
    {
      if(check_if_function_allowed(F_ICONIFY,tmp_win,True,0) == 0)
      {
	XBell(dpy, 0);
	return;
      }
      Iconify(tmp_win,eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
    }
  }
}
