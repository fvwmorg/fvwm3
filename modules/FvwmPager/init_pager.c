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
#include "libs/Parse.h"
#include "libs/Graphics.h"
#include "libs/Picture.h"
#include "libs/ColorUtils.h"
#include "libs/FShape.h"
#include "libs/FEvent.h"
#include "libs/FGettext.h"
#include "fvwm/fvwm.h"
#include "FvwmPager.h"

static Pixmap default_pixmap;
static bool usposition = false;
static bool xneg = false;
static bool yneg = false;
static bool icon_usposition = false;
static bool icon_xneg = false;
static bool icon_yneg = false;

static void initialize_viz_pager(void);
static void initialize_mouse_bindings(void);
static void initialize_desks_and_monitors(void);
static void initialize_colorsets(void);
static int fvwm_error_handler(Display *dpy, XErrorEvent *event);
static void initialize_fonts(void);
static void initialize_transient(void);
static void initialise_common_pager_fragments(void);
static void initialize_pager_size(void);
static void initialize_balloon_window(void);
static void initialize_desk_windows(int desk);
static void initialize_pager_window(void);
static void set_desk_style_cset(char *arg1, char *arg2, void *offset_style);
static void set_desk_style_pixel(char *arg1, char *arg2, void *offset_style);
static void set_desk_style_bool(char *arg1, char *arg2, void *offset_style);
static void parse_options(void);

void init_fvwm_pager(void)
{
	int i;
	DeskStyle *style;
	struct fpmonitor *fp;

	/* make a temp window for any pixmaps, deleted later */
	initialize_viz_pager();

	/* Run initializations */
	initialize_mouse_bindings();
	initialize_fonts();
	initialize_desks_and_monitors();
	parse_options();
	initialize_colorsets();
	initialise_common_pager_fragments();
	initialize_pager_size();
	if (is_transient)
		initialize_transient();
	initialize_pager_window();

	/* Update styles with user configurations. */
	TAILQ_FOREACH(style, &desk_style_q, entry) {
		update_desk_style_gcs(style);
	}
	/* Initialize any non configured desks. */
	for (i = 0; i < ndesks; i++) {
		style = FindDeskStyle(i);
	}

	/* After DeskStyles are initialized, initialize desk windows. */
	for (i = 0; i < ndesks; i++) {
		initialize_desk_windows(i);
	}

	/* Hilight monitor windows. */
	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		initialize_fpmonitor_windows(fp);
	}

	initialize_balloon_window();

	/* Create a list of all windows, by requesting a list of all
	 * windows, and wait for ConfigureWindow packets.
	 */
	SendInfo(fd, "Send_WindowList", 0);

	/* Initialization done! */
	XMapRaised(dpy, Scr.pager_w);
}


/*  Procedure:
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
	}
	Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.pager_w, 0, NULL);

	xgcv.plane_mask = AllPlanes;
	Scr.MiniIconGC = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCPlaneMask, &xgcv);

	/* Transparent background are only allowed when the depth matched the
	 * root */
	if (Pdepth == DefaultDepth(dpy, Scr.screen))
		default_pixmap = ParentRelative;
}

void initialize_mouse_bindings(void)
{
	int i;

	/* Bindings are stored as 'button - 1'. */
	mouse_action[0] = P_MOUSE_MOVE;
	mouse_action[1] = P_MOUSE_WIN_MOVE;
	mouse_action[2] = P_MOUSE_SCROLL;
	mouse_action[3] = P_MOUSE_NOP;
	mouse_action[4] = P_MOUSE_NOP;

	mouse_cmd = fxmalloc((P_MOUSE_BUTTONS) * sizeof(char *));
	for (i = 0; i < P_MOUSE_BUTTONS; i++) {
		mouse_cmd[i] = NULL;
	}
	mouse_cmd[1] = fxstrdup("FlipFocus NoWarp");
}

void initialize_desks_and_monitors(void)
{
	DeskStyle *default_style;
	struct monitor *mon;
	int i;

	TAILQ_INIT(&fp_monitor_q);
	TAILQ_INIT(&desk_style_q);

	RB_FOREACH(mon, monitors, &monitor_q)
		(void)fpmonitor_new(mon);

	/* Initialize default DeskStyle, stored on desk -1. */
	default_style = fxcalloc(1, sizeof(DeskStyle));
	default_style->desk = -1;
	default_style->label = fxstrdup("-");

	default_style->use_label_pixmap = true;
	default_style->cs = -1;
	default_style->hi_cs = -1;
	default_style->win_cs = -1;
	default_style->focus_cs = -1;
	default_style->balloon_cs = -1;
	default_style->fg = GetSimpleColor("black");
	default_style->bg = GetSimpleColor("white");
	default_style->hi_fg = GetSimpleColor("black");
	default_style->hi_bg = GetSimpleColor("grey");
	/* Use ULONG_MAX for default undefined value because "black" is 0. */
	default_style->win_fg = ULONG_MAX;
	default_style->win_bg = ULONG_MAX;
	default_style->focus_fg = ULONG_MAX;
	default_style->focus_bg = ULONG_MAX;
	default_style->balloon_fg = ULONG_MAX;
	default_style->balloon_bg = ULONG_MAX;
	default_style->balloon_border = ULONG_MAX;
	default_style->bgPixmap = NULL;
	default_style->hiPixmap = NULL;
	TAILQ_INSERT_TAIL(&desk_style_q, default_style, entry);

	Desks = fxcalloc(1, ndesks*sizeof(DeskInfo));
	for(i = 0; i < ndesks; i++) {
		Desks[i].style = FindDeskStyle(i);
		Desks[i].fp = fpmonitor_from_desk(i + desk1);
	}
}

void initialize_colorset(DeskStyle *style)
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
		style->balloon_bg = Colorset[style->balloon_cs].bg;
	}
}

void initialize_colorsets(void)
{
	DeskStyle *style;

	TAILQ_FOREACH(style, &desk_style_q, entry) {
		initialize_colorset(style);
	}
}

int fvwm_error_handler(Display *dpy, XErrorEvent *event)
{
	/* Ignore errors from windows dying unexpectedly. */
	error_occured = true;
	return 0;
}

void initialize_fonts(void)
{
	/* Initialize fonts. */
	FlocaleInit(LC_CTYPE, "", "", "FvwmPager");

	/* Initialize translations. */
	FGettextInit("fvwm3", LOCALEDIR, "fvwm3");

	/* load a default font. */
	Scr.Ffont = FlocaleLoadFont(dpy, NULL, MyName);

	/* init our Flocale window string */
	FlocaleAllocateWinString(&FwinString);

	WindowLabelFormat = fxstrdup("%i");
}

void initialize_transient(void)
{
	int i;
	Window junk_w;
	unsigned junk_mask;
	struct fpmonitor *fp;
	bool is_pointer_grabbed = false;
	bool is_keyboard_grabbed = false;

	/* Place window at pointer. */
	usposition = true;
	xneg = false;
	yneg = false;
	FQueryPointer(
		dpy, Scr.Root, &junk_w, &junk_w, &pwindow.x,
		&pwindow.y, &i, &i, &junk_mask);

	/* Find monitor pointer is on. */
	fp = fpmonitor_from_xy(pwindow.x, pwindow.y);
	if (fp == NULL)
		fp = fpmonitor_this(NULL);

	/* Center window on pointer. This ignores window decor size. */
	pwindow.x -= pwindow.width / 2;
	pwindow.y -= pwindow.height / 2;

	/* Adjust if pager is outside of monitor boundary.
	 * This ignores EWMH working area.
	 */
	if (pwindow.x < fp->m->si->x)
		pwindow.x = fp->m->si->x;
	if (pwindow.y < fp->m->si->y)
		pwindow.y = fp->m->si->y;
	if (pwindow.x + pwindow.width > fp->m->si->x + fp->m->si->w)
		pwindow.x = fp->m->si->x + fp->m->si->w - pwindow.width;
	if (pwindow.y + pwindow.height > fp->m->si->y + fp->m->si->h)
		pwindow.y = fp->m->si->y + fp->m->si->h - pwindow.height;

	/* Grab pointer. */
	XSync(dpy, 0);
	for (i = 0; i < 50 &&
	     !(is_pointer_grabbed && is_keyboard_grabbed); i++)
	{
		if (!is_pointer_grabbed && XGrabPointer(
		    dpy, Scr.Root, true,
		    ButtonPressMask | ButtonReleaseMask |
		    ButtonMotionMask | PointerMotionMask |
		    EnterWindowMask | LeaveWindowMask,
		    GrabModeAsync, GrabModeAsync,
		    None, None, CurrentTime) == GrabSuccess)
			is_pointer_grabbed = true;

		if (!is_keyboard_grabbed && XGrabKeyboard(
		   dpy, Scr.Root, true, GrabModeAsync, GrabModeAsync,
		   CurrentTime) == GrabSuccess)
			is_keyboard_grabbed = true;

		/* If you go too fast, other windows may not get a
		 * change to release any grab that they have.
		 */
		usleep(20000);
	}

	if (!is_pointer_grabbed) {
		XBell(dpy, 0);
		fprintf(stderr,
			"%s: could not grab pointer in transient mode. "
			"Exiting.\n", MyName);
		ExitPager();
	}
	XSync(dpy, 0);
}

void initialise_common_pager_fragments(void)
{
	DeskStyle *style = FindDeskStyle(-1);

	/* I don't think that this is necessary - just let pager die */
	/* domivogt (07-mar-1999): But it is! A window being moved in the pager
	 * might die at any moment causing the Xlib calls to generate BadMatch
	 * errors. Without an error handler the pager will die! */
	XSetErrorHandler(fvwm_error_handler);

	wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

	/* Check that shape extension exists. */
	if (FHaveShapeExtension && ShapeLabels)
		ShapeLabels = (FShapesSupported) ? 1 : 0;

	/* Load pixmaps for mono use */
	if (Pdepth < 2)
	{
		/* assorted gray bitmaps for decorative borders */
		int g_width = 2, g_height = 2;
		int l_g_width = 4, l_g_height = 2;
		int s_g_width = 4, s_g_height = 4;
		char g_bits[] = {0x02, 0x01};
		char l_g_bits[] = {0x08, 0x02};
		char s_g_bits[] = {0x01, 0x02, 0x04, 0x08};

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
}

void initialize_pager_size(void)
{
	struct fpmonitor *fp = fpmonitor_this(NULL);
	int VxPages = fp->virtual_scr.VxPages;
	int VyPages = fp->virtual_scr.VyPages;

	/* Grid size */
	if (Rows < 0)
	{
		if (Columns <= 0)
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
	if (Columns < 0)
	{
		if (Rows == 0)
			Rows = 1;
		Columns = ndesks/Rows;
		if(Rows*Columns < ndesks)
			Columns++;
	}

	if (Rows*Columns < ndesks)
	{
		if (Columns == 0)
			Columns = 1;
		Rows = ndesks/Columns;
		if (Rows*Columns < ndesks)
			Rows++;
	}
	set_desk_size(true);

	/* Set window size if not fully set by user to match */
	/* aspect ratio of monitor(s) being shown. */
	if (pwindow.width == 0 || pwindow.height == 0) {
		int vWidth  = fpmonitor_get_all_widths();
		int vHeight = fpmonitor_get_all_heights();

		if (monitor_to_track != NULL || IsShared) {
			vWidth = fp->m->si->w;
			vHeight = fp->m->si->h;
		}

		if (pwindow.width > 0) {
			pwindow.height = (pwindow.width * vHeight) /
					 vWidth + label_h * Rows + Rows;
		} else if (pwindow.height > label_h * Rows) {
			pwindow.width = ((pwindow.height - label_h * Rows +
					Rows) * vWidth) / vHeight + Columns;
		} else {
			pwindow.width = (VxPages * vWidth * Columns) /
					Scr.VScale + Columns;
			pwindow.height = (VyPages * vHeight * Rows) /
					 Scr.VScale + label_h * Rows + Rows;
		}
	}
	set_desk_size(false);

	/* Icon window size. */
	if (icon.width < 1)
		icon.width = (pwindow.width - Columns + 1) / Columns;
	if (icon.height < 1)
		icon.height = (pwindow.height - Rows * label_h - Rows + 1) /
			      Rows;

	/* Icon window location. */
	if (icon.x != -10000) {
		icon_usposition = true;
		if (icon_xneg)
			icon.x = fpmonitor_get_all_widths() + icon.x - icon.width;
	} else {
		icon.x = 0;
	}
	if (icon.y != -10000) {
		icon_usposition = true;
		if (icon_yneg)
			icon.y = fpmonitor_get_all_heights() + icon.y - icon.height;
	} else {
		icon.y = 0;
	}

	/* Make each page have same number of pixels. */
	icon.width = (icon.width / (VxPages + 1)) * (VxPages + 1) + VxPages;
	icon.height = (icon.height / (VyPages + 1)) * (VyPages + 1) + VyPages;
}

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
	Balloon.show = (StartIconic) ?
		Balloon.show_in_icon : Balloon.show_in_pager;
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
	if (!Balloon.Ffont)
		Balloon.Ffont = FlocaleLoadFont(dpy, NULL, MyName);

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
	Balloon.gc = fvwmlib_XCreateGC(
		dpy, Scr.balloon_w, valuemask, &xgcv);
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

void initialize_desk_style_gcs(DeskStyle *style)
{
	XGCValues gcv;
	char dash_list[2];

	/* Desk labels GC. */
	gcv.foreground = style->fg;
	style->label_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);

	/* Hilight GC, used for monitors and labels backgrounds. */
	gcv.foreground = (Pdepth < 2) ? style->bg : style->hi_bg;
	style->hi_bg_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);

	/* create the hilight desk title drawing GC */
	gcv.foreground = (Pdepth < 2) ? style->fg : style->hi_fg;
	style->hi_fg_gc = fvwmlib_XCreateGC(
		dpy, Scr.pager_w, GCForeground, &gcv);

	/* create the virtual page boundary GC */
	gcv.foreground = style->fg;
	gcv.line_width = 1;
	gcv.line_style = LineOnOffDash;
	style->dashed_gc = fvwmlib_XCreateGC(dpy,
		Scr.pager_w, GCForeground | GCLineStyle | GCLineWidth, &gcv);
	/* Although this should already be the default for a freshly
	 * created GC, some X servers do not draw properly dashed
	 * lines if the dash style is not set explicitly.
	 */
	dash_list[0] = 4;
	dash_list[1] = 4;
	XSetDashes(dpy, style->dashed_gc, 0, dash_list, 2);

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

void initialize_pager_window(void)
{
	XWMHints wmhints;
	XClassHint class1;
	XTextProperty name;
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int button;

	XSizeHints sizehints = {
		(PWinGravity),		/* flags */
		0, 0, 100, 100,		/* x, y, width and height (legacy) */
		1, 1,			/* Min width and height */
		0, 0,			/* Max width and height */
		1, 1,			/* Width and height increments */
		{0, 0}, {0, 0},		/* Aspect ratio */
		1, 1,			/* base size */
		(NorthWestGravity)	/* gravity */
	};

	if (xneg) {
		sizehints.win_gravity = NorthEastGravity;
		pwindow.x = fpmonitor_get_all_widths() - pwindow.width + pwindow.x;
	}
	if (yneg) {
		if(sizehints.win_gravity == NorthEastGravity)
			sizehints.win_gravity = SouthEastGravity;
		else
			sizehints.win_gravity = SouthWestGravity;
		pwindow.y = fpmonitor_get_all_heights() - pwindow.height + pwindow.y;
	}
	if (usposition)
		sizehints.flags |= USPosition;

	valuemask = (CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask);
	attributes.background_pixmap = default_pixmap;
	attributes.border_pixel = 0;
	attributes.colormap = Pcmap;
	attributes.event_mask = StructureNotifyMask;

	/* destroy the temp window first, don't worry if it's the Root */
	if (Scr.pager_w != Scr.Root)
		XDestroyWindow(dpy, Scr.pager_w);

	Scr.pager_w = XCreateWindow(
		dpy, Scr.Root, pwindow.x, pwindow.y, pwindow.width,
		pwindow.height, 0, Pdepth, InputOutput, Pvisual,
		valuemask, &attributes);

	XSetWMProtocols(dpy, Scr.pager_w, &wm_del_win, 1);
	/* hack to prevent mapping on wrong screen with StartsOnScreen */
	FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &sizehints);
	XSetWMNormalHints(dpy, Scr.pager_w, &sizehints);
	if (is_transient)
		XSetTransientForHint(dpy, Scr.pager_w, Scr.Root);

	if (FlocaleTextListToTextProperty(dpy,
		&(Desks[0].style->label), 1, XStdICCTextStyle, &name) == 0)
	{
		fprintf(stderr,"%s: fatal error: cannot allocate desk name", MyName);
		ExitPager();
	}

	/* Icon Window */
	attributes.event_mask = StructureNotifyMask | ExposureMask;
	Scr.icon_w = XCreateWindow (
		dpy, Scr.Root, pwindow.x, pwindow.y,
		icon.width, icon.height, 0, Pdepth,
		InputOutput, Pvisual, valuemask, &attributes);

	/* Grab button events in Icon window. */
	valuemask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
	for (button = 1; button <= P_MOUSE_BUTTONS; button++) {
		XGrabButton(dpy, button, AnyModifier, Scr.icon_w, True,
			    valuemask, GrabModeAsync, GrabModeAsync,
			    None, None);
	}

	wmhints.initial_state = (StartIconic) ? IconicState : NormalState;
	wmhints.flags = 0;
	wmhints.icon_x = icon.x;
	wmhints.icon_y = icon.y;
	wmhints.flags = InputHint | StateHint | IconWindowHint;
	if (icon_usposition)
		wmhints.flags |= IconPositionHint;
	wmhints.icon_window = Scr.icon_w;
	wmhints.input = True;

	class1.res_name = MyName;
	class1.res_class = "FvwmPager";

	XSetWMProperties(dpy, Scr.pager_w, &name, &name, NULL, 0,
			 &sizehints, &wmhints, &class1);
	XFree((char *)name.value);
}

/* These functions are really tricky. An offset of the struct member
 * in the DeskStyle strut is used to set the desired value.
 * The lines accessing this info look very ugly, but they work.
 */
void set_desk_style_cset(char *arg1, char *arg2, void *offset_style)
{
	int cset = 0;
	int desk;
	unsigned long offset = (unsigned long)offset_style;
	DeskStyle *style;

	sscanf(arg2, "%d", &cset);
	AllocColorset(cset);
	if (arg1[0] == '*') {
		TAILQ_FOREACH(style, &desk_style_q, entry) {
			*(int *)(((char *)style) + offset) = cset;
		}
	} else {
		desk = desk1;
		sscanf(arg1, "%d", &desk);

		style = FindDeskStyle(desk);
		*(int *)(((char *)style) + offset) = cset;
	}
}

void set_desk_style_pixel(char *arg1, char *arg2, void *offset_style)
{
	int desk;
	unsigned long offset = (unsigned long)offset_style;
	DeskStyle *style;
	Pixel pix;

	pix = GetSimpleColor(arg2);
	if (arg1[0] == '*') {
		TAILQ_FOREACH(style, &desk_style_q, entry) {
			*(unsigned long *)(((char *)style) + offset) = pix;
		}
	} else {
		desk = desk1;
		sscanf(arg1, "%d", &desk);

		style = FindDeskStyle(desk);
		*(unsigned long *)(((char *)style) + offset) = pix;
	}
}

void set_desk_style_bool(char *arg1, char *arg2, void *offset_style)
{
	int desk;
	unsigned long offset = (unsigned long)offset_style;
	DeskStyle *style;
	bool val;

	/* We are a bit greedy here, but it is okay. */
	if (arg2[0] == 't' || arg2[0] == 'T' || arg2[0] == '1') {
		val = true;
	} else if (arg2[0] == 'f' || arg2[0] == 'F' || arg2[0] == '0') {
		val = false;
	} else {
		/* But not too greedy. */
		return;
	}

	if (arg1[0] == '*') {
		TAILQ_FOREACH(style, &desk_style_q, entry) {
			*(bool *)(((char *)style) + offset) = val;
		}
	} else {
		desk = desk1;
		sscanf(arg1, "%d", &desk);

		style = FindDeskStyle(desk);
		*(bool *)(((char *)style) + offset) = val;
	}
}

/* This routine is responsible for reading and parsing the config */
void parse_options(void)
{
	char *tline= NULL;
	char *mname;
	bool MoveThresholdSetForModule = false;

	FvwmPictureAttributes fpa;
	Scr.VScale = 32;

	fpa.mask = 0;
	if (Pdepth <= 8)
		fpa.mask |= FPAM_DITHER;

	xasprintf(&mname, "*%s", MyName);
	InitGetConfigLine(fd, mname);
	free(mname);

	for (GetConfigLine(fd, &tline); tline != NULL;
	     GetConfigLine(fd, &tline))
	{
		int g_x, g_y, flags;
		unsigned width, height;
		char *resource;
		char *arg1 = NULL;
		char *arg2 = NULL;
		char *tline2;
		char *token;
		char *next;

		token = PeekToken(tline, &next);
		/* Step 1: Initial configuration broadcasts are parsed here. */
		if (token[0] == '*') {
			/* Module configuration item, skip to next step. */
		} else if (StrEquals(token, "MoveThreshold")) {
			if (MoveThresholdSetForModule)
				continue;

			int val;
			if (GetIntegerArguments(next, NULL, &val, 1) > 0)
				MoveThreshold = (val >= 0) ? val :
						DEFAULT_PAGER_MOVE_THRESHOLD;
			continue;
		} else {
			/* Deal with any M_CONFIG_INFO lines. */
			process_config_info_line(tline, true);
			continue;
		}

		/* Step 2: Parse module configuration options. */
		tline2 = GetModuleResource(tline, &resource, MyName);
		if (!resource)
			continue;

		/* Start by looking for options with no parameters. */
		flags = 1;
		if (StrEquals(resource, "DeskLabels")) {
			use_desk_label = true;
		} else if (StrEquals(resource, "NoDeskLabels")) {
			use_desk_label = false;
		} else if (StrEquals(resource, "MonitorLabels")) {
			use_monitor_label = true;
		} else if (StrEquals(resource, "NoMonitorLabels")) {
			use_monitor_label = false;
		} else if (StrEquals(resource, "CurrentDeskPerMonitor")) {
			CurrentDeskPerMonitor = true;
		} else if (StrEquals(resource, "CurrentDeskGlobal")) {
			CurrentDeskPerMonitor = false;
		} else if (StrEquals(resource, "IsShared")) {
			IsShared = true;
		} else if (StrEquals(resource, "IsNotShared")) {
			IsShared = false;
		} else if (StrEquals(resource, "DeskHilight")) {
			HilightDesks = true;
		} else if (StrEquals(resource, "NoDeskHilight")) {
			HilightDesks = false;
		} else if (StrEquals(resource, "LabelHilight")) {
			HilightLabels = true;
		} else if (StrEquals(resource, "NoLabelHilight")) {
			HilightLabels = false;
		} else if (StrEquals(resource, "MiniIcons")) {
			MiniIcons = true;
		} else if (StrEquals(resource, "StartIconic")) {
			StartIconic = true;
		} else if (StrEquals(resource, "NoStartIconic")) {
			StartIconic = false;
		} else if (StrEquals(resource, "LabelsBelow")) {
			LabelsBelow = true;
		} else if (StrEquals(resource, "LabelsAbove")) {
			LabelsBelow = false;
		} else if (StrEquals(resource, "ShapeLabels")) {
			if (FHaveShapeExtension)
				ShapeLabels = true;
		} else if (StrEquals(resource, "NoShapeLabels")) {
			ShapeLabels = false;
		} else if (StrEquals(resource, "HideSmallWindows")) {
			HideSmallWindows = true;
		} else if (StrEquals(resource, "Window3dBorders")) {
			WindowBorders3d = true;
		} else if (StrEquals(resource,"UseSkipList")) {
			UseSkipList = true;
		} else if (StrEquals(resource, "SloppyFocus")) {
			do_focus_on_enter = true;
		} else if (StrEquals(resource, "SendCmdAfterMove")) {
			SendCmdAfterMove = true;
		} else if (StrEquals(resource, "SolidSeparators")) {
			use_dashed_separators = false;
			use_no_separators = false;
		} else if (StrEquals(resource, "NoSeparators")) {
			use_no_separators = true;
		} else if (StrEquals(resource, "IgnoreWorkingArea")) {
			ewmhiwa = true;
		} else {
			/* No Match, set this to continue parsing. */
			flags = 0;
		}
		if (flags == 1) {
			free(resource);
			continue;
		}

		/* Now look for options with additional inputs.
		 * Many inputs are of the form: [desk] value
		 * Since desk is optional, inputs are stored as:
		 *     One input:  arg1 = '*';     arg2 = input1
		 *     Two inputs: arg1 = input1;  arg2 = input2
		 * Options that expect one input should use "next".
		 */
		tline2 = GetNextToken(tline2, &arg1);
		if (!arg1)
			/* No inputs, nothing to do. */
			continue;

		tline2 = GetNextToken(tline2, &arg2);
		if (!arg2)
		{
			arg2 = arg1;
			arg1 = NULL;
			arg1 = fxmalloc(1);
			arg1[0] = '*';
		}

		next = SkipSpaces(next, NULL, 0);
		if (StrEquals(resource, "Monitor")) {
			update_monitor_to_track(
				&monitor_to_track, next, true);
		} else if (StrEquals(resource, "CurrentMonitor")) {
			update_monitor_to_track(
				&current_monitor, next, false);
		} else if(StrEquals(resource, "Colorset")) {
			set_desk_style_cset(arg1, arg2,
				&(((DeskStyle *)(NULL))->cs));
		} else if(StrEquals(resource, "BalloonColorset")) {
			set_desk_style_cset(arg1, arg2,
				&(((DeskStyle *)(NULL))->balloon_cs));
		} else if(StrEquals(resource, "HilightColorset")) {
			set_desk_style_cset(arg1, arg2,
				&(((DeskStyle *)(NULL))->hi_cs));
		} else if (StrEquals(resource, "WindowColorset")) {
			set_desk_style_cset(arg1, arg2,
				&(((DeskStyle *)(NULL))->win_cs));
		} else if (StrEquals(resource, "FocusColorset")) {
			set_desk_style_cset(arg1, arg2,
				&(((DeskStyle *)(NULL))->focus_cs));
		} else if (StrEquals(resource, "WindowColorsets")) {
			/* Backwards compatibility. */
			if (arg1[0] != '*') {
				set_desk_style_cset("*", arg1,
					&(((DeskStyle *)(NULL))->win_cs));
				set_desk_style_cset("*", arg2,
					&(((DeskStyle *)(NULL))->focus_cs));
			}
		} else if (StrEquals(resource, "Fore")) {
			if (Pdepth > 0)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->fg));
		} else if (StrEquals(resource, "Back") ||
			   StrEquals(resource, "DeskColor"))
		{
			if (Pdepth > 0)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->bg));
		} else if (StrEquals(resource, "HiFore")) {
			if (Pdepth > 0)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->hi_fg));
		} else if (StrEquals(resource, "HiBack") ||
			   StrEquals(resource, "Hilight"))
		{
			if (Pdepth > 0)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->hi_bg));
		} else if (StrEquals(resource, "WindowFore")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->win_fg));
		} else if (StrEquals(resource, "WindowBack")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->win_bg));
		} else if (StrEquals(resource, "FocusFore")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->focus_fg));
		} else if (StrEquals(resource, "FocusBack")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->focus_bg));
		} else if (StrEquals(resource, "WindowColors")) {
			/* Backwards compatibility. */
			if (Pdepth > 1 && arg1[0] != '*')
			{
				set_desk_style_pixel("*", arg1,
					&(((DeskStyle *)(NULL))->win_bg));
				set_desk_style_pixel("*", arg2,
					&(((DeskStyle *)(NULL))->win_bg));
				free(arg2);
				tline2 = GetNextToken(tline2, &arg2);
				set_desk_style_pixel("*", arg2,
					&(((DeskStyle *)(NULL))->focus_fg));
				free(arg2);
				tline2 = GetNextToken(tline2, &arg2);
				set_desk_style_pixel("*", arg2,
					&(((DeskStyle *)(NULL))->focus_bg));
			}
		} else if (StrEquals(resource, "BalloonFore")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_fg));
		} else if (StrEquals(resource, "BalloonBack")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_bg));
		} else if (StrEquals(resource, "BalloonBorderColor")) {
			if (Pdepth > 1)
				set_desk_style_pixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_border));
		} else if (StrEquals(resource, "LabelPixmap")) {
			set_desk_style_bool(arg1, arg2,
				&(((DeskStyle *)(NULL))->use_label_pixmap));
		} else if (StrEquals(resource, "Geometry")) {
			flags = FScreenParseGeometry(
					next, &g_x, &g_y, &width, &height);
			if (flags & WidthValue)
				pwindow.width = width;
			if (flags & HeightValue)
				pwindow.height = height;
			if (flags & XValue) {
				pwindow.x = g_x;
				usposition = true;
				if (flags & XNegative)
					xneg = true;
			}
			if (flags & YValue) {
				pwindow.y = g_y;
				usposition = true;
				if (flags & YNegative)
					yneg = true;
			}
		} else if (StrEquals(resource, "IconGeometry")) {
			flags = FScreenParseGeometry(
					next, &g_x, &g_y, &width, &height);
			if (flags & WidthValue)
				icon.width = width;
			if (flags & HeightValue)
				icon.height = height;
			if (flags & XValue) {
				icon.x = g_x;
				if (flags & XNegative)
					icon_xneg = true;
			}
			if (flags & YValue) {
				icon.y = g_y;
				if (flags & YNegative)
					icon_yneg = true;
			}
		} else if (StrEquals(resource, "Font")) {
			if (strncasecmp(next, "none", 4) == 0) {
				use_desk_label = false;
				use_monitor_label = false;
			} else {
				FlocaleUnloadFont(dpy, Scr.Ffont);
				Scr.Ffont = FlocaleLoadFont(dpy, next, MyName);
			}
		} else if (StrEquals(resource, "Pixmap") ||
			   StrEquals(resource, "DeskPixmap"))
		{
			if (Pdepth == 0)
				goto free_vars;

			DeskStyle *style;
			if (arg1[0] == '*') {
				TAILQ_FOREACH(style, &desk_style_q, entry) {
					if (style->bgPixmap != NULL) {
						PDestroyFvwmPicture(
							dpy, style->bgPixmap);
						style->bgPixmap = NULL;
					}
					style->bgPixmap = PCacheFvwmPicture(
						dpy, Scr.pager_w, ImagePath,
						arg2, fpa);
				}
			} else {
				int desk = 0;
				sscanf(arg1, "%d", &desk);
				style = FindDeskStyle(desk);
				if (style->bgPixmap != NULL) {
					PDestroyFvwmPicture(
						dpy, style->bgPixmap);
					style->bgPixmap = NULL;
				}
				style->bgPixmap = PCacheFvwmPicture(dpy,
					Scr.pager_w, ImagePath, arg2, fpa);
			}
		} else if (StrEquals(resource, "HilightPixmap")) {
			if (Pdepth == 0)
				goto free_vars;

			DeskStyle *style;
			if (arg1[0] == '*') {
				TAILQ_FOREACH(style, &desk_style_q, entry) {
					if (style->hiPixmap != NULL) {
						PDestroyFvwmPicture(
							dpy, style->hiPixmap);
						style->hiPixmap = NULL;
					}
					style->hiPixmap = PCacheFvwmPicture(
						dpy, Scr.pager_w, ImagePath,
						arg2, fpa);
				}
			} else {
				int desk = 0;
				sscanf(arg1, "%d", &desk);
				style = FindDeskStyle(desk);
				if (style->hiPixmap != NULL) {
					PDestroyFvwmPicture(
						dpy, style->hiPixmap);
					style->hiPixmap = NULL;
				}
				style->hiPixmap = PCacheFvwmPicture(dpy,
					Scr.pager_w, ImagePath, arg2, fpa);
			}
		} else if (StrEquals(resource, "WindowFont") ||
			   StrEquals(resource, "SmallFont"))
		{
			FlocaleUnloadFont(dpy, Scr.winFfont);
			Scr.winFfont = NULL;
			if (strncasecmp(next, "none", 4) != 0)
				Scr.winFfont = FlocaleLoadFont(
					dpy, next, MyName);
		} else if (StrEquals(resource, "Rows")) {
			sscanf(next, "%d", &Rows);
		} else if (StrEquals(resource, "Columns")) {
			sscanf(next, "%d", &Columns);
		} else if (StrEquals(resource, "DesktopScale")) {
			sscanf(next, "%d", &Scr.VScale);
		} else if (StrEquals(resource, "WindowBorderWidth")) {
			MinSize = MinSize - 2*WindowBorderWidth;
			sscanf(next, "%d", &WindowBorderWidth);
			if (WindowBorderWidth > 0)
				MinSize = 2 * WindowBorderWidth + MinSize;
			else
				MinSize = MinSize + 2 *
					  DEFAULT_PAGER_WINDOW_BORDER_WIDTH;
		} else if (StrEquals(resource, "WindowMinSize")) {
			sscanf(next, "%d", &MinSize);
			if (MinSize > 0)
				MinSize = 2 * WindowBorderWidth + MinSize;
			else
				MinSize = 2 * WindowBorderWidth +
					  DEFAULT_PAGER_WINDOW_MIN_SIZE;
		} else if (StrEquals(resource, "WindowLabelFormat")) {
			free(WindowLabelFormat);
			CopyString(&WindowLabelFormat, next);
		} else if (StrEquals(resource, "MoveThreshold")) {
			int val;
			if (GetIntegerArguments(next, NULL, &val, 1) > 0 &&
			    val >= 0)
			{
				MoveThreshold = val;
				MoveThresholdSetForModule = true;
			}
		} else if (StrEquals(resource, "Balloons")) {
			if (StrEquals(next, "Pager")) {
				Balloon.show_in_pager = true;
				Balloon.show_in_icon = false;
			} else if (StrEquals(next, "Icon")) {
				Balloon.show_in_pager = false;
				Balloon.show_in_icon = true;
			} else if (StrEquals(next, "None")) {
				Balloon.show_in_pager = false;
				Balloon.show_in_icon = false;
			} else {
				Balloon.show_in_pager = true;
				Balloon.show_in_icon = true;
			}
		} else if (StrEquals(resource, "BalloonFont")) {
			FlocaleUnloadFont(dpy, Balloon.Ffont);
			Balloon.Ffont = FlocaleLoadFont(dpy, next, MyName);
		} else if (StrEquals(resource, "BalloonBorderWidth")) {
			sscanf(next, "%d", &(Balloon.border_width));
		} else if (StrEquals(resource, "BalloonYOffset")) {
			sscanf(next, "%d", &(Balloon.y_offset));
		} else if (StrEquals(resource, "BalloonStringFormat")) {
			free(Balloon.label_format);
			CopyString(&(Balloon.label_format), next);
		} else if (StrEquals(resource, "Mouse")) {
			int button;

			if (arg1[0] == '*')
				goto free_vars; /* Not enough inputs. */

			sscanf(arg1, "%d", &button);
			button--;
			if (button < 0 || button >= P_MOUSE_BUTTONS)
				goto free_vars;

			/* Determine mouse action */
			if (StrEquals(arg2, "ChangePage")) {
				mouse_action[button] = P_MOUSE_MOVE;
			} else if (StrEquals(arg2, "MoveWindow")) {
				mouse_action[button] = P_MOUSE_WIN_MOVE;
				if (tline2 != NULL && tline2[0] != '\0') {
					free(mouse_cmd[button]);
					mouse_cmd[button] = fxstrdup(tline2);
				}
			} else if (StrEquals(arg2, "WindowCmd")) {
				if (tline2 == NULL || tline2[0] == '\0')
					goto free_vars;
				free(mouse_cmd[button]);
				mouse_action[button] = P_MOUSE_WIN_CMD;
				mouse_cmd[button] = fxstrdup(tline2);
			} else if (StrEquals(arg2, "Scroll")) {
				mouse_action[button] = P_MOUSE_SCROLL;
			} else if (StrEquals(arg2, "Cmd")) {
				if (tline2 == NULL || tline2[0] == '\0')
					goto free_vars;
				free(mouse_cmd[button]);
				mouse_action[button] = P_MOUSE_CMD;
				mouse_cmd[button] = fxstrdup(tline2);
			} else if (StrEquals(arg2, "Nop")) {
				mouse_action[button] = P_MOUSE_NOP;
			}
		}

free_vars:
		free(resource);
		free(arg1);
		free(arg2);
	}

	return;
}
