/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"
#include "libs/Module.h"
#include "libs/Graphics.h"
#include "libs/PictureGraphics.h"
#include "fvwm/fvwm.h"
#include "FvwmPager.h"

#define label_border 2

static void do_label_window(PagerWindow *t, Window w, rectangle r);
static void do_picture_window(PagerWindow *t, Window w, rectangle r);
static void do_border_window(PagerWindow *t, Window w, rectangle r);

void update_monitor_locations(int i)
{
	if (!HilightDesks)
		return;

	struct fpmonitor *fp;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->disabled)
			continue;

		rectangle vp = set_vp_size_and_loc(fp, false);

		/* Conditions to show a monitor view port are:
		 *   + Never show monitors not being tracked.
		 *   + Always shown when CurrntDeskPerMonitor is in use.
		 *   + Otherwise only show monitors on desk.
		 */
		if ((monitor_to_track == NULL || monitor_to_track == fp) &&
		    ((CurrentDeskPerMonitor && fAlwaysCurrentDesk) ||
		    fp->m->virtual_scr.CurrentDesk == i + desk1))
		{
			XMoveResizeWindow(dpy, fp->CPagerWin[i],
				vp.x, vp.y, vp.width, vp.height);
		} else {
			XMoveResizeWindow(dpy, fp->CPagerWin[i],
				-32768, -32768, vp.width, vp.height);
		}
	}
}

void update_monitor_backgrounds(int desk)
{
	if (!HilightDesks)
		return;

	rectangle vp;
	struct fpmonitor *fp;
	DeskStyle *style = Desks[desk].style;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (CurrentDeskPerMonitor && fAlwaysCurrentDesk)
			style = FindDeskStyle(fp->m->virtual_scr.CurrentDesk);

		if (style->hi_cs < 0) {
			if (style->hiPixmap != NULL)
				XSetWindowBackgroundPixmap(dpy,
					fp->CPagerWin[desk],
					style->hiPixmap->picture);
			else
				XSetWindowBackground(dpy,
					fp->CPagerWin[desk], style->hi_bg);
		} else {
			vp = set_vp_size_and_loc(fp, false);
			SetWindowBackground(dpy,
				fp->CPagerWin[desk], vp.width, vp.height,
				&Colorset[style->hi_cs], Pdepth,
				style->hi_bg_gc, True);
		}
		XLowerWindow(dpy, fp->CPagerWin[desk]);
		XClearWindow(dpy, fp->CPagerWin[desk]);
	}
}

void update_desk_background(int desk)
{
	DeskStyle *style = Desks[desk].style;

	XSetWindowBorder(dpy, Desks[desk].title_w, style->fg);
	XSetWindowBorder(dpy, Desks[desk].w, style->fg);

	if (style->cs < 0) {
		XSetWindowBorder(dpy, Desks[desk].title_w, style->fg);
		XSetWindowBorder(dpy, Desks[desk].w, style->fg);
		if (style->bgPixmap != NULL) {
			if (style->use_label_pixmap)
				XSetWindowBackgroundPixmap(dpy,
					Desks[desk].title_w,
					style->bgPixmap->picture);
			else
				XSetWindowBackgroundPixmap(dpy,
					Desks[desk].w,
					style->bgPixmap->picture);
		} else {
			XSetWindowBackground(dpy,
				Desks[desk].title_w, style->bg);
			XSetWindowBackground(dpy,
				Desks[desk].w, style->bg);
		}
	} else {
		if (style->use_label_pixmap)
			SetWindowBackground(dpy,
				Desks[desk].title_w, desk_w, desk_h + label_h,
				&Colorset[style->cs], Pdepth, Scr.NormalGC,
				True);
		else
			SetWindowBackground(dpy,
				Desks[desk].w, desk_w, desk_h,
				&Colorset[style->cs], Pdepth, Scr.NormalGC,
				True);
	}

	XClearWindow(dpy, Desks[desk].title_w);
	XClearWindow(dpy, Desks[desk].w);
}

void update_window_background(PagerWindow *t)
{
	if (t == NULL || t->PagerView == None || t->IconView == None)
		return;

	if (Pdepth < 2)
	{
		if (t == FocusWin) {
			XSetWindowBackgroundPixmap(
				dpy, t->PagerView, Scr.gray_pixmap);
			XSetWindowBackgroundPixmap(
				dpy, t->IconView, Scr.gray_pixmap);
		} else if (IS_STICKY_ACROSS_DESKS(t) ||
			   IS_STICKY_ACROSS_PAGES(t))
		{
			XSetWindowBackgroundPixmap(dpy, t->PagerView,
						   Scr.sticky_gray_pixmap);
			XSetWindowBackgroundPixmap(dpy, t->IconView,
						   Scr.sticky_gray_pixmap);
		} else {
			XSetWindowBackgroundPixmap(dpy, t->PagerView,
						   Scr.light_gray_pixmap);
			XSetWindowBackgroundPixmap(dpy, t->IconView,
						   Scr.light_gray_pixmap);
		}
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
		return;
	}

	int cs = -1;
	Pixel pix = None;
	DeskStyle *style = FindDeskStyle(t->desk);

	if (t == FocusWin) {
		if (style->focus_cs >= 0) {
			cs = style->focus_cs;
		} else if (style->focus_bg) {
			pix = style->focus_bg;
		} else {
			pix = Scr.focus_win_bg;
		}
	} else {
		if (style->win_cs >= 0) {
			cs = style->win_cs;
		} else if (style->win_bg) {
			pix = style->win_bg;
		} else {
			pix = t->back;
		}
	}

	if (cs >= 0) {
		SetWindowBackground(dpy, t->PagerView,
			t->pager_view.width, t->pager_view.height,
			&Colorset[cs], Pdepth, Scr.NormalGC, True);
		SetWindowBackground(dpy, t->IconView,
			t->icon_view.width, t->icon_view.height,
			&Colorset[cs], Pdepth, Scr.NormalGC, True);
	} else if (pix != None) {
		XSetWindowBackground(dpy, t->PagerView, pix);
		XSetWindowBackground(dpy, t->IconView, pix);
	}

	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
}

/* update all the parental relative windows: pr background change */
void update_pr_transparent_windows(void)
{
	int cset;
	PagerWindow *t;

	for(cset = 0; cset < ndesks; cset++) {
		if (CSET_IS_TRANSPARENT_PR(Desks[cset].style->cs))
			update_desk_background(cset);
		if (CSET_IS_TRANSPARENT_PR(Desks[cset].style->hi_cs))
			update_monitor_backgrounds(cset);
	}

	/* subordinate windows with a pr parent desk */
	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		DeskStyle *style = FindDeskStyle(t->desk);

		cset = (t == FocusWin) ? style->focus_cs : style->win_cs;
		if (!CSET_IS_TRANSPARENT_PR(cset) ||
		    (fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[0].style->cs)) ||
		    (!fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[t->desk].style->cs)))
		{
			continue;
		}
		update_window_background(t);
	}

	/* ballon */
	if (Balloon.is_mapped && CSET_IS_TRANSPARENT_PR(Balloon.cs))
			XClearArea(dpy, Scr.balloon_w, 0, 0, 0, 0, True);
}

void update_desk_style_gcs(DeskStyle *style)
{
	initialize_colorset(style);

	/* Don't update GC's for default style. */
	if (style->desk < 0)
		return;

	XSetForeground(dpy, style->label_gc, style->fg);
	XSetForeground(dpy, style->dashed_gc, style->fg);
	XSetForeground(dpy, style->hi_bg_gc, style->hi_bg);
	XSetForeground(dpy, style->hi_fg_gc, style->hi_fg);
	if (WindowBorders3d && style->win_cs > 0) {
		XSetForeground(dpy, style->win_hi_gc,
			       Colorset[style->win_cs].hilite);
		XSetForeground(dpy, style->win_sh_gc,
			       Colorset[style->win_cs].shadow);
	}
	if (WindowBorders3d && style->focus_cs > 0) {
		XSetForeground(dpy, style->focus_hi_gc,
			       Colorset[style->focus_cs].hilite);
		XSetForeground(dpy, style->focus_sh_gc,
			       Colorset[style->focus_cs].shadow);
	}
}

void update_pager_window_decor(PagerWindow *t)
{
	do_label_window(t, t->PagerView, t->pager_view);
	do_picture_window(t, t->PagerView, t->pager_view);
	do_border_window(t, t->PagerView, t->pager_view);
}

void update_icon_window_decor(PagerWindow *t)
{
	do_label_window(t, t->IconView, t->icon_view);
	do_picture_window(t, t->IconView, t->icon_view);
	do_border_window(t, t->IconView, t->icon_view);
}

void update_window_decor(PagerWindow *t)
{
	update_pager_window_decor(t);
	update_icon_window_decor(t);
}

void do_label_window(PagerWindow *t, Window w, rectangle r)
{
	if (t == NULL || w == None || Scr.winFfont == NULL ||
	    (MiniIcons && t->mini_icon.picture) || /* Draw image later. */
	    t->icon_name == NULL)
		return;

	int cs;
	Pixel pix;
	DeskStyle *style = FindDeskStyle(t->desk);

	if (t == FocusWin) {
		pix = (style->focus_fg < ULONG_MAX) ? style->focus_fg : Scr.focus_win_fg;
		cs = style->focus_cs;
	} else {
		pix = (style->win_fg < ULONG_MAX) ? style->win_fg : t->text;
		cs = style->win_cs;
	}
	XSetForeground(dpy, Scr.NormalGC, pix);

	free(t->window_label);
	t->window_label = get_label(t, WindowLabelFormat);

	if (FftSupport)
		XClearWindow(dpy, w);
	FwinString->win = w;
	FwinString->gc = Scr.NormalGC;
	FwinString->flags.has_colorset = False;
	if (cs >= 0) {
		FwinString->colorset = &Colorset[cs];
		FwinString->flags.has_colorset = True;
	}
	FwinString->x = label_border;
	FwinString->y = Scr.winFfont->ascent+2;

	/* draw the window label with simple greedy wrapping */
	char *cur, *next, *end;
	int space_width, cur_width;

	space_width = FlocaleTextWidth(Scr.winFfont, " ", 1);
	cur_width   = 0;
	cur = next = t->window_label;
	end = cur + strlen(cur);

	while (*cur) {
		while (*next) {
			int width;
			char *p;

			if (!(p = strchr(next, ' ')))
				p = end;

			width = FlocaleTextWidth(
				Scr.winFfont, next, p - next );
			if (width > r.width - cur_width -
			    space_width - 2*label_border)
				break;
			cur_width += width + space_width;
			next = *p ? p + 1 : p;
		}

		if (cur == next) {
			/* word too large for window */
			while (*next) {
				int len, width;

				len = FlocaleStringNumberOfBytes(
					Scr.winFfont, next);
				width = FlocaleTextWidth(
					Scr.winFfont, next, len);

				if (width > r.width - cur_width -
				    2 * label_border && cur != next)
					break;

				next += len;
				cur_width += width;
			}
		}

		FwinString->str = fxmalloc(next - cur + 1);
		strncpy(FwinString->str, cur, next - cur);
		FwinString->str[next - cur] = 0;

		FlocaleDrawString(dpy, Scr.winFfont, FwinString, 0);

		free(FwinString->str);
		FwinString->str = NULL;

		FwinString->y += Scr.winFfont->height;
		cur = next;
		cur_width = 0;
	}
}

void do_picture_window(PagerWindow *t, Window w, rectangle r)
{
	if (t == NULL || !MiniIcons || !(t->mini_icon.picture))
		return;

	int iconX, iconY;
	int src_x = 0, src_y = 0;
	int dest_w, dest_h;
	int cs;
	FvwmRenderAttributes fra;
	DeskStyle *style = FindDeskStyle(t->desk);

	cs = (t != FocusWin) ? style->win_cs : style->focus_cs;
	dest_w = t->mini_icon.width;
	dest_h = t->mini_icon.height;
	if (r.width > t->mini_icon.width) {
		iconX = (r.width - t->mini_icon.width) / 2;
	} else if (r.width < t->mini_icon.width) {
		iconX = 0;
		src_x = (t->mini_icon.width - r.width) / 2;
		dest_w = r.width;
	} else {
		iconX = 0;
	}
	if (r.height > t->mini_icon.height) {
		iconY = (r.height - t->mini_icon.height) / 2;
	} else if (r.height < t->mini_icon.height) {
		iconY = 0;
		src_y = (t->mini_icon.height - r.height) / 2;
		dest_h = r.height;
	} else {
		iconY = 0;
	}

	fra.mask = FRAM_DEST_IS_A_WINDOW;
	if (t->mini_icon.alpha != None ||
	    (cs >= 0 && Colorset[cs].icon_alpha_percent < 100))
	{
		XClearArea(dpy, w, iconX, iconY, t->mini_icon.width,
			   t->mini_icon.height, False);
	}

	if (cs >= 0) {
		fra.mask |= FRAM_HAVE_ICON_CSET;
		fra.colorset = &Colorset[cs];
	}

	PGraphicsRenderPicture(dpy, w, &t->mini_icon, &fra, w,
			       Scr.MiniIconGC, None, None, src_x, src_y,
			       t->mini_icon.width - src_x,
			       t->mini_icon.height - src_y,
			       iconX, iconY, dest_w, dest_h, False);
}

void do_border_window(PagerWindow *t, Window w, rectangle r)
{
	if (t == NULL)
		return;

	DeskStyle *style = FindDeskStyle(t->desk);

	if (WindowBorders3d && t == FocusWin && style->focus_cs > 0) {
		RelieveRectangle(
				dpy, w, 0, 0, r.width - 1, r.height - 1,
				style->focus_hi_gc, style->focus_sh_gc,
				WindowBorderWidth);
		return;
	} else if (WindowBorders3d && t != FocusWin && style->win_cs > 0) {
		RelieveRectangle(
				dpy, w, 0, 0, r.width - 1, r.height - 1,
				style->win_hi_gc, style->win_sh_gc,
				WindowBorderWidth);
		return;
	}

	if (t == FocusWin) {
		if (style->focus_fg < ULONG_MAX)
			XSetForeground(dpy,
				style->focus_hi_gc, style->focus_fg);
		else
			XSetForeground(dpy,
				style->focus_hi_gc, Scr.focus_win_fg);
		RelieveRectangle(
				dpy, w, 0, 0, r.width - 1, r.height - 1,
				style->focus_hi_gc, style->focus_hi_gc,
				WindowBorderWidth);
	} else {
		if (style->win_fg < ULONG_MAX)
			XSetForeground(dpy,
				style->win_hi_gc, style->win_fg);
		else
			XSetForeground(dpy,
				style->win_hi_gc, t->text);
		RelieveRectangle(
				dpy, w, 0, 0, r.width - 1, r.height - 1,
				style->win_hi_gc, style->win_hi_gc,
				WindowBorderWidth);
	}
}
