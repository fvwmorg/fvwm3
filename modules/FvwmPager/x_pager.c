/* -*-c-*- */
/*
 * This module is all new
 * by Rob Nation
 */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 *
 * fvwm pager handling code
 *
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <stdbool.h>

#include "libs/FScreen.h"
#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Colorset.h"
#include "libs/Graphics.h"
#include "fvwm/fvwm.h"
#include "libs/PictureGraphics.h"
#include "libs/FEvent.h"
#include "FvwmPager.h"

static Atom wm_del_win;
static int desk_w = 0;
static int desk_h = 0;
static int label_h = 0;
static int Wait = 0;
static int MyVx, MyVy;		/* copy of Scr.Vx/y for drag logic */
static struct fpmonitor *ScrollFp = NULL;	/* Stash monitor drag logic */

/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = {0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = {0x08, 0x02};

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] = {0x01, 0x02, 0x04, 0x08};

#define label_border g_width

static int FvwmErrorHandler(Display *, XErrorEvent *);
static rectangle CalcGeom(PagerWindow *, bool);
static void HideWindow(PagerWindow *, Window);
static void MoveResizeWindow(PagerWindow *, bool, bool);
static void DrawInBalloonWindow(void);
static rectangle set_vp_size_and_loc(struct fpmonitor *, bool is_icon);
static struct fpmonitor *fpmonitor_from_xy(int x, int y);
static struct fpmonitor *fpmonitor_from_n(int n);
static int fpmonitor_count(void);
static void fvwmrec_to_pager(rectangle *, bool, struct fpmonitor *);
static void pagerrec_to_fvwm(rectangle *, bool, struct fpmonitor *);
static char *get_label(const PagerWindow *pw,const char *fmt);
static int is_window_visible(PagerWindow *t);
static void draw_desk_label(const char *label, const char *small, int desk,
			int x, int y, int width, int height, bool hilight);

#define  MAX_UNPROCESSED_MESSAGES 1
/* sums up pixels to scroll. If do_send_message is true a Scroll command is
 * sent back to fvwm. The function shall be called with is_message_recieved
 * true when the Scroll command has been processed by fvwm. This is checked
 * by talking to ourself. */
static void do_scroll(int sx, int sy, bool do_send_message,
		      bool is_message_recieved)
{
	static int psx = 0;
	static int psy = 0;
	static int messages_sent = 0;
	char command[256], screen[32] = "";
	psx+=sx;
	psy+=sy;
	if (is_message_recieved)
	{
		/* There might be other modules with the same name, or someone
		 might send ScrollDone other than the module, so just treat
		 any negative count as zero. */
		if (--messages_sent < 0)
		{
			messages_sent = 0;
		}
	}
	if ((do_send_message || messages_sent < MAX_UNPROCESSED_MESSAGES) &&
	    ( psx != 0 || psy != 0 ))
	{
		if (monitor_to_track != NULL)
			snprintf(screen, sizeof(screen), "screen %s",
				 monitor_to_track->m->si->name);
		else if (ScrollFp != NULL)
			snprintf(screen, sizeof(screen), "screen %s",
				 ScrollFp->m->si->name);
		snprintf(command, sizeof(command), "Scroll %s %dp %dp",
					 screen, psx, psy);
		SendText(fd, command, 0);
		messages_sent++;
		SendText(fd, "Send_Reply ScrollDone", 0);
		psx = 0;
		psy = 0;
	}
}

void HandleScrollDone(void)
{
	do_scroll(0, 0, true, true);
	ScrollFp = NULL;
}

typedef struct
{
	int event_type;
	XEvent *ret_last_event;
} _weed_window_events_args;

static int _pred_weed_window_events(
	Display *display, XEvent *current_event, XPointer arg)
{
	_weed_window_events_args *args = (_weed_window_events_args *)arg;

	if (current_event->type == args->event_type)
	{
		if (args->ret_last_event != NULL)
		{
			*args->ret_last_event = *current_event;
		}

		return 1;
	}

	return 0;
}

/* discard certain events on a window */
static void discard_events(long event_type, Window w, XEvent *last_ev)
{
	_weed_window_events_args args;

	XSync(dpy, 0);
	args.event_type = event_type;
	args.ret_last_event = last_ev;
	FWeedIfWindowEvents(dpy, w, _pred_weed_window_events, (XPointer)&args);

	return;
}

int fpmonitor_get_all_widths(void)
{
	struct fpmonitor *fp = TAILQ_FIRST(&fp_monitor_q);

	if (fp->scr_width <= 0)
		return (monitor_get_all_widths());

	return (fp->scr_width);
}

int fpmonitor_get_all_heights(void)
{
	struct fpmonitor *fp = TAILQ_FIRST(&fp_monitor_q);

	if (fp->scr_height <= 0)
		return (monitor_get_all_heights());
	return (fp->scr_height);
}

int fpmonitor_count(void)
{
	struct fpmonitor *fp;
	int c = 0;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry)
		if (!fp->disabled)
			c++;
	return (c);
}

static struct fpmonitor *fpmonitor_from_n(int n)
{
	struct fpmonitor *fp;
	int c = 0;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->disabled)
			continue;
		if (c == n)
			return fp;
		c++;
	}
	return fp;
}

struct fpmonitor *fpmonitor_from_desk(int desk)
{
	struct fpmonitor *fp;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry)
		if (!fp->disabled && fp->m->virtual_scr.CurrentDesk == desk)
			return fp;

	return NULL;
}

static struct fpmonitor *fpmonitor_from_xy(int x, int y)
{
	struct fpmonitor *fp;

	if (monitor_to_track == NULL && monitor_mode != MONITOR_TRACKING_G) {
		x %= fpmonitor_get_all_widths();
		y %= fpmonitor_get_all_heights();

		TAILQ_FOREACH(fp, &fp_monitor_q, entry)
			if (x >= fp->m->si->x &&
			    x < fp->m->si->x + fp->m->si->w &&
			    y >= fp->m->si->y &&
			    y < fp->m->si->y + fp->m->si->h)
				return fp;
	} else {
		return fpmonitor_this(NULL);
	}

	return NULL;
}

static rectangle CalcGeom(PagerWindow *t, bool is_icon)
{
	/* Place initial rectangle off screen. */
	rectangle rec = {-32768, -32768, MinSize, MinSize};
	struct fpmonitor *fp;

	/* Determine what monitor to use. */
	if (monitor_to_track != NULL) {
		fp = monitor_to_track;
		if (t->m != fp->m)
			return rec;
	} else if (IsShared && !is_icon && monitor_to_track == NULL) {
		int desk = t->desk - desk1;
		if (desk >= 0 && desk < ndesks)
			fp = Desks[desk].fp;
		else
			return rec;
		if (fp == NULL || fp->m != t->m)
			return rec;
	} else {
		fp = fpmonitor_this(t->m);
	}
	if (fp == NULL)
		return rec;

	rec.x = fp->virtual_scr.Vx + t->x;
	rec.y = fp->virtual_scr.Vy + t->y;
	rec.width = t->width;
	rec.height = t->height;

	fvwmrec_to_pager(&rec, is_icon, fp);

	/* Set geometry to MinSize and snap to page boundary if needed */
	if (rec.width < MinSize)
	{
		int page_w = desk_w;
		if (is_icon)
			page_w = icon.width;
		page_w = page_w / fp->virtual_scr.VxPages;

		if (page_w == 0)
			page_w = 1;

		rec.width = MinSize;
		if (rec.x > page_w - MinSize &&
			(rec.x + rec.width)%page_w < MinSize)
		{
			rec.x = ((rec.x / page_w) + 1)*page_w - MinSize;
		}
	}
	if (rec.height < MinSize)
	{
		int page_h = desk_h;
		if (is_icon)
			page_h = icon.height;
		page_h = page_h / fp->virtual_scr.VyPages;

		if (page_h == 0)
			page_h = 1;

		rec.height = MinSize;
		if (rec.y > page_h - MinSize &&
			(rec.y + rec.height)%page_h < MinSize)
		{
			rec.y = ((rec.y / page_h) + 1)*page_h - MinSize;
		}
	}
	return rec;
}

/*
 *
 *  Procedure:
 *	Initialize_viz_pager - creates a temp window of the correct visual
 *	so that pixmaps may be created for use with the main window
 */
void initialize_viz_pager(void)
{
	XSetWindowAttributes attr;
	XGCValues xgcv;

	/* FIXME: I think that we only need that Pdepth ==
	 * DefaultDepth(dpy, Scr.screen) to use the Scr.Root for Scr.pager_w */
	if (Pdefault)
	{
		Scr.pager_w = Scr.Root;
	}
	else
	{
		attr.background_pixmap = None;
		attr.border_pixel = 0;
		attr.colormap = Pcmap;
		Scr.pager_w = XCreateWindow(
			dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth,
			InputOutput, Pvisual,
			CWBackPixmap|CWBorderPixel|CWColormap, &attr);
		Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.pager_w, 0, &xgcv);
	}
	Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.pager_w, 0, NULL);

	xgcv.plane_mask = AllPlanes;
	Scr.MiniIconGC = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCPlaneMask, &xgcv);

	/* Transparent background are only allowed when the depth matched the
	 * root */
	if (Pdepth == DefaultDepth(dpy, Scr.screen))
	{
		default_pixmap = ParentRelative;
	}

	return;
}

void set_desk_size(bool update_label)
{
	if (update_label) {
		label_h = 0;
		if (use_desk_label)
			label_h += Ffont->height + 2;
		if (use_monitor_label)
			label_h += Ffont->height + 2;
	}

	desk_w = (pwindow.width - Columns + 1) / Columns;
	desk_h = (pwindow.height - Rows * label_h - Rows + 1) / Rows;

	return;
}

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

/*
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 */
XSizeHints sizehints =
{
	(PWinGravity),		/* flags */
	0, 0, 100, 100,		/* x, y, width and height (legacy) */
	1, 1,			/* Min width and height */
	0, 0,			/* Max width and height */
	1, 1,			/* Width and height increments */
	{0, 0}, {0, 0},		/* Aspect ratio */
	1, 1,			/* base size */
	(NorthWestGravity)	/* gravity */
};

/* create balloon window -- ric@giccs.georgetown.edu */
void initialize_balloon_window(void)
{
	XGCValues xgcv;
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	Balloon.is_mapped = false;
	if (!(Balloon.show_in_pager || Balloon.show_in_icon)) {
		Scr.balloon_w = None;
		return;
	}

	/* Set some defaults */
	if (Balloon.border_width <= 0)
		Balloon.border_width = DEFAULT_BALLOON_BORDER_WIDTH;
	if (Balloon.y_offset == 0)
		/* we don't allow y_offset of 0 because it allows direct
		 * transit from pager window to balloon window, setting
		 * up a LeaveNotify/EnterNotify event loop.
		 */
		Balloon.y_offset = DEFAULT_BALLOON_Y_OFFSET;
	if (Balloon.label_format == NULL)
		Balloon.label_format = fxstrdup("%i");

	valuemask = CWOverrideRedirect | CWEventMask | CWColormap;
	/* tell WM to ignore this window */
	attributes.override_redirect = true;
	attributes.event_mask = ExposureMask;
	attributes.colormap = Pcmap;
	/* now create the window */
	Scr.balloon_w = XCreateWindow(
		dpy, Scr.Root, 0, 0, /* coords set later */ 1, 1,
		Balloon.border_width, Pdepth, InputOutput, Pvisual, valuemask,
		&attributes);

	/* Initialize, actual values will be updated later. */
	valuemask = GCForeground;
	xgcv.foreground = GetSimpleColor("black");
	Balloon.Ffont = FlocaleLoadFont(dpy, BalloonFont, MyName);
	Balloon.gc = fvwmlib_XCreateGC(
		dpy, Scr.balloon_w, valuemask, &xgcv);
	free(BalloonFont);
}

/* Computes pager size rectangle from fvwm size or visa versa */
static void
fvwmrec_to_pager(rectangle *rec, bool is_icon, struct fpmonitor *fp)
{
	if (fp == NULL)
		return;

	int m_width = fpmonitor_get_all_widths();
	int m_height = fpmonitor_get_all_heights();

	if (monitor_to_track != NULL || (IsShared && !is_icon)) {
		/* Offset window location based on monitor location. */
		rec->x -= (m_width - fp->m->si->w) *
			  (rec->x / m_width) + fp->m->si->x;
		rec->y -= (m_height - fp->m->si->h) *
			  (rec->y / m_height) + fp->m->si->y;
		m_width = fp->m->si->w;
		m_height = fp->m->si->h;
	}
	m_width *= fp->virtual_scr.VxPages;
	m_height *= fp->virtual_scr.VyPages;

	int scale_w = desk_w, scale_h = desk_h;
	if ( is_icon ) {
		scale_w = icon.width;
		scale_h = icon.height;
	}

	/* To deal with rounding issues, first compute the right/bottom
	 * location of the window so the rounding is consistent for windows
	 * of different sizes at the same position.
	 */
	rec->width = ((rec->x + rec->width) * scale_w) / m_width;
	rec->height = ((rec->y + rec->height) * scale_h) / m_height;
	rec->x = (rec->x * scale_w) / m_width;
	rec->y = (rec->y * scale_h) / m_height;
	rec->width -= rec->x;
	rec->height -= rec->y;
}
static void
pagerrec_to_fvwm(rectangle *rec, bool is_icon, struct fpmonitor *fp)
{
	if (fp == NULL)
		return;

	int m_width = fpmonitor_get_all_widths();
	int m_height = fpmonitor_get_all_heights();
	int offset_x = 0, offset_y = 0;

	int scale_w = desk_w, scale_h = desk_h;
	if ( is_icon ) {
		scale_w = icon.width;
		scale_h = icon.height;
	}

	if (monitor_to_track != NULL || (IsShared && !is_icon)) {
		offset_x = (m_width - fp->m->si->w) *
			   ((fp->virtual_scr.VxPages *
			   rec->x) / scale_w) + fp->m->si->x;
		offset_y = (m_height - fp->m->si->h) *
			   ((fp->virtual_scr.VyPages *
			   rec->y) / scale_h) + fp->m->si->y;
		m_width = fp->m->si->w;
		m_height = fp->m->si->h;
	}

	m_width = m_width * fp->virtual_scr.VxPages;
	m_height = m_height * fp->virtual_scr.VyPages;
	rec->width = (rec->width * m_width) / scale_w;
	rec->height = (rec->height * m_height) / scale_h;
	rec->x = (rec->x * m_width) / scale_w + offset_x;
	rec->y = (rec->y * m_height) / scale_h + offset_y;
}

static rectangle
set_vp_size_and_loc(struct fpmonitor *m, bool is_icon)
{
	int Vx = 0, Vy = 0, vp_w, vp_h;
	rectangle vp = {0, 0, 0, 0};
	struct fpmonitor *fp = (m != NULL) ? m : fpmonitor_this(NULL);
	int scale_w = desk_w, scale_h = desk_h;
	if (is_icon) {
		scale_w = icon.width;
		scale_h = icon.height;
	}

	Vx = fp->virtual_scr.Vx;
	Vy = fp->virtual_scr.Vy;
	vp_w = fp->virtual_scr.VWidth / fp->virtual_scr.VxPages;
	vp_h = fp->virtual_scr.VHeight / fp->virtual_scr.VyPages;
	if (m != NULL && monitor_to_track == NULL &&
	   !(IsShared && !is_icon)) {
		Vx += fp->m->si->x;
		Vy += fp->m->si->y;
		vp_w = fp->m->si->w;
		vp_h = fp->m->si->h;
	}

	vp.x = (Vx * scale_w) / fp->virtual_scr.VWidth;
	vp.y = (Vy * scale_h) / fp->virtual_scr.VHeight;
	vp.width = (Vx + vp_w) * scale_w / fp->virtual_scr.VWidth - vp.x;
	vp.height = (Vy + vp_h) * scale_h / fp->virtual_scr.VHeight - vp.y;
	return vp;
}

void initialise_common_pager_fragments(void)
{
	extern char *font_string, *smallFont;
	DeskStyle *style = FindDeskStyle(-1);

	/* I don't think that this is necessary - just let pager die */
	/* domivogt (07-mar-1999): But it is! A window being moved in the pager
	 * might die at any moment causing the Xlib calls to generate BadMatch
	 * errors. Without an error handler the pager will die! */
	XSetErrorHandler(FvwmErrorHandler);

	wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

	/* load the font */
	/* Note: "font" is always created, whether labels are used or not
	   because a GC below is set to use a font. dje Dec 2001.
	   OK, I fixed the GC below, but now something else is blowing up.
	   Right now, I've got to do some Real Life stuff, so this kludge is
	   in place, its still better than I found it.
	   I hope that I've fixed this (olicha)
	*/
	Ffont = FlocaleLoadFont(dpy, font_string, MyName);

	/* init our Flocale window string */
	FlocaleAllocateWinString(&FwinString);

	/* Check that shape extension exists. */
	if (FHaveShapeExtension && ShapeLabels)
	{
		ShapeLabels = (FShapesSupported) ? 1 : 0;
	}

	if(smallFont != NULL)
	{
		FwindowFont = FlocaleLoadFont(dpy, smallFont, MyName);
	}

	/* Load pixmaps for mono use */
	if (Pdepth < 2)
	{
		Scr.gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.pager_w, g_bits, g_width, g_height,
			style->fg, style->bg, Pdepth);
		Scr.light_gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.pager_w, l_g_bits, l_g_width, l_g_height,
			style->fg, style->bg, Pdepth);
		Scr.sticky_gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.pager_w, s_g_bits, s_g_width, s_g_height,
			style->fg, style->bg, Pdepth);
	}

	/* Size the window */
	if(Rows < 0)
	{
		if(Columns <= 0)
		{
			Columns = ndesks;
			Rows = 1;
		}
		else
	{
			Rows = ndesks/Columns;
			if(Rows*Columns < ndesks)
			Rows++;
		}
	}
	if(Columns < 0)
	{
		if (Rows == 0)
			Rows = 1;
		Columns = ndesks/Rows;
		if(Rows*Columns < ndesks)
			Columns++;
	}

	if(Rows*Columns < ndesks)
	{
		if (Columns == 0)
			Columns = 1;
		Rows = ndesks/Columns;
		if (Rows*Columns < ndesks)
			Rows++;
	}
	set_desk_size(true);
}

void initialize_fpmonitor_windows(struct fpmonitor *fp)
{
	/* No need to create windows if not using HilightDesks. */
	if (!HilightDesks)
		return;

	int i;
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	rectangle vp = set_vp_size_and_loc(fp, false);

	attributes.colormap = Pcmap;
	for (i = 0; i < ndesks; i++) {
		valuemask = CWColormap;
		if (Desks[i].style->hiPixmap ||
		    (Desks[i].style->hi_cs > -1 &&
		    Colorset[Desks[i].style->hi_cs].pixmap))
		{
			valuemask |= CWBackPixmap;
			attributes.background_pixmap = None; /* set later */
		} else {
			valuemask |= CWBackPixel;
			attributes.background_pixel = Desks[i].style->hi_bg;
		}

		fp->CPagerWin[i] = XCreateWindow(dpy, Desks[i].w,
				-32768, -32768, vp.width, vp.height, 0,
				CopyFromParent, InputOutput,
				CopyFromParent, valuemask,
				&attributes);
		XMapRaised(dpy, fp->CPagerWin[i]);
		update_monitor_locations(i);
		update_monitor_backgrounds(i);
	}
}

void update_desk_style_gcs(DeskStyle *style)
{
	if (style->cs >= 0) {
		style->fg = Colorset[style->cs].fg;
		style->bg = Colorset[style->cs].bg;
	}
	if (style->hi_cs >= 0) {
		style->hi_fg = Colorset[style->hi_cs].fg;
		style->hi_bg = Colorset[style->hi_cs].bg;
	}
	if (style->win_cs >= 0) {
		style->win_fg = Colorset[style->win_cs].fg;
		style->win_bg = Colorset[style->win_cs].bg;
	}
	if (style->focus_cs >= 0) {
		style->focus_fg = Colorset[style->focus_cs].fg;
		style->focus_bg = Colorset[style->focus_cs].bg;
	}
	if (style->balloon_cs >= 0) {
		style->balloon_fg = Colorset[style->balloon_cs].fg;
		style->balloon_bg = Colorset[style->balloon_bg].bg;
	}

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

void initialize_desk_style_gcs(DeskStyle *style)
{
	XGCValues gcv;
	char dash_list[2];

	/* Desk labels GC. */
	gcv.foreground = style->fg;
	if (Ffont && Ffont->font) {
		gcv.font = Ffont->font->fid;
		style->label_gc = fvwmlib_XCreateGC(
			dpy, Scr.pager_w, GCForeground | GCFont, &gcv);
	} else {
		style->label_gc = fvwmlib_XCreateGC(
			dpy, Scr.pager_w, GCForeground, &gcv);
	}

	/* Hilight GC, used for monitors and labels backgrounds. */
	gcv.foreground = (Pdepth < 2) ? style->bg : style->hi_bg;
	style->hi_bg_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);

	/* create the hilight desk title drawing GC */
	gcv.foreground = (Pdepth < 2) ? style->fg : style->hi_fg;
	if (Ffont && Ffont->font)
		style->hi_fg_gc = fvwmlib_XCreateGC(
			dpy, Scr.pager_w, GCForeground | GCFont, &gcv);
	else
		style->hi_fg_gc = fvwmlib_XCreateGC(
			dpy, Scr.pager_w, GCForeground, &gcv);

	/* create the virtual page boundary GC */
	gcv.foreground = style->fg;
	gcv.line_width = 1;
	gcv.line_style = (use_dashed_separators) ? LineOnOffDash : LineSolid;
	style->dashed_gc = fvwmlib_XCreateGC(dpy,
		Scr.pager_w, GCForeground | GCLineStyle | GCLineWidth, &gcv);
	if (use_dashed_separators) {
		/* Although this should already be the default for a freshly
		 * created GC, some X servers do not draw properly dashed
		 * lines if the dash style is not set explicitly.
		 */
		dash_list[0] = 4;
		dash_list[1] = 4;
		XSetDashes(dpy, style->dashed_gc, 0, dash_list, 2);
	}

	/* Window borders. */
	gcv.foreground = (style->win_cs) ?
			 Colorset[style->win_cs].hilite : style->fg;
	style->win_hi_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);
	gcv.foreground = (style->win_cs) ?
			 Colorset[style->win_cs].shadow : style->fg;
	style->win_sh_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);

	/* Focus window borders. */
	gcv.foreground = (style->focus_cs) ?
			 Colorset[style->focus_cs].hilite : style->fg;
	style->focus_hi_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);
	gcv.foreground = (style->focus_cs) ?
			 Colorset[style->focus_cs].shadow : style->fg;
	style->focus_sh_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);
}

void initialize_desk_windows(int desk)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	DeskStyle *style = Desks[desk].style;

	/* Common settings for both title and desk window. */
	attributes.colormap = Pcmap;
	attributes.background_pixmap = default_pixmap;
	attributes.border_pixel = style->fg;

	/* Note, pixmaps are set to None to be updated later. */
	valuemask = (CWBorderPixel | CWColormap | CWEventMask);
	if (style->use_label_pixmap && style->cs >= 0) {
		if (Colorset[style->cs].pixmap) {
			valuemask |= CWBackPixmap;
			attributes.background_pixmap = None;
		} else {
			valuemask |= CWBackPixel;
			attributes.background_pixel = style->bg;
		}
	} else if (style->use_label_pixmap && style->bgPixmap) {
		valuemask |= CWBackPixmap;
		attributes.background_pixmap = None;
	} else {
		valuemask |= CWBackPixel;
		attributes.background_pixel = style->bg;
	}

	attributes.event_mask = (ExposureMask | ButtonReleaseMask);
	Desks[desk].title_w = XCreateWindow(dpy, Scr.pager_w,
			-1, -1, desk_w, desk_h + label_h, 1,
			CopyFromParent, InputOutput, CopyFromParent,
			valuemask, &attributes);

	/* Create desk windows. */
	valuemask &= ~(CWBackPixel | CWBackPixmap);
	if (style->cs >= 0) {
		valuemask |= CWBackPixmap;
		if (style->use_label_pixmap) {
			attributes.background_pixmap = ParentRelative;
			attributes.background_pixel = None;
		} else {
			valuemask |= CWBackPixel;
			attributes.background_pixmap = None;
			attributes.background_pixel = style->bg;
		}
	} else if (Desks[desk].style->bgPixmap) {
		valuemask |= CWBackPixmap;
		if (Desks[desk].style->use_label_pixmap)
			attributes.background_pixmap = ParentRelative;
		else
			attributes.background_pixmap = None;
	} else {
		valuemask |= CWBackPixel;
		attributes.background_pixel = Desks[desk].style->bg;
	}

	attributes.event_mask = (ExposureMask | ButtonReleaseMask |
				 ButtonPressMask | ButtonMotionMask);
	Desks[desk].w = XCreateWindow(dpy, Desks[desk].title_w,
			-1, LabelsBelow ? -1 : label_h - 1, desk_w, desk_h, 1,
			CopyFromParent, InputOutput, CopyFromParent,
			valuemask, &attributes);

	XMapRaised(dpy, Desks[desk].title_w);
	XMapRaised(dpy, Desks[desk].w);
}

void initialize_pager(void)
{
  XWMHints wmhints;
  XClassHint class1;
  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i = 0;
  struct fpmonitor *fp = fpmonitor_this(NULL);
  struct fpmonitor *m;
  DeskStyle *style;

  int VxPages = fp->virtual_scr.VxPages, VyPages = fp->virtual_scr.VyPages;

  /* Set window size if not fully set by user to match */
  /* aspect ratio of monitor(s) being shown. */
  if ( pwindow.width == 0 || pwindow.height == 0 ) {
	  int vWidth  = fpmonitor_get_all_widths();
	  int vHeight = fpmonitor_get_all_heights();
	  if (monitor_to_track != NULL || IsShared) {
		  vWidth = fp->m->si->w;
		  vHeight = fp->m->si->h;
	  }

	  if (pwindow.width > 0) {
		  pwindow.height = (pwindow.width * vHeight) / vWidth +
			  label_h * Rows + Rows;
	  } else if (pwindow.height > label_h * Rows) {
		  pwindow.width = ((pwindow.height - label_h * Rows + Rows) *
				  vWidth) / vHeight + Columns;
	  } else {
		  pwindow.width = (VxPages * vWidth * Columns) / Scr.VScale +
			  Columns;
		  pwindow.height = (VyPages * vHeight * Rows) / Scr.VScale +
			  label_h * Rows + Rows;
	  }
  }
  set_desk_size(false);

  if (is_transient)
  {
    rectangle screen_g;
    fscreen_scr_arg fscr;

    fscr.xypos.x = pwindow.x;
    fscr.xypos.y = pwindow.y;
    FScreenGetScrRect(
      &fscr, FSCREEN_XYPOS,
      &screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
    /* FIXME: Recalculate what to do if window is off screen. */
    /* Leaving alone for now */
    if (pwindow.width + pwindow.x > screen_g.x + screen_g.width)
    {
      pwindow.x = screen_g.x + screen_g.width - fp->m->si->w; //fp->m->virtual_scr.MyDisplayWidth;
      xneg = true;
    }
    if (pwindow.height + pwindow.y > screen_g.y + screen_g.height)
    {
      pwindow.y = screen_g.y + screen_g.height - fp->m->si->h; //fp->m->virtual_scr.MyDisplayHeight;
      yneg = true;
    }
  }
  if (xneg)
  {
    sizehints.win_gravity = NorthEastGravity;
    pwindow.x = fpmonitor_get_all_widths() - pwindow.width + pwindow.x;
  }
  if (yneg)
  {
    pwindow.y = fpmonitor_get_all_heights() - pwindow.height + pwindow.y;
    if(sizehints.win_gravity == NorthEastGravity)
      sizehints.win_gravity = SouthEastGravity;
    else
      sizehints.win_gravity = SouthWestGravity;
  }

  if(usposition)
    sizehints.flags |= USPosition;

  valuemask = (CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask);
  attributes.background_pixmap = default_pixmap;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  attributes.event_mask = (StructureNotifyMask);

  /* destroy the temp window first, don't worry if it's the Root */
  if (Scr.pager_w != Scr.Root)
    XDestroyWindow(dpy, Scr.pager_w);
  Scr.pager_w = XCreateWindow (dpy, Scr.Root, pwindow.x, pwindow.y, pwindow.width,
			       pwindow.height, 0, Pdepth, InputOutput, Pvisual,
			       valuemask, &attributes);
  XSetWMProtocols(dpy,Scr.pager_w,&wm_del_win,1);
  /* hack to prevent mapping on wrong screen with StartsOnScreen */
  FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &sizehints);
  XSetWMNormalHints(dpy,Scr.pager_w,&sizehints);
  if (is_transient)
  {
    XSetTransientForHint(dpy, Scr.pager_w, Scr.Root);
  }

  if (FlocaleTextListToTextProperty(
	 dpy, &(Desks[0].style->label), 1, XStdICCTextStyle, &name) == 0)
  {
    fprintf(stderr,"%s: fatal error: cannot allocate desk name", MyName);
    exit(0);
  }

  attributes.event_mask = (StructureNotifyMask| ExposureMask);
  if(icon.width < 1)
    icon.width = (pwindow.width - Columns + 1) / Columns;
  if(icon.height < 1)
    icon.height = (pwindow.height - Rows * label_h - Rows + 1) / Rows;

  icon.width = (icon.width / (VxPages + 1)) * (VxPages + 1) + VxPages;
  icon.height = (icon.height / (VyPages + 1)) * (VyPages + 1) + VyPages;
  Scr.icon_w = XCreateWindow (dpy, Scr.Root, pwindow.x, pwindow.y,
			    icon.width, icon.height,
			    0, Pdepth, InputOutput, Pvisual, valuemask,
			    &attributes);
  XGrabButton(dpy, 1, AnyModifier, Scr.icon_w,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 2, AnyModifier, Scr.icon_w,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 3, AnyModifier, Scr.icon_w,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);

  if (StartIconic) {
	  Balloon.show = Balloon.show_in_icon;
	  wmhints.initial_state = IconicState;
  } else {
	  Balloon.show = Balloon.show_in_pager;
	  wmhints.initial_state = NormalState;
  }
  wmhints.flags = 0;
  if (icon.x != -10000)
  {
    if (icon_xneg)
      icon.x = fpmonitor_get_all_widths() + icon.x - icon.width;
    if (icon.y != -10000)
    {
      if (icon_yneg)
	icon.y = fpmonitor_get_all_heights() + icon.y - icon.height;
    }
    else
    {
      icon.y = 0;
    }
    icon_xneg = false;
    icon_yneg = false;
    wmhints.icon_x = icon.x;
    wmhints.icon_y = icon.y;
    wmhints.flags = IconPositionHint;
  }
  wmhints.icon_window = Scr.icon_w;
  wmhints.input = True;
  wmhints.flags |= InputHint | StateHint | IconWindowHint;

  class1.res_name = MyName;
  class1.res_class = "FvwmPager";

  XSetWMProperties(dpy,Scr.pager_w,&name,&name,NULL,0,
		   &sizehints,&wmhints,&class1);
  XFree((char *)name.value);

  /* change colour/font for labelling mini-windows */
  XSetForeground(dpy, Scr.NormalGC, GetSimpleColor("black"));

  if (FwindowFont != NULL && FwindowFont->font != NULL)
    XSetFont(dpy, Scr.NormalGC, FwindowFont->font->fid);

  /* Initialize DeskStyle GCs */
  for (i = 0; i < ndesks; i++) {
	  /* Create any missing DeskStyles. */
	  style = FindDeskStyle(i);
  }
  TAILQ_FOREACH(style, &desk_style_q, entry) {
	  initialize_desk_style_gcs(style);
  }

  for(i=0;i<ndesks;i++)
  {
    initialize_desk_windows(i);
  }

  TAILQ_FOREACH(m, &fp_monitor_q, entry) {
    initialize_fpmonitor_windows(m);
  }
  initialize_balloon_window();
  XMapRaised(dpy,Scr.pager_w);
}


void UpdateWindowShape(void)
{
  if (FHaveShapeExtension)
  {
    int i, j, cnt, shape_count, x_pos, y_pos;
    XRectangle *shape;
    struct fpmonitor *fp = fpmonitor_this(NULL);

    if (!fp || !ShapeLabels || label_h == 0)
      return;

    shape_count =
      ndesks + ((fp->m->virtual_scr.CurrentDesk < desk1 || fp->m->virtual_scr.CurrentDesk >desk2) ? 0 : 1);

    shape = fxmalloc(shape_count * sizeof (XRectangle));

    cnt = 0;
    y_pos = (LabelsBelow ? 0 : label_h);

    for (i = 0; i < Rows; ++i)
    {
      x_pos = 0;
      for (j = 0; j < Columns; ++j)
      {
	if (cnt < ndesks)
	{
	  shape[cnt].x = x_pos;
	  shape[cnt].y = y_pos;
	  shape[cnt].width = desk_w + 1;
	  shape[cnt].height = desk_h + 2;

	  if (cnt == fp->m->virtual_scr.CurrentDesk - desk1)
	  {
	    shape[ndesks].x = x_pos;
	    shape[ndesks].y =
	      (LabelsBelow ? y_pos + desk_h + 2 : y_pos - label_h);
	    shape[ndesks].width = desk_w;
	    shape[ndesks].height = label_h + 2;
	  }
	}
	++cnt;
	x_pos += desk_w + 1;
      }
      y_pos += desk_h + 2 + label_h;
    }

    FShapeCombineRectangles(
      dpy, Scr.pager_w, FShapeBounding, 0, 0, shape, shape_count, FShapeSet, 0);
    free(shape);
  }
}



/*
 *
 * Decide what to do about received X events
 *
 */
void DispatchEvent(XEvent *Event)
{
  int i,x,y;
  Window JunkRoot, JunkChild;
  Window w;
  int JunkX, JunkY;
  unsigned JunkMask;
  char keychar;
  KeySym keysym;
  bool do_move_page = false;
  short dx = 0;
  short dy = 0;
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL)
      return;

  switch(Event->xany.type)
  {
  case EnterNotify:
    HandleEnterNotify(Event);
    break;
  case LeaveNotify:
    if (Balloon.show)
      UnmapBalloonWindow();
    break;
  case ConfigureNotify:
    fev_sanitise_configure_notify(&Event->xconfigure);
    w = Event->xconfigure.window;
    discard_events(ConfigureNotify, Event->xconfigure.window, Event);
    fev_sanitise_configure_notify(&Event->xconfigure);
    if (w != Scr.icon_w)
    {
      /* icon_win is not handled here */
      discard_events(Expose, w, NULL);
      ReConfigure();
    }
    break;
  case Expose:
    HandleExpose(Event);
    break;
  case KeyPress:
    if (is_transient)
    {
      XLookupString(&(Event->xkey), &keychar, 1, &keysym, NULL);
      switch(keysym)
      {
      case XK_Up:
	dy = -100;
	do_move_page = true;
	break;
      case XK_Down:
	dy = 100;
	do_move_page = true;
	break;
      case XK_Left:
	dx = -100;
	do_move_page = true;
	break;
      case XK_Right:
	dx = 100;
	do_move_page = true;
	break;
      default:
	/* does not return */
	ExitPager();
	break;
      }
      if (do_move_page)
      {
	char command[64];
	snprintf(command, sizeof(command),"Scroll %d %d", dx, dy);
	SendText(fd, command, 0);
      }
    }
    break;
  case ButtonRelease:
    if (do_ignore_next_button_release)
    {
      do_ignore_next_button_release = false;
      break;
    }
    if (Event->xbutton.button == 3)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	  Scroll(x, y, i, false);
	}
      }
      if(Event->xany.window == Scr.icon_w)
      {
	FQueryPointer(dpy, Scr.icon_w, &JunkRoot, &JunkChild,
		  &JunkX, &JunkY,&x, &y, &JunkMask);
	Scroll(x, y, -1, true);
      }
      /* Flush any pending scroll operations */
      do_scroll(0, 0, true, false);
      ScrollFp = NULL;
    }
    else if((Event->xbutton.button == 1)||
	    (Event->xbutton.button == 2))
    {
      /* Location of monitor labels. */
      int m_count = fpmonitor_count();
      int ymin = 0, ymax = label_h;
      if (use_desk_label)
	ymin = ymax / 2;
      if (LabelsBelow) {
	ymin += desk_h;
	ymax += desk_h;
      }

      for(i=0;i<ndesks;i++)
      {
	if (Event->xany.window == Desks[i].w)
	{
		SwitchToDeskAndPage(i, Event);
		break;
	}
	/* Check for clicks on monitor labels first, since these clicks
	 * always indicate what monitor to switch to in all modes.
	 */
	else if (use_monitor_label && monitor_to_track == NULL &&
		Event->xany.window == Desks[i].title_w &&
		Event->xbutton.y >= ymin && Event->xbutton.y <= ymax)
	{
		struct fpmonitor *fp2 = fpmonitor_from_n(
				m_count * Event->xbutton.x / desk_w);
		SwitchToDesk(i, fp2);
		break;
	}
	/* Title clicks only change desk in global configuration, or when the
	 * pager is being asked to track a specific monitor.  Or, regardless
	 * of the DesktopConfiguration setting, there is only one monitor in
	 * use.
	 */
	else if (Event->xany.window == Desks[i].title_w &&
		((monitor_mode == MONITOR_TRACKING_G &&
		!is_tracking_shared) ||
		monitor_to_track != NULL || m_count == 1))
	{
		SwitchToDesk(i, NULL);
	}
      }
      if(Event->xany.window == Scr.icon_w)
      {
	IconSwitchPage(Event);
      }
    }
    if (is_transient)
    {
      /* does not return */
      ExitPager();
    }
    break;
  case ButtonPress:
    do_ignore_next_button_release = false;
    if (Balloon.show)
      UnmapBalloonWindow();
    if (((Event->xbutton.button == 2)||
	 ((Event->xbutton.button == 3)&&
	  (Event->xbutton.state & Mod1Mask)))&&
	(Event->xbutton.subwindow != None))
    {
      MoveWindow(Event);
    }
    else if (Event->xbutton.button == 3)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	  if (monitor_mode == MONITOR_TRACKING_G && is_tracking_shared)
		fp = fpmonitor_from_desk(i + desk1);
	  else
		fp = fpmonitor_from_xy(x * fp->virtual_scr.VWidth / desk_w,
			   y * fp->virtual_scr.VHeight / desk_h);

	  if (fp == NULL)
	    break;
	  /* Save initial virtual desk info. */
	  ScrollFp = fp;
	  MyVx = fp->virtual_scr.Vx;
	  MyVy = fp->virtual_scr.Vy;
	  Scroll(x, y, fp->m->virtual_scr.CurrentDesk, false);
	  if (fp->m->virtual_scr.CurrentDesk != i + desk1)
	  {
	    Wait = 0;
	    SwitchToDesk(i, fp);
	  }
	  break;
	}
      }
      if(Event->xany.window == Scr.icon_w)
      {
	FQueryPointer(dpy, Scr.icon_w, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask);
	struct fpmonitor *fp2 = fpmonitor_this(NULL);
	if (monitor_mode == MONITOR_TRACKING_G && is_tracking_shared)
		fp = fp2;
	else {
		fp = fpmonitor_from_xy(
			x * fp->virtual_scr.VWidth / icon.width,
			y * fp->virtual_scr.VHeight / icon.height);
		/* Make sure monitor is on current desk. */
		if (fp->m->virtual_scr.CurrentDesk !=
				fp2->m->virtual_scr.CurrentDesk)
			break;
	}
	if (fp == NULL)
	  break;
	/* Save initial virtual desk info. */
	ScrollFp = fp;
	MyVx = fp->virtual_scr.Vx;
	MyVy = fp->virtual_scr.Vy;
	Scroll(x, y, -1, true);
      }
    }
    break;
  case MotionNotify:
    do_ignore_next_button_release = false;
    while(FCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask,Event))
      ;

    if(Event->xmotion.state & Button3MotionMask)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	  Scroll(x, y, i, false);
	}
      }
      if(Event->xany.window == Scr.icon_w)
      {
	FQueryPointer(dpy, Scr.icon_w, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask);
	Scroll(x, y, -1, true);
      }

    }
    break;

  case ClientMessage:
    if ((Event->xclient.format==32) &&
	(Event->xclient.data.l[0]==wm_del_win))
    {
      /* does not return */
      ExitPager();
    }
    break;
  }
}

void HandleEnterNotify(XEvent *Event)

{
	PagerWindow *t;

	if (!Balloon.show && !do_focus_on_enter)
		/* nothing to do */
		return;

	/* is this the best way to match X event window ID to PagerWindow ID? */
	for (t = Start; t != NULL; t = t->next) {
		if (t->PagerView == Event->xcrossing.window) {
			if (Balloon.show)
				MapBalloonWindow(t, false);
			break;
		}
		if (t->IconView == Event->xcrossing.window) {
			if (Balloon.show)
				MapBalloonWindow(t, true);
			break;
		}
	}
	if (t == NULL)
		return;

	if (do_focus_on_enter)
		SendText(fd, "Silent FlipFocus NoWarp", t->w);
}

#if 0
/* JS This code was getting in my way, and I'm not quite sure what
 * it was doing. I believe it was meant to just redraw part of the
 * desk label exposed, but with the addition of monitor labels, the
 * logic is a bit more complicated. Just redraw the whole label since
 * the rest of the grid is being redrawn anyways. This simplifies things
 * drastically, and this optimization might not have noticeable gain.
 * If this functionality is added back, draw_desk_grid needs to be
 * correctly updated to deal with the bound logic.
 */
XRectangle get_expose_bound(XEvent *Event)
{
	XRectangle r;
	int ex, ey, ex2, ey2;

	ex = Event->xexpose.x;
	ey = Event->xexpose.y;
	ex2 = Event->xexpose.x + Event->xexpose.width;
	ey2 = Event->xexpose.y + Event->xexpose.height;
	while (FCheckTypedWindowEvent(dpy, Event->xany.window, Expose, Event))
	{
		ex = min(ex, Event->xexpose.x);
		ey = min(ey, Event->xexpose.y);
		ex2 = max(ex2, Event->xexpose.x + Event->xexpose.width);
		ey2= max(ey2 , Event->xexpose.y + Event->xexpose.height);
	}
	r.x = ex;
	r.y = ey;
	r.width = ex2-ex;
	r.height = ey2-ey;
	return r;
}
#endif

void HandleExpose(XEvent *Event)
{
	int i;
	PagerWindow *t;

	/* Determine event window. */
	if (Event->xany.window == Scr.icon_w) {
		draw_icon_grid(0);
		return;
	}
	if (Event->xany.window == Scr.balloon_w) {
		DrawInBalloonWindow();
		return;
	}

	for(i = 0; i < ndesks; i++) {
		/* ric@giccs.georgetown.edu */
		if (Event->xany.window == Desks[i].w ||
		    Event->xany.window == Desks[i].title_w)
		{
			draw_desk_grid(i);
			return;
		}
	}

	for (t = Start; t != NULL; t = t->next) {
		if (t->PagerView == Event->xany.window) {
			update_pager_window_decor(t);
			return;
		} else if (t->IconView == Event->xany.window) {
			update_icon_window_decor(t);
			return;
		}
	}

	discard_events(Expose, Event->xany.window, NULL);
}

/*
 *
 * Respond to a change in window geometry.
 *
 */
void ReConfigure(void)
{
	Window root;
	unsigned dummy;
	int i, j, k;


	/* Using i for a dummy, as no return needed. */
	if (!XGetGeometry(dpy, Scr.pager_w, &root, &i, &i,
	    (unsigned *)&pwindow.width, (unsigned *)&pwindow.height,
	    &dummy, &dummy))
		return;

	set_desk_size(false);
	for(k = 0; k < Rows; k++) {
		for(j = 0; j < Columns; j++) {
			i = k * Columns + j;
			if (i >= ndesks)
				continue;

			XMoveResizeWindow(dpy, Desks[i].title_w,
					  (desk_w + 1) * j - 1,
					  (desk_h + label_h + 1) * k - 1,
					  desk_w, desk_h + label_h);
			XMoveResizeWindow(dpy, Desks[i].w, -1,
					  (LabelsBelow) ? -1 : label_h - 1,
					  desk_w, desk_h);

			update_desk_background(i);
			update_monitor_locations(i);
			update_monitor_backgrounds(i);
			draw_desk_grid(i);
		}
	}

	/* reconfigure all the subordinate windows */
	ReConfigureAll();
}

/*
 *
 * Respond to a "background" change: update the Parental Relative cset
 *
 */

/* layout:
 * Root -> pager window (pr) -> title window -> desk window -> window view
 *  |                                                |-> hilight desk
 *  |-> icon_window -> icon view
 *  |-> ballon window
 */

#if 0
/* update the hilight desk and the windows: desk color change */
static
void update_pr_transparent_subwindows(int i)
{
	int cset;
	PagerWindow *t;

	if (CSET_IS_TRANSPARENT_PR(Desks[i].style->hi_cs))
		update_monitor_backgrounds(i);

	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		cset = (t != FocusWin) ? windowcolorset : activecolorset;
		if (t->desk != i && !CSET_IS_TRANSPARENT_PR(cset))
		{
			continue;
		}
		SetWindowBackground(
			dpy, t->PagerView,
			t->pager_view.width, t->pager_view.height,
			&Colorset[cset], Pdepth, Scr.NormalGC, True);
		SetWindowBackground(
			dpy, t->IconView,
			t->icon_view.width, t->icon_view.height,
			&Colorset[cset], Pdepth, Scr.NormalGC, True);
	}
}
#endif

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

void MovePage()
{
	/* No monitor highlights, nothing to do. */
	if (!HilightDesks)
		return;

	int i;

	/* Unsure which desk was updated, so update all desks. */
	for(i = 0; i < ndesks; i++) {
		update_monitor_locations(i);
		update_monitor_backgrounds(i);
	}
	draw_icon_grid(true);
	Wait = 0;
}

void ReConfigureAll(void)
{
	PagerWindow *t;

	t = Start;
	while (t != NULL) {
		MoveResizePagerView(t, true);
		t = t->next;
	}
}

/*
 *
 * Draw grid lines for desk #i
 *
 */
void draw_desk_label(const char *label, const char *small, int desk,
		      int x, int y, int width, int height, bool hilight)
{
	int w, cs;
	char *str = fxstrdup(label);

	w = FlocaleTextWidth(Ffont, str, strlen(label));
	if (w > width) {
		free(str);
		str = fxstrdup(small);
		w = FlocaleTextWidth(Ffont, str, strlen(str));
	}

	if (w <= width) {
		FwinString->str = str;
		FwinString->win = Desks[desk].title_w;
		if (hilight)
		{
			cs = Desks[desk].style->hi_cs;
			FwinString->gc = Desks[desk].style->hi_fg_gc;
			if (HilightLabels)
				XFillRectangle(dpy, Desks[desk].title_w,
					       Desks[desk].style->hi_bg_gc,
					       x, y, width, height);
		} else if (CurrentDeskPerMonitor && fAlwaysCurrentDesk) {
			/* Draw label colors based on location of monitor. */
			struct fpmonitor *fp = fpmonitor_from_n(x / width);
			DeskStyle *style;

			style = FindDeskStyle(fp->m->virtual_scr.CurrentDesk);
			cs = style->hi_cs;
			FwinString->gc = style->hi_fg_gc;

			if (HilightLabels)
				XFillRectangle(dpy, Desks[desk].title_w,
					       style->hi_bg_gc,
					       x, y, width, height);
		} else {
			cs = Desks[desk].style->cs;
			FwinString->gc = Desks[desk].style->label_gc;
		}

		FwinString->flags.has_colorset = False;
		if (cs >= 0)
		{
			FwinString->colorset = &Colorset[cs];
			FwinString->flags.has_colorset = True;
		}
		FwinString->x = x + (width - w)/2;
		FwinString->y = y + Ffont->ascent + 1;
		FwinString->flags.has_clip_region = False;
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
	}

	free(str);
	return;
}

void draw_desk_grid(int desk)
{
	int y, y1, y2, x, x1, x2, d;
	char str[15];
	struct fpmonitor *fp = fpmonitor_this(NULL);
	DeskStyle *style;

	if (fp == NULL || desk < 0 || desk >= ndesks)
		return;

	style = Desks[desk].style;
	/* desk grid */
	x = fpmonitor_get_all_widths();
	y1 = 0;
	y2 = desk_h;
	while (x < fp->virtual_scr.VWidth)
	{
		x1 = (x * desk_w) / fp->virtual_scr.VWidth;
		if (!use_no_separators)
		{
			XDrawLine(dpy, Desks[desk].w, style->dashed_gc,
				  x1, y1, x1, y2);
		}
		x += fpmonitor_get_all_widths();
	}
	y = fpmonitor_get_all_heights();
	x1 = 0;
	x2 = desk_w;
	while(y < fp->virtual_scr.VHeight)
	{
		y1 = (y * desk_h) / fp->virtual_scr.VHeight;
		if (!use_no_separators)
		{
			XDrawLine(dpy, Desks[desk].w, style->dashed_gc,
				  x1, y1, x2, y1);
		}
		y += fpmonitor_get_all_heights();
	}

	/* desk label location */
	y1 = (LabelsBelow) ? desk_h : 0;
	d = desk1 + desk;
	int m_count = 1;
	int y_loc = y1;
	int height = label_h;
	if (use_desk_label && use_monitor_label) {
		height /= 2;
		y_loc += height;
	}
	if (monitor_to_track == NULL)
		m_count = fpmonitor_count();

	XClearArea(dpy, Desks[desk].title_w,
			   0, y1, desk_w, label_h, False);

	if (use_desk_label && CurrentDeskPerMonitor && fAlwaysCurrentDesk) {
		struct fpmonitor *tm;
		int loop = 0;
		int label_width = desk_w / m_count;
		DeskStyle *s;

		TAILQ_FOREACH(tm, &fp_monitor_q, entry) {
			if (tm->disabled)
				continue;
			if (monitor_to_track != NULL && fp != tm)
				continue;

			s = FindDeskStyle(tm->m->virtual_scr.CurrentDesk);
			snprintf(str, sizeof(str), "D%d", s->desk);
			draw_desk_label(s->label, str, desk,
					 loop * label_width, y1,
					 label_width, height,
					 false);
			loop++;
		}
	} else if (use_desk_label) {
		snprintf(str, sizeof(str), "D%d", d);
		draw_desk_label(style->label, str,
				desk, 0, y1, desk_w, height,
				(!use_monitor_label &&
				fp->m->virtual_scr.CurrentDesk == d));
	}

	if (use_monitor_label) {
		struct fpmonitor *tm;
		int loop = 0;
		int label_width = desk_w / m_count;

		TAILQ_FOREACH(tm, &fp_monitor_q, entry) {
			if (tm->disabled)
				continue;
			if (monitor_to_track != NULL && fp != tm)
				continue;

			snprintf(str, sizeof(str), "%d", loop + 1);
			draw_desk_label(
				tm->m->si->name, str,
				desk, loop * label_width, y_loc,
				label_width, height,
				!(CurrentDeskPerMonitor &&
				fAlwaysCurrentDesk) &&
				(tm->m->virtual_scr.CurrentDesk == d));
			loop++;
		}
	}

	/* Draw monitor labels last to ensure hilighted labels have borders. */
	if (use_monitor_label) {
		int i;
		int yy = y_loc;
		int hh = height;

		XDrawLine(
			dpy, Desks[desk].title_w, style->label_gc,
			0, y_loc, desk_w, y_loc);

		if (use_desk_label && CurrentDeskPerMonitor &&
		    fAlwaysCurrentDesk) {
			yy = y1;
			hh = label_h;
		}

		for (i = 1; i < m_count; i++)
			XDrawLine(dpy,
				Desks[desk].title_w,
				style->label_gc,
				i * desk_w / m_count,
				yy, i * desk_w / m_count,
				yy + hh);
	}

	if (FShapesSupported)
	{
		UpdateWindowShape();
	}
}


void draw_icon_grid(int erase)
{
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (fp == NULL)
		return;

	/* desk_i may not be shown, so find it's style directly. */
	DeskStyle *style = FindDeskStyle(desk_i);
	rectangle rec;
	int i, tmp = (fp->m->virtual_scr.CurrentDesk - desk1);

	if (tmp < 0 || tmp >= ndesks)
		tmp = 0;

	if (erase) {
		if (style->bgPixmap)
			XSetWindowBackgroundPixmap(dpy, Scr.icon_w,
				style->bgPixmap->picture);
		else
			XSetWindowBackground(dpy, Scr.icon_w, style->bg);
		XClearWindow(dpy, Scr.icon_w);
	}

	rec.x = fp->virtual_scr.VxPages;
	rec.y = fp->virtual_scr.VyPages;
	rec.width = icon.width / rec.x;
	rec.height = icon.height / rec.y;
	if (!use_no_separators) {
		for(i = 1; i < rec.x; i++)
			XDrawLine(dpy, Scr.icon_w, style->dashed_gc,
				i * rec.width, 0, i * rec.width, icon.height);
		for(i = 1; i < rec.y; i++)
			XDrawLine(dpy, Scr.icon_w, style->dashed_gc,
				0, i * rec.height, icon.width, i * rec.height);
	}

	if (HilightDesks) {
		TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
			int desk = (CurrentDeskPerMonitor) ?
				fp->m->virtual_scr.CurrentDesk : desk_i;

			if (fp->disabled ||
			   fp->m->virtual_scr.CurrentDesk != desk ||
			   (monitor_to_track != NULL &&
			   fp != monitor_to_track))
				continue;

			style = FindDeskStyle(desk);
			desk -= desk1;
			if (desk < 0 || desk >= ndesks)
				desk = tmp;

			rec = set_vp_size_and_loc(fp, true);
			if (style->hiPixmap) {
				XCopyArea(dpy, style->hiPixmap->picture,
					Scr.icon_w, Scr.NormalGC, 0, 0,
					rec.width, rec.height, rec.x, rec.y);
			} else {
				XFillRectangle(dpy, Scr.icon_w,
					style->hi_bg_gc,
					rec.x, rec.y, rec.width, rec.height);
			}
		}
	}
}


void SwitchToDesk(int Desk, struct fpmonitor *m)
{
	char command[256];
	struct fpmonitor *fp = (m == NULL) ? fpmonitor_this(NULL) : m;

	if (fp == NULL) {
		fprintf(stderr, "%s: couldn't find monitor (fp is NULL)\n",
		    __func__);
		return;
	}
	snprintf(command, sizeof(command),
		"GotoDesk screen %s 0 %d", fp->m->si->name, Desk + desk1);
	SendText(fd,command,0);
}


void SwitchToDeskAndPage(int Desk, XEvent *Event)
{
	char command[256];
	int x, y;
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (fp == NULL) {
		fprintf(stderr, "%s: couldn't find monitor (fp is NULL)\n",
			 __func__);
		return;
	}

	/* If tracking / showing a single monitor, just need to find page. */
	if (IsShared || monitor_to_track != NULL || fpmonitor_count() == 1) {
		if (monitor_to_track != NULL)
			fp = monitor_to_track;
		else if (IsShared)
			fp = Desks[Desk].fp;

		/* Ignore clicks if the monitor is not occupying the desk. */
		Desk += desk1;
		if (IsShared && fp->m->virtual_scr.CurrentDesk != Desk)
			return;

		x = Event->xbutton.x * fp->virtual_scr.VxPages / desk_w;
		y = Event->xbutton.y * fp->virtual_scr.VyPages / desk_h;
		goto send_cmd;
	}

	/* Determine which monitor was in the region clicked. */
	Desk += desk1;
	x = Event->xbutton.x * fp->virtual_scr.VWidth / desk_w;
	y = Event->xbutton.y * fp->virtual_scr.VHeight / desk_h;
	if (is_tracking_shared)
		fp = fpmonitor_from_desk(Desk);
	else
		fp = fpmonitor_from_xy(x, y);

	/* No monitor found, ignore click. */
	if (fp == NULL)
		return;

	/* Fix for buggy XFree86 servers that report button release
	 * events incorrectly when moving fast. Not perfect, but
	 * should at least prevent that we get a random page.
	 */
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x > fp->virtual_scr.VxMax)
		x = fp->virtual_scr.VxMax;
	if (y > fp->virtual_scr.VyMax)
		y = fp->virtual_scr.VyMax;
	x /= fpmonitor_get_all_widths();
	y /= fpmonitor_get_all_heights();

send_cmd:
	if (desk1 != desk2 && fp->m->virtual_scr.CurrentDesk != Desk) {
		/* patch to let mouse button 3 change desks and not cling */
		snprintf(command, sizeof(command),
			"GotoDeskAndPage screen %s %d %d %d",
			fp->m->si->name, Desk, x, y);
		SendText(fd, command, 0);
	} else {
		snprintf(command, sizeof(command),
			"GotoPage screen %s %d %d", fp->m->si->name, x, y);
		SendText(fd, command, 0);
	}
	Wait = 1;
}

void IconSwitchPage(XEvent *Event)
{
  int vx, vy;
  char command[34];
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL) {
    fprintf(stderr, "%s: couldn't find monitor (fp is NULL)\n", __func__);
    return;
  }

  /* Determine which monitor occupied the clicked region. */
  vx = (icon.width == 0) ? 0 : Event->xbutton.x * fp->virtual_scr.VWidth /
	icon.width;
  vy = (icon.height == 0) ? 0 : Event->xbutton.y * fp->virtual_scr.VHeight /
	icon.height;
  fp = fpmonitor_from_xy(vx, vy);
  if (fp == NULL)
    return;

  snprintf(command,sizeof(command),"GotoPage screen %s %d %d",
	  fp->m->si->name,
	  Event->xbutton.x * fp->virtual_scr.VWidth /
		  (icon.width * fpmonitor_get_all_widths()),
	  Event->xbutton.y * fp->virtual_scr.VHeight /
		  (icon.height * fpmonitor_get_all_heights()));
  SendText(fd, command, 0);
  Wait = 1;
}

void HideWindow(PagerWindow *t, Window w)
{
	if (w == None)
	{
		XMoveWindow(dpy, t->PagerView, -32768, -32768);
		XMoveWindow(dpy, t->IconView, -32768, -32768);
	} else {
		XMoveWindow(dpy, w, -32768, -32768);
	}

	return;
}

void MoveResizeWindow(PagerWindow *t, bool do_force_redraw, bool is_icon)
{
	Window w = t->PagerView;
	rectangle old_r = t->pager_view;
	rectangle new_r = CalcGeom(t, is_icon);
	bool position_changed, size_changed;

	if (is_icon)
	{
		w = t->IconView;
		old_r = t->icon_view;
	}

	position_changed = (old_r.x != new_r.x || old_r.y != new_r.y);
	size_changed = (old_r.width != new_r.width ||
			old_r.height != new_r.height);
	if (is_icon)
		t->icon_view = new_r;
	else
		t->pager_view = new_r;

	if (HideSmallWindows &&
	    (new_r.width == MinSize || new_r.height == MinSize))
	{
		HideWindow(t, w);
	}
	else if (size_changed || position_changed || do_force_redraw)
	{
		DeskStyle *style = FindDeskStyle(t->desk);
		int cset = (t != FocusWin) ? style->win_cs : style->focus_cs;

		XMoveResizeWindow(dpy, w, new_r.x, new_r.y,
				  new_r.width, new_r.height);
		if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
			update_window_background(t);
	}

	return;
}

void AddNewWindow(PagerWindow *t)
{
	int desk = 0; /* Desk 0 holds all windows not on any pager desk. */
	rectangle r = {-32768, -32768, MinSize, MinSize};
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	DeskStyle *style = FindDeskStyle(t->desk);

	if (t->desk >= desk1 && t->desk <= desk2) {
		desk = t->desk - desk1;

		/* Initialize monitors for desks that did not start with
		 * a monitor by using windows to best guess which monitor
		 * was last viewing the desk.
		 */
		if (Desks[desk].fp == NULL)
			Desks[desk].fp = fpmonitor_this(t->m);
	}

	valuemask = CWBackPixel | CWEventMask;
	attributes.background_pixel = (style->win_bg) ?
				      style->win_bg : t->back;
	/* ric@giccs.georgetown.edu -- added Enter and Leave events for
	   popping up balloon window */
	attributes.event_mask =
		ExposureMask | EnterWindowMask | LeaveWindowMask;

	t->pager_view = r;
	t->PagerView = XCreateWindow(
		dpy, Desks[desk].w, r.x, r.y, r.width, r.height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		valuemask, &attributes);
	XMapRaised(dpy, t->PagerView);

	t->icon_view = r;
	t->IconView = XCreateWindow(
		dpy, Scr.icon_w, r.x, r.y, r.width, r.height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		valuemask, &attributes);
	XMapRaised(dpy, t->IconView);

	MoveResizePagerView(t, true);
	update_window_background(t);

	return;
}

/* Determines if a window is shown for both pager view and icon view. */
#define SHOW_PAGER_VIEW 0x1
#define SHOW_ICON_VIEW 0x2

int is_window_visible(PagerWindow *t)
{
	int do_show_window = 0;
	struct fpmonitor *fp = fpmonitor_this(t->m);

	if (fp == NULL || (UseSkipList && DO_SKIP_WINDOW_LIST(t)))
		return do_show_window;

	if (CurrentDeskPerMonitor && monitor_to_track == NULL) {
		if (!fAlwaysCurrentDesk &&
		    (t->desk >= desk1 && t->desk <= desk2))
			do_show_window |= SHOW_PAGER_VIEW;
		if (fp->m->virtual_scr.CurrentDesk == t->desk) {
			if (fAlwaysCurrentDesk)
				do_show_window |= SHOW_PAGER_VIEW;
			do_show_window |= SHOW_ICON_VIEW;
		}
	} else if (IsShared && monitor_to_track == NULL) {
		int desk = t->desk - desk1;

		if (desk >= 0 && desk < ndesks && Desks[desk].fp == fp)
			do_show_window |= SHOW_PAGER_VIEW;
		if (t->desk == desk_i)
			do_show_window |= SHOW_ICON_VIEW;
	} else {
		if (t->desk >= desk1 && t->desk <= desk2)
			do_show_window |= SHOW_PAGER_VIEW;
		if (t->desk == desk_i)
			do_show_window |= SHOW_ICON_VIEW;
	}

	return do_show_window;
}

void ChangeDeskForWindow(PagerWindow *t, long newdesk)
{
	int do_show_window;
	t->desk = newdesk;
	do_show_window = is_window_visible(t);

	if (do_show_window & SHOW_PAGER_VIEW) {
		int desk = (CurrentDeskPerMonitor && fAlwaysCurrentDesk) ?
			0 : newdesk - desk1;

		XReparentWindow(dpy, t->PagerView, Desks[desk].w,
			t->pager_view.x, t->pager_view.y);
		update_window_background(t);
	} else {
		/* Hide windows on desk 0. */
		XReparentWindow(dpy, t->PagerView, Desks[0].w, -32768, -32768);
	}

	if (do_show_window & SHOW_ICON_VIEW) {
		MoveResizeWindow(t, true, true);
		update_window_background(t);
	} else {
		HideWindow(t, t->IconView);
	}
}

void MoveResizePagerView(PagerWindow *t, bool do_force_redraw)
{
	int do_show_window = is_window_visible(t);

	if (do_show_window & SHOW_PAGER_VIEW)
		MoveResizeWindow(t, do_force_redraw, false);
	else
		HideWindow(t, t->PagerView);

	if (do_show_window & SHOW_ICON_VIEW)
		MoveResizeWindow(t, do_force_redraw, true);
	else
		HideWindow(t, t->IconView);

	return;
}

void MoveStickyWindows(bool is_new_page, bool is_new_desk)
{
	PagerWindow *t;
	struct fpmonitor *fp;

	for (t = Start; t != NULL; t = t->next) {
		fp = fpmonitor_this(t->m);
		if (fp == NULL)
			continue;

		if (is_new_desk &&
		    t->desk != fp->m->virtual_scr.CurrentDesk &&
		    (IS_STICKY_ACROSS_DESKS(t) ||
		    (IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t))))
			ChangeDeskForWindow(t, fp->m->virtual_scr.CurrentDesk);

		if (is_new_page &&
		    (IS_STICKY_ACROSS_PAGES(t) ||
		    (IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_PAGES(t))))
			MoveResizePagerView(t, true);
	}
}

/* Use Desk == -1 to scroll the icon window */
void Scroll(int x, int y, int Desk, bool do_scroll_icon)
{
	static int last_sx = -999999;
	static int last_sy = -999999;
	int window_w = desk_w, window_h = desk_h, sx, sy, adjx, adjy;
	struct fpmonitor *fp = ScrollFp;

	if (fp == NULL)
		return;

	/* Desk < 0 means we want to scroll an icon window */
	if (Desk >= 0 && Desk + desk1 != fp->m->virtual_scr.CurrentDesk)
	{
		return;
	}

	if (do_scroll_icon) {
		window_w = icon.width;
		window_h = icon.height;
	}

	/* center around mouse */
	if (monitor_mode == MONITOR_TRACKING_G && !is_tracking_shared) {
		adjx = window_w / fp->virtual_scr.VxPages;
		adjy = window_h / fp->virtual_scr.VyPages;
	} else {
		adjx = (2 * fp->m->si->x + fp->m->si->w) * window_w /
			fp->virtual_scr.VWidth;
		adjy = (2 * fp->m->si->y + fp->m->si->h) * window_h /
			fp->virtual_scr.VHeight;
	}
	x -= adjx/2;
	y -= adjy/2;

	/* adjust for pointer going out of range */
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}

	if (x > window_w)
	{
		x = window_w;
	}
	if (y > window_h)
	{
		y = window_h;
	}

	sx = 0;
	sy = 0;
	if (window_w != 0)
	{
		sx = (x * fp->virtual_scr.VWidth / window_w - MyVx);
	}
	if (window_h != 0)
	{
		sy = (y * fp->virtual_scr.VHeight / window_h - MyVy);
	}
	if (sx == 0 && sy == 0)
	{
		return;
	}
	if (Wait == 0 || last_sx != sx || last_sy != sy)
	{
		do_scroll(sx, sy, true, false);

		/* Here we need to track the view offset on the desk. */
		/* sx/y are are pixels on the screen to scroll. */
		/* We don't use Scr.Vx/y since they lag the true position. */
		MyVx += sx;
		if (MyVx < 0)
		{
			MyVx = 0;
		}
		MyVy += sy;

		if (MyVy < 0)
		{
			MyVy = 0;
		}
		Wait = 1;
	}
	if (Wait == 0)
	{
		last_sx = sx;
		last_sy = sy;
	}

	return;
}

void MoveWindow(XEvent *Event)
{
	int x, y, xi = 0, yi = 0;
	int finished = 0;
	Window dumwin;
	PagerWindow *t;
	int NewDesk, KeepMoving = 0;
	int moved = 0;
	int row, column;
	Window JunkRoot, JunkChild;
	int JunkX, JunkY;
	unsigned JunkMask;
	struct fpmonitor *fp;
	rectangle rec;

	t = Start;
	while (t != NULL && t->PagerView != Event->xbutton.subwindow)
		t = t->next;
	if (t == NULL) {
		t = Start;
		while (t != NULL && t->IconView != Event->xbutton.subwindow)
			t = t->next;
		if (t != NULL) {
			IconMoveWindow(Event, t);
			return;
		}
	}

	if (t == NULL || !t->allowed_actions.is_movable)
		return;

	NewDesk = t->desk - desk1;
	if (NewDesk < 0 || NewDesk >= ndesks)
		return;

	fp = fpmonitor_this(t->m);
	rec.x = fp->virtual_scr.Vx + t->x;
	rec.y = fp->virtual_scr.Vy + t->y;
	rec.width = t->width;
	rec.height = t->height;
	fvwmrec_to_pager(&rec, false, fp);

	rec.x += (desk_w + 1) * (NewDesk % Columns);
	rec.y += (desk_h + label_h + 1) * (NewDesk / Columns);
	if (!LabelsBelow)
		rec.y += label_h;

	XReparentWindow(dpy, t->PagerView, Scr.pager_w, rec.x, rec.y);
	XRaiseWindow(dpy, t->PagerView);

	XTranslateCoordinates(dpy, Event->xany.window, t->PagerView,
			      Event->xbutton.x, Event->xbutton.y, &rec.x, &rec.y,
			      &dumwin);

	xi = rec.x;
	yi = rec.y;
	while (!finished) {
		FMaskEvent(dpy, ButtonReleaseMask | ButtonMotionMask |
			   ExposureMask, Event);
		if (Event->type == MotionNotify) {
			XTranslateCoordinates(dpy, Event->xany.window,
					      Scr.pager_w, Event->xmotion.x,
					      Event->xmotion.y, &x, &y,
					      &dumwin);

			if (moved == 0) {
				xi = x;
				yi = y;
				moved = 1;
			}
			if (x < -5 || y < -5 || x > pwindow.width + 5 ||
			   y > pwindow.height + 5) {
				KeepMoving = 1;
				finished = 1;
			}
			XMoveWindow(dpy, t->PagerView, x - rec.x, y - rec.y);
		}
		else if (Event->type == ButtonRelease) {
			XTranslateCoordinates(dpy, Event->xany.window,
					      Scr.pager_w, Event->xbutton.x,
					      Event->xbutton.y, &x, &y,
					      &dumwin);
			x -= rec.x;
			y -= rec.y;
			XMoveWindow(dpy, t->PagerView, x, y);
			finished = 1;
		} else if (Event->type == Expose) {
			HandleExpose(Event);
		}
	}

	if (moved && abs(x - xi) < MoveThreshold && abs(y - yi) < MoveThreshold)
			moved = 0;

	if (KeepMoving)
	{
		NewDesk = fp->m->virtual_scr.CurrentDesk;

		if (NewDesk >= desk1 && NewDesk <= desk2) {
			XReparentWindow(dpy, t->PagerView,
					Desks[NewDesk-desk1].w, 0, 0);
		} else {
			XReparentWindow(dpy, t->PagerView,
					Desks[0].w, -32768, -32768);
		}
		FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask);
		XUngrabPointer(dpy,CurrentTime);
		XSync(dpy,0);

		if (NewDesk != t->desk)	{
			/* griph: This used to move to NewDesk, but NewDesk
			 * is current desk, and if fvwm is on another
			 * desk (due to async operation) we have to move
			 * the window to it anyway or "Move Pointer" will
			 * move an invisible window. */

			SendText(fd, "Silent MoveToDesk", t->w);
			t->desk = NewDesk;
		}
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent Move Pointer", t->w);
		return;
	}

	column = x / desk_w;
	if (column >= Columns)
		column = Columns - 1;
	if (column < 0)
		column = 0;

	row = y / (desk_h + label_h);
	if (row >= Rows)
		row = Rows - 1;
	if (row < 0)
		row = 0;

	NewDesk = column + row * Columns;
	if (NewDesk >= ndesks) {
		/* Bail, window dropped in an unused area.
		 * Stash window then recompute its old position.
		 */
		XReparentWindow(dpy,
			t->PagerView, Desks[0].w, -32768, -32768);
		ChangeDeskForWindow(t, t->desk);
		return;
	}
	XTranslateCoordinates(dpy, Scr.pager_w, Desks[NewDesk].w,
			      x, y, &rec.x, &rec.y, &dumwin);

	fp = fpmonitor_from_xy(
		rec.x * fp->virtual_scr.VWidth / desk_w,
		rec.y * fp->virtual_scr.VHeight / desk_h);
	if (fp == NULL)
		fp = fpmonitor_this(NULL);

	/* Sticky windows should stick to the monitor in the region
	 * they are placed in. If that monitor is on a desk not
	 * currently shown, then stash the window on Desks[0].
	 */
	if ((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t)) ||
	    IS_STICKY_ACROSS_DESKS(t))
		NewDesk = fp->m->virtual_scr.CurrentDesk - desk1;
	if (NewDesk < 0 || NewDesk >= ndesks) {
		XReparentWindow(
			dpy, t->PagerView, Desks[0].w, -32768, -32768);
	} else {
		/* Place window in new desk. */
		XReparentWindow(
			dpy, t->PagerView, Desks[NewDesk].w, rec.x, rec.y);
		XClearArea(dpy, Desks[NewDesk].w, 0, 0, 0, 0, True);
	}

	if (!moved) {
		MoveResizePagerView(t, true);
		goto done_moving;
	}

	char buf[64];

	/* Move using the virtual screen's coordinates "+vXp +vYp", to avoid
	 * guessing which monitor fvwm will use. The "desk" option is sent
	 * here to both move and change the desk in a single command, and to
	 * prevent fvwm from changing a window's desk if it changes monitors.
	 * "ewmhiwa" disables clipping to monitor/ewmh boundaries.
	 */
	pagerrec_to_fvwm(&rec, false, fp);
	if (CurrentDeskPerMonitor && fAlwaysCurrentDesk) {
		NewDesk = fp->m->virtual_scr.CurrentDesk;
		/* Let fvwm handle any desk changes in this case. */
		snprintf(buf, sizeof(buf), "Silent Move v+%dp v+%dp ewmhiwa",
			 rec.x, rec.y);
	} else {
		NewDesk += desk1;
		snprintf(buf, sizeof(buf),
			 "Silent Move desk %d v+%dp v+%dp ewmhiwa",
			 NewDesk, rec.x, rec.y);
	}
	SendText(fd, buf, t->w);
	XSync(dpy,0);
	SendText(fd, "Silent Raise", t->w);

	if (FocusAfterMove && fp->m->virtual_scr.CurrentDesk == NewDesk) {
		XSync(dpy,0);
		usleep(5000);
		XSync(dpy,0);

		SendText(fd, "Silent FlipFocus NoWarp", t->w);
	}

done_moving:
	if (is_transient)
		ExitPager(); /* does not return */
}


/*
 *
 *  Procedure:
 *	FvwmErrorHandler - displays info on internal errors
 *
 */
int FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
#if 1
  extern bool error_occured;
  error_occured = true;
  return 0;
#else
  /* really should just exit here... */
  /* domivogt (07-mar-1999): No, not really. See comment above. */
  PrintXErrorAndCoredump(dpy, event, MyName);
  return 0;
#endif /* 1 */
}

static void draw_window_border(PagerWindow *t, Window w, rectangle r)
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
		if (style->focus_fg)
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
		if (style->win_fg)
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

/* draw the window label with simple greedy wrapping */
static void label_window_wrap(PagerWindow *t, rectangle win_r)
{
  char *cur, *next, *end;
  int space_width, cur_width;

  space_width = FlocaleTextWidth(FwindowFont, " ", 1);
  cur_width   = 0;

  cur = next = t->window_label;
  end = cur + strlen(cur);

  while (*cur) {
    while (*next) {
      int width;
      char *p;

      if (!(p = strchr(next, ' ')))
        p = end;

      width = FlocaleTextWidth(FwindowFont, next, p - next );
      if (width > win_r.width - cur_width - space_width - 2*label_border)
        break;
      cur_width += width + space_width;
      next = *p ? p + 1 : p;
    }

    if (cur == next) {
      /* word too large for window */
      while (*next) {
        int len, width;

        len   = FlocaleStringNumberOfBytes(FwindowFont, next);
        width = FlocaleTextWidth(FwindowFont, next, len);

        if (width > win_r.width - cur_width - 2*label_border && cur != next)
          break;

        next      += len;
        cur_width += width;
      }
    }

    FwinString->str = fxmalloc(next - cur + 1);
    strncpy(FwinString->str, cur, next - cur);
    FwinString->str[next - cur] = 0;

    FlocaleDrawString(dpy, FwindowFont, FwinString, 0);

    free(FwinString->str);
    FwinString->str = NULL;

    FwinString->y += FwindowFont->height;
    cur = next;
    cur_width = 0;
  }
}

static void do_label_window(PagerWindow *t, Window w, rectangle r)
{
	if (t == NULL || w == None || FwindowFont == NULL ||
	    (MiniIcons && t->mini_icon.picture) || /* Draw image later. */
	    t->icon_name == NULL)
		return;

	int cs;
	Pixel pix;
	DeskStyle *style = FindDeskStyle(t->desk);

	if (t == FocusWin) {
		pix = (style->focus_fg) ? style->focus_fg : Scr.focus_win_fg;
		cs = style->focus_cs;
	} else {
		pix = (style->win_fg) ? style->win_fg : t->text;
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
	FwinString->y = FwindowFont->ascent+2;

	label_window_wrap(t, r);
}

static void do_picture_window(PagerWindow *t, Window w, rectangle r)
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

void update_pager_window_decor(PagerWindow *t)
{
	do_label_window(t, t->PagerView, t->pager_view);
	do_picture_window(t, t->PagerView, t->pager_view);
	draw_window_border(t, t->PagerView, t->pager_view);
}

void update_icon_window_decor(PagerWindow *t)
{
	do_label_window(t, t->IconView, t->icon_view);
	do_picture_window(t, t->IconView, t->icon_view);
	draw_window_border(t, t->IconView, t->icon_view);
}

void update_window_decor(PagerWindow *t)
{
	update_pager_window_decor(t);
	update_icon_window_decor(t);
}

void IconMoveWindow(XEvent *Event, PagerWindow *t)
{
	int finished = 0, x = 0, y = 0, xi = 0, yi = 0;
	Window dumwin;
	int moved = 0;
	int KeepMoving = 0;
	Window JunkRoot, JunkChild;
	int JunkX, JunkY;
	unsigned JunkMask;
	rectangle rec;
	struct fpmonitor *fp;

	if (t == NULL || !t->allowed_actions.is_movable)
		return;

	fp = fpmonitor_this(t->m);
	rec.x = fp->virtual_scr.Vx + t->x;
	rec.y = fp->virtual_scr.Vy + t->y;
	rec.width = t->width;
	rec.height = t->height;
	if (fp == NULL)
		return;

	fvwmrec_to_pager(&rec, true, fp);

	XRaiseWindow(dpy, t->IconView);

	XTranslateCoordinates(dpy, Event->xany.window, t->IconView,
			      Event->xbutton.x, Event->xbutton.y,
			      &rec.x, &rec.y, &dumwin);

	while (!finished) {
		FMaskEvent(dpy, ButtonReleaseMask | ButtonMotionMask |
			   ExposureMask, Event);

		if (Event->type == MotionNotify) {
			x = Event->xbutton.x;
			y = Event->xbutton.y;
			if (moved == 0) {
				xi = x;
				yi = y;
				moved = 1;
			}

			XMoveWindow(dpy, t->IconView, x - rec.x, y - rec.y);
			if (x < -5 || y < -5 || x > icon.width + 5 ||
			    y > icon.height + 5) {
				finished = 1;
				KeepMoving = 1;
			}
		} else if (Event->type == ButtonRelease) {
			x = Event->xbutton.x;
			y = Event->xbutton.y;
			XMoveWindow(dpy, t->PagerView, x - rec.x, y - rec.y);
			finished = 1;
		} else if (Event->type == Expose) {
			HandleExpose(Event);
		}
	}

	if (moved && (abs(x - xi) < MoveThreshold && abs(y - yi) < MoveThreshold))
			moved = 0;

	if (KeepMoving) {
		FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask);
		XUngrabPointer(dpy, CurrentTime);
		XSync(dpy, 0);
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent Move Pointer", t->w);
	} else if (moved) {
		char buf[64];

		rec.x = x - rec.x;
		rec.y = y - rec.y;
		fp = fpmonitor_from_xy(
			rec.x * fp->virtual_scr.VWidth / icon.width,
			rec.y * fp->virtual_scr.VHeight / icon.height);
		if (fp == NULL)
			fp = fpmonitor_this(NULL);
		pagerrec_to_fvwm(&rec, true, fp);
		if (CurrentDeskPerMonitor)
			snprintf(buf, sizeof(buf),
				 "Silent Move v+%dp v+%dp ewmhiwa",
				 rec.x, rec.y);
		else
			/* Keep window on current desk, even if another
			 * monitor is currently on a different desk.
			 */
			snprintf(buf, sizeof(buf),
				 "Silent Move desk %d v+%dp v+%dp ewmhiwa",
				 desk_i, rec.x, rec.y);
		SendText(fd, buf, t->w);
		XSync(dpy, 0);
		SendText(fd, "Silent Raise", t->w);
		if (FocusAfterMove)
			SendText(fd, "Silent FlipFocus NoWarp", t->w);
	} else {
		MoveResizePagerView(t, true);
	}
	if (is_transient)
		ExitPager(); /* does not return */
}

void setup_balloon_window(PagerWindow *t)
{
	Pixel fg;
	XSetWindowAttributes xswa;
	unsigned long valuemask;
	DeskStyle *style = FindDeskStyle(t->desk);

	/* Store Balloon.cs for future events. */
	Balloon.cs = style->balloon_cs;

	fg = (style->balloon_fg) ? style->balloon_fg :
	     (style->win_fg) ? style->win_fg : t->text;
	XSetForeground(dpy, Balloon.gc, fg);

	XUnmapWindow(dpy, Scr.balloon_w);
	xswa.border_pixel = (style->balloon_border) ? style->balloon_border :
			    (style->balloon_fg) ? style->balloon_fg : t->text;
	xswa.background_pixel = (style->balloon_bg) ? style->balloon_bg :
				(style->win_bg) ? style->win_bg : t->back;
	if (style->balloon_cs >= 0 && Colorset[style->balloon_cs].pixmap) {
		/* set later */
		xswa.background_pixmap = None;
		valuemask = CWBackPixmap | CWBorderPixel;;
	} else {
		valuemask = CWBackPixel | CWBorderPixel;
	}
	XChangeWindowAttributes(dpy, Scr.balloon_w, valuemask, &xswa);
}

/* Just maps window ... draw stuff in it later after Expose event
   -- ric@giccs.georgetown.edu */
void MapBalloonWindow(PagerWindow *t, bool is_icon)
{
	Window view, dummy;
	int width, height;
	int x, y;
	rectangle new_g;

	/* Keep track of which window is being mapped. */
	Balloon.is_icon = is_icon;

	if (is_icon) {
		view = t->IconView;
		width = t->icon_view.width;
		height = t->icon_view.height;
	} else {
		view = t->PagerView;
		width = t->pager_view.width;
		height = t->pager_view.height;
	}

	setup_balloon_window(t);
	/* get the label for this balloon */
	free(Balloon.label);
	Balloon.label = get_label(t, Balloon.label_format);

	/* calculate window width to accommodate string */
	new_g.height = Balloon.Ffont->height + 1;
	new_g.width = 4;
	if (Balloon.label) {
		new_g.width += FlocaleTextWidth(
			Balloon.Ffont, Balloon.label,
			strlen(Balloon.label));
	}

	/* get x and y coords relative to view window */
	x = (width / 2) - (new_g.width / 2) - Balloon.border_width;
	if (Balloon.y_offset > 0)
		y = height + Balloon.y_offset - 1;
	else
		y = Balloon.y_offset - new_g.height + 1 -
			(2 * Balloon.border_width);

	/* balloon is a top-level window, therefore need to
	   translate pager window coords to root window coords */
	XTranslateCoordinates(
		dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	/* make sure balloon doesn't go off screen
	 * (actually 2 pixels from edge rather than 0 just to be pretty :-) */
	/* too close to top ... make yoffset +ve */
	if (new_g.y < 2)
	{
		y = - Balloon.y_offset - 1 + height;
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to bottom ... make yoffset -ve */
	else if (new_g.y + new_g.height >
		 fpmonitor_get_all_heights() - (2 * Balloon.border_width) - 2)
	{
		y = - Balloon.y_offset + 1 - new_g.height -
			(2 * Balloon.border_width);
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to left */
	if (new_g.x < 2)
	{
		new_g.x = 2;
	}
	/* too close to right */
	else if (new_g.x + new_g.width >
		 fpmonitor_get_all_widths() - (2 * Balloon.border_width) - 2 )
	{
		new_g.x = fpmonitor_get_all_widths() - new_g.width -
			(2 * Balloon.border_width) - 2;
	}
	/* make changes to window */
	XMoveResizeWindow(dpy,
		Scr.balloon_w, new_g.x, new_g.y, new_g.width, new_g.height);

	/* Update the background if using a colorset. */
	if (Balloon.cs > -1)
		SetWindowBackground(
			dpy, Scr.balloon_w, new_g.width, new_g.height,
			&Colorset[Balloon.cs], Pdepth,
			Scr.NormalGC, True);

	XMapRaised(dpy, Scr.balloon_w);
	Balloon.is_mapped = true;
}


static void InsertExpand(char **dest, char *s)
{
  int len;
  char *tmp = *dest;

  if (!s || !*s)
    return;
  len = strlen(*dest) + strlen(s) + 1;
  *dest = fxmalloc(len);
  strcpy(*dest, tmp);
  free(tmp);
  strcat(*dest, s);
  return;
}


/* Generate the BallonLabel from the format string
   -- disching@fmi.uni-passau.de */
char *get_label(const PagerWindow *pw,const char *fmt)
{
  char *dest = fxstrdup("");
  const char *pos = fmt;
  char buffer[2];

  buffer[1] = '\0';

  while (*pos) {
    if (*pos=='%' && *(pos+1)!='\0') {
      pos++;
      switch (*pos) {
      case 'i':
	InsertExpand(&dest, pw->icon_name);
	break;
      case 't':
	InsertExpand(&dest, pw->window_name);
	break;
      case 'r':
	InsertExpand(&dest, pw->res_name);
	break;
      case 'c':
	InsertExpand(&dest, pw->res_class);
	break;
      case '%':
	buffer[0] = '%';
	InsertExpand(&dest, buffer);
	break;
      default:;
      }
    } else {
      buffer[0] = *pos;
      InsertExpand(&dest, buffer);
    }
    pos++;
  }
  return dest;
}


/* -- ric@giccs.georgetown.edu */
void UnmapBalloonWindow (void)
{
  XUnmapWindow(dpy, Scr.balloon_w);
  Balloon.is_mapped = false;
}


/* Draws string in balloon window -- call after it's received Expose event
   -- ric@giccs.georgetown.edu */
void DrawInBalloonWindow(void)
{
	FwinString->str = Balloon.label;
	FwinString->win = Scr.balloon_w;
	FwinString->gc = Balloon.gc;
	if (Balloon.cs >= 0)
	{
		FwinString->colorset = &Colorset[Balloon.cs];
		FwinString->flags.has_colorset = True;
	}
	else
	{
		FwinString->flags.has_colorset = False;
	}
	FwinString->x = 2;
	FwinString->y = Balloon.Ffont->ascent;
	FlocaleDrawString(dpy, Balloon.Ffont, FwinString, 0);
}
