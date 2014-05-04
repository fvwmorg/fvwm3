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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Graphics.h"
#include "libs/PictureGraphics.h"
#include "libs/Rectangles.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "menudim.h"
#include "menustyle.h"
#include "menuitem.h"
#include "decorations.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */
static void clear_menu_item_background(
	MenuPaintItemParameters *mpip, int x, int y, int w, int h)
{
	MenuStyle *ms = mpip->ms;

	if (!ST_HAS_MENU_CSET(ms) &&
	    ST_FACE(ms).type == GradientMenu &&
	    (ST_FACE(ms).gradient_type == D_GRADIENT ||
	     ST_FACE(ms).gradient_type == B_GRADIENT))
	{
		XEvent e;

		e.xexpose.x = x;
		e.xexpose.y = y;
		e.xexpose.width = w;
		e.xexpose.height = h;
		mpip->cb_reset_bg(mpip->cb_mr, &e);
	}
	else
	{
		XClearArea(dpy, mpip->w, x, y, w, h, False);
	}
}

/*
 *
 *  Draws two horizontal lines to form a separator
 *
 */
static void draw_separator(
	Window w, GC TopGC, GC BottomGC, int x1, int y, int x2)
{
	XDrawLine(dpy, w, TopGC   , x1,   y,   x2,   y);
	XDrawLine(dpy, w, BottomGC, x1-1, y+1, x2+1, y+1);

	return;
}

/*
 *
 *  Draws a tear off bar.  Similar to a separator, but with a dashed line.
 *
 */
static void draw_tear_off_bar(
	Window w, GC TopGC, GC BottomGC, int x1, int y, int x2)
{
	XGCValues xgcv;
	int width;
	int offset;

	xgcv.line_style = LineOnOffDash;
	xgcv.dashes = MENU_TEAR_OFF_BAR_DASH_WIDTH;
	XChangeGC(dpy, TopGC, GCLineStyle | GCDashList, &xgcv);
	XChangeGC(dpy, BottomGC, GCLineStyle | GCDashList, &xgcv);
	width = (x2 - x1 + 1);
	offset = (width / MENU_TEAR_OFF_BAR_DASH_WIDTH) *
		MENU_TEAR_OFF_BAR_DASH_WIDTH;
	offset = (width - offset) / 2;
	x1 += offset;
	x2 += offset;
	XDrawLine(dpy, w, TopGC,    x1, y,     x2, y);
	XDrawLine(dpy, w, BottomGC, x1, y + 1, x2, y + 1);
	xgcv.line_style = LineSolid;
	XChangeGC(dpy, TopGC, GCLineStyle, &xgcv);
	XChangeGC(dpy, BottomGC, GCLineStyle, &xgcv);

	return;
}

static void draw_highlight_background(
	struct MenuPaintItemParameters *mpip, int x, int y, int width,
	int height, colorset_t *cs, GC gc)
{
	if (cs != NULL && cs->pixmap && cs->pixmap_type != PIXMAP_TILED)
	{
		Pixmap p;

		p = CreateOffsetBackgroundPixmap(
			dpy, mpip->w, 0, 0, width, height, cs, Pdepth, gc,
			False);
		switch (cs->pixmap_type)
		{
		case PIXMAP_STRETCH_X:
			/* todo: optimize to only create one pixmap and gc per
			 * mr. */
		case PIXMAP_STRETCH_Y:
		{
			XGCValues gcv;
			int gcm;
			GC bgc;

			gcv.tile = p;
			gcv.fill_style = FillTiled;
			gcm = GCFillStyle | GCTile;

			/* vertcal gradients has to be aligned properly */
			if (cs->pixmap_type == PIXMAP_STRETCH_Y)
			{
				gcv.ts_y_origin = y;
				gcm|=GCTileStipYOrigin;
			}
			else if (cs->pixmap_type == PIXMAP_STRETCH_X)
			{
				gcv.ts_x_origin = x;
				gcm|=GCTileStipXOrigin;
			}
			bgc = fvwmlib_XCreateGC(dpy, mpip->w, gcm, &gcv);
			XFillRectangle(dpy, mpip->w, bgc, x, y, width, height);
			XFreeGC(dpy, bgc);
			break;
		}
		default:
			XCopyArea(dpy, p, mpip->w, gc, 0, 0, width, height,
				  x, y);
			break;
		}
		XFreePixmap(dpy, p);
	}
	else
	{
		XFillRectangle(dpy, mpip->w, gc, x, y, width, height);
	}
}

/* ---------------------------- interface functions ------------------------ */

/* Allocates a new, empty menu item */
struct MenuItem *menuitem_create(void)
{
	MenuItem *mi;

	mi = xcalloc(1, sizeof *mi);
	return mi;
}

/* Frees a menu item and all of its allocated resources. */
void menuitem_free(struct MenuItem *mi)
{
	int i;

	if (!mi)
	{
		return;
	}
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		if (MI_LABEL(mi)[i] != NULL)
		{
			free(MI_LABEL(mi)[i]);
		}
	}
	if (MI_ACTION(mi) != NULL)
	{
		free(MI_ACTION(mi));
	}
	if (MI_PICTURE(mi))
	{
		PDestroyFvwmPicture(dpy, MI_PICTURE(mi));
	}
	for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
	{
		if (MI_MINI_ICON(mi)[i])
		{
			PDestroyFvwmPicture(dpy, MI_MINI_ICON(mi)[i]);
		}
	}
	free(mi);

	return;
}

/* Duplicate a menu item into newly allocated memory.  The new item is
 * completely independent of the old one. */
struct MenuItem *menuitem_clone(struct MenuItem *mi)
{
	MenuItem *new_mi;
	int i;

	/* copy everything */
	new_mi = xmalloc(sizeof(MenuItem));
	memcpy(new_mi, mi, sizeof(MenuItem));
	/* special treatment for a few parts */
	MI_NEXT_ITEM(new_mi) = NULL;
	MI_PREV_ITEM(new_mi) = NULL;
	MI_WAS_DESELECTED(new_mi) = 0;
	if (MI_ACTION(mi) != NULL)
	{
		MI_ACTION(new_mi) = xstrdup(MI_ACTION(mi));
	}
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		if (MI_LABEL(mi)[i] != NULL)
		{
			MI_LABEL(new_mi)[i] = xstrdup(MI_LABEL(mi)[i]);
		}
	}
	if (MI_PICTURE(mi) != NULL)
	{
		MI_PICTURE(new_mi) = PCloneFvwmPicture(MI_PICTURE(mi));
	}
	for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
	{
		if (MI_MINI_ICON(mi)[i] != NULL)
		{
			MI_MINI_ICON(new_mi)[i] =
				PCloneFvwmPicture(MI_MINI_ICON(mi)[i]);
		}
	}

	return new_mi;
}

/* Calculate the size of the various parts of the item.  The sizes are returned
 * through mipst. */
void menuitem_get_size(
	struct MenuItem *mi, struct MenuItemPartSizesT *mipst,
	FlocaleFont *font, Bool do_reverse_icon_order)
{
	int i;
	int j;
	int w;

	memset(mipst, 0, sizeof(MenuItemPartSizesT));
	if (MI_IS_POPUP(mi))
	{
		mipst->triangle_width = MENU_TRIANGLE_WIDTH;
	}
	else if (MI_IS_TITLE(mi) && !MI_HAS_PICTURE(mi))
	{
		Bool is_formatted = False;

		/* titles stretch over the whole menu width, so count the
		 * maximum separately if the title is unformatted. */
		for (j = 1; j < MAX_MENU_ITEM_LABELS; j++)
		{
			if (MI_LABEL(mi)[j] != NULL)
			{
				is_formatted = True;
				break;
			}
			else
			{
				MI_LABEL_OFFSET(mi)[j] = 0;
			}
		}
		if (!is_formatted && MI_LABEL(mi)[0] != NULL)
		{
			MI_LABEL_STRLEN(mi)[0] =
				strlen(MI_LABEL(mi)[0]);
			w = FlocaleTextWidth(
				font, MI_LABEL(mi)[0], MI_LABEL_STRLEN(mi)[0]);
			MI_LABEL_OFFSET(mi)[0] = w;
			MI_IS_TITLE_CENTERED(mi) = True;
			if (mipst->title_width < w)
			{
				mipst->title_width = w;
			}
			return;
		}
	}
	/* regular item or formatted title */
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		if (MI_LABEL(mi)[i])
		{
			MI_LABEL_STRLEN(mi)[i] = strlen(MI_LABEL(mi)[i]);
			w = FlocaleTextWidth(
				font, MI_LABEL(mi)[i], MI_LABEL_STRLEN(mi)[i]);
			MI_LABEL_OFFSET(mi)[i] = w;
			if (mipst->label_width[i] < w)
			{
				mipst->label_width[i] = w;
			}
		}
	}
	if (MI_PICTURE(mi) && mipst->picture_width < MI_PICTURE(mi)->width)
	{
		mipst->picture_width = MI_PICTURE(mi)->width;
	}
	for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
	{
		if (MI_MINI_ICON(mi)[i])
		{
			int k;

			/* Reverse mini icon order for left submenu style. */
			k = (do_reverse_icon_order == True) ?
				MAX_MENU_ITEM_MINI_ICONS - 1 - i : i;
			mipst->icon_width[k] = MI_MINI_ICON(mi)[i]->width;
		}
	}

	return;
}

/*
 *
 *  Procedure:
 *      menuitem_paint - draws a single entry in a popped up menu
 *
 *      mr - the menu instance that holds the menu item
 *      mi - the menu item to redraw
 *      fw - the FvwmWindow structure to check against allowed functions
 *
 */
void menuitem_paint(
	struct MenuItem *mi, struct MenuPaintItemParameters *mpip)
{
	struct MenuStyle *ms = mpip->ms;
	struct MenuDimensions *dim = mpip->dim;

	static FlocaleWinString *fws = NULL;
	int y_offset;
	int text_y;
	int y_height;
	int x;
	int y;
	int lit_x_start;
	int lit_x_end;
	gc_quad_t gcs;
	gc_quad_t off_gcs;
	int cs = -1;
	int off_cs;
	FvwmRenderAttributes fra;
	/*Pixel fg, fgsh;*/
	int relief_thickness = ST_RELIEF_THICKNESS(ms);
	Bool is_item_selected;
	Bool item_cleared = False;
	Bool xft_clear = False;
	Bool empty_inter = False;
	XRectangle b;
	Region region = None;
	int i;
	int sx1;
	int sx2;
	FlocaleFont* font;

	if (!mi)
	{
		return;
	}
	is_item_selected = (mi == mpip->selected_item);

	if (MI_IS_TITLE(mi))
	{
		font = ST_PTITLEFONT(ms);
	}
	else
	{
		font = ST_PSTDFONT(ms);
	}

	y_offset = MI_Y_OFFSET(mi);
	y_height = MI_HEIGHT(mi);
	if (MI_IS_SELECTABLE(mi))
	{
		text_y = y_offset + MDIM_ITEM_TEXT_Y_OFFSET(*dim);
	}
	else
	{
		text_y = y_offset + font->ascent +
			ST_TITLE_GAP_ABOVE(ms);
	}
	/* center text vertically if the pixmap is taller */
	if (MI_PICTURE(mi))
	{
		text_y += MI_PICTURE(mi)->height;
	}
	for (i = 0; i < mpip->used_mini_icons; i++)
	{
		y = 0;
		if (MI_MINI_ICON(mi)[i])
		{
			if (MI_MINI_ICON(mi)[i]->height > y)
			{
				y = MI_MINI_ICON(mi)[i]->height;
			}
		}
		y -= font->height;
		if (y > 1)
		{
			text_y += y / 2;
		}
	}

	off_cs = ST_HAS_MENU_CSET(ms) ? ST_CSET_MENU(ms) : -1;
	/* Note: it's ok to pass a NULL label to is_function_allowed. */
	if (
		!IS_EWMH_DESKTOP_FW(mpip->fw) &&
		!is_function_allowed(
			MI_FUNC_TYPE(mi), MI_LABEL(mi)[0], mpip->fw,
			RQORIG_PROGRAM_US, False))
	{
		gcs = ST_MENU_STIPPLE_GCS(ms);
		off_gcs = gcs;
		off_cs = ST_HAS_GREYED_CSET(ms) ? ST_CSET_GREYED(ms) : -1;
	}
	else if (is_item_selected)
	{
		gcs = ST_MENU_ACTIVE_GCS(ms);
		off_gcs = ST_MENU_INACTIVE_GCS(ms);
	}
	else if (MI_IS_TITLE(mi))
	{
		gcs = ST_MENU_TITLE_GCS(ms);
		off_gcs = ST_MENU_INACTIVE_GCS(ms);
	}
	else
	{
		gcs = ST_MENU_INACTIVE_GCS(ms);
		off_gcs = ST_MENU_INACTIVE_GCS(ms);
	}
	if (is_item_selected)
	{
		cs = (ST_HAS_ACTIVE_CSET(ms)) ? ST_CSET_ACTIVE(ms) : -1;
	}
	else if (MI_IS_TITLE(mi))
	{
		cs = (ST_HAS_TITLE_CSET(ms)) ? ST_CSET_TITLE(ms) : off_cs;
	}
	else
	{
		cs = off_cs;
	}

	/*
	 * Hilight the item.
	 */
	if (FftSupport && ST_PSTDFONT(ms)->fftf.fftfont != NULL)
	{
		xft_clear = True;
	}

	/* Hilight or clear the background. */
	lit_x_start = -1;
	lit_x_end = -1;
	if (is_item_selected &&
	    (ST_DO_HILIGHT_BACK(ms) || ST_DO_HILIGHT_FORE(ms)))
	{
		/* Hilight the background. */
		if (MDIM_HILIGHT_WIDTH(*dim) - 2 * relief_thickness > 0)
		{
			lit_x_start = MDIM_HILIGHT_X_OFFSET(*dim) +
				relief_thickness;
			lit_x_end = lit_x_start + MDIM_HILIGHT_WIDTH(*dim) -
				2 * relief_thickness;
			if (ST_DO_HILIGHT_BACK(ms))
			{
				draw_highlight_background(
					mpip, lit_x_start,
					y_offset + relief_thickness,
					lit_x_end - lit_x_start,
					y_height - relief_thickness,
					(cs >= 0 ? &Colorset[cs] : NULL),
					gcs.back_gc);
				item_cleared = True;
			}
		}
	}
	else if ((MI_WAS_DESELECTED(mi) &&
		  (relief_thickness > 0 ||
		   ST_DO_HILIGHT_BACK(ms) || ST_DO_HILIGHT_FORE(ms)) &&
		  (ST_FACE(ms).type != GradientMenu || ST_HAS_MENU_CSET(ms))))
	{
		int x1;
		int x2;
		/* we clear if xft_clear and !ST_HAS_MENU_CSET(ms) as the
		 * non colorset code is too complicate ... olicha */
		int d = 0;

		if (MI_PREV_ITEM(mi) &&
		    mpip->selected_item == MI_PREV_ITEM(mi))
		{
			/* Don't paint over the hilight relief. */
			d = relief_thickness;
		}
		/* Undo the hilighting. */
		x1 = min(
			MDIM_HILIGHT_X_OFFSET(*dim), MDIM_ITEM_X_OFFSET(*dim));
		x2 = max(
			MDIM_HILIGHT_X_OFFSET(*dim) + MDIM_HILIGHT_WIDTH(*dim),
			MDIM_ITEM_X_OFFSET(*dim) + MDIM_ITEM_WIDTH(*dim));
		clear_menu_item_background(
			mpip, x1, y_offset + d, x2 - x1,
			y_height + relief_thickness - d);
		item_cleared = True;
	}
	else if (MI_IS_TITLE(mi))
	{
		lit_x_start = MDIM_ITEM_X_OFFSET(*dim);
		lit_x_end = lit_x_start + MDIM_ITEM_WIDTH(*dim);
		/* Hilight the background. */
		if (
			MDIM_HILIGHT_WIDTH(*dim) > 0 &&
			ST_DO_HILIGHT_TITLE_BACK(ms))
		{
			draw_highlight_background(
				mpip, lit_x_start,
				y_offset + relief_thickness,
				lit_x_end - lit_x_start,
				y_height - relief_thickness,
				(cs >= 0 ? &Colorset[cs] : NULL),
				gcs.back_gc);
			item_cleared = True;
		}
	}

	MI_WAS_DESELECTED(mi) = False;
	memset(&fra, 0, sizeof(fra));
	fra.mask = 0;

	/* Hilight 3D */
	if (is_item_selected && relief_thickness > 0)
	{
		GC rgc;
		GC sgc;

		rgc = gcs.hilight_gc;
		sgc = gcs.shadow_gc;
		if (ST_IS_ITEM_RELIEF_REVERSED(ms))
		{
			GC tgc = rgc;

			/* swap gcs for reversed relief */
			rgc = sgc;
			sgc = tgc;
		}
		if (MDIM_HILIGHT_WIDTH(*dim) - 2 * relief_thickness > 0)
		{
			/* The relief reaches down into the next item, hence
			 * the value for the second y coordinate:
			 * MI_HEIGHT(mi) + 1 */
			RelieveRectangle(
				dpy, mpip->w,  MDIM_HILIGHT_X_OFFSET(*dim),
				y_offset, MDIM_HILIGHT_WIDTH(*dim) - 1,
				MI_HEIGHT(mi) - 1 + relief_thickness, rgc, sgc,
				relief_thickness);
		}
	}


	/*
	 * Draw the item itself.
	 */

	/* Calculate the separator offsets. */
	if (ST_HAS_LONG_SEPARATORS(ms))
	{
		sx1 = MDIM_ITEM_X_OFFSET(*dim) + relief_thickness;
		sx2 = MDIM_ITEM_X_OFFSET(*dim) + MDIM_ITEM_WIDTH(*dim) - 1 -
			relief_thickness;
	}
	else
	{
		sx1 = MDIM_ITEM_X_OFFSET(*dim) + relief_thickness +
			MENU_SEPARATOR_SHORT_X_OFFSET;
		sx2 = MDIM_ITEM_X_OFFSET(*dim) + MDIM_ITEM_WIDTH(*dim) - 1 -
			relief_thickness - MENU_SEPARATOR_SHORT_X_OFFSET;
	}
	if (MI_IS_SEPARATOR(mi))
	{
		if (sx1 < sx2)
		{
			/* It's a separator. */
			draw_separator(
				mpip->w, gcs.shadow_gc, gcs.hilight_gc, sx1,
				y_offset + y_height - MENU_SEPARATOR_HEIGHT,
				sx2);
			/* Nothing else to do. */
		}
		return;
	}
	else if (MI_IS_TEAR_OFF_BAR(mi))
	{
		int tx1;
		int tx2;

		tx1 = MDIM_ITEM_X_OFFSET(*dim) + relief_thickness +
			MENU_TEAR_OFF_BAR_X_OFFSET;
		tx2 = MDIM_ITEM_X_OFFSET(*dim) + MDIM_ITEM_WIDTH(*dim) - 1 -
			relief_thickness -
			MENU_TEAR_OFF_BAR_X_OFFSET;
		if (tx1 < tx2)
		{

			/* It's a tear off bar. */
			draw_tear_off_bar(
				mpip->w, gcs.shadow_gc, gcs.hilight_gc, tx1,
				y_offset + relief_thickness +
				MENU_TEAR_OFF_BAR_Y_OFFSET, tx2);
		}
		/* Nothing else to do. */
		return;
	}
	else if (MI_IS_TITLE(mi))
	{
		/* Separate the title. */
		if (ST_TITLE_UNDERLINES(ms) > 0 && !mpip->flags.is_first_item)
		{
			int add = (MI_IS_SELECTABLE(MI_PREV_ITEM(mi))) ?
				relief_thickness : 0;

			text_y += MENU_SEPARATOR_HEIGHT + add;
			y = y_offset + add;
			if (sx1 < sx2)
			{
				draw_separator(
					mpip->w, gcs.shadow_gc, gcs.hilight_gc,
					sx1, y, sx2);
			}
		}
		/* Underline the title. */
		switch (ST_TITLE_UNDERLINES(ms))
		{
		case 0:
			break;
		case 1:
			if (MI_NEXT_ITEM(mi) != NULL)
			{
				y = y_offset + y_height - MENU_SEPARATOR_HEIGHT;
				draw_separator(
					mpip->w, gcs.shadow_gc, gcs.hilight_gc,
					sx1, y, sx2);
			}
			break;
		default:
			for (i = ST_TITLE_UNDERLINES(ms); i-- > 0; )
			{
				y = y_offset + y_height - 1 -
					i * MENU_UNDERLINE_HEIGHT;
				XDrawLine(
					dpy, mpip->w, gcs.shadow_gc, sx1, y,
					sx2, y);
			}
			break;
		}
	}

	/*
	 * Draw the labels.
	 */
	if (fws == NULL)
	{
		FlocaleAllocateWinString(&fws);
	}
	fws->win = mpip->w;
	fws->y = text_y;
	fws->flags.has_colorset = 0;
	b.y = text_y - font->ascent;
	b.height = font->height + 1; /* ? */
	if (!item_cleared && mpip->ev)
	{
		int u,v;
		if (!frect_get_seg_intersection(
			    mpip->ev->xexpose.y, mpip->ev->xexpose.height,
			    b.y, b.height, &u, &v))
		{
			/* empty intersection */
			empty_inter = True;
		}
		b.y = u;
		b.height = v;
	}

	for (i = MAX_MENU_ITEM_LABELS; i-- > 0; )
	{
		if (!empty_inter && MI_LABEL(mi)[i] && *(MI_LABEL(mi)[i]))
		{
			Bool draw_string = True;
			int text_width;
			int tmp_cs;

			if (MI_LABEL_OFFSET(mi)[i] >= lit_x_start &&
			    MI_LABEL_OFFSET(mi)[i] < lit_x_end)
			{
				/* label is in hilighted area */
				fws->gc = gcs.fore_gc;
				tmp_cs = cs;
			}
			else
			{
				/* label is in unhilighted area */
				fws->gc = off_gcs.fore_gc;
				tmp_cs = off_cs;
			}
			if (tmp_cs >= 0)
			{
				fws->colorset = &Colorset[tmp_cs];
				fws->flags.has_colorset = 1;
			}
			fws->str = MI_LABEL(mi)[i];
			b.x = fws->x = MI_LABEL_OFFSET(mi)[i];
			b.width = text_width = FlocaleTextWidth(
				font, fws->str, strlen(fws->str));

			if (!item_cleared && mpip->ev)
			{
				int s_x,s_w;
				if (frect_get_seg_intersection(
					    mpip->ev->xexpose.x,
					    mpip->ev->xexpose.width,
					    fws->x, text_width,
					    &s_x, &s_w))
				{
					b.x = s_x;
					b.width = s_w;
					region = XCreateRegion();
					XUnionRectWithRegion(
						&b, region, region);
					fws->flags.has_clip_region = True;
					fws->clip_region = region;
					draw_string = True;
					XSetRegion(dpy, fws->gc, region);
				}
				else
				{
					/* empty intersection */
					draw_string = False;
				}
			}
			if (draw_string)
			{
				if (!item_cleared && xft_clear)
				{
					clear_menu_item_background(
						mpip, b.x, b.y, b.width,
						b.height);
				}
				FlocaleDrawString(dpy, font, fws, 0);

				/* hot key */
				if (MI_HAS_HOTKEY(mi) && !MI_IS_TITLE(mi) &&
				    (!MI_IS_HOTKEY_AUTOMATIC(mi) ||
				     ST_USE_AUTOMATIC_HOTKEYS(ms)) &&
				    MI_HOTKEY_COLUMN(mi) == i)
				{
					FlocaleDrawUnderline(
						dpy, ST_PSTDFONT(ms), fws,
						MI_HOTKEY_COFFSET(mi));
				}
			}
		}
		if (region)
		{
			XDestroyRegion(region);
			region = None;
			fws->flags.has_clip_region = False;
			fws->clip_region = None;
			XSetClipMask(dpy, fws->gc, None);
		}
	}

	/*
	 * Draw the submenu triangle.
	 */

	if (MI_IS_POPUP(mi))
	{
		GC tmp_gc;

		if (MDIM_TRIANGLE_X_OFFSET(*dim) >= lit_x_start &&
		    MDIM_TRIANGLE_X_OFFSET(*dim) < lit_x_end &&
		    is_item_selected)
		{
			/* triangle is in hilighted area */
			if (ST_TRIANGLES_USE_FORE(ms))
			{
				tmp_gc = gcs.fore_gc;
			}
			else
			{
				tmp_gc = gcs.hilight_gc;
			}
		}
		else
		{
			/* triangle is in unhilighted area */
			if (ST_TRIANGLES_USE_FORE(ms))
			{
				tmp_gc = off_gcs.fore_gc;
			}
			else
			{
				tmp_gc = off_gcs.hilight_gc;
			}
		}
		y = y_offset + (y_height - MENU_TRIANGLE_HEIGHT +
				relief_thickness) / 2;

		if (ST_TRIANGLES_USE_FORE(ms))
		{
			DrawTrianglePattern(
				dpy, mpip->w, tmp_gc, tmp_gc, tmp_gc,
				MDIM_TRIANGLE_X_OFFSET(*dim), y,
				MENU_TRIANGLE_WIDTH, MENU_TRIANGLE_HEIGHT, 0,
				(mpip->flags.is_left_triangle) ? 'l' : 'r',
				ST_HAS_TRIANGLE_RELIEF(ms),
				!ST_HAS_TRIANGLE_RELIEF(ms), is_item_selected);

		}
		else
		{
			DrawTrianglePattern(
				dpy, mpip->w, gcs.hilight_gc, gcs.shadow_gc,
				tmp_gc,	MDIM_TRIANGLE_X_OFFSET(*dim), y,
				MENU_TRIANGLE_WIDTH, MENU_TRIANGLE_HEIGHT, 0,
				(mpip->flags.is_left_triangle) ? 'l' : 'r',
				ST_HAS_TRIANGLE_RELIEF(ms),
				!ST_HAS_TRIANGLE_RELIEF(ms), is_item_selected);
		}
	}

	/*
	 * Draw the item picture.
	 */

	if (MI_PICTURE(mi))
	{
		GC tmp_gc;
		int tmp_cs;
		Bool draw_picture = True;

		x = menudim_middle_x_offset(mpip->dim) -
			MI_PICTURE(mi)->width / 2;
		y = y_offset + ((MI_IS_SELECTABLE(mi)) ? relief_thickness : 0);
		if (x >= lit_x_start && x < lit_x_end)
		{
			tmp_gc = gcs.fore_gc;
			tmp_cs = cs;
		}
		else
		{
			tmp_gc = off_gcs.fore_gc;
			tmp_cs = off_cs;
		}
		fra.mask = FRAM_DEST_IS_A_WINDOW;
		if (tmp_cs >= 0)
		{
			fra.mask |= FRAM_HAVE_ICON_CSET;
			fra.colorset = &Colorset[tmp_cs];
		}
		b.x = x;
		b.y = y;
		b.width = MI_PICTURE(mi)->width;
		b.height = MI_PICTURE(mi)->height;
		if (!item_cleared && mpip->ev)
		{
			if (!frect_get_intersection(
				    mpip->ev->xexpose.x, mpip->ev->xexpose.y,
				    mpip->ev->xexpose.width,
				    mpip->ev->xexpose.height,
				    b.x, b.y, b.width, b.height, &b))
			{
				draw_picture = False;
			}
		}
		if (draw_picture)
		{
			if (
				!item_cleared &&
				(MI_PICTURE(mi)->alpha != None ||
				 (tmp_cs >=0 &&
				  Colorset[tmp_cs].icon_alpha_percent < 100)))
			{
				clear_menu_item_background(
					mpip, b.x, b.y, b.width, b.height);
			}
			PGraphicsRenderPicture(
				dpy, mpip->w, MI_PICTURE(mi), &fra,
				mpip->w, tmp_gc, Scr.MonoGC, Scr.AlphaGC,
				b.x - x, b.y - y, b.width, b.height,
				b.x, b.y, b.width, b.height, False);
		}
	}

	/*
	 * Draw the mini icons.
	 */

	for (i = 0; i < mpip->used_mini_icons; i++)
	{
		int k;
		Bool draw_picture = True;

		/* We need to reverse the mini icon order for left submenu
		 * style. */
		k = (ST_USE_LEFT_SUBMENUS(ms)) ?
			mpip->used_mini_icons - 1 - i : i;

		if (MI_MINI_ICON(mi)[i])
		{
			GC tmp_gc;
			int tmp_cs;

			if (MI_PICTURE(mi))
			{
				y = y_offset + MI_HEIGHT(mi) -
					MI_MINI_ICON(mi)[i]->height;
			}
			else
			{
				y = y_offset +
					(MI_HEIGHT(mi) +
					 ((MI_IS_SELECTABLE(mi)) ?
					  relief_thickness : 0) -
					 MI_MINI_ICON(mi)[i]->height) / 2;
			}
			if (MDIM_ICON_X_OFFSET(*dim)[k] >= lit_x_start &&
			    MDIM_ICON_X_OFFSET(*dim)[k] < lit_x_end)
			{
				/* icon is in hilighted area */
				tmp_gc = gcs.fore_gc;
				tmp_cs = cs;
			}
			else
			{
				/* icon is in unhilighted area */
				tmp_gc = off_gcs.fore_gc;
				tmp_cs = off_cs;
			}
			fra.mask = FRAM_DEST_IS_A_WINDOW;
			if (tmp_cs >= 0)
			{
				fra.mask |= FRAM_HAVE_ICON_CSET;
				fra.colorset = &Colorset[tmp_cs];
			}
			b.x = MDIM_ICON_X_OFFSET(*dim)[k];
			b.y = y;
			b.width = MI_MINI_ICON(mi)[i]->width;
			b.height = MI_MINI_ICON(mi)[i]->height;
			if (!item_cleared && mpip->ev)
			{
				if (!frect_get_intersection(
					    mpip->ev->xexpose.x,
					    mpip->ev->xexpose.y,
					    mpip->ev->xexpose.width,
					    mpip->ev->xexpose.height,
					    b.x, b.y, b.width, b.height, &b))
				{
					draw_picture = False;
				}
			}
			if (draw_picture)
			{
				if (!item_cleared &&
				    (MI_MINI_ICON(mi)[i]->alpha != None
				     || (tmp_cs >=0 &&
					 Colorset[tmp_cs].icon_alpha_percent <
					 100)))
				{
					clear_menu_item_background(
						mpip,
						b.x, b.y, b.width, b.height);
				}
				PGraphicsRenderPicture(
					dpy, mpip->w, MI_MINI_ICON(mi)[i],
					&fra, mpip->w, tmp_gc, Scr.MonoGC,
					Scr.AlphaGC,
					b.x - MDIM_ICON_X_OFFSET(*dim)[k],
					b.y - y, b.width, b.height,
					b.x, b.y, b.width, b.height, False);
			}
		}
	}

	return;
}

/* returns the center y coordinate of the menu item */
int menuitem_middle_y_offset(struct MenuItem *mi, struct MenuStyle *ms)
{
	int r;

	if (!mi)
	{
		return ST_BORDER_WIDTH(ms);
	}
	r = (MI_IS_SELECTABLE(mi)) ? ST_RELIEF_THICKNESS(ms) : 0;

	return MI_Y_OFFSET(mi) + (MI_HEIGHT(mi) + r) / 2;
}
