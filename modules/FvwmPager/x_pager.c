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


extern ScreenInfo Scr;
extern Display *dpy;
extern char *monitor_to_track;

Pixel back_pix, fore_pix, hi_pix;
Pixel focus_pix;
Pixel focus_fore_pix;
extern int windowcolorset, activecolorset;
extern Pixel win_back_pix, win_fore_pix, win_hi_back_pix, win_hi_fore_pix;
extern Bool win_pix_set, win_hi_pix_set;
extern rectangle pwindow;
extern int usposition,uselabel,xneg,yneg;
extern int StartIconic;
extern int MiniIcons;
extern int LabelsBelow;
extern int ShapeLabels;
extern int ShowBalloons, ShowPagerBalloons, ShowIconBalloons;
extern char *BalloonFormatString;
extern char *WindowLabelFormat;
extern char *PagerFore, *PagerBack, *HilightC;
extern char *BalloonFore, *BalloonBack, *BalloonBorderColor;
extern Window BalloonView;
extern unsigned int WindowBorderWidth;
extern unsigned int MinSize;
extern Bool WindowBorders3d;
extern Bool UseSkipList;
extern Bool HideSmallWindows;
extern FvwmPicture *PixmapBack;
extern FvwmPicture *HilightPixmap;
extern int HilightDesks;
extern char fAlwaysCurrentDesk;

extern int MoveThreshold;

extern rectangle icon;
extern int icon_xneg, icon_yneg;
FlocaleFont *Ffont, *FwindowFont;
FlocaleWinString *FwinString;

extern PagerWindow *Start;
extern PagerWindow *FocusWin;
static Atom wm_del_win;

extern char *MyName;

extern int desk1, desk2, ndesks;
extern int Rows,Columns;
extern int fd[2];

int desk_w = 0;
int desk_h = 0;
int label_h = 0;

DeskInfo *Desks;
int Wait = 0;
int FvwmErrorHandler(Display *, XErrorEvent *);
extern Bool is_transient;
extern Bool do_ignore_next_button_release;
extern Bool use_dashed_separators;
extern Bool use_no_separators;
extern int BalloonBorderWidth;


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

Window icon_win;	       /* icon window */

static int MyVx, MyVy;		/* copy of Scr.Vx/y for drag logic */
static struct fpmonitor *ScrollFp = NULL;	/* Stash monitor drag logic */

static rectangle CalcGeom(PagerWindow *, bool);
static rectangle set_vp_size_and_loc(struct fpmonitor *, bool is_icon);
static struct fpmonitor *fpmonitor_from_xy(int x, int y, bool allow_null);
static int fpmonitor_count(void);
static void fvwmrec_to_pager(rectangle *, bool, struct fpmonitor *);
static void pagerrec_to_fvwm(rectangle *, bool, struct fpmonitor *);
static char *GetBalloonLabel(const PagerWindow *pw,const char *fmt);
extern void ExitPager(void);

Pixmap default_pixmap = None;

#define  MAX_UNPROCESSED_MESSAGES 1
/* sums up pixels to scroll. If do_send_message is True a Scroll command is
 * sent back to fvwm. The function shall be called with is_message_recieved
 * True when the Scroll command has been processed by fvwm. This is checked
 * by talking to ourself. */
static void do_scroll(int sx, int sy, Bool do_send_message,
		      Bool is_message_recieved)
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
				 monitor_to_track);
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
	do_scroll(0, 0, True, True);
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
		c++;
	return (c);
}

static struct fpmonitor *fpmonitor_from_xy(int x, int y, bool allow_null)
{
	struct fpmonitor *m;
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (monitor_to_track == NULL && monitor_mode != MONITOR_TRACKING_G) {
		if (allow_null)
			fp = NULL;
		x %= fpmonitor_get_all_widths();
		y %= fpmonitor_get_all_heights();

		TAILQ_FOREACH(m, &fp_monitor_q, entry) {
		if (x >= m->m->si->x && x < m->m->si->x + m->m->si->w &&
		    y >= m->m->si->y && y < m->m->si->y + m->m->si->h)
			fp = m;
		}
	}

	return fp;
}

static rectangle CalcGeom(PagerWindow *t, bool is_icon)
{
	rectangle rec = {0, 0, 0, 0};
	struct fpmonitor *fp = fpmonitor_this(NULL);

	/* Only track windows on the appropriate monitor if per-monitor mode. */
	if (monitor_to_track != NULL && t->m != fp->m)
		return rec;
	else
		fp = fpmonitor_this(t->m);

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
			rec.x = ((rec.x / page_w) + 1)*page_w - MinSize + 1;
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
			rec.y = ((rec.y / page_h) + 1)*page_h - MinSize + 1;
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
	 * DefaultDepth(dpy, Scr.screen) to use the Scr.Root for Scr.Pager_w */
	if (Pdefault)
	{
		Scr.Pager_w = Scr.Root;
	}
	else
	{
		attr.background_pixmap = None;
		attr.border_pixel = 0;
		attr.colormap = Pcmap;
		Scr.Pager_w = XCreateWindow(
			dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth,
			InputOutput, Pvisual,
			CWBackPixmap|CWBorderPixel|CWColormap, &attr);
		Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, 0, &xgcv);
	}
	Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, 0, NULL);

	xgcv.plane_mask = AllPlanes;
	Scr.MiniIconGC = fvwmlib_XCreateGC(
		dpy, Scr.Pager_w, GCPlaneMask, &xgcv);
	Scr.black = GetColor("Black");

	/* Transparent background are only allowed when the depth matched the
	 * root */
	if (Pdepth == DefaultDepth(dpy, Scr.screen))
	{
		default_pixmap = ParentRelative;
	}

	return;
}

/* see also change colorset */
void draw_desk_background(int i, int page_w, int page_h, struct fpmonitor *fp)
{
	if (Desks[i].colorset > -1)
	{
		XSetWindowBorder(
			dpy, Desks[i].title_w, Colorset[Desks[i].colorset].fg);
		XSetWindowBorder(
			dpy, Desks[i].w, Colorset[Desks[i].colorset].fg);
		XSetForeground(
			dpy, Desks[i].NormalGC,Colorset[Desks[i].colorset].fg);
		XSetForeground(
			dpy, Desks[i].DashedGC,Colorset[Desks[i].colorset].fg);
		if (uselabel)
		{
			if (CSET_IS_TRANSPARENT(Desks[i].colorset))
			{
					SetWindowBackground(
						dpy, Desks[i].title_w,
						desk_w, label_h,
						&Colorset[Desks[i].colorset],
						Pdepth,
						Scr.NormalGC, True);
			}
			else
			{
				SetWindowBackground(
					dpy, Desks[i].title_w, desk_w,
					desk_h + label_h,
					&Colorset[Desks[i].colorset], Pdepth,
					Scr.NormalGC, True);
			}
		}
		if (label_h != 0 && uselabel && !LabelsBelow &&
		    !CSET_IS_TRANSPARENT(Desks[i].colorset))
		{
			SetWindowBackgroundWithOffset(
				dpy, Desks[i].w, 0, -label_h, desk_w,
				desk_h + label_h, &Colorset[Desks[i].colorset],
				Pdepth, Scr.NormalGC, True);
		}
		else
		{
			if (CSET_IS_TRANSPARENT(Desks[i].colorset))
			{
				SetWindowBackground(
					dpy, Desks[i].w, desk_w, desk_h,
					&Colorset[Desks[i].colorset], Pdepth,
					Scr.NormalGC, True);
			}
			else
			{
				SetWindowBackground(
					dpy, Desks[i].w, desk_w,
					desk_h + label_h,
					&Colorset[Desks[i].colorset],
					Pdepth, Scr.NormalGC, True);
			}
		}
	}
	XClearArea(dpy,Desks[i].w, 0, 0, 0, 0,True);
	if (Desks[i].highcolorset > -1)
	{
		XSetForeground(
			dpy, Desks[i].HiliteGC,
			Colorset[Desks[i].highcolorset].bg);
		XSetForeground(
			dpy, Desks[i].rvGC, Colorset[Desks[i].highcolorset].fg);
		if (HilightDesks)
		{
			SetWindowBackground(
				dpy, fp->CPagerWin[i], page_w, page_h,
				&Colorset[Desks[i].highcolorset],
				Pdepth, Scr.NormalGC, True);
		}
	}
	if (uselabel)
	{
		XClearArea(dpy,Desks[i].title_w, 0, 0, 0, 0,True);
	}

	return;
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
char *pager_name = "Fvwm Pager";
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

void initialize_balloon_window(void)
{
	XGCValues xgcv;
	unsigned long valuemask;
	XSetWindowAttributes attributes;

	/* create balloon window
	   -- ric@giccs.georgetown.edu */
	if (!ShowBalloons)
	{
		Scr.balloon_w = None;
		return;
	}
	valuemask = CWOverrideRedirect | CWEventMask | CWColormap;
	/* tell WM to ignore this window */
	attributes.override_redirect = True;
	attributes.event_mask = ExposureMask;
	attributes.colormap = Pcmap;
	/* now create the window */
	Scr.balloon_w = XCreateWindow(
		dpy, Scr.Root, 0, 0, /* coords set later */ 1, 1,
		BalloonBorderWidth, Pdepth, InputOutput, Pvisual, valuemask,
		&attributes);
	Scr.balloon_gc = fvwmlib_XCreateGC(dpy, Scr.balloon_w, 0, &xgcv);
	/* Make sure we don't get balloons initially with the Icon option. */
	ShowBalloons = ShowPagerBalloons;

	return;
}

/* Computes pager size rectangle from fvwm size or visa versa */
static void
fvwmrec_to_pager(rectangle *rec, bool is_icon, struct fpmonitor *fp)
{
	if (fp == NULL)
		return;

	int m_width = fpmonitor_get_all_widths();
	int m_height = fpmonitor_get_all_heights();

	if (monitor_to_track != NULL) {
		/* Offset window location based on monitor location. */
		rec->x -= (m_width - fp->m->si->w) * (rec->x / m_width) + fp->m->si->x;
		rec->y -= (m_height - fp->m->si->h) * (rec->y / m_height) + fp->m->si->y;
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

	if (monitor_to_track != NULL) {
		offset_x = (m_width - fp->m->si->w) * ((fp->virtual_scr.VxPages *
			rec->x) / scale_w) + fp->m->si->x;
		offset_y = (m_height - fp->m->si->h) * ((fp->virtual_scr.VyPages *
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
	if (m != NULL && monitor_to_track == NULL) {
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
	extern char *WindowBack, *WindowFore, *WindowHiBack, *WindowHiFore;
	extern char *font_string, *smallFont;

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

	label_h = (uselabel) ? Ffont->height + 2 : 0;

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

	/* Load the colors */
	fore_pix = GetColor(PagerFore);
	back_pix = GetColor(PagerBack);
	hi_pix = GetColor(HilightC);

	if (windowcolorset >= 0)
	{
		win_back_pix = Colorset[windowcolorset].bg;
		win_fore_pix = Colorset[windowcolorset].fg;
		win_pix_set = True;
	}
	else if (WindowBack && WindowFore)
	{
		win_back_pix = GetColor(WindowBack);
		win_fore_pix = GetColor(WindowFore);
		win_pix_set = True;
	}

	if (activecolorset >= 0)
	{
		win_hi_back_pix = Colorset[activecolorset].bg;
		win_hi_fore_pix = Colorset[activecolorset].fg;
		win_hi_pix_set = True;
	}
	else if (WindowHiBack && WindowHiFore)
	{
		win_hi_back_pix = GetColor(WindowHiBack);
		win_hi_fore_pix = GetColor(WindowHiFore);
		win_hi_pix_set = True;
	}

	/* Load pixmaps for mono use */
	if(Pdepth<2)
	{
		Scr.gray_pixmap = XCreatePixmapFromBitmapData(dpy, Scr.Pager_w,
			g_bits, g_width,g_height, fore_pix,back_pix,Pdepth);
		Scr.light_gray_pixmap = XCreatePixmapFromBitmapData(dpy,
			Scr.Pager_w,l_g_bits,l_g_width, l_g_height,
			fore_pix,back_pix,Pdepth);
		Scr.sticky_gray_pixmap = XCreatePixmapFromBitmapData(dpy,
			Scr.Pager_w,s_g_bits,s_g_width, s_g_height,
			fore_pix,back_pix,Pdepth);
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
}

void initialize_fpmonitor_windows(struct fpmonitor *fp)
{
	rectangle vp = set_vp_size_and_loc(fp, false);
	for (int i = 0; i < ndesks; i++) {
		fp->CPagerWin[i] = XCreateWindow(dpy, Desks[i].w,
				-32768, -32768, vp.width, vp.height, 0,
				CopyFromParent, InputOutput,
				CopyFromParent, Desks[i].fp_mask,
				&Desks[i].fp_attr);
		draw_desk_background(i, vp.width, vp.height, fp);
		XMapRaised(dpy, fp->CPagerWin[i]);
	}
}

void initialize_pager(void)
{
  XWMHints wmhints;
  XClassHint class1;
  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i = 0;
  XGCValues gcv;
  extern char *BalloonFont;
  char dash_list[2];
  FlocaleFont *balloon_font;
  struct fpmonitor *fp = fpmonitor_this(NULL);
  struct fpmonitor *m;

  int VxPages = fp->virtual_scr.VxPages, VyPages = fp->virtual_scr.VyPages;

  /* Set window size if not fully set by user to match */
  /* aspect ratio of monitor(s) being shown. */
  if ( pwindow.width == 0 || pwindow.height == 0 ) {
	  int vWidth  = fpmonitor_get_all_widths();
	  int vHeight = fpmonitor_get_all_heights();
	  if (monitor_to_track != NULL) {
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
  desk_w = (pwindow.width - Columns + 1) / Columns;
  desk_h = (pwindow.height - Rows * label_h - Rows + 1) / Rows;

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
      xneg = 1;
    }
    if (pwindow.height + pwindow.y > screen_g.y + screen_g.height)
    {
      pwindow.y = screen_g.y + screen_g.height - fp->m->si->h; //fp->m->virtual_scr.MyDisplayHeight;
      yneg = 1;
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
  if (Scr.Pager_w != Scr.Root)
    XDestroyWindow(dpy, Scr.Pager_w);
  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, pwindow.x, pwindow.y, pwindow.width,
			       pwindow.height, 0, Pdepth, InputOutput, Pvisual,
			       valuemask, &attributes);
  XSetWMProtocols(dpy,Scr.Pager_w,&wm_del_win,1);
  /* hack to prevent mapping on wrong screen with StartsOnScreen */
  FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &sizehints);
  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);
  if (is_transient)
  {
    XSetTransientForHint(dpy, Scr.Pager_w, Scr.Root);
  }

  if((desk1==desk2)&&(Desks[0].label != NULL))
  {
    if (FlocaleTextListToTextProperty(
	 dpy, &Desks[0].label, 1, XStdICCTextStyle, &name) == 0)
    {
      fprintf(stderr,"%s: fatal error: cannot allocate desk name", MyName);
      exit(0);
    }
  }
  else
  {
    if (FlocaleTextListToTextProperty(
	 dpy, &Desks[0].label, 1, XStdICCTextStyle, &name) == 0)
    {
      fprintf(stderr,"%s: fatal error: cannot allocate pager name", MyName);
      exit(0);
    }
  }

  attributes.event_mask = (StructureNotifyMask| ExposureMask);
  if(icon.width < 1)
    icon.width = (pwindow.width - Columns + 1) / Columns;
  if(icon.height < 1)
    icon.height = (pwindow.height - Rows * label_h - Rows + 1) / Rows;

  icon.width = (icon.width / (VxPages + 1)) * (VxPages + 1) + VxPages;
  icon.height = (icon.height / (VyPages + 1)) * (VyPages + 1) + VyPages;
  icon_win = XCreateWindow (dpy, Scr.Root, pwindow.x, pwindow.y,
			    icon.width, icon.height,
			    0, Pdepth, InputOutput, Pvisual, valuemask,
			    &attributes);
  XGrabButton(dpy, 1, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 2, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 3, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);

  if(!StartIconic)
    wmhints.initial_state = NormalState;
  else
    wmhints.initial_state = IconicState;
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
    icon_xneg = 0;
    icon_yneg = 0;
    wmhints.icon_x = icon.x;
    wmhints.icon_y = icon.y;
    wmhints.flags = IconPositionHint;
  }
  wmhints.icon_window = icon_win;
  wmhints.input = False;
  wmhints.flags |= InputHint | StateHint | IconWindowHint;

  class1.res_name = MyName;
  class1.res_class = "FvwmPager";

  XSetWMProperties(dpy,Scr.Pager_w,&name,&name,NULL,0,
		   &sizehints,&wmhints,&class1);
  XFree((char *)name.value);

  /* change colour/font for labelling mini-windows */
  XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);

  if (FwindowFont != NULL && FwindowFont->font != NULL)
    XSetFont(dpy, Scr.NormalGC, FwindowFont->font->fid);

  /* create the 3d bevel GC's if necessary */
  if (windowcolorset >= 0) {
    gcv.foreground = Colorset[windowcolorset].hilite;
    Scr.whGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[windowcolorset].shadow;
    Scr.wsGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }
  if (activecolorset >= 0) {
    gcv.foreground = Colorset[activecolorset].hilite;
    Scr.ahGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[activecolorset].shadow;
    Scr.asGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }

  balloon_font = FlocaleLoadFont(dpy, BalloonFont, MyName);

  for(i=0;i<ndesks;i++)
  {
    /* create the GC for desk labels */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    if (uselabel && Ffont && Ffont->font) {
      gcv.font = Ffont->font->fid;
      Desks[i].NormalGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont, &gcv);
    } else {
      Desks[i].NormalGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    }
    /* create the active desk hilite GC */
    if(Pdepth < 2)
      gcv.foreground = fore_pix;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? hi_pix
	: Colorset[Desks[i].highcolorset].bg;
    Desks[i].HiliteGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);

    /* create the hilight desk title drawing GC */
    if ((Pdepth < 2) || (fore_pix == hi_pix))
      gcv.foreground = (Desks[i].highcolorset < 0) ? back_pix
	: Colorset[Desks[i].highcolorset].fg;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? fore_pix
	: Colorset[Desks[i].highcolorset].fg;

    if (uselabel && Ffont && Ffont->font) {
      Desks[i].rvGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont, &gcv);
    } else {
      Desks[i].rvGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    }
    /* create the virtual page boundary GC */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    gcv.line_width = 1;
    gcv.line_style = (use_dashed_separators) ? LineOnOffDash : LineSolid;
    Desks[i].DashedGC =
      fvwmlib_XCreateGC(
	dpy, Scr.Pager_w, GCForeground | GCLineStyle | GCLineWidth, &gcv);
    if (use_dashed_separators)
    {
      /* Although this should already be the default for a freshly created GC,
       * some X servers do not draw properly dashed lines if the dash style is
       * not set explicitly. */
      dash_list[0] = 4;
      dash_list[1] = 4;
      XSetDashes(dpy, Desks[i].DashedGC, 0, dash_list, 2);
    }

    valuemask = (CWBorderPixel | CWColormap | CWEventMask);
    if (Desks[i].colorset >= 0 && Colorset[Desks[i].colorset].pixmap)
    {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = Colorset[Desks[i].colorset].pixmap;
    }
    else
    {
	valuemask |= CWBackPixel;
	attributes.background_pixel = (Desks[i].colorset < 0) ?
	    (Desks[i].Dcolor ? GetColor(Desks[i].Dcolor) : back_pix)
	    : Colorset[Desks[i].colorset].bg;
    }

    attributes.border_pixel = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    attributes.event_mask = (ExposureMask | ButtonReleaseMask);
    Desks[i].title_w = XCreateWindow(
      dpy, Scr.Pager_w, -1, -1, desk_w, desk_h + label_h, 1,
      CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    attributes.event_mask = (ExposureMask | ButtonReleaseMask |
			     ButtonPressMask |ButtonMotionMask);

    valuemask &= ~(CWBackPixel);
    if (Desks[i].colorset > -1 &&
	Colorset[Desks[i].colorset].pixmap)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = None; /* set later */
    }
    else if (Desks[i].bgPixmap)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = Desks[i].bgPixmap->picture;
    }
    else if (Desks[i].Dcolor)
    {
      valuemask |= CWBackPixel;
      attributes.background_pixel = GetColor(Desks[i].Dcolor);
    }
    else if (PixmapBack)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = PixmapBack->picture;
    }
    else
    {
      valuemask |= CWBackPixel;
      attributes.background_pixel = (Desks[i].colorset < 0) ? back_pix
	: Colorset[Desks[i].colorset].bg;
    }

    Desks[i].w = XCreateWindow(
	    dpy, Desks[i].title_w, -1, LabelsBelow ? -1 : label_h - 1,
	    desk_w, desk_h, 1, CopyFromParent, InputOutput, CopyFromParent,
	    valuemask, &attributes);

    if (HilightDesks)
    {
      valuemask &= ~(CWBackPixel | CWBackPixmap);

      attributes.event_mask = 0;

      if (Desks[i].highcolorset > -1 &&
	  Colorset[Desks[i].highcolorset].pixmap)
      {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = None; /* set later */
      }
      else if (HilightPixmap)
      {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = HilightPixmap->picture;
      }
      else
      {
	valuemask |= CWBackPixel;
	attributes.background_pixel = (Desks[i].highcolorset < 0) ? hi_pix
	  : Colorset[Desks[i].highcolorset].bg;
      }
    }
    else
    {
      draw_desk_background(i, 0, 0, fp);
    }

    Desks[i].fp_mask = valuemask;
    Desks[i].fp_attr = attributes;
    XMapRaised(dpy,Desks[i].w);
    XMapRaised(dpy,Desks[i].title_w);

    /* get font for balloon */
    Desks[i].balloon.Ffont = balloon_font;
    Desks[i].balloon.height = Desks[i].balloon.Ffont->height + 1;
  }

  TAILQ_FOREACH(m, &fp_monitor_q, entry) {
    initialize_fpmonitor_windows(m);
  }
  initialize_balloon_window();
  XMapRaised(dpy,Scr.Pager_w);
}


void UpdateWindowShape(void)
{
  if (FHaveShapeExtension)
  {
    int i, j, cnt, shape_count, x_pos, y_pos;
    XRectangle *shape;
    struct fpmonitor *fp = fpmonitor_this(NULL);

    if (!fp || !ShapeLabels || !uselabel || label_h<=0)
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
      dpy, Scr.Pager_w, FShapeBounding, 0, 0, shape, shape_count, FShapeSet, 0);
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
  Bool do_move_page = False;
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
    if ( ShowBalloons )
      UnmapBalloonWindow();
    break;
  case ConfigureNotify:
    fev_sanitise_configure_notify(&Event->xconfigure);
    w = Event->xconfigure.window;
    discard_events(ConfigureNotify, Event->xconfigure.window, Event);
    fev_sanitise_configure_notify(&Event->xconfigure);
    if (w != icon_win)
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
	do_move_page = True;
	break;
      case XK_Down:
	dy = 100;
	do_move_page = True;
	break;
      case XK_Left:
	dx = -100;
	do_move_page = True;
	break;
      case XK_Right:
	dx = 100;
	do_move_page = True;
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
      do_ignore_next_button_release = False;
      break;
    }
    if (Event->xbutton.button == 3)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  Scroll(x, y, i, False);
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	Scroll(x, y, -1, True);
      }
      /* Flush any pending scroll operations */
      do_scroll(0, 0, True, False);
      ScrollFp = NULL;
    }
    else if((Event->xbutton.button == 1)||
	    (Event->xbutton.button == 2))
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	  SwitchToDeskAndPage(i,Event);
	/* Title clicks only change desk in global configuration, or when the
	 * pager is being asked to track a specific monitor.  Or, regardless
	 * of the DesktopConfiguration setting, there is only one monitor in
	 * use.
	 */
	else if(Event->xany.window == Desks[i].title_w &&
		((monitor_mode == MONITOR_TRACKING_G) ||
		 (monitor_to_track != NULL) || (fpmonitor_count() == 1))) {
			SwitchToDesk(i, NULL);
			break;
	}
      }
      if(Event->xany.window == icon_win)
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
    do_ignore_next_button_release = False;
    if ( ShowBalloons )
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
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  fp = fpmonitor_from_xy(x * fp->virtual_scr.VWidth / desk_w,
			   y * fp->virtual_scr.VHeight / desk_h, true);
	  if (fp == NULL)
	    break;
	  /* Save initial virtual desk info. */
	  ScrollFp = fp;
	  MyVx = fp->virtual_scr.Vx;
	  MyVy = fp->virtual_scr.Vy;
	  Scroll(x, y, fp->m->virtual_scr.CurrentDesk, False);
	  if (fp->m->virtual_scr.CurrentDesk != i + desk1)
	  {
	    Wait = 0;
	    SwitchToDesk(i, fp);
	  }
	  break;
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	fp = fpmonitor_from_xy(x * fp->virtual_scr.VWidth / icon.width,
			 y * fp->virtual_scr.VHeight / icon.height, true);
	if (fp == NULL)
	  break;
	/* Save initial virtual desk info. */
	ScrollFp = fp;
	MyVx = fp->virtual_scr.Vx;
	MyVy = fp->virtual_scr.Vy;
	Scroll(x, y, -1, True);
      }
    }
    break;
  case MotionNotify:
    do_ignore_next_button_release = False;
    while(FCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask,Event))
      ;

    if(Event->xmotion.state & Button3MotionMask)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  Scroll(x, y, i, False);
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	Scroll(x, y, -1, True);
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
  Bool is_icon_view = False;
  extern Bool do_focus_on_enter;

  if (!ShowBalloons && !do_focus_on_enter)
    /* nothing to do */
    return;

  /* is this the best way to match X event window ID to PagerWindow ID? */
  for ( t = Start; t != NULL; t = t->next )
  {
    if ( t->PagerView == Event->xcrossing.window )
    {
      break;
    }
    if ( t->IconView == Event->xcrossing.window )
    {
      is_icon_view = True;
      break;
    }
  }
  if (t == NULL)
  {
    return;
  }

  if (ShowBalloons)
  {
    MapBalloonWindow(t, is_icon_view);
  }

  if (do_focus_on_enter)
  {
    SendText(fd, "Silent FlipFocus NoWarp", t->w);
  }

}

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

void HandleExpose(XEvent *Event)
{
	int i;
	PagerWindow *t;
	XRectangle r;

	/* it will be good to have full "clipping redraw". Do that for
	 * desk label only for now */
	for(i=0;i<ndesks;i++)
	{
		/* ric@giccs.georgetown.edu */
		if (Event->xany.window == Desks[i].w ||
		    Event->xany.window == Desks[i].title_w)
		{
			r = get_expose_bound(Event);
			DrawGrid(i, 0, Event->xany.window, &r);
			return;
		}
	}
	if (Event->xany.window == Scr.balloon_w)
	{
		DrawInBalloonWindow(Scr.balloon_desk);
		return;
	}
	if(Event->xany.window == icon_win)
		DrawIconGrid(0);

	for (t = Start; t != NULL; t = t->next)
	{
		if (t->PagerView == Event->xany.window)
		{
			LabelWindow(t);
			PictureWindow(t);
			BorderWindow(t);
		}
		else if(t->IconView == Event->xany.window)
		{
			LabelIconWindow(t);
			PictureIconWindow(t);
			BorderIconWindow(t);
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
  unsigned border_width, depth;
  rectangle vp = {0, 0, 0, 0};
  int i = 0, j = 0, k = 0;
  struct fpmonitor *fp = fpmonitor_this(NULL);
  struct fpmonitor *m;

  if (fp == NULL)
    return;

  if (!XGetGeometry(dpy, Scr.Pager_w, &root, &vp.x, &vp.y, (unsigned *)&pwindow.width,
		    (unsigned *)&pwindow.height, &border_width,&depth))
  {
    return;
  }
  desk_w = (pwindow.width - Columns + 1) / Columns;
  desk_h = (pwindow.height - Rows * label_h - Rows + 1) / Rows;

  for(k=0;k<Rows;k++)
  {
    for(j=0;j<Columns;j++)
    {
      i = k*Columns+j;
      if (i<ndesks)
      {
	XMoveResizeWindow(
	  dpy,Desks[i].title_w, (desk_w+1)*j-1,(desk_h+label_h+1)*k-1,
	  desk_w,desk_h+label_h);
	XMoveResizeWindow(
	  dpy,Desks[i].w, -1, (LabelsBelow) ? -1 : label_h - 1, desk_w,desk_h);
	if (HilightDesks)
	{
	  TAILQ_FOREACH(m, &fp_monitor_q, entry) {
	    if (m->disabled)
	      continue;
	    vp = set_vp_size_and_loc(m, false);
	    if (i == m->m->virtual_scr.CurrentDesk - desk1 &&
		(monitor_to_track == NULL ||
		strcmp(m->m->si->name, monitor_to_track) == 0))
	    {
	      XMoveResizeWindow(dpy, m->CPagerWin[i],
				vp.x, vp.y, vp.width, vp.height);
	    } else {
	      XMoveResizeWindow(dpy, m->CPagerWin[i],
				-32768, -32768, vp.width, vp.height);
	    }
	    draw_desk_background(i, vp.width, vp.height, m);
	  }
	} else {
	  vp = set_vp_size_and_loc(NULL, false);
	  draw_desk_background(i, vp.width, vp.height, fp);
	}
      }
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

/* update the hilight desk and the windows: desk color change */
static
void update_pr_transparent_subwindows(int i)
{
	int cset;
	rectangle vp;
	PagerWindow *t;
	struct fpmonitor *m;

	if (CSET_IS_TRANSPARENT_PR(Desks[i].highcolorset) && HilightDesks)
	{
		TAILQ_FOREACH(m, &fp_monitor_q, entry) {
			vp = set_vp_size_and_loc(m, false);
			SetWindowBackground(
				dpy, m->CPagerWin[i], vp.width, vp.height,
				&Colorset[Desks[i].highcolorset],
				Pdepth, Scr.NormalGC, True);
		}
	}

	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		cset = (t != FocusWin) ? windowcolorset : activecolorset;
		if (t->desk != i && !CSET_IS_TRANSPARENT_PR(cset))
		{
			continue;
		}
		if (t->PagerView != None)
		{
			SetWindowBackground(
				dpy, t->PagerView, t->pager_view.width,
				t->pager_view.height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
		if (t->IconView)
		{
			SetWindowBackground(
				dpy, t->IconView, t->icon_view.width,
				t->icon_view.height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
	}
}

/* update all the parental relative windows: pr background change */
void update_pr_transparent_windows(void)
{
	int i,j,k,cset;
	rectangle vp;
	PagerWindow *t;
	struct fpmonitor *m;

	for(k=0;k<Rows;k++)
	{
		for(j=0;j<Columns;j++)
		{
			i = k*Columns+j;
			if (i < ndesks)
			{
				TAILQ_FOREACH(m, &fp_monitor_q, entry) {
					vp = set_vp_size_and_loc(m, false);
					if (CSET_IS_TRANSPARENT_PR(
						Desks[i].colorset))
					{
						draw_desk_background(i,
							vp.width,
							vp.height, m);
					}
					else if (CSET_IS_TRANSPARENT_PR(
						Desks[i].highcolorset) &&
						HilightDesks)
					{
						SetWindowBackground(
							dpy, m->CPagerWin[i],
							vp.width, vp.height,
							&Colorset[Desks[i].
								highcolorset],
							Pdepth, Scr.NormalGC,
							True);
					}
				}
			}
		}
	}
	/* subordinate windows with a pr parent desk */
	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		cset = (t != FocusWin) ? windowcolorset : activecolorset;
		if (!CSET_IS_TRANSPARENT_PR(cset) ||
		    (fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[0].colorset)) ||
		    (!fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[t->desk].colorset)))
		{
			continue;
		}
		if (t->PagerView != None)
		{
			SetWindowBackground(
				dpy, t->PagerView, t->pager_view.width,
				t->pager_view.height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
		if (t->desk && t->IconView)
		{
			SetWindowBackground(
				dpy, t->IconView, t->icon_view.width,
				t->icon_view.height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
	}

	/* ballon */
	if (BalloonView != None)
	{
		cset = Desks[Scr.balloon_desk].ballooncolorset;
		if (CSET_IS_TRANSPARENT_PR(cset))
		{
			XClearArea(dpy, Scr.balloon_w, 0, 0, 0, 0, True);
		}
	}
}

void MovePage(Bool is_new_desk)
{
  int i;
  rectangle vp;
  XTextProperty name;
  char str[100],*sptr;
  static int icon_desk_shown = -1000;
  struct fpmonitor *fp = fpmonitor_this(NULL);
  struct fpmonitor *m;

  if (fp == NULL)
    return;

  Wait = 0;

  for(i=0;i<ndesks;i++)
  {
    if (HilightDesks)
    {
      TAILQ_FOREACH(m, &fp_monitor_q, entry) {
	if (!m->disabled && i == m->m->virtual_scr.CurrentDesk - desk1 &&
		(monitor_to_track == NULL ||
		strcmp(m->m->si->name, monitor_to_track) == 0))
	{
		vp = set_vp_size_and_loc(m, false);
		XMoveResizeWindow(dpy, m->CPagerWin[i],
				vp.x, vp.y, vp.width, vp.height);
		XLowerWindow(dpy, m->CPagerWin[i]);
		if (CSET_IS_TRANSPARENT(Desks[i].highcolorset))
		{
			SetWindowBackground(
				dpy, m->CPagerWin[i], vp.width, vp.height,
				&Colorset[Desks[i].highcolorset], Pdepth,
				Scr.NormalGC, True);
		}
	} else {
		XMoveWindow(dpy, m->CPagerWin[i], -32768,-32768);
	}
      }
    }
  }
  DrawIconGrid(1);

  ReConfigureIcons(!is_new_desk);

  if(fp->m->virtual_scr.CurrentDesk != icon_desk_shown)
  {
    icon_desk_shown = fp->m->virtual_scr.CurrentDesk;

    if((fp->m->virtual_scr.CurrentDesk >= desk1)&&(fp->m->virtual_scr.CurrentDesk <=desk2))
      sptr = Desks[fp->m->virtual_scr.CurrentDesk -desk1].label;
    else
    {
      snprintf(str, sizeof(str), "GotoDesk screen %s %d", fp->m->si->name, fp->m->virtual_scr.CurrentDesk);
      sptr = &str[0];
    }

    if (FlocaleTextListToTextProperty(
	  dpy, &sptr, 1, XStdICCTextStyle, &name) == 0)
    {
      return;
    }
    XSetWMIconName(dpy,Scr.Pager_w,&name);
    XFree(name.value);
  }
}

void ReConfigureAll(void)
{
	PagerWindow *t;

	t = Start;
	while(t != NULL) {
		MoveResizePagerView(t, True);
		t = t->next;
	}
	ReConfigureIcons(False);
}

void ReConfigureIcons(Bool do_reconfigure_desk_only)
{
  PagerWindow *t;
  rectangle rec;
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL)
    return;

  for (t = Start; t != NULL; t = t->next)
  {
    if (do_reconfigure_desk_only && t->desk != fp->m->virtual_scr.CurrentDesk)
      continue;
    rec = CalcGeom(t, true);
    t->icon_view = rec;
    if ((fp->m->virtual_scr.CurrentDesk != t->desk) ||
        (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize)))
    {
      rec.x = -32768;
      rec.y = -32768;
    }
    XMoveResizeWindow(dpy, t->IconView, rec.x, rec.y, rec.width, rec.height);
  }
}

/*
 *
 * Draw grid lines for desk #i
 *
 */
void DrawGrid(int desk, int erase, Window ew, XRectangle *r)
{
	int y, y1, y2, x, x1, x2,d,w;
	char str[15], *ptr;
	int cs;
	XRectangle bound;
	Region region = 0;
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (fp == NULL)
		return;

	if((desk < 0 ) || (desk >= ndesks))
		return;

	/* desk grid */
	if (!ew || ew == Desks[desk].w)
	{
		x = fpmonitor_get_all_widths();
		y1 = 0;
		y2 = desk_h;
		while (x < fp->virtual_scr.VWidth)
		{
			x1 = (x * desk_w) / fp->virtual_scr.VWidth;
			if (!use_no_separators)
			{
				XDrawLine(
					dpy,Desks[desk].w,Desks[desk].DashedGC,
					x1,y1,x1,y2);
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
				XDrawLine(
					dpy,Desks[desk].w,Desks[desk].DashedGC,
					x1,y1,x2,y1);
			}
			y += fpmonitor_get_all_heights();
		}
	}

	if (ew && ew != Desks[desk].title_w)
	{
		return;
	}

	/* desk label */
	if (r)
	{
		bound.x = r->x;
		bound.y = r->y;
		bound.width = r->width;
		bound.height = r->height;
		region = XCreateRegion();
		XUnionRectWithRegion (&bound, region, region);
	}
	else
	{
		bound.x = 0;
		bound.y = (LabelsBelow ? desk_h : 0);
		bound.width = desk_w;
		bound.height = label_h;
	}

	if (FftSupport)
	{
		erase = True;
	}
	if(((fp->m->virtual_scr.CurrentDesk - desk1) == desk) && !ShapeLabels)
	{
		if (uselabel)
		{
			XFillRectangle(
				dpy,Desks[desk].title_w,Desks[desk].HiliteGC,
				bound.x, bound.y, bound.width, bound.height);
		}
	}
	else
	{
		if(uselabel && erase)
		{
			XClearArea(dpy,Desks[desk].title_w,
				   bound.x, bound.y, bound.width, bound.height,
				   False);
		}
	}

	d = desk1+desk;
	ptr = Desks[desk].label;
	w = FlocaleTextWidth(Ffont,ptr,strlen(ptr));
	if( w > desk_w)
	{
		snprintf(str,sizeof(str),"%d",d);
		ptr = str;
		w = FlocaleTextWidth(Ffont,ptr,strlen(ptr));
	}
	if((w <= desk_w)&&(uselabel))
	{
		FwinString->str = ptr;
		FwinString->win = Desks[desk].title_w;
		if(desk == (fp->m->virtual_scr.CurrentDesk - desk1))
		{
			cs = Desks[desk].highcolorset;
			FwinString->gc = Desks[desk].rvGC;
		}
		else
		{
			cs = Desks[desk].colorset;
			FwinString->gc = Desks[desk].NormalGC;
		}

		FwinString->flags.has_colorset = False;
		if (cs >= 0)
		{
			FwinString->colorset = &Colorset[cs];
			FwinString->flags.has_colorset = True;
		}
		FwinString->x = (desk_w - w)/2;
		FwinString->y = (LabelsBelow ?
				 desk_h + Ffont->ascent + 1 : Ffont->ascent + 1);
		if (region)
		{
			FwinString->flags.has_clip_region = True;
			FwinString->clip_region = region;
			XSetRegion(dpy, FwinString->gc, region);
		}
		else
		{
			FwinString->flags.has_clip_region = False;
		}
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
		if (region)
		{
			XDestroyRegion(region);
			FwinString->flags.has_clip_region = False;
			FwinString->clip_region = None;
			XSetClipMask(dpy, FwinString->gc, None);
		}
	}

	if (FShapesSupported)
	{
		UpdateWindowShape ();
	}
}


void DrawIconGrid(int erase)
{
	rectangle rec;
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (fp == NULL)
		return;

	int i, tmp = (fp->m->virtual_scr.CurrentDesk - desk1);

	if (tmp < 0 || tmp >= ndesks)
		tmp = 0;

	if (erase) {
		if (Desks[tmp].bgPixmap)
			XSetWindowBackgroundPixmap(dpy, icon_win,
				Desks[tmp].bgPixmap->picture);
		else if (Desks[tmp].Dcolor)
			XSetWindowBackground(dpy, icon_win,
				GetColor(Desks[tmp].Dcolor));
		else if (PixmapBack)
			XSetWindowBackgroundPixmap(dpy, icon_win,
				PixmapBack->picture);
		else if (Desks[tmp].colorset > 0)
			XSetWindowBackground(dpy, icon_win,
				Colorset[Desks[tmp].colorset].bg);
		else
			XSetWindowBackground(dpy, icon_win, back_pix);

		XClearWindow(dpy,icon_win);
	}

	rec.x = fp->virtual_scr.VxPages;
	rec.y = fp->virtual_scr.VyPages;
	rec.width = icon.width / rec.x;
	rec.height = icon.height / rec.y;
	if (!use_no_separators) {
		for(i = 1; i < rec.x; i++)
			XDrawLine(dpy, icon_win, Desks[tmp].DashedGC,
				i * rec.width, 0, i * rec.width, icon.height);
		for(i = 1; i < rec.y; i++)
			XDrawLine(dpy, icon_win, Desks[tmp].DashedGC,
				0, i * rec.height, icon.width, i * rec.height);
	}

	if (HilightDesks) {
		TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
			if (fp->disabled ||
			   fp->m->virtual_scr.CurrentDesk != tmp ||
			   (monitor_to_track != NULL &&
			   strcmp(fp->m->si->name, monitor_to_track) != 0))
				continue;
			rec = set_vp_size_and_loc(fp, true);
			if (HilightPixmap) {
				XCopyArea(dpy, HilightPixmap->picture, icon_win,
					Scr.NormalGC, 0, 0, rec.width, rec.height,
					rec.x, rec.y);
			} else {
				XFillRectangle(dpy, icon_win, Desks[tmp].HiliteGC,
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
  int vx, vy;
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL) {
    fprintf(stderr, "%s: couldn't find monitor (fp is NULL)\n", __func__);
    return;
  }

  /* Determine which monitor occupied the clicked region. */
  vx = (desk_w == 0) ? 0 : Event->xbutton.x * fp->virtual_scr.VWidth / desk_w;
  vy = (desk_h == 0) ? 0 : Event->xbutton.y * fp->virtual_scr.VHeight / desk_h;
  fp = fpmonitor_from_xy(vx, vy, true);
  if (fp == NULL)
    return;

  if (fp->m->virtual_scr.CurrentDesk != (Desk + desk1))
  {
    /* patch to let mouse button 3 change desks and do not cling to a page */
    fp->m->virtual_scr.Vx = vx;
    fp->m->virtual_scr.Vy = vy;
    vx /= fpmonitor_get_all_widths();
    vy /= fpmonitor_get_all_heights();
    snprintf(command, sizeof(command), "GotoDeskAndPage screen %s %d %d %d",
		fp->m->si->name, Desk + desk1, vx, vy);
    SendText(fd, command, 0);

  }
  else
  {
    /* Fix for buggy XFree86 servers that report button release events
     * incorrectly when moving fast. Not perfect, but should at least prevent
     * that we get a random page. */
    if (vx < 0)
      vx = 0;
    if (vy < 0)
      vy = 0;
    if (vx > fp->virtual_scr.VxMax)
      vx = fp->virtual_scr.VxMax;
    if (vy > fp->virtual_scr.VyMax)
      vy = fp->virtual_scr.VyMax;
    vx /= fpmonitor_get_all_widths();
    vy /= fpmonitor_get_all_heights();

    snprintf(command, sizeof(command), "GotoPage screen %s %d %d", fp->m->si->name, vx, vy);
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
  fp = fpmonitor_from_xy(vx, vy, true);
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


void AddNewWindow(PagerWindow *t)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	rectangle rec;
	int i;
	struct fpmonitor *fp = fpmonitor_this(t->m);

	if (fp == NULL)
		return;

	i = t->desk - desk1;
	rec = CalcGeom(t, false);
	t->pager_view = rec;
	valuemask = CWBackPixel | CWEventMask;
	attributes.background_pixel = t->back;
	attributes.event_mask = ExposureMask;

	if (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize))
	{
		rec.x = -32768;
		rec.y = -32768;
	}

	/* ric@giccs.georgetown.edu -- added Enter and Leave events for
	   popping up balloon window */
	attributes.event_mask =
		ExposureMask | EnterWindowMask | LeaveWindowMask;

	if ((i >= 0) && (i < ndesks))
	{
		if (monitor_to_track != NULL && (t->m != NULL && t->m != fp->m))
			return;

		t->PagerView = XCreateWindow(
			dpy,Desks[i].w, rec.x, rec.y, rec.width, rec.height,
			0, CopyFromParent, InputOutput, CopyFromParent,
			valuemask, &attributes);
		if (windowcolorset > -1)
		{
			SetWindowBackground(
				dpy, t->PagerView, rec.width, rec.height,
				&Colorset[windowcolorset], Pdepth,
				Scr.NormalGC, True);
		}
		if (!UseSkipList || !DO_SKIP_WINDOW_LIST(t))
		{
			if (IS_ICONIFIED(t))
			{
				XMoveResizeWindow(
					dpy, t->PagerView, -32768, -32768, 1,
					1);
			}
			XMapRaised(dpy, t->PagerView);
		}
	}
	else
	{
		t->PagerView = None;
	}

	rec = CalcGeom(t, true);
	t->icon_view = rec;
	if((fp->m->virtual_scr.CurrentDesk != t->desk) ||
		(HideSmallWindows && (rec.width == MinSize ||
		rec.height == MinSize)))
	{
		rec.x = -32768;
		rec.y = -32768;
	}
	t->IconView = XCreateWindow(
		dpy, icon_win, rec.x, rec.y, rec.width, rec.height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		valuemask, &attributes);
	if (windowcolorset > -1)
	{
		SetWindowBackground(
			dpy, t->IconView, rec.width, rec.height,
			&Colorset[windowcolorset], Pdepth, Scr.NormalGC, True);
	}
	if(fp->m->virtual_scr.CurrentDesk == t->desk)
	{
		XGrabButton(
			dpy, 2, AnyModifier, t->IconView, True,
			ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);
	}
	if (!UseSkipList || !DO_SKIP_WINDOW_LIST(t))
	{
		if (IS_ICONIFIED(t))
		{
			XMoveResizeWindow(
				dpy, t->IconView, -32768, -32768, 1,
				1);
		}
		XMapRaised(dpy, t->IconView);
		t->myflags.is_mapped = 1;
	}
	else
	{
		t->myflags.is_mapped = 0;
	}
	Hilight(t, False);

	return;
}


void ChangeDeskForWindow(PagerWindow *t,long newdesk)
{
  rectangle rec;
  int i;
  Bool size_changed = False;
  struct fpmonitor *fp = fpmonitor_this(t->m);

  if (fp == NULL)
	return;

  i = newdesk - desk1;
  if(t->PagerView == None)
  {
    t->desk = newdesk;
    XDestroyWindow(dpy,t->IconView);
    AddNewWindow( t);
    return;
  }

  rec = CalcGeom(t, false);
  size_changed = (t->pager_view.width != rec.width ||
	  t->pager_view.height != rec.height);
  t->pager_view = rec;

  if (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize))
  {
    rec.x = -32768;
    rec.y = -32768;
  }

  if ((i >= 0) && (i < ndesks))
  {
    int cset;

    XReparentWindow(dpy, t->PagerView, Desks[i].w, rec.x, rec.y);
    if (size_changed)
      XResizeWindow(dpy, t->PagerView, rec.width, rec.height);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->PagerView, t->pager_view.width, t->pager_view.height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
  else
  {
    XDestroyWindow(dpy,t->PagerView);
    t->PagerView = None;
  }
  t->desk = i+desk1;

  rec = CalcGeom(t, true);
  size_changed = (t->icon_view.width != rec.width ||
	  t->icon_view.height != rec.height);
  t->icon_view = rec;
  if (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize))
  {
    rec.x = -32768;
    rec.y = -32768;
  }
  if(fp->m->virtual_scr.CurrentDesk != t->desk)
    XMoveResizeWindow(dpy,t->IconView,-32768,-32768,rec.width,rec.height);
  else
  {
    int cset;

    XMoveResizeWindow(dpy,t->IconView,rec.x,rec.y,rec.width,rec.height);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->IconView, t->icon_view.width, t->icon_view.height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
}

void MoveResizePagerView(PagerWindow *t, Bool do_force_redraw)
{
  rectangle rec;
  Bool size_changed;
  Bool position_changed;
  struct fpmonitor *fp = fpmonitor_this(t->m);

  if (fp == NULL)
	return;

  if (UseSkipList && DO_SKIP_WINDOW_LIST(t) && t->myflags.is_mapped)
  {
    if (t->PagerView)
      XUnmapWindow(dpy, t->PagerView);
    if (t->IconView)
      XUnmapWindow(dpy, t->IconView);
    t->myflags.is_mapped = 0;
  }
  else if (UseSkipList && !DO_SKIP_WINDOW_LIST(t) && !t->myflags.is_mapped)
  {
    if (t->PagerView)
      XMapRaised(dpy, t->PagerView);
    if (t->IconView)
      XMapRaised(dpy, t->IconView);
    t->myflags.is_mapped = 1;
  }

  rec = CalcGeom(t, false);
  position_changed = (t->pager_view.x != rec.x || t->pager_view.y != rec.y);
  size_changed = (t->pager_view.width != rec.width ||
	  t->pager_view.height != rec.height);
  t->pager_view = rec;
  if (t->PagerView != None)
  {
    if (size_changed || position_changed || do_force_redraw)
    {
      int cset;

      if (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize))
      {
        rec.x = -32768;
        rec.y = -32768;
      }
      XMoveResizeWindow(dpy, t->PagerView, rec.x, rec.y, rec.width, rec.height);
      cset = (t != FocusWin) ? windowcolorset : activecolorset;
      if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
      {
	SetWindowBackground(
	  dpy, t->PagerView, t->pager_view.width, t->pager_view.height,
	  &Colorset[cset], Pdepth, Scr.NormalGC, True);
      }
    }
  }
  else if (t->desk >= desk1 && t->desk <= desk2)
  {
    XDestroyWindow(dpy, t->IconView);
    AddNewWindow(t);
    return;
  }

  rec = CalcGeom(t, true);
  position_changed = (t->icon_view.x != rec.x || t->icon_view.y != rec.y);
  size_changed = (t->icon_view.width != rec.width ||
	  t->icon_view.height != rec.height);
  t->icon_view = rec;
  if (fp->m->virtual_scr.CurrentDesk == t->desk)
  {
    int cset;

    if (HideSmallWindows && (rec.width == MinSize || rec.height == MinSize))
    {
      rec.x = -32768;
      rec.y = -32768;
    }
    XMoveResizeWindow(dpy, t->IconView, rec.x, rec.y, rec.width, rec.height);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->IconView, t->icon_view.width, t->icon_view.height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
  else
  {
    XMoveResizeWindow(dpy, t->IconView, -32768, -32768, rec.width, rec.height);
  }
}


void MoveStickyWindow(Bool is_new_page, Bool is_new_desk)
{
	PagerWindow *t;
	struct fpmonitor *fp;

	for (t = Start; t != NULL; t = t->next)
	{
		fp = fpmonitor_this(t->m);
		if (fp == NULL)
			continue;
		if (
			is_new_desk && t->desk != fp->m->virtual_scr.CurrentDesk &&
			((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t)) ||
			 IS_STICKY_ACROSS_DESKS(t)))
		{
			ChangeDeskForWindow(t, fp->m->virtual_scr.CurrentDesk);
		}
		else if (
			is_new_page &&
			((IS_ICONIFIED(t) &&
			  IS_ICON_STICKY_ACROSS_PAGES(t)) ||
			 IS_STICKY_ACROSS_PAGES(t)))
		{
			MoveResizePagerView(t, True);
		}
	}
}

void Hilight(PagerWindow *t, int on)
{
  if(!t)
    return;

  if (monitor_to_track != NULL && strcmp(t->m->si->name, monitor_to_track) != 0)
	  return;

  if(Pdepth < 2)
  {
    if(on)
    {
      if(t->PagerView != None)
	XSetWindowBackgroundPixmap(dpy,t->PagerView,Scr.gray_pixmap);
      XSetWindowBackgroundPixmap(dpy,t->IconView,Scr.gray_pixmap);
    }
    else
    {
      if(IS_STICKY_ACROSS_DESKS(t) || IS_STICKY_ACROSS_PAGES(t))
      {
	if(t->PagerView != None)
	  XSetWindowBackgroundPixmap(dpy,t->PagerView,
				     Scr.sticky_gray_pixmap);
	XSetWindowBackgroundPixmap(dpy,t->IconView,
				   Scr.sticky_gray_pixmap);
      }
      else
      {
	if(t->PagerView != None)
	  XSetWindowBackgroundPixmap(dpy,t->PagerView,
				     Scr.light_gray_pixmap);
	XSetWindowBackgroundPixmap(dpy,t->IconView,
				   Scr.light_gray_pixmap);
      }
    }
    if(t->PagerView != None)
      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
    XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
  }
  else
  {
    int cset;
    Pixel pix;

    cset = (on) ? activecolorset : windowcolorset;
    pix = (on) ? focus_pix : t->back;
    if (cset < 0)
    {
      if (t->PagerView != None)
      {
	XSetWindowBackground(dpy,t->PagerView,pix);
	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
      }
      XSetWindowBackground(dpy,t->IconView,pix);
      XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
    }
    else
    {
      if (t->PagerView != None)
      {
	SetWindowBackground(
	  dpy, t->PagerView, t->pager_view.width, t->pager_view.height,
	  &Colorset[cset], Pdepth, Scr.NormalGC, True);
      }
      SetWindowBackground(
	dpy, t->IconView, t->icon_view.width, t->icon_view.height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
}

/* Use Desk == -1 to scroll the icon window */
void Scroll(int x, int y, int Desk, Bool do_scroll_icon)
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
	if (monitor_mode == MONITOR_TRACKING_G) {
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
		do_scroll(sx, sy, True, False);

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
	char command[100];
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
	int do_switch_desk_later = 0;
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

	XReparentWindow(dpy, t->PagerView, Scr.Pager_w, rec.x, rec.y);
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
					      Scr.Pager_w, Event->xmotion.x,
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
					      Scr.Pager_w, Event->xbutton.x,
					      Event->xbutton.y, &x, &y,
					      &dumwin);
			XMoveWindow(dpy, t->PagerView, x - rec.x, y - rec.y);
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
			XDestroyWindow(dpy, t->PagerView);
			t->PagerView = None;
		}
		if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask) == False) {
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
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
	} else {
		column = x / (desk_w + 1);
		if (column >= Columns)
			column = Columns - 1;
		if (column < 0)
			column = 0;

		row = y / (desk_h + label_h + 1);
		if (row >= Rows)
			row = Rows - 1;
		if (row < 0)
			row = 0;

		NewDesk = column + row * Columns;
		while (NewDesk < 0) {
			NewDesk += Columns;
			if (NewDesk >= ndesks)
				NewDesk = 0;
		}
		while (NewDesk >= ndesks) {
			NewDesk -= Columns;
			if (NewDesk < 0)
				NewDesk = ndesks - 1;
		}

		XTranslateCoordinates(dpy, Scr.Pager_w, Desks[NewDesk].w,
				      x - rec.x, y - rec.y, &rec.x, &rec.y,
				      &dumwin);

		fp = fpmonitor_from_xy(rec.x * fp->virtual_scr.VWidth / desk_w,
			rec.y * fp->virtual_scr.VHeight / desk_h, false);
		pagerrec_to_fvwm(&rec, false, fp);

		if (NewDesk + desk1 != t->desk &&
			((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t))
			|| (IS_STICKY_ACROSS_DESKS(t))))
		{
			NewDesk = fp->m->virtual_scr.CurrentDesk - desk1;
			if (t->desk != fp->m->virtual_scr.CurrentDesk)
				ChangeDeskForWindow(t,
					fp->m->virtual_scr.CurrentDesk);
		} else {
			do_switch_desk_later = 1;
		}

		XReparentWindow(dpy, t->PagerView,
				Desks[NewDesk].w, rec.x, rec.y);
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		if (moved) {
			char buf[64];
			/* XXX: Note the use of ewmhiwa to disable
			 * clipping the coordinates to the wrong area!
			 */
			snprintf(buf, sizeof(buf),
				 "Silent Move v+%dp v+%dp ewmhiwa",
				 rec.x, rec.y);
			SendText(fd, buf, t->w);
			XSync(dpy,0);
			t->m = fp->m;
		} else {
			MoveResizePagerView(t, True);
		}
		SendText(fd, "Silent Raise", t->w);

		if (do_switch_desk_later) {
			snprintf(command, sizeof(command),
				 "Silent MoveToDesk 0 %d",
				 NewDesk + desk1);
			SendText(fd, command, t->w);
			t->desk = NewDesk + desk1;
		}

		if (fp->m->virtual_scr.CurrentDesk == t->desk) {
			XSync(dpy,0);
			usleep(5000);
			XSync(dpy,0);

			SendText(fd, "Silent FlipFocus NoWarp", t->w);
		}
	}

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
  extern Bool error_occured;
  error_occured = True;
  return 0;
#else
  /* really should just exit here... */
  /* domivogt (07-mar-1999): No, not really. See comment above. */
  PrintXErrorAndCoredump(dpy, event, MyName);
  return 0;
#endif /* 1 */
}

static void draw_window_border(PagerWindow *t, Window w, int width, int height)
{
  if (w != None)
  {
    if (!WindowBorders3d || windowcolorset < 0 || activecolorset < 0)
    {
      XSetForeground(dpy, Scr.NormalGC, Scr.black);
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, height - 1,
	Scr.NormalGC, Scr.NormalGC, WindowBorderWidth);
    }
    else if (t == FocusWin)
    {
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, height - 1,
	Scr.ahGC, Scr.asGC, WindowBorderWidth);
    }
    else
    {
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, height - 1,
	Scr.whGC, Scr.wsGC, WindowBorderWidth);
    }
  }
}

void BorderWindow(PagerWindow *t)
{
  draw_window_border(
    t, t->PagerView, t->pager_view.width, t->pager_view.height);
}

void BorderIconWindow(PagerWindow *t)
{
  draw_window_border(
    t, t->IconView, t->icon_view.width, t->icon_view.height);
}

/* draw the window label with simple greedy wrapping */
static void label_window_wrap(PagerWindow *t)
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
      if (width > t->pager_view.width - cur_width - space_width - 2*label_border)
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

        if (width > t->pager_view.width - cur_width - 2*label_border && cur != next)
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

static void do_label_window(PagerWindow *t, Window w)
{
  int cs;

  if (t == FocusWin)
  {
    XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);
    cs =  activecolorset;
  }
  else
  {
    XSetForeground(dpy, Scr.NormalGC, t->text);
    cs = windowcolorset;
  }

  if (FwindowFont == NULL)
  {
    return;
  }
  if (MiniIcons && t->mini_icon.picture && (t->PagerView != None))
  {
    /* will draw picture instead... */
    return;
  }
  if (t->icon_name == NULL)
  {
    return;
  }
  free(t->window_label);
  t->window_label = GetBalloonLabel(t, WindowLabelFormat);
  if (w != None)
  {
    if (FftSupport)
      XClearWindow(dpy, w);
    FwinString->win = w;
    FwinString->gc = Scr.NormalGC;
    FwinString->flags.has_colorset = False;
    if (cs >= 0)
    {
	    FwinString->colorset = &Colorset[cs];
	    FwinString->flags.has_colorset = True;
    }
    FwinString->x = label_border;
    FwinString->y = FwindowFont->ascent+2;

    label_window_wrap(t);
  }
}

void LabelWindow(PagerWindow *t)
{
  do_label_window(t, t->PagerView);
}

void LabelIconWindow(PagerWindow *t)
{
  do_label_window(t, t->IconView);
}

static void do_picture_window(
	PagerWindow *t, Window w, int width, int height)
{
	int iconX;
	int iconY;
	int src_x = 0;
	int src_y = 0;
	int dest_w, dest_h;
	int cs;
	FvwmRenderAttributes fra;

	if (MiniIcons)
	{
		if (t->mini_icon.picture && w != None)
		{
			dest_w = t->mini_icon.width;
			dest_h = t->mini_icon.height;
			if (width > t->mini_icon.width)
			{
				iconX = (width - t->mini_icon.width) / 2;
			}
			else if (width < t->mini_icon.width)
			{
				iconX = 0;
				src_x = (t->mini_icon.width - width) / 2;
				dest_w = width;
			}
			else
			{
				iconX = 0;
			}
			if (height > t->mini_icon.height)
			{
				iconY = (height - t->mini_icon.height) / 2;
			}
			else if (height < t->mini_icon.height)
			{
				iconY = 0;
				src_y = (t->mini_icon.height - height) / 2;
				dest_h = height;
			}
			else
			{
				iconY = 0;
			}
			fra.mask = FRAM_DEST_IS_A_WINDOW;
			if (t == FocusWin)
			{
				cs =  activecolorset;
			}
			else
			{
				cs = windowcolorset;
			}
			if (t->mini_icon.alpha != None ||
			    (cs >= 0 &&
			     Colorset[cs].icon_alpha_percent < 100))
			{
				XClearArea(dpy, w, iconX, iconY,
					   t->mini_icon.width,
					   t->mini_icon.height, False);
			}
			if (cs >= 0)
			{
				fra.mask |= FRAM_HAVE_ICON_CSET;
				fra.colorset = &Colorset[cs];
			}
			PGraphicsRenderPicture(
				dpy, w, &t->mini_icon, &fra, w,
				Scr.MiniIconGC, None, None, src_x, src_y,
				t->mini_icon.width - src_x,
				t->mini_icon.height - src_y,
				iconX, iconY, dest_w, dest_h, False);
		}
	}
}

void PictureWindow (PagerWindow *t)
{
  do_picture_window(t, t->PagerView, t->pager_view.width, t->pager_view.height);
}

void PictureIconWindow (PagerWindow *t)
{
  do_picture_window(t, t->IconView, t->icon_view.width, t->icon_view.height);
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

	if (KeepMoving)	{
		if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask) == False) {
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		XUngrabPointer(dpy, CurrentTime);
		XSync(dpy, 0);
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent Move Pointer", t->w);
	} else {
		rec.x = x - rec.x;
		rec.y = y - rec.y;

		if (moved) {
			char buf[64];
			fp = fpmonitor_from_xy(
				rec.x * fp->virtual_scr.VWidth / icon.width,
				rec.y * fp->virtual_scr.VHeight / icon.height,
				false);
			pagerrec_to_fvwm(&rec, true, fp);
			t->m = fp->m;
			snprintf(buf, sizeof(buf),
				 "Silent Move v+%dp v+%dp ewmhiwa",
				 rec.x, rec.y);
			SendText(fd, buf, t->w);
			XSync(dpy, 0);
		} else {
			MoveResizePagerView(t, True);
		}
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent FlipFocus NoWarp", t->w);
	}
	if (is_transient)
		ExitPager(); /* does not return */
}


void setup_balloon_window(int i)
{
	XSetWindowAttributes xswa;
	unsigned long valuemask;

	if (!ShowBalloons && Scr.balloon_w == None)
	{
		return;
	}
	XUnmapWindow(dpy, Scr.balloon_w);
	if (Desks[i].ballooncolorset < 0)
	{
		xswa.border_pixel = (!BalloonBorderColor) ?
			fore_pix : GetColor(BalloonBorderColor);
		xswa.background_pixel =
			(!BalloonBack) ? back_pix : GetColor(BalloonBack);
		valuemask = CWBackPixel | CWBorderPixel;
	}
	else
	{
		xswa.border_pixel = Colorset[Desks[i].ballooncolorset].fg;
		xswa.background_pixel = Colorset[Desks[i].ballooncolorset].bg;
		if (Colorset[Desks[i].ballooncolorset].pixmap)
		{
			/* set later */
			xswa.background_pixmap = None;
			valuemask = CWBackPixmap | CWBorderPixel;;
		}
		else
		{
			valuemask = CWBackPixel | CWBorderPixel;
		}
	}
	XChangeWindowAttributes(dpy, Scr.balloon_w, valuemask, &xswa);

	return;
}

void setup_balloon_gc(int i, PagerWindow *t)
{
	XGCValues xgcv;
	unsigned long valuemask;

	valuemask = GCForeground;
	if (Desks[i].ballooncolorset < 0)
	{
		xgcv.foreground =
			(!BalloonFore) ? fore_pix : GetColor(BalloonFore);
		if (BalloonFore == NULL)
		{
			XSetForeground(dpy, Scr.balloon_gc, t->text);
		}
	}
	else
	{
		xgcv.foreground = Colorset[Desks[i].ballooncolorset].fg;
	}
	if (Desks[i].balloon.Ffont->font != NULL)
	{
		xgcv.font = Desks[i].balloon.Ffont->font->fid;
		valuemask |= GCFont;
	}
	XChangeGC(dpy, Scr.balloon_gc, valuemask, &xgcv);

	return;
}

/* Just maps window ... draw stuff in it later after Expose event
   -- ric@giccs.georgetown.edu */
void MapBalloonWindow(PagerWindow *t, Bool is_icon_view)
{
	extern int BalloonYOffset;
	Window view, dummy;
	int view_width, view_height;
	int x, y;
	rectangle new_g;
	int i;

	if (!is_icon_view)
	{
		view = t->PagerView;
		view_width = t->pager_view.width;
		view_height = t->pager_view.height;
	}
	else
	{
		view = t->IconView;
		view_width = t->icon_view.width;
		view_height = t->icon_view.height;
	}
	BalloonView = view;
	/* associate balloon with its pager window */
	if (fAlwaysCurrentDesk)
	{
		i = 0;
	}
	else
	{
		i = t->desk - desk1;
	}

	if (i < 0)
		return;

	Scr.balloon_desk = i;
	/* get the label for this balloon */
	if (Scr.balloon_label)
	{
		free(Scr.balloon_label);
	}
	Scr.balloon_label = GetBalloonLabel(t, BalloonFormatString);
#if 0
	if (Scr.balloon_label == NULL)
	{
		/* dont draw empty labels */
		return;
	}
#endif
	setup_balloon_window(i);
	setup_balloon_gc(i, t);
	/* calculate window width to accommodate string */
	new_g.height = Desks[i].balloon.height;
	new_g.width = 4;
	if (Scr.balloon_label)
	{
		new_g.width += FlocaleTextWidth(
			Desks[i].balloon.Ffont,	 Scr.balloon_label,
			strlen(Scr.balloon_label));
	}
	/* get x and y coords relative to pager window */
	x = (view_width / 2) - (new_g.width / 2) - BalloonBorderWidth;
	if (BalloonYOffset > 0)
	{
		y = view_height + BalloonYOffset - 1;
	}
	else
	{
		y = BalloonYOffset - new_g.height + 1 -
			(2 * BalloonBorderWidth);
	}
	/* balloon is a top-level window, therefore need to
	   translate pager window coords to root window coords */
	XTranslateCoordinates(
		dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y,
		&dummy);
	/* make sure balloon doesn't go off screen
	 * (actually 2 pixels from edge rather than 0 just to be pretty :-) */
	/* too close to top ... make yoffset +ve */
	if ( new_g.y < 2 )
	{
		y = - BalloonYOffset - 1 + view_height;
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to bottom ... make yoffset -ve */
	else if ( new_g.y + new_g.height >
		  fpmonitor_get_all_heights() - (2 * BalloonBorderWidth) - 2 )
	{
		y = - BalloonYOffset + 1 - new_g.height -
			(2 * BalloonBorderWidth);
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to left */
	if ( new_g.x < 2 )
	{
		new_g.x = 2;
	}
	/* too close to right */
	else if (new_g.x + new_g.width >
		 fpmonitor_get_all_widths() - (2 * BalloonBorderWidth) - 2 )
	{
		new_g.x = fpmonitor_get_all_widths() - new_g.width -
			(2 * BalloonBorderWidth) - 2;
	}
	/* make changes to window */
	XMoveResizeWindow(
		dpy, Scr.balloon_w, new_g.x, new_g.y, new_g.width,
		new_g.height);
	/* if background not set in config make it match pager window */
	if ( BalloonBack == NULL )
	{
		XSetWindowBackground(dpy, Scr.balloon_w, t->back);
	}
	if (Desks[i].ballooncolorset > -1)
	{
		SetWindowBackground(
			dpy, Scr.balloon_w, new_g.width, new_g.height,
			&Colorset[Desks[i].ballooncolorset], Pdepth,
			Scr.NormalGC, True);
		XSetWindowBorder(
			dpy, Scr.balloon_w,
			Colorset[Desks[i].ballooncolorset].fg);
	}
	XMapRaised(dpy, Scr.balloon_w);

	return;
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
char *GetBalloonLabel(const PagerWindow *pw,const char *fmt)
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
  BalloonView = None;
}


/* Draws string in balloon window -- call after it's received Expose event
   -- ric@giccs.georgetown.edu */
void DrawInBalloonWindow (int i)
{
	FwinString->str = Scr.balloon_label;
	FwinString->win = Scr.balloon_w;
	FwinString->gc = Scr.balloon_gc;
	if (Desks[i].ballooncolorset >= 0)
	{
		FwinString->colorset = &Colorset[Desks[i].ballooncolorset];
		FwinString->flags.has_colorset = True;
	}
	else
	{
		FwinString->flags.has_colorset = False;
	}
	FwinString->x = 2;
	FwinString->y = Desks[i].balloon.Ffont->ascent;
	FlocaleDrawString(dpy, Desks[i].balloon.Ffont, FwinString, 0);

	return;
}

static void set_window_colorset_background(
  PagerWindow *t, colorset_t *csetp)
{
  if (t->PagerView != None)
  {
    SetWindowBackground(
      dpy, t->PagerView, t->pager_view.width, t->pager_view.height,
      csetp, Pdepth, Scr.NormalGC, True);
  }
  SetWindowBackground(
    dpy, t->IconView, t->icon_view.width, t->icon_view.height,
    csetp, Pdepth, Scr.NormalGC, True);

  return;
}

/* should be in sync with  draw_desk_background */
void change_colorset(int colorset)
{
  int i;
  PagerWindow *t;
  struct fpmonitor *fp = fpmonitor_this(NULL);
  struct fpmonitor *m;

  /* just in case */
  if (colorset < 0)
    return;

  XSetWindowBackgroundPixmap(dpy, Scr.Pager_w, default_pixmap);
  for(i=0;i<ndesks;i++)
  {
    if (Desks[i].highcolorset == colorset)
    {
      XSetForeground(dpy, Desks[i].HiliteGC,Colorset[colorset].bg);
      XSetForeground(dpy, Desks[i].rvGC, Colorset[colorset].fg);
      if (HilightDesks)
      {
	if (uselabel && (i == fp->m->virtual_scr.CurrentDesk))
	{
	  SetWindowBackground(
	    dpy, Desks[i].title_w, desk_w, desk_h, &Colorset[colorset], Pdepth,
	    Scr.NormalGC, True);
	}
	TAILQ_FOREACH(m, &fp_monitor_q, entry) {
	  SetWindowBackground(
	    dpy, m->CPagerWin[i], 0, 0, &Colorset[colorset], Pdepth,
	    Scr.NormalGC, True);
	  XLowerWindow(dpy, m->CPagerWin[i]);
	}
      }
    }

    if (Desks[i].colorset == colorset)
    {
      XSetWindowBorder(dpy, Desks[i].title_w, Colorset[colorset].fg);
      XSetWindowBorder(dpy, Desks[i].w, Colorset[colorset].fg);
      XSetForeground(dpy, Desks[i].NormalGC,Colorset[colorset].fg);
      XSetForeground(dpy, Desks[i].DashedGC,Colorset[colorset].fg);
      if (uselabel)
      {
	SetWindowBackground(
	  dpy, Desks[i].title_w, desk_w, desk_h + label_h, &Colorset[colorset],
	  Pdepth, Scr.NormalGC, True);
      }
      if (label_h != 0 && uselabel && !LabelsBelow &&
	  !CSET_IS_TRANSPARENT_PR(Desks[i].colorset))
      {
	SetWindowBackgroundWithOffset(
	  dpy, Desks[i].w, 0, -label_h, desk_w, desk_h + label_h,
	  &Colorset[colorset], Pdepth, Scr.NormalGC, True);
      }
      else
      {
	SetWindowBackground(
	  dpy, Desks[i].w, desk_w, desk_h + label_h,
	  &Colorset[Desks[i].colorset], Pdepth, Scr.NormalGC, True);
      }
      update_pr_transparent_subwindows(i);
    }
    else if (Desks[i].highcolorset == colorset && uselabel)
    {
      SetWindowBackground(
	dpy, Desks[i].title_w, 0, 0, &Colorset[Desks[i].colorset], Pdepth,
	Scr.NormalGC, True);
    }
  }
  if (ShowBalloons && Desks[Scr.balloon_desk].ballooncolorset == colorset)
  {
    XSetWindowBackground(dpy, Scr.balloon_w, Colorset[colorset].fg);
    XSetForeground(dpy,Scr.balloon_gc, Colorset[colorset].fg);
    XClearArea(dpy, Scr.balloon_w, 0, 0, 0, 0, True);
  }

  if (windowcolorset == colorset)
  {
    colorset_t *csetp = &Colorset[colorset];

    XSetForeground(dpy, Scr.whGC, csetp->hilite);
    XSetForeground(dpy, Scr.wsGC, csetp->shadow);
    win_back_pix = csetp->bg;
    win_fore_pix = csetp->fg;
    win_pix_set = True;
    for (t = Start; t != NULL; t = t->next)
    {
      t->text = Colorset[colorset].fg;
      if (t != FocusWin)
      {
	set_window_colorset_background(t, csetp);
      }
    }
  }
  if (activecolorset == colorset)
  {
    colorset_t *csetp = &Colorset[colorset];

    focus_fore_pix = csetp->fg;
    XSetForeground(dpy, Scr.ahGC, csetp->hilite);
    XSetForeground(dpy, Scr.asGC, csetp->shadow);
    win_hi_back_pix = csetp->bg;
    win_hi_fore_pix = csetp->fg;
    win_hi_pix_set = True;
    if (FocusWin)
    {
      set_window_colorset_background(FocusWin, csetp);
    }
  }

  return;
}