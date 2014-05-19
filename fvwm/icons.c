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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 *
 * fvwm icon code
 *
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/Parse.h"
#include "libs/Picture.h"
#include "libs/Graphics.h"
#include "libs/PictureGraphics.h"
#include "libs/FRenderInit.h"
#include "libs/Rectangles.c"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "commands.h"
#include "bindings.h"
#include "events.h"
#include "eventmask.h"
#include "eventhandler.h"
#include "misc.h"
#include "screen.h"
#include "icons.h"
#include "borders.h"
#include "frame.h"
#include "focus.h"
#include "colormaps.h"
#include "stack.h"
#include "virtual.h"
#include "decorations.h"
#include "module_interface.h"
#include "ewmh.h"
#include "geometry.h"

static int do_all_iconboxes(FvwmWindow *t, icon_boxes **icon_boxes_ptr);
static void GetIconFromFile(FvwmWindow *fw);
static void GetIconWindow(FvwmWindow *fw);
static void GetIconBitmap(FvwmWindow *fw);

static void clear_icon_dimensions(FvwmWindow *fw)
{
	int px;
	int py;
	int tx;
	int ty;

	px = fw->icon_g.picture_w_g.x;
	py = fw->icon_g.picture_w_g.y;
	tx = fw->icon_g.title_w_g.x;
	ty = fw->icon_g.title_w_g.y;
	memset(&fw->icon_g, 0, sizeof(fw->icon_g));
	fw->icon_g.picture_w_g.x = px;
	fw->icon_g.picture_w_g.y = py;
	fw->icon_g.title_w_g.x = tx;
	fw->icon_g.title_w_g.y = ty;

	return;
}
/* erase all traces of the last used icon in the window structure */
void clear_icon(FvwmWindow *fw)
{
	FW_W_ICON_PIXMAP(fw) = None;
	fw->iconPixmap = None;
	fw->icon_maskPixmap = None;
	fw->icon_alphaPixmap = None;
	fw->icon_nalloc_pixels = 0;
	fw->icon_alloc_pixels = NULL;
	fw->icon_no_limit = 0;
	if (IS_ICON_MOVED(fw))
	{
		clear_icon_dimensions(fw);
	}
	else
	{
		memset(&fw->icon_g, 0, sizeof(fw->icon_g));
	}

	return;
}

int get_visible_icon_window_count(FvwmWindow *fw)
{
	int count = 0;

	if (fw == NULL || !IS_ICONIFIED(fw) ||
	    IS_ICON_SUPPRESSED(fw))
	{
		return 0;
	}
	if (FW_W_ICON_PIXMAP(fw) != None)
	{
		count++;
	}
	if (FW_W_ICON_TITLE(fw) != None)
	{
		count++;
	}

	return count;
}

void setup_icon_title_size(FvwmWindow *fw)
{
	if (HAS_NO_ICON_TITLE(fw))
	{
		fw->icon_g.title_text_width = 0;
		fw->icon_g.title_w_g.width = 0;
		fw->icon_g.title_w_g.height = 0;
	}
	else
	{
		fw->icon_g.title_text_width =
			FlocaleTextWidth(
				fw->icon_font, fw->visible_icon_name,
				strlen(fw->visible_icon_name));
		fw->icon_g.title_w_g.height = ICON_HEIGHT(fw);
		if (fw->icon_g.picture_w_g.width == 0)
		{
			fw->icon_g.title_w_g.width =
				fw->icon_g.title_text_width +
				2 * (ICON_TITLE_TEXT_GAP_COLLAPSED +
				     abs(fw->icon_title_relief));
			if (IS_STICKY_ACROSS_PAGES(fw) ||
			    IS_ICON_STICKY_ACROSS_PAGES(fw) ||
			    IS_STICKY_ACROSS_DESKS(fw) ||
			    IS_ICON_STICKY_ACROSS_DESKS(fw))
			{
				fw->icon_g.title_w_g.width +=
					2 * (ICON_TITLE_TO_STICK_EXTRA_GAP +
					     ICON_TITLE_STICK_MIN_WIDTH);
			}
		}
		else
		{
			fw->icon_g.title_w_g.width =
				fw->icon_g.picture_w_g.width;
		}
	}

	return;
}

/*
 *
 * Resizes the given icon Pixmap.
 *
 */
static void SetIconPixmapSize(
	Pixmap *icon, int width, int height, int depth, int newWidth,
	int newHeight, Bool force_centering, int resize_type, int *nrx,
	int *nry, int freeOldPixmap)
{
	Pixmap oldPixmap;
	Pixmap resizedPixmap = None;
	int r_w,r_h;
	GC gc;
	XGCValues gc_init;

	*nrx = 0;
	*nry = 0;

	/* Check for invalid dimensions */
	if (newWidth == 0 || newHeight == 0)
	{
		return;
	}

	/* Save the existing Pixmap */
	oldPixmap = *icon;

	gc = XCreateGC(dpy, oldPixmap, 0, &gc_init);

	switch(resize_type)
	{
	case ICON_RESIZE_TYPE_ADJUSTED:
		if (newWidth != width || newHeight != height)
		{
			*icon = CreateStretchPixmap(
				dpy, oldPixmap, width, height, depth, newWidth,
				newHeight, gc);
		}
		break;
	case ICON_RESIZE_TYPE_STRETCHED:
		if (width < newWidth || height < newHeight)
		{
			r_w = max(newWidth, width);
			r_h = max(newHeight, height);
			resizedPixmap = CreateStretchPixmap(
				dpy, oldPixmap, width, height, depth, r_w, r_h,
				gc);
			width = r_w;
			height = r_h;
		}
		break;
	case ICON_RESIZE_TYPE_SHRUNK:
		if (width > newWidth || height > newHeight)
		{
			r_w = min(newWidth, width);
			r_h = min(newHeight, height);
			resizedPixmap = CreateStretchPixmap(
				dpy, oldPixmap, width, height, depth, r_w, r_h,
				gc);
			width = r_w;
			height = r_h;
		}
		break;
	default:
		break;
	}

	if (resize_type != ICON_RESIZE_TYPE_ADJUSTED)
	{
		*icon = XCreatePixmap(
			dpy, oldPixmap, newWidth, newHeight, depth);
		XSetForeground(dpy, gc, 0);
		XFillRectangle(dpy, *icon, gc, 0, 0, newWidth, newHeight);

		/*
		 * Copy old Pixmap onto new.  Center horizontally.  Center
		 * vertically if the new height is smaller than the old.
		 * Otherwise, place the icon on the bottom, along the title bar.
		 */
		*nrx = (newWidth - width) / 2;
		*nry = (newHeight > height && !force_centering) ?
			newHeight - height : (newHeight - height) / 2;
		XCopyArea(
			dpy, (resizedPixmap)? resizedPixmap:oldPixmap, *icon,
			gc, 0, 0, width, height, *nrx, *nry);
	}

	XFreeGC(dpy, gc);

	if (freeOldPixmap)
	{
		XFreePixmap(dpy, oldPixmap);
	}
}

/* Move the icon of a window by dx/dy pixels */
/*
 *
 * Get the Icon for the icon window (also used by ewmh_icon)
 *
 */
void GetIconPicture(FvwmWindow *fw, Bool no_icon_window)
{
	char icon_order[4];
	int i;

	/* First, see if it was specified in the .fvwmrc */
	if (ICON_OVERRIDE_MODE(fw) == ICON_OVERRIDE)
	{
		/* try fvwm provided icons before application provided icons */
		icon_order[0] = 0;
		icon_order[1] = 1;
		icon_order[2] = 2;
		icon_order[3] = 3;
ICON_DBG((stderr,"ciw: hint order: file iwh iph '%s'\n", fw->name.name));
	}
	else if (ICON_OVERRIDE_MODE(fw) == NO_ACTIVE_ICON_OVERRIDE)
	{
		if (fw->wmhints && (fw->wmhints->flags & IconPixmapHint) &&
		    WAS_ICON_HINT_PROVIDED(fw) == ICON_HINT_MULTIPLE)
		{
			/* use application provided icon window or pixmap
			 * first, then fvwm provided icons. */
			icon_order[0] = 1;
			icon_order[1] = 2;
			icon_order[2] = 3;
			icon_order[3] = 0;
ICON_DBG((stderr,"ciw: hint order: iwh iph file '%s'\n", fw->name.name));
		}
		else if (Scr.DefaultIcon &&
			 fw->icon_bitmap_file == Scr.DefaultIcon)
		{
			/* use application provided icon window/pixmap first,
			 * then fvwm provided default icon */
			icon_order[0] = 1;
			icon_order[1] = 2;
			icon_order[2] = 3;
			icon_order[3] = 0;
ICON_DBG((stderr,"ciw: hint order: iwh iph file '%s'\n", fw->name.name));
		}
		else
		{
			/* use application provided icon window or ewmh icon
			 * first, then fvwm provided icons and then application
			 * provided icon pixmap */
			icon_order[0] = 1;
			icon_order[1] = 2;
			icon_order[2] = 0;
			icon_order[3] = 3;
ICON_DBG((stderr,"ciw: hint order: iwh file iph '%s'\n", fw->name.name));
		}
	}
	else
	{
		/* use application provided icon rather than fvwm provided
		 * icon */
		icon_order[0] = 1;
		icon_order[1] = 2;
		icon_order[2] = 3;
		icon_order[3] = 0;
ICON_DBG((stderr,"ciw: hint order: iwh iph file '%s'\n", fw->name.name));
	}

	fw->icon_g.picture_w_g.width = 0;
	fw->icon_g.picture_w_g.height = 0;
	fw->iconPixmap = None;
	fw->icon_maskPixmap = None;
	fw->icon_alphaPixmap= None;
	FW_W_ICON_PIXMAP(fw) = None;
	for (i = 0; i < 4 && fw->icon_g.picture_w_g.width == 0 &&
		     fw->icon_g.picture_w_g.height == 0; i++)
	{
		switch (icon_order[i])
		{
		case 0:
			/* Next, check for a color pixmap */
			if (fw->icon_bitmap_file)
			{
				GetIconFromFile(fw);
			}
ICON_DBG((stderr,"ciw: file%s used '%s'\n",
	  (fw->icon_g.picture_w_g.height)?"":" not", fw->name.name));
			break;
		case 1:
			/* Next, See if the app supplies its own icon window */
			if (no_icon_window)
			{
				break;
			}
			if (fw->wmhints &&
			    (fw->wmhints->flags & IconWindowHint))
			{
				GetIconWindow(fw);
			}
ICON_DBG((stderr,"ciw: iwh%s used '%s'\n",
	  (fw->icon_g.picture_w_g.height)?"":" not",fw->name.name));
			break;
		case 2:
			/* try an ewmh icon */
			if (HAS_EWMH_WM_ICON_HINT(fw) == EWMH_TRUE_ICON)
			{
				if (EWMH_SetIconFromWMIcon(fw, NULL, 0, False))
				{
					SET_USE_EWMH_ICON(fw, True);
				}
			}
ICON_DBG((stderr,"ciw: inh%s used '%s'\n",
	  (fw->icon_g.picture_w_g.height)?"":" not",fw->name.name));
			break;
		case 3:
			/* Finally, try to get icon bitmap from the
			 * application */
			if (fw->wmhints &&
			    (fw->wmhints->flags & IconPixmapHint))
			{
				GetIconBitmap(fw);
			}
ICON_DBG((stderr,"ciw: iph%s used '%s'\n",
	  (fw->icon_g.picture_w_g.height)?"":" not",fw->name.name));
			break;
		default:
			/* can't happen */
			break;
		}
	}

	/* Resize icon if necessary */
	if ((IS_ICON_OURS(fw)) && fw->icon_g.picture_w_g.height > 0 &&
	    fw->icon_g.picture_w_g.height > 0)
	{
		int newWidth = fw->icon_g.picture_w_g.width;
		int newHeight = fw->icon_g.picture_w_g.height;
		Boolean resize = False;

		if (newWidth < fw->min_icon_width)
		{
			newWidth = fw->min_icon_width;
			resize = True;
		}
		else
		{
			if (newWidth > fw->max_icon_width)
			{
				newWidth = fw->max_icon_width;
				resize = True;
			}
		}
		if (newHeight < fw->min_icon_height)
		{
			newHeight = fw->min_icon_height;
			resize = True;
		}
		else
		{
			if (newHeight > fw->max_icon_height)
			{
				newHeight = fw->max_icon_height;
				resize = True;
			}
		}
		if (resize)
		{
			/* Resize the icon Pixmap */
			int force_centering = False;
			int nrx, nry;

			ICON_DBG((stderr,
				  "ciw: Changing icon (%s) from %dx%d to"
				  " %dx%d\n", fw->name.name,
				  fw->icon_g.picture_w_g.width,
				  fw->icon_g.picture_w_g.height,
				  newWidth, newHeight));
			/* Resize the icon Pixmap */
			/* force to center if the icon has a bg */
			if (fw->icon_background_cs >= 0 ||
			    fw->icon_maskPixmap == None)
			{
				force_centering = True;
			}
			SetIconPixmapSize(
				&(fw->iconPixmap),
				fw->icon_g.picture_w_g.width,
				fw->icon_g.picture_w_g.height, fw->iconDepth,
				newWidth, newHeight, force_centering,
				fw->icon_resize_type, &nrx, &nry,
				IS_PIXMAP_OURS(fw));

			/* Resize the icon mask Pixmap if one was defined */
			if (fw->icon_maskPixmap)
			{
				SetIconPixmapSize(
					&(fw->icon_maskPixmap),
					fw->icon_g.picture_w_g.width,
					fw->icon_g.picture_w_g.height, 1,
					newWidth, newHeight, force_centering,
					fw->icon_resize_type, &nrx, &nry,
					IS_PIXMAP_OURS(fw));
			}
			else if ((nrx > 0 || nry > 0) && fw->iconDepth > 1)
			{
				fw->icon_maskPixmap = XCreatePixmap(
					dpy, fw->iconPixmap,
					newWidth, newHeight, 1);
				XSetForeground(dpy, Scr.MonoGC, 0);
				XFillRectangle(
					dpy, fw->icon_maskPixmap, Scr.MonoGC,
					0, 0, newWidth, newHeight);
				XSetForeground(dpy, Scr.MonoGC, 1);
				XFillRectangle(
					dpy, fw->icon_maskPixmap, Scr.MonoGC,
					nrx, nry, fw->icon_g.picture_w_g.width,
					fw->icon_g.picture_w_g.height);
				XSetForeground(dpy, Scr.MonoGC, 0);
				/* set it shaped ? YES */
				SET_ICON_SHAPED(fw, 1);
			}

			/* Resize the icon alpha Pixmap if one was defined */
			if (fw->icon_alphaPixmap)
			{
				SetIconPixmapSize(
					&(fw->icon_alphaPixmap),
					fw->icon_g.picture_w_g.width,
					fw->icon_g.picture_w_g.height,
					FRenderGetAlphaDepth(), newWidth,
					newHeight, force_centering,
					fw->icon_resize_type, &nrx, &nry,
					IS_PIXMAP_OURS(fw));
			}

			/* Set the new dimensions of the icon window */
			fw->icon_g.picture_w_g.width = newWidth;
			fw->icon_g.picture_w_g.height = newHeight;
		}
	}

	return;
}

/*
 *
 * set the icon pixmap window background
 *
 */
static void set_icon_pixmap_background(FvwmWindow *fw)
{
	if (fw->iconPixmap != None &&
	    (Pdefault || fw->iconDepth == 1 || fw->iconDepth == Pdepth ||
	     IS_PIXMAP_OURS(fw)))
	{
		if (fw->icon_background_cs >= 0)
		{
			SetWindowBackground(
				dpy, FW_W_ICON_PIXMAP(fw),
				fw->icon_g.picture_w_g.width,
				fw->icon_g.picture_w_g.height,
				&Colorset[fw->icon_background_cs],
				Pdepth, Scr.StdGC, False);
		}
		else if (FShapesSupported &&
			 Pdepth == DefaultDepth(dpy, (DefaultScreen(dpy))))
		{
			XSetWindowBackgroundPixmap(
				dpy, FW_W_ICON_PIXMAP(fw), ParentRelative);
		}
		else if (Scr.DefaultColorset >= 0)
		{
			SetWindowBackground(
				dpy, FW_W_ICON_PIXMAP(fw),
				fw->icon_g.picture_w_g.width,
				fw->icon_g.picture_w_g.height,
				&Colorset[Scr.DefaultColorset], Pdepth,
				Scr.StdGC, False);
		}
	}
}

/*
 *
 * Creates an icon window as needed
 *
 */
void CreateIconWindow(FvwmWindow *fw, int def_x, int def_y)
{
	/* mask for create windows */
	unsigned long valuemask;
	/* attributes for create windows */
	XSetWindowAttributes attributes;
	XWindowChanges xwc;
	Window old_icon_pixmap_w;
	Window old_icon_w;
	Bool is_old_icon_shaped = IS_ICON_SHAPED(fw);

	old_icon_w = FW_W_ICON_TITLE(fw);
	old_icon_pixmap_w = (IS_ICON_OURS(fw)) ? FW_W_ICON_PIXMAP(fw) : None;
	if (!IS_ICON_OURS(fw) && FW_W_ICON_PIXMAP(fw))
	{
		XUnmapWindow(dpy, FW_W_ICON_PIXMAP(fw));
	}
	SET_ICON_OURS(fw, 1);
	SET_PIXMAP_OURS(fw, 0);
	SET_ICON_SHAPED(fw, 0);
	FW_W_ICON_PIXMAP(fw) = None;
	fw->iconPixmap = None;
	fw->iconDepth = 0;

	if (IS_ICON_SUPPRESSED(fw))
	{
		return;
	}

	/*
	 * set up the icon picture
	 */
	GetIconPicture(fw, False);
	/* make space for relief to be drawn outside the icon */
	/* this does not happen if fvwm is using a non-default visual (with
	 * private colormap) and the client has supplied a pixmap (not a
	 * bitmap) */
	if ((IS_ICON_OURS(fw)) && (fw->icon_g.picture_w_g.height > 0)
	    && (Pdefault || fw->iconDepth == 1 || fw->iconDepth == Pdepth ||
		IS_PIXMAP_OURS(fw)))
	{
		fw->icon_g.picture_w_g.width +=
			2 * abs(fw->icon_background_relief)
			+ 2 * fw->icon_background_padding;
		fw->icon_g.picture_w_g.height +=
			2 * abs(fw->icon_background_relief)
			+ 2 * fw->icon_background_padding;
	}

	/*
	 * set up the icon title geometry
	 */
	setup_icon_title_size(fw);

	/*
	 * set up icon position
	 */
	set_icon_position(fw, def_x, def_y);

	/*
	 * create the icon title window
	 */
	valuemask = CWColormap | CWBorderPixel | CWBackPixel | CWCursor |
		CWEventMask;
	attributes.colormap = Pcmap;
	attributes.background_pixel = Scr.StdBack;
	attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
	attributes.border_pixel = 0;
	attributes.event_mask = XEVMASK_ICONW;
	if (HAS_NO_ICON_TITLE(fw))
	{
		if (FW_W_ICON_TITLE(fw))
		{
			XDeleteContext(dpy, FW_W_ICON_TITLE(fw), FvwmContext);
			XDestroyWindow(dpy, FW_W_ICON_TITLE(fw));
			XFlush(dpy);
			FW_W_ICON_TITLE(fw) = None;
		}
	}
	else
	{
		if (FW_W_ICON_TITLE(fw) == None)
		{
			FW_W_ICON_TITLE(fw) = XCreateWindow(
				dpy, Scr.Root, fw->icon_g.title_w_g.x,
				fw->icon_g.title_w_g.y,
				fw->icon_g.title_w_g.width,
				fw->icon_g.title_w_g.height, 0, Pdepth,
				InputOutput, Pvisual, valuemask, &attributes);
		}
		else
		{
			XMoveResizeWindow(
				dpy, FW_W_ICON_TITLE(fw),
				fw->icon_g.title_w_g.x, fw->icon_g.title_w_g.y,
				fw->icon_g.title_w_g.width,
				fw->icon_g.title_w_g.height);
		}
	}
	if (Scr.DefaultColorset >= 0)
	{
		SetWindowBackground(
			dpy, FW_W_ICON_TITLE(fw), fw->icon_g.title_w_g.width,
			fw->icon_g.title_w_g.height,
			&Colorset[Scr.DefaultColorset], Pdepth, Scr.StdGC,
			False);
	}

	/*
	 * create the icon picture window
	 */
	if (IS_ICON_OURS(fw) && fw->icon_g.picture_w_g.width > 0 &&
	    fw->icon_g.picture_w_g.height > 0)
	{
		/* use fvwm's visuals in these cases */
		if (Pdefault || fw->iconDepth == 1 || fw->iconDepth == Pdepth ||
		    IS_PIXMAP_OURS(fw))
		{
			if (!old_icon_pixmap_w)
			{
				FW_W_ICON_PIXMAP(fw) = XCreateWindow(
					dpy, Scr.Root, fw->icon_g.picture_w_g.x,
					fw->icon_g.picture_w_g.y,
					fw->icon_g.picture_w_g.width,
					fw->icon_g.picture_w_g.height, 0,
					Pdepth, InputOutput, Pvisual,
					valuemask, &attributes);
			}
			else
			{
				FW_W_ICON_PIXMAP(fw) = old_icon_pixmap_w;
				XMoveResizeWindow(
					dpy, FW_W_ICON_PIXMAP(fw),
					fw->icon_g.picture_w_g.x,
					fw->icon_g.picture_w_g.y,
					fw->icon_g.picture_w_g.width,
					fw->icon_g.picture_w_g.height);
			}
			set_icon_pixmap_background(fw);
		}
		else
		{
			/* client supplied icon pixmap and fvwm is using
			 * another visual.
			 * use it as the background pixmap, don't try to put
			 * relief on it because fvwm will not have the correct
			 * colors the Exceed server has problems maintaining
			 * the icon window, it usually fails to refresh the
			 * icon leaving it black so ask for expose events */
			attributes.background_pixmap = fw->iconPixmap;
			attributes.colormap = DefaultColormap(dpy, Scr.screen);
			valuemask &= ~CWBackPixel;
			valuemask |= CWBackPixmap;
			FW_W_ICON_PIXMAP(fw) = XCreateWindow(
				dpy, Scr.Root, fw->icon_g.picture_w_g.x,
				fw->icon_g.picture_w_g.y,
				fw->icon_g.picture_w_g.width,
				fw->icon_g.picture_w_g.height, 0,
				DefaultDepth(dpy, Scr.screen), InputOutput,
				DefaultVisual(dpy, Scr.screen), valuemask,
				&attributes);
		}
	}
	else if (FW_W_ICON_PIXMAP(fw) != None)
	{
		/* client supplied icon window: select events on it */
		attributes.event_mask = XEVMASK_ICONPW;
		valuemask = CWEventMask;
		XChangeWindowAttributes(
			dpy, FW_W_ICON_PIXMAP(fw), valuemask,&attributes);
		if (!IS_ICON_OURS(fw))
		{
			XMoveWindow(
				dpy, FW_W_ICON_PIXMAP(fw),
				fw->icon_g.picture_w_g.x,
				fw->icon_g.picture_w_g.y);
		}
	}
	if (old_icon_pixmap_w != None &&
	    old_icon_pixmap_w != FW_W_ICON_PIXMAP(fw))
	{
		/* destroy the old window */
		XDestroyWindow(dpy, old_icon_pixmap_w);
		XDeleteContext(dpy, old_icon_pixmap_w, FvwmContext);
		XFlush(dpy);
		is_old_icon_shaped = False;
	}

	if (FShapesSupported)
	{
		if (IS_ICON_SHAPED(fw) && fw->icon_background_cs < 0)
		{
			/* when fvwm is using the non-default visual client
			 * supplied icon pixmaps are drawn in a window with no
			 * relief */
			int off = 0;

			if (Pdefault || fw->iconDepth == 1 ||
			    fw->iconDepth == Pdepth || IS_PIXMAP_OURS(fw))
			{
				off = abs(fw->icon_background_relief) +
					fw->icon_background_padding;
			}
			SUPPRESS_UNUSED_VAR_WARNING(off);
			FShapeCombineMask(
				dpy, FW_W_ICON_PIXMAP(fw), FShapeBounding, off,
				off, fw->icon_maskPixmap, FShapeSet);
		}
		else if (is_old_icon_shaped &&
			 FW_W_ICON_PIXMAP(fw) == old_icon_pixmap_w)
		{
			/* remove the shape */
			XRectangle r;

			r.x = 0;
			r.y = 0;
			r.width = fw->icon_g.picture_w_g.width;
			r.height = fw->icon_g.picture_w_g.height;
			SUPPRESS_UNUSED_VAR_WARNING(r);
			FShapeCombineRectangles(
				dpy, FW_W_ICON_PIXMAP(fw), FShapeBounding, 0,
				0, &r, 1, FShapeSet, 0);
		}
	}

	if (FW_W_ICON_TITLE(fw) != None && FW_W_ICON_TITLE(fw) != old_icon_w)
	{
		XSaveContext(
			dpy, FW_W_ICON_TITLE(fw), FvwmContext, (caddr_t)fw);
		XDefineCursor(
			dpy, FW_W_ICON_TITLE(fw), Scr.FvwmCursors[CRS_DEFAULT]);
		GrabAllWindowKeysAndButtons(
			dpy, FW_W_ICON_TITLE(fw), Scr.AllBindings, C_ICON,
			GetUnusedModifiers(), Scr.FvwmCursors[CRS_DEFAULT],
			True);
		xwc.sibling = FW_W_FRAME(fw);
		xwc.stack_mode = Below;
		XConfigureWindow(
			dpy, FW_W_ICON_TITLE(fw), CWSibling|CWStackMode, &xwc);
	}
	if (FW_W_ICON_PIXMAP(fw) != None &&
	    FW_W_ICON_PIXMAP(fw) != old_icon_pixmap_w)
	{
		XSaveContext(
			dpy, FW_W_ICON_PIXMAP(fw), FvwmContext, (caddr_t)fw);
		XDefineCursor(
			dpy, FW_W_ICON_PIXMAP(fw),
			Scr.FvwmCursors[CRS_DEFAULT]);
		GrabAllWindowKeysAndButtons(
			dpy, FW_W_ICON_PIXMAP(fw), Scr.AllBindings, C_ICON,
			GetUnusedModifiers(), Scr.FvwmCursors[CRS_DEFAULT],
			True);
		xwc.sibling = FW_W_FRAME(fw);
		xwc.stack_mode = Below;
		XConfigureWindow(
			dpy,FW_W_ICON_PIXMAP(fw),CWSibling|CWStackMode,&xwc);
	}

	return;
}

/*
 *
 * Draws the icon window
 *
 */
static
void DrawIconTitleWindow(
	FvwmWindow *fw, XEvent *pev, Pixel BackColor, GC Shadow, GC Relief,
	int cs, int title_cs)
{
	int is_expanded = IS_ICON_ENTERED(fw);
	FlocaleWinString fstr;
	Region region = None;
	XRectangle clip, r;
	int relief = abs(fw->icon_title_relief);
	int x_title;
	int x_title_min = 0;
	int w_title = fw->icon_g.title_text_width;
	int x_title_w = fw->icon_g.picture_w_g.x;
	int w_title_w = fw->icon_g.picture_w_g.width;
	int x_stipple = relief;
	int w_title_text_gap = 0;
	int w_stipple = 0;
	int is_sticky;
	int is_stippled;
	int use_unexpanded_size = 1;
	Bool draw_string = True;

	is_sticky =
		(IS_STICKY_ACROSS_PAGES(fw) || IS_ICON_STICKY_ACROSS_PAGES(fw));
	is_sticky |=
		(IS_STICKY_ACROSS_DESKS(fw) || IS_ICON_STICKY_ACROSS_DESKS(fw));
	is_stippled = ((is_sticky && HAS_STICKY_STIPPLED_ICON_TITLE(fw)) ||
		HAS_STIPPLED_ICON_TITLE(fw));
	if (is_expanded && FW_W_ICON_PIXMAP(fw) != None)
	{
		int sx;
		int sy;
		int sw;
		int sh;

		use_unexpanded_size = 0;
		w_title_text_gap = ICON_TITLE_TEXT_GAP_EXPANDED;
		x_title_min = w_title_text_gap + relief;
		if (is_stippled)
		{
			w_stipple = ICON_TITLE_STICK_MIN_WIDTH;
			x_title_min +=
				w_stipple + ICON_TITLE_TO_STICK_EXTRA_GAP;
		}
		/* resize the icon name window */
		w_title_w = w_title + 2 * x_title_min;
		if (w_title_w <= fw->icon_g.picture_w_g.width)
		{
			/* the expanded title is smaller, so do not
			 * expand at all */
			is_expanded = 1;
			w_stipple = 0;
			use_unexpanded_size = 1;
		}
		else
		{
			x_title_w = fw->icon_g.picture_w_g.x -
				(w_title_w - fw->icon_g.picture_w_g.width) / 2;
			FScreenGetScrRect(NULL, FSCREEN_CURRENT,
					  &sx, &sy, &sw, &sh);
			/* start keep label on screen. dje 8/7/97 */
			if (x_title_w < sx) {
				/* if new loc neg (off left edge) */
				x_title_w = sx;      /* move to edge */
			}
			else
			{
				/* if not on left edge */
				/* if (new loc + width) > screen width
				 * (off edge on right) */
				if ((x_title_w + w_title_w) >sx + sw) {
					/* off right */
					/* position up against right
					 * edge */
					x_title_w = sx + sw - w_title_w;
				}
				/* end keep label on screen. dje
				 * 8/7/97 */
			}
		}
	}
	if (use_unexpanded_size)
	{
		w_title_text_gap = ICON_TITLE_TEXT_GAP_COLLAPSED;
		x_title_min = w_title_text_gap + relief;
		/* resize the icon name window */
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			w_title_w = fw->icon_g.picture_w_g.width;
			x_title_w = fw->icon_g.picture_w_g.x;
		}
		else
		{
			w_title_w = fw->icon_g.title_w_g.width;
			x_title_w = fw->icon_g.title_w_g.x;
		}
	}

	if (fw->icon_g.title_w_g.width != w_title_w ||
	    fw->icon_g.title_w_g.x != x_title_w ||
	    fw->icon_g.title_w_g.height != ICON_HEIGHT(fw))
	{
		fw->icon_g.title_w_g.width = w_title_w;
		fw->icon_g.title_w_g.x = x_title_w;
		fw->icon_g.title_w_g.height = ICON_HEIGHT(fw);
		pev = NULL; /* resize && redraw all */
	}

	if (!pev)
	{
		XMoveResizeWindow(
			dpy, FW_W_ICON_TITLE(fw), fw->icon_g.title_w_g.x,
			fw->icon_g.title_w_g.y, w_title_w,
			ICON_HEIGHT(fw));
	}

	if (title_cs >= 0)
	{
		SetWindowBackground(
			dpy, FW_W_ICON_TITLE(fw), w_title_w,
			ICON_HEIGHT(fw), &Colorset[title_cs], Pdepth,
			Scr.TitleGC, False);
	}
	else
	{
		XSetWindowBackground(
			dpy, FW_W_ICON_TITLE(fw), BackColor);
	}

	/* text position */
	x_title = (w_title_w - w_title) / 2;
	if (x_title < x_title_min)
		x_title = x_title_min;
	/* text rectangle */
	r.x = x_title;
	r.y = relief;
	r.width = w_title_w - x_title - relief;
	r.height = ICON_HEIGHT(fw) - 2*relief;
	if (is_stippled)
	{

		if (w_stipple == 0)
		{
			w_stipple = ((w_title_w - 2 *
				      (x_stipple + w_title_text_gap) -
				      w_title) + 1) / 2;
		}
		if (w_stipple < ICON_TITLE_STICK_MIN_WIDTH)
		{
			w_stipple = ICON_TITLE_STICK_MIN_WIDTH;
		}
		if (x_title < x_stipple + w_stipple + w_title_text_gap)
		{
			x_title = x_stipple + w_stipple +
				w_title_text_gap;
		}
		r.x = x_title;
		r.width = w_title_w - 2 * x_title;
		if (r.width < 1)
			r.width = 1;
	}

	memset(&fstr, 0, sizeof(fstr));

	if (pev || is_stippled)
	{
		if (pev)
		{
			if (!frect_get_intersection(
				pev->xexpose.x, pev->xexpose.y,
				pev->xexpose.width,
				pev->xexpose.height,
				r.x, r.y, r.width, r.height, &clip))
			{
				draw_string = False;
			}
		}
		else
		{
			clip.x = r.x;
			clip.y = r.y;
			clip.width = r.width;
			clip.height = r.height;
		}
		if (draw_string)
		{
			XSetClipRectangles(
				dpy, Scr.TitleGC, 0, 0, &clip, 1, Unsorted);
			region = XCreateRegion();
			XUnionRectWithRegion (&clip, region, region);
			fstr.flags.has_clip_region = True;
			fstr.clip_region = region;
		}
	}
	if (!pev)
	{
		clip.x = relief;
		clip.y = relief;
		clip.width = w_title_w - 2*relief;
		clip.height = ICON_HEIGHT(fw) - 2*relief;
		XClearWindow(dpy, FW_W_ICON_TITLE(fw));
	}
	else
	{
		/* needed for first drawing */
		if (x_title - relief >= 1)
		{
			/* clear before the text */
			XClearArea(
				dpy, FW_W_ICON_TITLE(fw),
				relief, relief, x_title - relief,
				ICON_HEIGHT(fw) - 2*relief, False);
		}
		if (is_stippled)
		{
			/* clear the stippled area after the text */
			XClearArea(
				dpy, FW_W_ICON_TITLE(fw),
				w_title_w - x_stipple - w_stipple -1, relief,
				w_stipple + 2, ICON_HEIGHT(fw) - 2*relief,
				False);
		}
	}

	if (draw_string)
	{
		if (pev)
		{
			/* needed by xft font and at first drawing */
			XClearArea(
				dpy, FW_W_ICON_TITLE(fw),
				clip.x, clip.y, clip.width, clip.height,
				False);
		}
		fstr.str = fw->visible_icon_name;
		fstr.win =  FW_W_ICON_TITLE(fw);
		fstr.gc = Scr.TitleGC;
		if (title_cs >= 0)
		{
			fstr.colorset = &Colorset[title_cs];
			fstr.flags.has_colorset = 1;
		}
		else if (cs >= 0)
		{
			fstr.colorset = &Colorset[cs];
			fstr.flags.has_colorset = 1;
		}
		fstr.x = x_title;
		fstr.y = fw->icon_g.title_w_g.height - relief
			- fw->icon_font->height + fw->icon_font->ascent;
		FlocaleDrawString(dpy, fw->icon_font, &fstr, 0);
		if (pev || is_stippled)
		{
			XSetClipMask(dpy, Scr.TitleGC, None);
			if (region)
			{
				XDestroyRegion(region);
			}
		}
	}
	RelieveRectangle(
		dpy, FW_W_ICON_TITLE(fw), 0, 0, w_title_w - 1,
		ICON_HEIGHT(fw) - 1,
		(fw->icon_title_relief > 0)? Relief:Shadow,
		(fw->icon_title_relief > 0)? Shadow:Relief, relief);
	if (is_stippled)
	{
		/* an odd number of lines every 4 pixels */
		int pseudo_height = ICON_HEIGHT(fw)- 2*relief + 2;
		int num = (pseudo_height /
			   ICON_TITLE_STICK_VERT_DIST / 2) * 2 - 1;
		int min = ICON_HEIGHT(fw) / 2 -
			num * 2 + 1;
		int max = ICON_HEIGHT(fw) / 2 +
			num * 2 - ICON_TITLE_STICK_VERT_DIST + 1;
		int i;

		for(i = min; w_stipple > 0 && i <= max;
		    i += ICON_TITLE_STICK_VERT_DIST)
		{
			RelieveRectangle(
				dpy, FW_W_ICON_TITLE(fw), x_stipple,
				i, w_stipple - 1, 1, Shadow,
				Relief, ICON_TITLE_STICK_HEIGHT);
			RelieveRectangle(
				dpy, FW_W_ICON_TITLE(fw),
				w_title_w - x_stipple - w_stipple, i,
				w_stipple - 1, 1, Shadow, Relief,
				ICON_TITLE_STICK_HEIGHT);
		}
	}

	return;
}

static
void DrawIconPixmapWindow(
	FvwmWindow *fw, Bool reset_bg, XEvent *pev, GC Shadow, GC Relief, int cs)
{
	XRectangle r,clip;
	Bool cleared = False;

	if (!pev)
	{
		XMoveWindow(
			dpy, FW_W_ICON_PIXMAP(fw), fw->icon_g.picture_w_g.x,
			fw->icon_g.picture_w_g.y);
		if (reset_bg &&
		    (fw->iconDepth == 1 || fw->iconDepth == Pdepth || Pdefault ||
		     IS_PIXMAP_OURS(fw)))
		{
			set_icon_pixmap_background(fw);
			XClearArea(dpy, FW_W_ICON_PIXMAP(fw), 0, 0, 0, 0, False);
			cleared = True;
		}
	}

	/* need to locate the icon pixmap */
	if (fw->iconPixmap != None)
	{
		if (fw->iconDepth == 1 || fw->iconDepth == Pdepth || Pdefault ||
		    IS_PIXMAP_OURS(fw))
		{
			FvwmRenderAttributes fra;
			Bool draw_icon = True;

			memset(&fra, 0, sizeof(fra));
			fra.mask = FRAM_DEST_IS_A_WINDOW;
			if (cs >= 0)
			{
				fra.mask |= FRAM_HAVE_ICON_CSET;
				fra.colorset = &Colorset[cs];
			}
			r.x = r.y = abs(fw->icon_background_relief) +
				fw->icon_background_padding;
			r.width = fw->icon_g.picture_w_g.width -
				2 * (abs(fw->icon_background_relief) +
				     fw->icon_background_padding);
			r.height = fw->icon_g.picture_w_g.height -
				2 * (abs(fw->icon_background_relief) +
				     fw->icon_background_padding);
			if (pev)
			{
				if (!frect_get_intersection(
					pev->xexpose.x, pev->xexpose.y,
					pev->xexpose.width, pev->xexpose.height,
					r.x, r.y, r.width, r.height, &clip))
				{
					draw_icon = False;
				}
			}
			else
			{
				clip.x = r.x;
				clip.y = r.y;
				clip.width = r.width;
				clip.height = r.height;
			}
			if (draw_icon)
			{
				if (!cleared &&
				    (fw->icon_alphaPixmap ||
				     (cs >= 0 &&
				      Colorset[cs].icon_alpha_percent < 100)))
				{
					XClearArea(
						dpy, FW_W_ICON_PIXMAP(fw),
						clip.x, clip.y, clip.width,
						clip.height, False);
				}
				PGraphicsRenderPixmaps(
					dpy, FW_W_ICON_PIXMAP(fw),
					fw->iconPixmap, fw->icon_maskPixmap,
					fw->icon_alphaPixmap, fw->iconDepth,
					&fra, FW_W_ICON_PIXMAP(fw),
					Scr.TitleGC, Scr.MonoGC, Scr.AlphaGC,
					clip.x - r.x, clip.y - r.y,
					clip.width, clip.height,
					clip.x, clip.y, clip.width, clip.height,
					False);
			}
		}
		else
		{
			/* it's a client pixmap and fvwm is not using
			 * the root visual The icon window has no 3d
			 * border so copy to (0,0) install the root
			 * colormap temporarily to help the Exceed
			 * server */
			if (Scr.bo.do_install_root_cmap)
				InstallRootColormap();
			XCopyArea(
				dpy, fw->iconPixmap, FW_W_ICON_PIXMAP(fw),
				DefaultGC(dpy, Scr.screen), 0, 0,
				fw->icon_g.picture_w_g.width,
				fw->icon_g.picture_w_g.height, 0, 0);
			if (Scr.bo.do_install_root_cmap)
				UninstallRootColormap();
		}
	}
	/* only relieve unshaped icons or icons with a bg that share fvwm's
	 * visual */
	if ((fw->iconPixmap != None) &&
	    (!IS_ICON_SHAPED(fw) || fw->icon_background_cs >= 0) &&
	    (Pdefault || fw->iconDepth == 1 || fw->iconDepth == Pdepth ||
	     IS_PIXMAP_OURS(fw)))
	{
		RelieveRectangle(
			dpy, FW_W_ICON_PIXMAP(fw), 0, 0,
			fw->icon_g.picture_w_g.width - 1,
			fw->icon_g.picture_w_g.height - 1,
			(fw->icon_background_relief > 0)? Relief:Shadow,
			(fw->icon_background_relief > 0)? Shadow:Relief,
			abs(fw->icon_background_relief));
	}

}

void DrawIconWindow(
	FvwmWindow *fw, Bool draw_title, Bool draw_pixmap, Bool focus_change,
	Bool reset_bg, XEvent *pev)
{
	GC Shadow;
	GC Relief;
	Pixel TextColor;
	Pixel BackColor;
	color_quad draw_colors;
	color_quad co_draw_colors;
	int cs, co_cs;
	int title_cs = -1;
	int co_title_cs = -1;
	int is_expanded = IS_ICON_ENTERED(fw);

	if (IS_ICON_SUPPRESSED(fw) || (pev && fw->Desk != Scr.CurrentDesk))
	{
		return;
	}

	if (Scr.Hilite == fw)
	{
		if (fw->icon_title_cs_hi >= 0)
		{
			title_cs = fw->icon_title_cs_hi;
			draw_colors.hilight = Colorset[title_cs].hilite;
			draw_colors.shadow = Colorset[title_cs].shadow;
			draw_colors.back = Colorset[title_cs].bg;
			draw_colors.fore = Colorset[title_cs].fg;
		}
		else
		{
			draw_colors.hilight = fw->hicolors.hilight;
			draw_colors.shadow = fw->hicolors.shadow;
			draw_colors.back = fw->hicolors.back;
			draw_colors.fore = fw->hicolors.fore;
		}
		if (fw->icon_title_cs >= 0)
		{
			co_title_cs = fw->icon_title_cs;
			co_draw_colors.hilight = Colorset[co_title_cs].hilite;
			co_draw_colors.shadow = Colorset[co_title_cs].shadow;
			co_draw_colors.back = Colorset[co_title_cs].bg;
			co_draw_colors.fore = Colorset[co_title_cs].fg;
		}
		else
		{
			co_draw_colors.hilight = fw->colors.hilight;
			co_draw_colors.shadow = fw->colors.shadow;
			co_draw_colors.back = fw->colors.back;
			co_draw_colors.fore = fw->colors.fore;
		}
		cs = fw->cs_hi;
		co_cs = fw->cs;
	}
	else
	{
		if (fw->icon_title_cs >= 0)
		{
			title_cs = fw->icon_title_cs;
			draw_colors.hilight = Colorset[title_cs].hilite;
			draw_colors.shadow = Colorset[title_cs].shadow;
			draw_colors.back = Colorset[title_cs].bg;
			draw_colors.fore = Colorset[title_cs].fg;
		}
		else
		{
			draw_colors.hilight = fw->colors.hilight;
			draw_colors.shadow = fw->colors.shadow;
			draw_colors.back = fw->colors.back;
			draw_colors.fore = fw->colors.fore;
		}
		if (fw->icon_title_cs_hi >= 0)
		{
			co_title_cs = fw->icon_title_cs_hi;
			co_draw_colors.hilight = Colorset[co_title_cs].hilite;
			co_draw_colors.shadow = Colorset[co_title_cs].shadow;
			co_draw_colors.back = Colorset[co_title_cs].bg;
			co_draw_colors.fore = Colorset[co_title_cs].fg;
		}
		else
		{
			co_draw_colors.hilight = fw->hicolors.hilight;
			co_draw_colors.shadow = fw->hicolors.shadow;
			co_draw_colors.back = fw->hicolors.back;
			co_draw_colors.fore = fw->hicolors.fore;
		}
		cs = fw->cs;
		co_cs = fw->cs_hi;
	}
	if (Pdepth < 2 && Scr.Hilite != fw)
	{
		Relief = Scr.StdReliefGC;
		Shadow = Scr.StdShadowGC;
	}
	else
	{
		if (Pdepth < 2 && Scr.Hilite == fw)
		{
			Relief = Scr.ScratchGC2;
		}
		else
		{
			Globalgcv.foreground = draw_colors.hilight;
			Globalgcm = GCForeground;
			XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
			Relief = Scr.ScratchGC1;
		}
		Globalgcv.foreground = draw_colors.shadow;
		XChangeGC(dpy,Scr.ScratchGC2, Globalgcm, &Globalgcv);
		Shadow = Scr.ScratchGC2;
	}
	TextColor = draw_colors.fore;
	BackColor = draw_colors.back;
	/* set up TitleGC for drawing the icon label */
	if (fw->icon_font != NULL)
	{
		NewFontAndColor(fw->icon_font, TextColor, BackColor);
	}

	if (draw_title && FW_W_ICON_TITLE(fw) != None)
	{
		if (pev && pev->xexpose.window != FW_W_ICON_TITLE(fw))
		{
			XEvent e;
			if (FCheckTypedWindowEvent(
			    dpy, FW_W_ICON_TITLE(fw), Expose, &e))
			{
				flush_accumulate_expose(
					FW_W_ICON_TITLE(fw), &e);
				DrawIconTitleWindow(
					fw, &e, BackColor, Shadow, Relief, cs,
					title_cs);
			}
		}
		else
		{
			if (!pev)
			{
				FCheckWeedTypedWindowEvents(
					dpy, FW_W_ICON_TITLE(fw), Expose,
					NULL);
			}
			DrawIconTitleWindow(
				fw, pev, BackColor, Shadow, Relief, cs,
				title_cs);
		}
	}

	if (draw_pixmap)
	{
		int bg_cs = fw->icon_background_cs;

		if (bg_cs >= 0 &&
		    (fw->iconDepth != 1 ||
		     fw->icon_background_padding > 0 ||
		     fw->icon_maskPixmap != None ||
		     fw->icon_alphaPixmap != None))
		{
			if (Pdepth < 2 && Scr.Hilite == fw)
			{
				Relief = Scr.ScratchGC2;
			}
			else
			{
				Globalgcv.foreground = Colorset[bg_cs].hilite;
				Globalgcm = GCForeground;
				XChangeGC(
					dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
				Relief = Scr.ScratchGC1;
			}
			Globalgcv.foreground = Colorset[bg_cs].shadow;
			XChangeGC(dpy,Scr.ScratchGC2, Globalgcm, &Globalgcv);
			Shadow = Scr.ScratchGC2;
		}
	}

	if (focus_change && draw_pixmap)
	{
		Bool alpha_change = False;
		Bool tint_change = False;
		Bool relief_change = False;
		Bool color_change = False;

		draw_pixmap = False;
		/* check if we have to draw the icons */

		if (Pdepth < 2)
		{
			relief_change = True;
		}
		else if (fw->iconDepth == 1)
		{
			color_change =
				(draw_colors.fore !=
				 co_draw_colors.back) ||
				(draw_colors.fore !=
				 co_draw_colors.back);
		}
		if (!relief_change &&
		    (fw->iconPixmap != None) && !IS_ICON_SHAPED(fw)
		    && (Pdefault || fw->iconDepth == Pdepth || fw->iconDepth == 1
			|| IS_PIXMAP_OURS(fw)))
		{
			relief_change =
				(draw_colors.hilight !=
				 co_draw_colors.hilight) ||
				(draw_colors.shadow !=
				 co_draw_colors.shadow);
		}
		if (cs >= 0 && co_cs >= 0)
		{
			alpha_change =
				(Colorset[cs].icon_alpha_percent !=
				 Colorset[co_cs].icon_alpha_percent);
			tint_change =
				(Colorset[cs].icon_tint_percent !=
				 Colorset[co_cs].icon_tint_percent) ||
				(Colorset[cs].icon_tint_percent > 0 &&
				 Colorset[cs].icon_tint !=
				 Colorset[co_cs].icon_tint);
		}
		else if (cs >= 0 && co_cs < 0)
		{
			alpha_change = (Colorset[cs].icon_alpha_percent < 100);
			tint_change = (Colorset[cs].icon_tint_percent > 0);
		}
		else if (cs < 0 && co_cs >= 0)
		{
			alpha_change =
				(Colorset[co_cs].icon_alpha_percent < 100);
			tint_change = (Colorset[co_cs].icon_tint_percent > 0);
		}
		if (alpha_change || tint_change || relief_change ||
		    color_change)
		{
			draw_pixmap = True;
		}
	}

	if (draw_pixmap && FW_W_ICON_PIXMAP(fw) != None)
	{
		if (pev && pev->xexpose.window != FW_W_ICON_PIXMAP(fw))
		{
			XEvent e;
			if (FCheckTypedWindowEvent(
			    dpy, FW_W_ICON_PIXMAP(fw), Expose, &e))
			{
				flush_accumulate_expose(
					FW_W_ICON_PIXMAP(fw), &e);
				DrawIconPixmapWindow(
					fw, reset_bg, &e, Shadow, Relief, cs);
			}
		}
		else
		{
			if (!pev)
			{
				FCheckWeedTypedWindowEvents(
					dpy, FW_W_ICON_PIXMAP(fw), Expose,
					NULL);
			}
			DrawIconPixmapWindow(
				fw, reset_bg, pev, Shadow, Relief, cs);
		}
	}

	if (is_expanded)
	{
		if (FW_W_ICON_TITLE(fw) != None)
		{
			XRaiseWindow(dpy, FW_W_ICON_TITLE(fw));
			raisePanFrames();
		}
	}
	else
	{
		XWindowChanges xwc;
		int mask;
		xwc.sibling = FW_W_FRAME(fw);
		xwc.stack_mode = Below;
		mask = CWSibling|CWStackMode;
		if (FW_W_ICON_TITLE(fw) != None)
		{
			XConfigureWindow(dpy, FW_W_ICON_TITLE(fw), mask, &xwc);
		}
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			XConfigureWindow(dpy, FW_W_ICON_PIXMAP(fw), mask, &xwc);
		}
	}
	/* wait for pending EnterNotify/LeaveNotify events to suppress race
	 * condition w/ expanding/collapsing icon titles */
	XFlush(dpy);
}

/*
 *
 *  Procedure:
 *    ChangeIconPixmap - procedure change the icon pixmap or "pixmap"
 *      window. Called in events.c and ewmh_events.c
 *
 */
void ChangeIconPixmap(FvwmWindow *fw)
{
	rectangle g;

	if (!IS_ICONIFIED(fw))
	{
		ICON_DBG((stderr,"hpn: postpone icon change '%s'\n",
			  fw->name.name));
		/* update the icon later when application is iconified */
		SET_HAS_ICON_CHANGED(fw, 1);
	}
	else if (IS_ICONIFIED(fw))
	{
		ICON_DBG((stderr,"hpn: applying new icon '%s'\n",
			  fw->name.name));
		SET_ICONIFIED(fw, 0);
		SET_ICON_UNMAPPED(fw, 0);
		get_icon_geometry(fw, &g);
		CreateIconWindow(fw, g.x, g.y);
		broadcast_icon_geometry(fw, False);
		/* domivogt (15-Sep-1999): BroadcastConfig informs modules of
		 * the configuration change including the iconified flag. So
		 * this flag must be set here. I'm not sure if the two calls of
		 * the SET_ICONIFIED macro after BroadcastConfig are necessary,
		 * but since it's only minimal overhead I prefer to be on the
		 * safe side. */
		SET_ICONIFIED(fw, 1);
		BroadcastConfig(M_CONFIGURE_WINDOW, fw);
		SET_ICONIFIED(fw, 0);

		if (!IS_ICON_SUPPRESSED(fw))
		{
			LowerWindow(fw, False);
			AutoPlaceIcon(fw, NULL, True);
			if (fw->Desk == Scr.CurrentDesk)
			{
				if (FW_W_ICON_TITLE(fw))
				{
					XMapWindow(dpy, FW_W_ICON_TITLE(fw));
				}
				if (FW_W_ICON_PIXMAP(fw) != None)
				{
					XMapWindow(dpy, FW_W_ICON_PIXMAP(fw));
				}
			}
		}
		SET_ICONIFIED(fw, 1);
		DrawIconWindow(fw, False, True, False, False, NULL);
	}

	return;
}

/*
 *
 *  Procedure:
 *      RedoIconName - procedure to re-position the icon window and name
 *
 */
void RedoIconName(FvwmWindow *fw)
{
	if (IS_ICON_SUPPRESSED(fw))
	{
		return;
	}
	if (FW_W_ICON_TITLE(fw) == None)
	{
		return;
	}
	setup_icon_title_size(fw);
	/* clear the icon window, and trigger a re-draw via an expose event */
	if (IS_ICONIFIED(fw))
	{
		DrawIconWindow(fw, True, False, False, False, NULL);
		XClearArea(dpy, FW_W_ICON_TITLE(fw), 0, 0, 0, 0, True);
	}

	return;
}

/*
 *
 *  Procedure:
 *      AutoPlace - Find a home for an icon
 *
 */
void AutoPlaceIcon(
	FvwmWindow *t, initial_window_options_t *win_opts,
	Bool do_move_immediately)
{
  int base_x, base_y;
  int width,height;
  FvwmWindow *test_fw;
  Bool loc_ok;
  Bool loc_ok_wrong_screen;
  Bool loc_ok_wrong_screen2;
  int real_x=10, real_y=10;
  int new_x, new_y;
  Bool do_move_icon = False;

#if 0
  /* dv (16-Mar-2003):  We need to place the icon even if there is no icon so
   * the 'position' can be communicated to the modules to decide whether to show
   * the icon or not. */
  if (FW_W_ICON_PIXMAP(t) == None && FW_W_ICON_TITLE(t) == None)
  {
	  return;
  }
#endif
  /* New! Put icon in same page as the center of the window */
  /* Not a good idea for StickyIcons. Neither for icons of windows that are
   * visible on the current page. */
  if (IS_ICON_STICKY_ACROSS_DESKS(t) || IS_STICKY_ACROSS_DESKS(t))
  {
    t->Desk = Scr.CurrentDesk;
  }
  if (IS_ICON_STICKY_ACROSS_PAGES(t) || IS_STICKY_ACROSS_PAGES(t))
  {
    base_x = 0;
    base_y = 0;
    /*Also, if its a stickyWindow, put it on the current page! */
    new_x = t->g.frame.x % Scr.MyDisplayWidth;
    new_y = t->g.frame.y % Scr.MyDisplayHeight;
    if (new_x + t->g.frame.width <= 0)
      new_x += Scr.MyDisplayWidth;
    if (new_y + t->g.frame.height <= 0)
      new_y += Scr.MyDisplayHeight;
    frame_setup_window(
	    t, new_x, new_y, t->g.frame.width, t->g.frame.height, False);
  }
  else if (IsRectangleOnThisPage(&(t->g.frame), t->Desk))
  {
    base_x = 0;
    base_y = 0;
  }
  else
  {
    base_x = ((t->g.frame.x + Scr.Vx + (t->g.frame.width >> 1)) /
      Scr.MyDisplayWidth) * Scr.MyDisplayWidth;
    base_y= ((t->g.frame.y + Scr.Vy + (t->g.frame.height >> 1)) /
      Scr.MyDisplayHeight) * Scr.MyDisplayHeight;
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
  if (IS_ICON_MOVED(t) ||
      (win_opts != NULL && win_opts->flags.use_initial_icon_xy))
  {
    rectangle g;
    int dx;
    int dy;

    get_icon_geometry(t, &g);
    if (win_opts != NULL && win_opts->flags.use_initial_icon_xy)
    {
      g.x = win_opts->initial_icon_x;
      g.y = win_opts->initial_icon_y;
    }
    dx = g.x;
    dy = g.y;

    /* just make sure the icon is on this page */
    g.x = g.x % Scr.MyDisplayWidth + base_x;
    g.y = g.y % Scr.MyDisplayHeight + base_y;
    if (g.x < 0)
    {
      g.x += Scr.MyDisplayWidth;
    }
    if (g.y < 0)
    {
      g.y += Scr.MyDisplayHeight;
    }
    dx = g.x - dx;
    dy = g.y - dy;
    modify_icon_position(t, dx, dy);
    do_move_icon = True;
  }
  else if (USE_ICON_POSITION_HINT(t) && t->wmhints &&
	   t->wmhints->flags & IconPositionHint)
  {
    set_icon_position(t, t->wmhints->icon_x, t->wmhints->icon_y);
    do_move_icon = True;
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
    rectangle g;

    get_icon_geometry(t, &g);
    get_icon_corner(t, &g);
    fscr.xypos.x = g.x + g.width / 2;
    fscr.xypos.y = g.y + g.height / 2;
    FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &sx, &sy, &sw, &sh);
    if (g.x < sx)
      g.x = sx;
    else if (g.x + g.width > sx + sw)
      g.x = sx + sw - g.width;
    if (g.y < sy)
      g.y = sy;
    else if (g.y + g.height > sy + sh)
      g.y = sy + sh - g.height;
    set_icon_position(t, g.x, g.y);
    do_move_icon = True;
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
    rectangle g;

    /* Hopefully this makes the following more readable. */
#define ICONBOX_LFT icon_boxes_ptr->IconBox[0]
#define ICONBOX_TOP icon_boxes_ptr->IconBox[1]
#define ICONBOX_RGT icon_boxes_ptr->IconBox[2]
#define ICONBOX_BOT icon_boxes_ptr->IconBox[3]
#define BOT_FILL icon_boxes_ptr->IconFlags & ICONFILLBOT
#define RGT_FILL icon_boxes_ptr->IconFlags & ICONFILLRGT
#define HRZ_FILL icon_boxes_ptr->IconFlags & ICONFILLHRZ

    /* needed later */
    fscr.xypos.x = t->g.frame.x + (t->g.frame.width / 2) - base_x;
    fscr.xypos.y = t->g.frame.y + (t->g.frame.height / 2) - base_y;
    get_icon_geometry(t, &g);
    /* unnecessary copy of width */
    width = g.width;
    /* total height */
    height = g.height;
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
      FScreenGetScrRect(&fscr, FSCREEN_CURRENT,
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
	  test_fw = Scr.FvwmRoot.next;
	  while((test_fw != (FvwmWindow *)0)
		&&(loc_ok == True || loc_ok_wrong_screen2))
	  {
	    /* test overlap */
	    if (test_fw->Desk == t->Desk)
	    {
	      rectangle g;

	      if ((IS_ICONIFIED(test_fw)) &&
		 (!IS_TRANSIENT(test_fw) ||
		  !IS_ICONIFIED_BY_PARENT(test_fw)) &&
		 (FW_W_ICON_TITLE(test_fw)||FW_W_ICON_PIXMAP(test_fw)) &&
		 (test_fw != t)) {
		get_icon_geometry(test_fw, &g);
		if ((g.x<(real_x+width+MIN_ICON_BOX_DIST))&&
		   ((g.x+g.width+MIN_ICON_BOX_DIST) > real_x)&&
		   (g.y<(real_y+height+MIN_ICON_BOX_DIST))&&
		   ((g.y+g.height + MIN_ICON_BOX_DIST)>real_y))
		{
		  /* don't accept this location */
		  loc_ok = False;
		  loc_ok_wrong_screen2 = False;
		} /* end if icons overlap */
	      } /* end if its an icon */
	    } /* end if same desk */
	    test_fw = test_fw->next;
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
    if (!loc_ok && !loc_ok_wrong_screen)
      /* If icon never found a home just leave it */
      return;
    set_icon_position(t, real_x, real_y);
    broadcast_icon_geometry(t, True);
    do_move_icon = True;
  }
  if (do_move_icon && do_move_immediately)
  {
    move_icon_to_position(t);
  }

  return;
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
	if (global_icon_box_ptr == 0)
	{
		/* if first time */
		int sx;
		int sy;
		int sw;
		int sh;
		/* Right now, the global box is hard-coded, fills the primary
		 * screen, uses an 80x80 grid, and fills top-bottom,
		 * left-right */
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
	if (*icon_boxes_ptr == NULL)
	{
		/* first time? */
		/* start at windows box */
		*icon_boxes_ptr = t->IconBoxes;
		if (!*icon_boxes_ptr)
		{
			/* if window has no box */
			/* use global box */
			*icon_boxes_ptr = global_icon_box_ptr;
		}
		/* use box */
		return (1);
	}

	/* Here its not the first call, we are either on the chain or at
	 * the global box */
	if (*icon_boxes_ptr == global_icon_box_ptr)
	{
		/* if the global box */
		/* completely out of boxes (unlikely) */
		return (0);
	}
	/* move to next one on chain */
	*icon_boxes_ptr = (*icon_boxes_ptr)->next;
	if (*icon_boxes_ptr)
	{
                /* if there is a next one */
		/* return it */
		return (1);
	}
	/* global box */
	*icon_boxes_ptr = global_icon_box_ptr;

	/* use it */
	return (1);
}

/*
 *
 * Looks for icon from a file
 *
 */
static void GetIconFromFile(FvwmWindow *fw)
{
	char *path = NULL;
	FvwmPictureAttributes fpa;

	fpa.mask = 0;
	if (fw->cs >= 0 && Colorset[fw->cs].do_dither_icon)
	{
		fpa.mask |= FPAM_DITHER;
	}
	fw->icon_g.picture_w_g.width = 0;
	fw->icon_g.picture_w_g.height = 0;
	path = PictureFindImageFile(fw->icon_bitmap_file, NULL, R_OK);
	if (path == NULL)
	{
		return;
	}
	if (!PImageLoadPixmapFromFile(
		dpy, Scr.NoFocusWin, path, &fw->iconPixmap,
		&fw->icon_maskPixmap, &fw->icon_alphaPixmap,
		&fw->icon_g.picture_w_g.width, &fw->icon_g.picture_w_g.height,
		&fw->iconDepth, &fw->icon_nalloc_pixels, &fw->icon_alloc_pixels,
		&fw->icon_no_limit, fpa))
	{
		fvwm_msg(ERR, "GetIconFromFile", "Failed to load %s", path);
		free(path);
		return;
	}
	SET_PIXMAP_OURS(fw, 1);
	free(path);
	if (FShapesSupported && fw->icon_maskPixmap)
	{
		SET_ICON_SHAPED(fw, 1);
	}

	return;
}

/*
 *
 * Looks for an application supplied icon window
 *
 */
static void GetIconWindow(FvwmWindow *fw)
{
	int w;
	int h;
	int bw;

	fw->icon_g.picture_w_g.width = 0;
	fw->icon_g.picture_w_g.height = 0;

	/* We are guaranteed that wmhints is non-null when calling this
	 * routine */
	if (XGetGeometry(
		    dpy, fw->wmhints->icon_window, &JunkRoot, &JunkX, &JunkY,
		    (unsigned int*)&w, (unsigned int*)&h,(unsigned int*)&bw,
		    (unsigned int*)&JunkDepth) == 0)
	{
		fvwm_msg(
			ERR,"GetIconWindow",
			"Window '%s' has a bad icon window! Ignoring icon"
			" window.", fw->name.name);
		/* disable the icon window hint */
		fw->wmhints->icon_window = None;
		fw->wmhints->flags &= ~IconWindowHint;
		return;
	}
	fw->icon_border_width = bw;
	fw->icon_g.picture_w_g.width = w + 2 * bw;
	fw->icon_g.picture_w_g.height = h + 2 * bw;
	/*
	 * Now make the new window the icon window for this window,
	 * and set it up to work as such (select for key presses
	 * and button presses/releases, set up the contexts for it,
	 * and define the cursor for it).
	 */
	FW_W_ICON_PIXMAP(fw) = fw->wmhints->icon_window;
	if (FShapesSupported)
	{
		if (fw->wmhints->flags & IconMaskHint)
		{
			SET_ICON_SHAPED(fw, 1);
			fw->icon_maskPixmap = fw->wmhints->icon_mask;
		}
	}
	/* Make sure that the window is a child of the root window ! */
	/* Olwais screws this up, maybe others do too! */
	XReparentWindow(dpy, FW_W_ICON_PIXMAP(fw), Scr.Root, 0,0);
	SET_ICON_OURS(fw, 0);

	return;
}


/*
 *
 * Looks for an application supplied bitmap or pixmap
 *
 */
static void GetIconBitmap(FvwmWindow *fw)
{
	int width, height, depth;

	fw->icon_g.picture_w_g.width = 0;
	fw->icon_g.picture_w_g.height = 0;

	/* We are guaranteed that wmhints is non-null when calling this routine
	 */
	if (!XGetGeometry(
		    dpy, fw->wmhints->icon_pixmap, &JunkRoot, &JunkX, &JunkY,
		    (unsigned int*)&width, (unsigned int*)&height,
		    (unsigned int*)&JunkBW, (unsigned int*)&depth))
	{
		fvwm_msg(
			ERR,"GetIconBitmap",
			"Window '%s' has a bad icon pixmap! Ignoring icon.",
			fw->name.name);
		/* disable icon pixmap hint */
		fw->wmhints->icon_pixmap = None;
		fw->wmhints->flags &= ~IconPixmapHint;
		return;
	}
	/* sanity check the pixmap depth, it must be the same as the root or 1
	 */
	if (depth != 1 && depth != Pdepth &&
	    depth != DefaultDepth(dpy,Scr.screen))
	{
		fvwm_msg(
			ERR, "GetIconBitmap",
			"Window '%s' has a bad icon bitmap depth %d (should"
			" be 1, %d or %d)! Ignoring icon bitmap.",
			fw->name.name, depth, Pdepth,
			DefaultDepth(dpy,Scr.screen));
		/* disable icon pixmap hint */
		fw->wmhints->icon_pixmap = None;
		fw->wmhints->flags &= ~IconPixmapHint;
		return;
	}
	fw->iconPixmap = fw->wmhints->icon_pixmap;
	fw->icon_g.picture_w_g.width = width;
	fw->icon_g.picture_w_g.height = height;
	fw->iconDepth = depth;
	if (FShapesSupported)
	{
		if (fw->wmhints->flags & IconMaskHint)
		{
			SET_ICON_SHAPED(fw, 1);
			fw->icon_maskPixmap = fw->wmhints->icon_mask;
		}
	}
	SET_PIXMAP_OURS(fw, 0);

	return;
}

/*
 *
 *  Procedure:
 *      DeIconify a window
 *
 */
void DeIconify(FvwmWindow *fw)
{
	FvwmWindow *t, *tmp, *ofw;
	FvwmWindow *sf = get_focus_window();
	rectangle icon_rect;
	XWindowAttributes winattrs = {0};

	if (!fw)
	{
		return;
	}
	if (!XGetWindowAttributes(dpy, FW_W(fw), &winattrs))
	{
		return;
	}

	/* make sure fw->flags.is_map_pending is OK */
	if (winattrs.map_state == IsViewable && IS_MAP_PENDING(fw))
	{
		SET_MAP_PENDING(fw, 0);
	}
	else if (IS_MAP_PENDING(fw))
	{
		/* final state: de-iconified */
		SET_ICONIFY_AFTER_MAP(fw, 0);
		return;
	}
	for (ofw = NULL; fw != ofw && IS_ICONIFIED_BY_PARENT(fw); )
	{
		t = get_transientfor_fvwmwindow(fw);
		if (t == NULL)
		{
			break;
		}
		ofw = fw;
		fw = t;
	}
	if (IS_ICONIFIED_BY_PARENT(fw))
	{
		SET_ICONIFIED_BY_PARENT(fw, 0);
	}

	/* AS dje  RaiseWindow(fw); */

	if (fw == sf)
	{
		/* take away the focus before mapping */
		DeleteFocus(True);
	}
	/* Note: DeleteFocus may delete the flags set by
	 * mark_transient_subtree(), so do it later. */
	mark_transient_subtree(fw, MARK_ALL_LAYERS, MARK_ALL, False, True);
	/* now de-iconify transients */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t == fw || IS_IN_TRANSIENT_SUBTREE(t))
		{
			SET_IN_TRANSIENT_SUBTREE(t, 0);
			SET_MAPPED(t, 1);
			SET_ICONIFIED_BY_PARENT(t, 0);
			if (Scr.Hilite == t)
			{
				border_draw_decorations(
					t, PART_ALL, False, True, CLEAR_ALL,
					NULL, NULL);
			}

			/* AS stuff starts here dje */
			if (FW_W_ICON_PIXMAP(t))
			{
				XUnmapWindow(dpy, FW_W_ICON_PIXMAP(t));
			}
			if (FW_W_ICON_TITLE(t))
			{
				XUnmapWindow(dpy, FW_W_ICON_TITLE(t));
			}
			XFlush(dpy);
			/* End AS */
			XMapWindow(dpy, FW_W(t));
			if (t->Desk == Scr.CurrentDesk)
			{
				rectangle r;

				get_icon_geometry(t, &r);
				/* update absoluthe geometry in case the icon
				 * was moved over a page boundary; the move
				 * code already takes care of keeping the frame
				 * geometry up to date */
				update_absolute_geometry(t);
				if (IsRectangleOnThisPage(&r, t->Desk) &&
				    !IsRectangleOnThisPage(
					    &(t->g.frame), t->Desk))
				{
					/* Make sure we keep it on screen when
					 * de-iconifying. */
					t->g.frame.x -=
						truncate_to_multiple(
							t->g.frame.x,
							Scr.MyDisplayWidth);
					t->g.frame.y -=
						truncate_to_multiple(
							t->g.frame.y,
							Scr.MyDisplayHeight);
					XMoveWindow(
						dpy, FW_W_FRAME(t),
						t->g.frame.x, t->g.frame.y);
					update_absolute_geometry(t);
					maximize_adjust_offset(t);
				}
			}
			/* domivogt (1-Mar-2000): The next block is a hack to
			 * prevent animation if the window has an icon, but
			 * neither a pixmap nor a title. */
			if (HAS_NO_ICON_TITLE(t) && FW_W_ICON_PIXMAP(t) == None)
			{
				memset(&fw->icon_g, 0, sizeof(fw->icon_g));
			}
			get_icon_geometry(t, &icon_rect);
			/* if this fails it does not overwrite icon_rect */
			EWMH_GetIconGeometry(t, &icon_rect);
			if (t == fw)
			{
				BroadcastPacket(
					M_DEICONIFY, 11, (long)FW_W(t),
					(long)FW_W_FRAME(t), (unsigned long)t,
					(long)icon_rect.x, (long)icon_rect.y,
					(long)icon_rect.width,
					(long)icon_rect.height,
					(long)t->g.frame.x,
					(long)t->g.frame.y,
					(long)t->g.frame.width,
					(long)t->g.frame.height);
			}
			else
			{
				BroadcastPacket(
					M_DEICONIFY, 7, (long)FW_W(t),
					(long)FW_W_FRAME(t),
					(unsigned long)t, (long)icon_rect.x,
					(long)icon_rect.y,
					(long)icon_rect.width,
					(long)icon_rect.height);
			}
			XMapWindow(dpy, FW_W_PARENT(t));
			if (t->Desk == Scr.CurrentDesk)
			{
				XMapWindow(dpy, FW_W_FRAME(t));
				SET_MAP_PENDING(t, 1);
			}
			SetMapStateProp(t, NormalState);
			SET_ICONIFIED(t, 0);
			SET_ICON_UNMAPPED(t, 0);
			SET_ICON_ENTERED(t, 0);
			/* Need to make sure the border is colored correctly,
			 * in case it was stuck or unstuck while iconified. */
			tmp = Scr.Hilite;
			Scr.Hilite = t;
			border_draw_decorations(
				t, PART_ALL, (sf == t) ? True : False, True,
				CLEAR_ALL, NULL, NULL);
			Scr.Hilite = tmp;
		}
	}

#if 1
	RaiseWindow(fw, False); /* moved dje */
#endif
	if (sf == fw)
	{
		/* update the focus to make sure the application knows its
		 * state */
		if (!FP_DO_UNFOCUS_LEAVE(FW_FOCUS_POLICY(fw)))
		{
			SetFocusWindow(fw, True, FOCUS_SET_FORCE);
		}
	}
	else if (FP_DO_SORT_WINDOWLIST_BY(FW_FOCUS_POLICY(fw)) ==
		 FPOL_SORT_WL_BY_OPEN)
	{
		SetFocusWindow(fw, True, FOCUS_SET_FORCE);
	}
	focus_grab_buttons_on_layer(fw->layer);

	return;
}


/*
 *
 * Iconifies the selected window
 *
 */
void Iconify(FvwmWindow *fw, initial_window_options_t *win_opts)
{
	FvwmWindow *t;
	FvwmWindow *sf;
	XWindowAttributes winattrs = {0};
	unsigned long eventMask;
	rectangle icon_rect;

	if (!fw)
	{
		return;
	}
	if (!XGetWindowAttributes(dpy, FW_W(fw), &winattrs))
	{
		return;
	}

	/* make sure fw->flags.is_map_pending is OK */
	if ((winattrs.map_state == IsViewable) && IS_MAP_PENDING(fw))
	{
		SET_MAP_PENDING(fw, 0);
	}

	if (IS_MAP_PENDING(fw))
	{
		/* final state: iconified */
		SET_ICONIFY_AFTER_MAP(fw, 1);
		return;
	}
	eventMask = winattrs.your_event_mask;

	mark_transient_subtree(fw, MARK_ALL_LAYERS, MARK_ALL, False, True);
	sf = get_focus_window();
	if (sf && IS_IN_TRANSIENT_SUBTREE(sf))
	{
		restore_focus_after_unmap(sf, True);
		/* restore_focus_after_unmap() destorys the flags set by
		 * mark_transient_subtree(), so we have to unfortunately call
		 * it again. */
		mark_transient_subtree(
			fw, MARK_ALL_LAYERS, MARK_ALL, False, True);
	}
	/* iconify transients first */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t == fw || IS_IN_TRANSIENT_SUBTREE(t))
		{
			SET_IN_TRANSIENT_SUBTREE(t, 0);
			SET_ICON_ENTERED(t, 0);
			/* Prevent the receipt of an UnmapNotify, since that
			 * would cause a transition to the Withdrawn state. */
			SET_MAPPED(t, 0);
			XSelectInput(
				dpy, FW_W(t), eventMask & ~StructureNotifyMask);
			XUnmapWindow(dpy, FW_W(t));
			XSelectInput(dpy, FW_W(t), eventMask);
			XUnmapWindow(dpy, FW_W_FRAME(t));
			border_undraw_decorations(t);
			t->DeIconifyDesk = t->Desk;
			if (FW_W_ICON_TITLE(t))
			{
				XUnmapWindow(dpy, FW_W_ICON_TITLE(t));
			}
			if (FW_W_ICON_PIXMAP(t))
			{
				XUnmapWindow(dpy, FW_W_ICON_PIXMAP(t));
			}

			SetMapStateProp(t, IconicState);
			border_draw_decorations(
				t, PART_ALL, False, False, CLEAR_ALL, NULL,
				NULL);
			if (t == fw && !IS_ICONIFIED_BY_PARENT(fw))
			{
				SET_ICONIFY_PENDING(t, 1);
			}
			else
			{
				rectangle g;

				SET_ICONIFIED(t, 1);
				SET_ICON_UNMAPPED(t, 1);
				SET_ICONIFIED_BY_PARENT(t, 1);
				get_icon_geometry(t, &g);
				BroadcastPacket(
					M_ICONIFY, 7, (long)FW_W(t),
					(long)FW_W_FRAME(t),
					(unsigned long)t, (long)-32768,
					(long)-32768, (long)g.width,
					(long)g.height);
				BroadcastConfig(M_CONFIGURE_WINDOW,t);
			}
		} /* if */
	} /* for */

	/* necessary during a recapture */
	if (IS_ICONIFIED_BY_PARENT(fw))
	{
		return;
	}

	if (FW_W_ICON_TITLE(fw) == None || HAS_ICON_CHANGED(fw))
	{
		if (IS_ICON_MOVED(fw) || win_opts->flags.use_initial_icon_xy)
		{
			rectangle g;

			get_icon_geometry(fw, &g);
			if (win_opts->flags.use_initial_icon_xy)
			{
				g.x = win_opts->initial_icon_x;
				g.y = win_opts->initial_icon_y;
			}
			CreateIconWindow(fw, g.x, g.y);
		}
		else
		{
			CreateIconWindow(
				fw, win_opts->default_icon_x,
				win_opts->default_icon_y);
		}
		SET_HAS_ICON_CHANGED(fw, 0);
	}
	else if (FW_W_ICON_TITLE(fw) && !FW_W_ICON_PIXMAP(fw))
	{
		/* if no pixmap we want icon width to change to text width
		 * every iconify; not necessary if the icon was created above */
		setup_icon_title_size(fw);
	}

	/* this condition will be true unless we restore a window to
	 * iconified state from a saved session. */
	if (win_opts->initial_state != IconicState ||
	    (!IS_ICON_MOVED(fw) && !win_opts->flags.use_initial_icon_xy))
	{
		AutoPlaceIcon(fw, win_opts, True);
	}
	/* domivogt (12-Mar-2003): Clean out the icon geometry if there is no
	 * icon.  Necessary to initialise the values and to suppress animation
	 * if there is no icon. */
	if (HAS_NO_ICON_TITLE(fw) && FW_W_ICON_PIXMAP(fw) == None)
	{
		clear_icon_dimensions(fw);
	}
	SET_ICONIFIED(fw, 1);
	SET_ICON_UNMAPPED(fw, 0);
	get_icon_geometry(fw, &icon_rect);
	/* if this fails it does not overwrite icon_rect */
	EWMH_GetIconGeometry(fw, &icon_rect);
	BroadcastPacket(
		M_ICONIFY, 11, (long)FW_W(fw), (long)FW_W_FRAME(fw),
		(unsigned long)fw, (long)icon_rect.x, (long)icon_rect.y,
		(long)icon_rect.width, (long)icon_rect.height,
		/* next 4 added for Animate module */
		(long)fw->g.frame.x, (long)fw->g.frame.y,
		(long)fw->g.frame.width, (long)fw->g.frame.height);
	BroadcastConfig(M_CONFIGURE_WINDOW,fw);

	if (win_opts->initial_state != IconicState ||
	    (!IS_ICON_MOVED(fw) && !win_opts->flags.use_initial_icon_xy))
	{
		LowerWindow(fw, False);
	}
	if (IS_ICON_STICKY_ACROSS_DESKS(fw) || IS_STICKY_ACROSS_DESKS(fw))
	{
		fw->Desk = Scr.CurrentDesk;
	}
	if (fw->Desk == Scr.CurrentDesk)
	{
		if (FW_W_ICON_TITLE(fw) != None)
		{
			XMapWindow(dpy, FW_W_ICON_TITLE(fw));
		}
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			XMapWindow(dpy, FW_W_ICON_PIXMAP(fw));
		}
	}
	focus_grab_buttons_on_layer(fw->layer);

	return;
}

/*
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 */
void SetMapStateProp(const FvwmWindow *fw, int state)
{
	/* "suggested" by ICCCM version 1 */
	unsigned long data[2];

	data[0] = (unsigned long) state;
	data[1] = (unsigned long) FW_W_ICON_TITLE(fw);
	/*  data[2] = (unsigned long) FW_W_ICON_PIXMAP(fw);*/

	XChangeProperty(
		dpy, FW_W(fw), _XA_WM_STATE, _XA_WM_STATE, 32, PropModeReplace,
		(unsigned char *) data, 2);

	return;
}

void CMD_Iconify(F_CMD_ARGS)
{
	int toggle;
	FvwmWindow * const fw = exc->w.fw;

	toggle = ParseToggleArgument(action, NULL, -1, 0);
	if (toggle == -1)
	{
		if (GetIntegerArguments(action, NULL, &toggle, 1) > 0)
		{
			if (toggle > 0)
			{
				toggle = 1;
			}
			else if (toggle < 0)
			{
				toggle = 0;
			}
			else
			{
				toggle = -1;
			}
		}
	}
	if (toggle == -1)
	{
		toggle = (IS_ICONIFIED(fw)) ? 0 : 1;
	}

	if (IS_ICONIFIED(fw))
	{
		if (toggle == 0)
		{
			DeIconify(fw);
			EWMH_SetWMState(fw, False);
		}
	}
	else
	{
		if (toggle == 1)
		{
			initial_window_options_t win_opts;

			if (
				!is_function_allowed(
					F_ICONIFY, NULL, fw, RQORIG_PROGRAM,
					True))
			{
				XBell(dpy, 0);

				return;
			}
			memset(&win_opts, 0, sizeof(win_opts));
			fev_get_evpos_or_query(
				dpy, Scr.Root, NULL, &win_opts.default_icon_x,
				&win_opts.default_icon_y);
			Iconify(fw, &win_opts);
			EWMH_SetWMState(fw, False);
		}
	}

	return;
}
