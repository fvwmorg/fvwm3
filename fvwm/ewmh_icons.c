/* Copyright (C) 2001  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifdef HAVE_EWMH

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/PictureBase.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/PictureImageLoader.h"
#include "libs/FRenderInit.h"
#include "fvwm.h"
#include "externs.h"
#include "window_flags.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "borders.h"
#include "icons.h"
#include "ewmh.h"
#include "ewmh_intern.h"

/* ***************************************************************************
 * net icon handler
 * ***************************************************************************/
int ewmh_WMIcon(EWMH_CMD_ARGS)
{
  CARD32 *list = NULL;
  CARD32 *new_list = NULL;
  CARD32 *dummy = NULL;
  unsigned int size = 0;

  if (ev != NULL && HAS_EWMH_WM_ICON_HINT(fwin) == EWMH_FVWM_ICON)
  {
    /* this event has been produced by fvwm itself */
    return 0;
  }

  list = ewmh_AtomGetByName(FW_W(fwin),"_NET_WM_ICON",
			    EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);

  if (list != NULL && HAS_EWMH_WM_ICON_HINT(fwin) == EWMH_NO_ICON)
  {
    /* the application have a true _NET_WM_ICON */
    SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_TRUE_ICON);
  }

  if (list == NULL || HAS_EWMH_WM_ICON_HINT(fwin) != EWMH_TRUE_ICON)
  {
    /* No net icon or we have set the net icon */
    if (DO_EWMH_DONATE_ICON(fwin) &&
	(new_list =
	 ewmh_SetWmIconFromPixmap(fwin, list, &size, False)) != NULL)
    {
      SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_FVWM_ICON);
    }
  }
  else if (ev != NULL && USE_EWMH_ICON(fwin))
  {
    /* client message. the application change its net icon */
    ChangeIconPixmap(fwin);
  }
  if (FMiniIconsSupported)
  {
    if (list == NULL || HAS_EWMH_WM_ICON_HINT(fwin) != EWMH_TRUE_ICON)
    {
      /* No net icon or we have set the net icon */
      if (DO_EWMH_DONATE_MINI_ICON(fwin) &&
	(dummy = ewmh_SetWmIconFromPixmap(fwin,
					  (new_list != NULL)? new_list : list,
					  &size, True)) != NULL)
      {
	SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_FVWM_ICON);
	free(dummy);
      }
    }
    else
    {
      /* the application has a true ewmh icon */
      if (EWMH_SetIconFromWMIcon(fwin, list, size, True))
	SET_HAS_EWMH_MINI_ICON(fwin, True);
    }
  }

  if (list != NULL)
    free(list);
  if (new_list != NULL)
    free(new_list);
  return 0;
}

/* ***************************************************************************
 * update
 * ***************************************************************************/
void EWMH_DoUpdateWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon)
{
  CARD32 *list = NULL;
  CARD32 *new_list = NULL;
  CARD32 *dummy = NULL;
  unsigned int size = 0;
  Bool icon_too = False;

  if (HAS_EWMH_WM_ICON_HINT(fwin) == EWMH_TRUE_ICON)
    return;

  /* first see if we have to delete */
  if (FMiniIconsSupported && mini_icon && !DO_EWMH_DONATE_MINI_ICON(fwin))
  {
    if (icon && !DO_EWMH_DONATE_ICON(fwin))
    {
      icon_too = True;
    }
    EWMH_DeleteWmIcon(fwin, True, icon_too);
  }
  if (!icon_too && icon && !DO_EWMH_DONATE_ICON(fwin))
  {
    EWMH_DeleteWmIcon(fwin, False, True);
  }

  /* now set if needed */
  if ((mini_icon && DO_EWMH_DONATE_MINI_ICON(fwin)) ||
      (icon && DO_EWMH_DONATE_ICON(fwin)))
  {
    list = ewmh_AtomGetByName(FW_W(fwin),"_NET_WM_ICON",
			    EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);
  }
  else
  {
    return;
  }

  /* we have to reset */
  if (icon && DO_EWMH_DONATE_ICON(fwin))
  {
    if ((new_list = ewmh_SetWmIconFromPixmap(fwin, list, &size, False))
	!= NULL)
    {
      SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_FVWM_ICON);
    }
  }
  if (FMiniIconsSupported && mini_icon && DO_EWMH_DONATE_MINI_ICON(fwin))
  {
    if ((dummy = ewmh_SetWmIconFromPixmap(
		 fwin, (new_list != NULL)? new_list : list, &size, True)) !=
	NULL)
    {
      SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_FVWM_ICON);
      free(dummy);
    }
  }
  if (list != NULL)
    free(list);
  if (new_list != NULL)
    free(new_list);
}

/* ***************************************************************************
 * build and set a net icon from a pixmap
 * ***************************************************************************/
CARD32 *ewmh_SetWmIconFromPixmap(FvwmWindow *fwin,
				 CARD32 *orig_icon,
				 unsigned int *orig_size,
				 Bool is_mini_icon)
{
	CARD32 *new_icon = NULL;
	unsigned char *c_new_icon;
	int keep_start = 0, keep_length = 0;
	int width = 0, height = 0;
	unsigned int i,j,k,l,m;
	int s;
	Pixmap pixmap = None;
	Pixmap mask = None;
	Pixmap alpha = None;
	XImage *image;
	XImage *m_image = NULL;
	XImage *a_image = NULL;
	int save_picture_w_g_width = 0;
	int save_picture_w_g_height = 0;
	int save_icon_depth = 0;
	Pixmap save_icon_pixmap = None;
	Pixmap save_icon_mask = None;
	Pixmap save_icon_alpha = None;
	int save_icon_nalloc_pixels = 0;
	Pixel *save_icon_alloc_pixels = NULL;
	int save_icon_no_limit = 0;
	Window save_icon_pixmap_w = None;
	Bool is_pixmap_ours = False;
	Bool is_icon_ours = False;
	Bool is_icon_shaped = False;
	Bool destroy_icon_pix = False;

	s = *orig_size / sizeof(CARD32);
	*orig_size = 0;

	if (is_mini_icon)
	{
		if (FMiniIconsSupported && fwin->mini_icon != NULL)
		{
			pixmap = fwin->mini_icon->picture;
			mask = fwin->mini_icon->mask;
			alpha = fwin->mini_icon->alpha;
			width = fwin->mini_icon->width;
			height = fwin->mini_icon->height;
		}
	}
	else
	{
		/* should save and restore any iformation modified by
		 * a call to GetIconPicture */
		save_picture_w_g_width = fwin->icon_g.picture_w_g.width;
		save_picture_w_g_height = fwin->icon_g.picture_w_g.height;
		save_icon_depth = fwin->iconDepth;
		save_icon_pixmap = fwin->iconPixmap;
		save_icon_mask = fwin->icon_maskPixmap;
		save_icon_alpha = fwin->icon_alphaPixmap;
		save_icon_nalloc_pixels = fwin->icon_nalloc_pixels;
		save_icon_alloc_pixels = fwin->icon_alloc_pixels;
		save_icon_no_limit = fwin->icon_no_limit;
		save_icon_pixmap_w =  FW_W_ICON_PIXMAP(fwin);
		is_pixmap_ours = IS_PIXMAP_OURS(fwin);
		is_icon_ours = IS_ICON_OURS(fwin);
		is_icon_shaped = IS_ICON_SHAPED(fwin);
		GetIconPicture(fwin, True);
		if (IS_PIXMAP_OURS(fwin))
			destroy_icon_pix = True;
		pixmap = fwin->iconPixmap;
		mask = fwin->icon_maskPixmap;
		alpha = fwin->icon_alphaPixmap;
		width = fwin->icon_g.picture_w_g.width;
		height = fwin->icon_g.picture_w_g.height;
		if (fwin->icon_alloc_pixels != NULL)
		{
			if (fwin->icon_nalloc_pixels != 0)
			{
				PictureFreeColors(
					dpy, Pcmap, fwin->icon_alloc_pixels,
					fwin->icon_nalloc_pixels, 0,
					fwin->icon_no_limit);
			}
			free(fwin->icon_alloc_pixels);
		}

		fwin->icon_g.picture_w_g.width = save_picture_w_g_width;
		fwin->icon_g.picture_w_g.height = save_picture_w_g_height;
		fwin->iconDepth = save_icon_depth;
		fwin->iconPixmap = save_icon_pixmap;
		fwin->icon_maskPixmap = save_icon_mask;
		fwin->icon_alphaPixmap = save_icon_alpha;
		fwin->icon_nalloc_pixels = save_icon_nalloc_pixels;
		fwin->icon_alloc_pixels = save_icon_alloc_pixels;
		fwin->icon_no_limit = save_icon_no_limit;
		FW_W_ICON_PIXMAP(fwin) = save_icon_pixmap_w;
		SET_ICON_OURS(fwin, is_icon_ours);
		SET_PIXMAP_OURS(fwin, is_pixmap_ours);
		SET_ICON_SHAPED(fwin, is_icon_shaped);
	}

	if (pixmap == None)
		return NULL;

	if (FMiniIconsSupported && orig_icon != NULL)
	{
		int k_width = (is_mini_icon)?
			fwin->ewmh_icon_width : fwin->ewmh_mini_icon_width;
		int k_height = (is_mini_icon)?
			fwin->ewmh_icon_height : fwin->ewmh_mini_icon_height;

		i = 0;
		while(i < s)
		{
			if (i+1 < s)
			{
				if (i + 1 + orig_icon[i]*orig_icon[i+1] < s)
				{
					if (orig_icon[i] == k_width &&
					    orig_icon[i+1] == k_height)
					{
						keep_start = i;
						keep_length =  2 +
						     orig_icon[i]*orig_icon[i+1];
						i = s;
					}
				}
				if (i != s && orig_icon[i]*orig_icon[i+1] > 0)
					i = i + 2 + orig_icon[i]*orig_icon[i+1];
				else
					i = s;
			}
			else
				i = s;
		}
	}

	image = XGetImage(dpy, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
	if (image == NULL)
	{
		fvwm_msg(ERR, "EWMH_SetWmIconFromPixmap",
			 "cannot create XImage\n");
		if (destroy_icon_pix)
		{
			XFreePixmap(dpy, pixmap);
			if (mask != None)
				XFreePixmap(dpy, mask);
			if (alpha != None)
				XFreePixmap(dpy, alpha);
		}
		return NULL;
	}

	if (mask != None)
	{
		m_image = XGetImage(dpy, mask, 0, 0, width, height,
				    AllPlanes, ZPixmap);
	}
	if (alpha != None)
	{
		a_image = XGetImage(dpy, alpha, 0, 0, width, height,
				    AllPlanes, ZPixmap);
	}
	*orig_size = (height*width + 2 + keep_length) * sizeof(CARD32);
	new_icon = (CARD32 *)safemalloc(*orig_size);
	if (keep_length > 0)
	{
		memcpy(new_icon, &orig_icon[keep_start],
		       keep_length * sizeof(CARD32));
	}
	new_icon[keep_length] = width;
	new_icon[1+keep_length] = height;

	c_new_icon = (unsigned char *)new_icon;
	k = 0;
	l = 4 * (2 + keep_length);
	m = 0;

	switch(image->depth)
	{
	case 1:
	{
		XColor colors[2];
		unsigned short fg[3];
		unsigned short bg[3];

		colors[0].pixel = fwin->colors.fore;
		colors[1].pixel = fwin->colors.back;
		XQueryColors(dpy, Pcmap, colors, 2);
		fg[0] = colors[0].blue >> 8;
		fg[1] = colors[0].green >> 8;
		fg[2] = colors[0].red >> 8;
		bg[0] = colors[1].blue >> 8;
		bg[1] = colors[1].green >> 8;
		bg[2] = colors[1].red >> 8;
		for (j = 0; j < height; j++)
		{
			for (i = 0; i < width; i++)
			{
				if (m_image != NULL &&
				    (XGetPixel(m_image, i, j) == 0))
				{
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
				}
				else
				{
					if (XGetPixel(image, i, j) == 0)
					{
						c_new_icon[l++] = bg[0];
						c_new_icon[l++] = bg[1];
						c_new_icon[l++] = bg[2];
						c_new_icon[l++] = 255;
					}
					else
					{
						c_new_icon[l++] = fg[0];
						c_new_icon[l++] = fg[1];
						c_new_icon[l++] = fg[2];
						c_new_icon[l++] = 255;
					}
				}
			}
		}
		break;

	}
	default: /* depth = Pdepth */
	{
		unsigned char *cm;
		XColor *colors;

		colors = (XColor *)safemalloc(width * height * sizeof(XColor));
		cm = (unsigned char *)safemalloc(width * height * sizeof(char));
		for (j = 0; j < height; j++)
		{
			for (i = 0; i < width; i++)
			{
				if (m_image != NULL &&
				    (XGetPixel(m_image, i, j) == 0))
				{
					cm[m++] = 0;
				}
				else
				{
					if (a_image != NULL)
					{
						cm[m++] = (unsigned char)
						    XGetPixel(a_image, i, j);
					}
					else
					{
						cm[m++] = 255;
					}
					colors[k++].pixel =
						XGetPixel(image, i, j);
				}
			}
		}
		for (i = 0; i < k; i += 256)
			XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));

		k = 0;m = 0;
		for (j = 0; j < height; j++)
		{
			for (i = 0; i < width; i++)
			{
				if (cm[m] > 0)
				{
					c_new_icon[l++] = colors[k].blue >> 8;
					c_new_icon[l++] = colors[k].green >> 8;
					c_new_icon[l++] = colors[k].red >> 8;
					c_new_icon[l++] = cm[m];
					k++;
				}
				else
				{
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
					c_new_icon[l++] = 0;
				}
				m++;
			}
		}
		free(colors);
		free(cm);
		break;
	}
	} /* switch */

#if 0 /* this is dramatically slow */
	for (j = 0; j < height; j++)
		for (i = 0; i < width; i++)
		{
			c.pixel = XGetPixel(image, i, j);
			XQueryColor(dpy, Pcmap, &c);
			k = 4*(i + j*width +2 +keep_length);
			c_new_icon[k] = (unsigned short)c.blue >> 8;
			c_new_icon[k+1] = (unsigned short)c.green >> 8;
			c_new_icon[k+2] = (unsigned short)c.red >> 8;
			if (m_image != None && (XGetPixel(m_image, i, j) == 0))
				c_new_icon[k+3] = 0;
			else
				c_new_icon[k+3] = 255;
		}
#endif

	if (is_mini_icon)
	{
		fwin->ewmh_mini_icon_width = width;
		fwin->ewmh_mini_icon_height = height;
	}
	else
	{
		fwin->ewmh_icon_width = width;
		fwin->ewmh_icon_height = height;
	}

	ewmh_ChangeProperty(FW_W(fwin),"_NET_WM_ICON",
			    EWMH_ATOM_LIST_PROPERTY_NOTIFY,
			    (unsigned char *)new_icon,
			    height*width + 2 + keep_length);

	if (destroy_icon_pix)
	{
		XFreePixmap(dpy, pixmap);
		if (mask != None)
			XFreePixmap(dpy, mask);
		if (alpha != None)
			XFreePixmap(dpy, alpha);
	}

	XDestroyImage(image);
	if (m_image != None)
		XDestroyImage(m_image);
	if (a_image != None)
		XDestroyImage(a_image);

	return new_icon;
}

/* ***************************************************************************
 * delete the mini icon and/or the icon from a ewmh icon
 * ***************************************************************************/
void EWMH_DeleteWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon)
{
  CARD32 *list;
  CARD32 *new_list = NULL;
  int keep_start = 0, keep_length = 0;
  int s,i;

  if (mini_icon && icon)
  {
    ewmh_DeleteProperty(
	    FW_W(fwin), "_NET_WM_ICON", EWMH_ATOM_LIST_PROPERTY_NOTIFY);
    fwin->ewmh_mini_icon_width = 0;
    fwin->ewmh_mini_icon_height = 0;
    fwin->ewmh_icon_width = 0;
    fwin->ewmh_icon_height = 0;
    /*SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_NO_ICON);*/
    return;
  }

  list = ewmh_AtomGetByName(FW_W(fwin),"_NET_WM_ICON",
			    EWMH_ATOM_LIST_PROPERTY_NOTIFY, &s);
  if (list == NULL)
    return;
  s = s / sizeof(CARD32);

  if (FMiniIconsSupported && list != NULL)
  {
    int k_width =
      (mini_icon)? fwin->ewmh_icon_width : fwin->ewmh_mini_icon_width;
    int k_height =
      (mini_icon)? fwin->ewmh_icon_height : fwin->ewmh_mini_icon_height;

    i = 0;
    while(i < s)
    {
      if (i+1 < s)
      {
	if (i + 1 + list[i]*list[i+1] < s)
	{
	  if (list[i] == k_width && list[i+1] == k_height)
	  {
	    keep_start = i;
	    keep_length =  2 + list[i]*list[i+1];
	    i = s;
	  }
	}
	if (i != s && list[i]*list[i+1] > 0)
	  i = i + 2 + list[i]*list[i+1];
	else
	  i = s;
      }
      else
	i = s;
    }
  }

  if (keep_length > 0)
  {
    new_list = (CARD32 *)safemalloc(keep_length * sizeof(CARD32));
    memcpy(new_list, &list[keep_start], keep_length * sizeof(CARD32));
  }

  if (new_list != NULL)
  {
    ewmh_ChangeProperty(FW_W(fwin),"_NET_WM_ICON",
			EWMH_ATOM_LIST_PROPERTY_NOTIFY,
			(unsigned char *)new_list,
			keep_length);
  }
  else
  {
    /*SET_HAS_EWMH_WM_ICON_HINT(fwin, EWMH_NO_ICON);*/
    ewmh_DeleteProperty(FW_W(fwin), "_NET_WM_ICON",
			EWMH_ATOM_LIST_PROPERTY_NOTIFY);
  }

  if (mini_icon)
  {
    fwin->ewmh_mini_icon_width = 0;
    fwin->ewmh_mini_icon_height = 0;
  }
  if (icon)
  {
    fwin->ewmh_icon_width = 0;
    fwin->ewmh_icon_height = 0;
  }
  if (new_list != NULL)
    free(new_list);
  free(list);
}

/* ***************************************************************************
 * Create an x image from a NET icon
 * ***************************************************************************/

#define SQUARE(X) ((X)*(X))
static
void extract_wm_icon(CARD32 *list, unsigned int size, int wanted_w, int wanted_h,
		     int *start_best, int *best_w, int *best_h)
{
  unsigned int i = 0, dist = 0;

  *start_best = 0;
  *best_w = 0;
  *best_h = 0;
  size = size / (sizeof(CARD32));

  while (i < size)
  {
    if (i+1 < size)
    {
      if (i + 1 + list[i]*list[i+1] < size)
      {
	if (*best_w == 0 && *best_h == 0)
	{
	  *start_best = i+2;
	  *best_w = list[i];
	  *best_h = list[i+1];
	  dist = SQUARE(list[i]-wanted_w) + SQUARE(list[i+1]-wanted_h);
	}
	else if (SQUARE(list[i]-wanted_w) + SQUARE(list[i+1]-wanted_h) < dist)
	{
	  *start_best = i+2;
	  *best_w = list[i];
	  *best_h = list[i+1];
	  dist = SQUARE(list[i]-wanted_w) + SQUARE(list[i+1]-wanted_h);
	}
      }
      if (list[i]*list[i+1] > 0)
	i = i + 2 + list[i]*list[i+1];
      else
	i = size;
    }
    else
      i = size;
  } /* while */

  return;
}

#define MINI_ICON_WANTED_WIDTH  16
#define MINI_ICON_WANTED_HEIGHT 16
#define MINI_ICON_MAX_WIDTH 22
#define MINI_ICON_MAX_HEIGHT 22
#define ICON_WANTED_WIDTH 56
#define ICON_WANTED_HEIGHT 56
#define ICON_MAX_WIDTH 100
#define ICON_MAX_HEIGHT 100

int EWMH_SetIconFromWMIcon(FvwmWindow *fwin, CARD32 *list, unsigned int size,
			   Bool is_mini_icon)
{
	int start, width, height;
	int wanted_w, wanted_h;
	int max_w, max_h;
	Pixmap pixmap = None;
	Pixmap mask = None;
	Pixmap alpha = None;
	Bool free_list = False;
	int have_alpha;
	int nalloc_pixels;
	Pixel *alloc_pixels;
	int no_limit;
	FvwmPictureAttributes fpa;

	if (list == NULL)
	{
		/* we are called from icons.c or update.c */
		list = ewmh_AtomGetByName(FW_W(fwin),"_NET_WM_ICON",
					  EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);
		free_list = True;
		if (list == NULL)
			return 0;
	}

	if (is_mini_icon)
	{
		wanted_w = MINI_ICON_WANTED_WIDTH;
		wanted_h = MINI_ICON_WANTED_HEIGHT;
		max_w = MINI_ICON_MAX_WIDTH;
		max_h = ICON_MAX_HEIGHT;
		fpa.mask = 0;
	}
	else
	{
		wanted_w = ICON_WANTED_WIDTH;
		wanted_h = ICON_WANTED_HEIGHT;
		max_w = ICON_MAX_WIDTH;
		max_h = ICON_MAX_HEIGHT;
		if (fwin->cs >= 0 && Colorset[fwin->cs].do_dither_icon)
		{
			fpa.mask = FPAM_DITHER;
		}
	}

	extract_wm_icon(list, size, wanted_w, wanted_h, &start, &width, &height);
	if (width == 0 || height == 0)
	{
		if (free_list)
			free(list);
		return 0;
	}

	pixmap = XCreatePixmap(dpy, Scr.NoFocusWin, width, height, Pdepth);
	mask = XCreatePixmap(dpy, Scr.NoFocusWin, width, height, 1);
	if (FRenderGetAlphaDepth())
	{
		alpha = XCreatePixmap(
			dpy, Scr.NoFocusWin, width, height,
			FRenderGetAlphaDepth());
	}
	if (!PImageCreatePixmapFromArgbData(
		dpy, Scr.Root, (unsigned char *)list, start, width, height,
		pixmap, mask, alpha, &have_alpha, &nalloc_pixels, &alloc_pixels,
		&no_limit, fpa) || pixmap == None)
	{
		fvwm_msg(ERR, "EWMH_SetIconFromWMIcon",
			 "fail to create a pixmap\n");
		if (pixmap != None)
			XFreePixmap(dpy, pixmap);
		if (mask != None)
			XFreePixmap(dpy, mask);
		if (alpha != None)
			XFreePixmap(dpy, alpha);
		if (free_list)
			free(list);
		return 0;
	}
	if (!have_alpha && alpha != None)
	{
		XFreePixmap(dpy, alpha);
		alpha = None;
	}

	if (width > max_w || height > max_h)
	{
		Pixmap np = None,nm =None, na = None;

		if (pixmap)
		{
			np = CreateStretchPixmap(dpy, pixmap, width, height,
						 Pdepth, wanted_w, wanted_h,
						 Scr.TitleGC);
			XFreePixmap(dpy, pixmap);
			pixmap = np;
		}
		if (mask)
		{
			nm = CreateStretchPixmap(dpy, mask, width, height,
						 1, wanted_w, wanted_h,
						 Scr.MonoGC);
			XFreePixmap(dpy, mask);
			mask = nm;
		}
		if (alpha)
		{
			GC my_gc = fvwmlib_XCreateGC(dpy, alpha, 0, 0);
			na = CreateStretchPixmap(dpy, alpha, width, height,
						 FRenderGetAlphaDepth(),
						 wanted_w, wanted_h,
						 my_gc);
			XFreePixmap(dpy, alpha);
			XFreeGC(dpy, my_gc);
			alpha = na;
		}
		width = wanted_w;
		height = wanted_h;
	}
	if (FMiniIconsSupported && is_mini_icon &&
	    !DO_EWMH_MINI_ICON_OVERRIDE(fwin))
	{
		char *name = NULL;

		CopyString(&name,"ewmh_mini_icon");
		if (fwin->mini_icon)
		{
			PDestroyFvwmPicture(dpy,fwin->mini_icon);
			fwin->mini_icon = 0;
		}
		fwin->mini_icon = PCacheFvwmPictureFromPixmap(dpy,
							      Scr.NoFocusWin,
							      name,
							      pixmap,mask,alpha,
							      width, height,
							      nalloc_pixels,
							      alloc_pixels,
							      no_limit);
		if (fwin->mini_icon != NULL)
		{
			fwin->mini_pixmap_file = name;
			BroadcastFvwmPicture(M_MINI_ICON,
					     FW_W(fwin), FW_W_FRAME(fwin),
					     (unsigned long)fwin,
					     fwin->mini_icon,
					     fwin->mini_pixmap_file);
			border_redraw_decorations(fwin);
		}
	}
	if (!is_mini_icon)
	{
		fwin->iconPixmap = pixmap;
		fwin->icon_maskPixmap = mask;
		fwin->icon_alphaPixmap = alpha;
		fwin->icon_nalloc_pixels = nalloc_pixels;
		fwin->icon_alloc_pixels = alloc_pixels;
		fwin->icon_no_limit = no_limit;
		fwin->icon_g.picture_w_g.width = width;
		fwin->icon_g.picture_w_g.height = height;
		fwin->iconDepth = Pdepth;
		SET_PIXMAP_OURS(fwin, 1);
		if (FShapesSupported && mask)
			SET_ICON_SHAPED(fwin, 1);
	}
	if (free_list)
		free(list);
	return 1;
}

#endif /* HAVE_EWMH */
