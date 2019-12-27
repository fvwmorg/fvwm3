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

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "Parse.h"
#include "FScreen.h"
#include "PictureBase.h"

#ifdef HAVE_XRANDR
#	include <X11/extensions/Xrandr.h>
#	define IS_RANDR_ENABLED 1
#else
#	define IS_RANDR_ENABLED 0
#endif

#define GLOBAL_SCREEN_NAME "_global"

/* In fact, only corners matter -- there will never be GRAV_NONE */
enum {GRAV_POS = 0, GRAV_NONE = 1, GRAV_NEG = 2};
static int grav_matrix[3][3] =
{
	{ NorthWestGravity, NorthGravity,  NorthEastGravity },
	{ WestGravity,      CenterGravity, EastGravity },
	{ SouthWestGravity, SouthGravity,  SouthEastGravity }
};
#define DEFAULT_GRAVITY NorthWestGravity

static Display *disp;
static int no_of_screens;

static struct monitor	*monitor_new(void);
static void		 monitor_create_randr_region(struct monitor *m,
	const char *, struct coord *, int);
static int		 monitor_check_stale(struct monitor *);

static void GetMouseXY(XEvent *eventp, int *x, int *y)
{
	XEvent e;

	if (eventp == NULL)
	{
		eventp = &e;
		e.type = 0;
	}
	fev_get_evpos_or_query(
		disp, DefaultRootWindow(disp), eventp, x, y);
}

Bool FScreenIsEnabled(void)
{
	return (IS_RANDR_ENABLED);
}

struct monitor *
monitor_new(void)
{
	struct monitor	*m;

	m = calloc(1, sizeof *m);

	return (m);
}

struct monitor *
monitor_get_current(void)
{
	int		 JunkX = 0, JunkY = 0, x, y;
	Window		 JunkRoot, JunkChild;
	unsigned int	 JunkMask;

	FQueryPointer(disp, DefaultRootWindow(disp), &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &x, &y, &JunkMask);

	return (FindScreenOfXY(x, y));
}

struct monitor *
monitor_by_name(const char *name)
{
	struct monitor	*m, *mret = NULL;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (strcmp(m->name, name) == 0) {
			mret = m;
			break;
		}
	}

	if (mret == NULL && (strcmp(name, GLOBAL_SCREEN_NAME) == 0)) {
		if (no_of_screens == 1) {
			/* In this case, the global screen was requested, but
			 * we've only one monitor in use.  Return this monitor
			 * instead.
			 */
		    return (TAILQ_FIRST(&monitor_q));
		} else {
			/* Return the current monitor. */
			mret = monitor_get_current();
		}
	}

	/* Then we couldn't find the named monitor at all.  Return the current
	 * monitor instead.
	 */

	if (mret == NULL) {
		mret = monitor_get_current();
		fprintf(stderr, "%s: couldn't find monitor: (%s)\n", __func__,
		    name);
		fprintf(stderr, "%s: returning current monitor (%s)\n",
		    __func__, mret->name);
	}

	return (mret);
}

struct monitor *
monitor_by_number(int number)
{
	struct monitor	*m, *mret = NULL;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->number == number) {
		       mret = m;
		       break;
		}
	}

	/* If 'm' is still NULL here, and the monitor number is -1, return
	 * the global  monitor instead.  This check can only succeed if we've
	 * requested the global screen whilst XRandR is in use, since the global
	 * monitor isn't stored in the monitor list directly.
	 */
	if (mret == NULL && number == -1)
		return (monitor_by_name(GLOBAL_SCREEN_NAME));

	/* Then we couldn't find the named monitor at all.  Return the current
	 * monitor instead.
	 */
	if (mret == NULL) {
		mret = monitor_get_current();
		fprintf(stderr, "%s: couldn't find monitor id: %d\n", __func__,
		    number);
		fprintf(stderr, "%s: returning current monitor (%s)\n",
		    __func__, mret->name);
	}

	return (mret);
}

void
FScreenSelect(Display *dpy)
{
	XRRSelectInput(disp, DefaultRootWindow(disp), RRScreenChangeNotifyMask);
}

void FScreenInit(Display *dpy)
{
	XRRScreenResources	*res = NULL;
	XRROutputInfo		*oinfo = NULL;
	XRRCrtcInfo		*crtc_info = NULL;
	RROutput		 rr_output, rr_output_primary;
	struct monitor		*m;
	struct coord		 coord;
	int			 err_base = 0;
	int			 is_randr_present = 0;
	int			 iscres, is_primary = 0;

	disp = dpy;
	randr_event = 0;

	/* Allocate for monitors. */
	TAILQ_INIT(&monitor_q);

	is_randr_present = XRRQueryExtension(dpy, &randr_event, &err_base);

	if (FScreenIsEnabled() && !is_randr_present) {
		/* Something went wrong. */
		fprintf(stderr, "Couldn't initialise XRandR: %s\n",
			strerror(errno));
		fprintf(stderr, "Falling back to single screen...\n");

		goto single_screen;
	}

	/* XRandR is present, so query the screens we have. */
	res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));

	if (res == NULL || (res != NULL && res->noutput == 0)) {
		XRRFreeScreenResources(res);
		goto single_screen;
	}

	for (iscres = 0; iscres < res->ncrtc; iscres++) {
		crtc_info = XRRGetCrtcInfo(dpy, res, res->crtcs[iscres]);

		if (crtc_info->noutput == 0) {
			fprintf(stderr, "Screen found; no crtc present\n");
			XRRFreeCrtcInfo(crtc_info);
			continue;
		}

		rr_output = crtc_info->outputs[0];
		rr_output_primary = XRRGetOutputPrimary(dpy,
			DefaultRootWindow(dpy));

		if (rr_output == rr_output_primary)
			is_primary = 1;
		else
			is_primary = 0;

		oinfo = XRRGetOutputInfo(dpy, res, rr_output);

		if (oinfo == NULL)
			continue;

		if (oinfo->connection != RR_Connected) {
			fprintf(stderr, "Tried to create monitor '%s' "
				"but not connected", oinfo->name);
			XRRFreeOutputInfo(oinfo);
			XRRFreeCrtcInfo(crtc_info);
			continue;
		}

		m = monitor_new();
		m->number = no_of_screens;
		coord.x = crtc_info->x;
		coord.y = crtc_info->y;
		coord.w = crtc_info->width;
		coord.h = crtc_info->height;

		monitor_create_randr_region(m, oinfo->name, &coord, is_primary);

		XRRFreeCrtcInfo(crtc_info);
		XRRFreeOutputInfo(oinfo);

		no_of_screens++;
	}
	XRRFreeScreenResources(res);

	return;

single_screen:
	m = monitor_new();
	m->number = -1;
	coord.x = 0;
	coord.y = 0;
	coord.w = DisplayWidth(disp, DefaultScreen(disp));
	coord.h = DisplayHeight(disp, DefaultScreen(disp));

	monitor_create_randr_region(m, GLOBAL_SCREEN_NAME, &coord, is_primary);

	if (++no_of_screens > 0)
		no_of_screens--;
}

static void
monitor_create_randr_region(struct monitor *m, const char *name,
	struct coord *coord, int is_primary)
{
	fprintf(stderr, "Monitor: %s %s (x: %d, y: %d, w: %d, h: %d)\n",
		name, is_primary ? "(PRIMARY)" : "",
		coord->x, coord->y, coord->w, coord->h);

	if (monitor_check_stale(m))
		free(m->name);

	m->name = fxstrdup(name);
	memcpy(&m->coord, coord, sizeof(*coord));

	if (is_primary) {
		TAILQ_INSERT_HEAD(&monitor_q, m, entry);
	} else {
		TAILQ_INSERT_TAIL(&monitor_q, m, entry);
	}
}

static int
monitor_check_stale(struct monitor *m)
{
	struct monitor	*mcheck = NULL;

	TAILQ_FOREACH(mcheck, &monitor_q, entry) {
		if (m == NULL || mcheck == NULL)
			break;
		if (m->name == NULL || mcheck->name == NULL) {
			/* Shouldn't happen. */
			break;
		}
		if (strcmp(mcheck->name, m->name) == 0)
			return (1);
	}

	return (0);
}

int
monitor_get_count(void)
{
	return (no_of_screens);
}

struct monitor *
FindScreenOfXY(int x, int y)
{
	struct monitor	*m;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		/* If we have more than one screen configured, then don't match
		 * on the global screen, as that's separate to XY positioning
		 * which is only concerned with the *specific* screen.
		 */
		if (no_of_screens > 0 &&
		    strcmp(m->name, GLOBAL_SCREEN_NAME) == 0)
			continue;
		if (x >= m->coord.x && x < m->coord.x + m->coord.w &&
		    y >= m->coord.y && y < m->coord.y + m->coord.h)
			return (m);
	}

	return (NULL);
}

static struct monitor *
FindScreen(fscreen_scr_arg *arg, fscreen_scr_t screen)
{
	struct monitor	*m = NULL;
	fscreen_scr_arg  tmp;

	if (no_of_screens == 0)
		return (monitor_by_name(GLOBAL_SCREEN_NAME));

	switch (screen)
	{
	case FSCREEN_GLOBAL:
		m = monitor_by_name(GLOBAL_SCREEN_NAME);
		break;
	case FSCREEN_PRIMARY:
	case FSCREEN_CURRENT:
		/* translate to xypos format */
		if (!arg)
		{
			tmp.mouse_ev = NULL;
			arg = &tmp;
		}
		GetMouseXY(arg->mouse_ev, &arg->xypos.x, &arg->xypos.y);
		/* fall through */
	case FSCREEN_XYPOS:
		/* translate to screen number */
		if (!arg)
		{
			tmp.xypos.x = 0;
			tmp.xypos.y = 0;
			arg = &tmp;
		}
		m = FindScreenOfXY(arg->xypos.x, arg->xypos.y);
		break;
	case FSCREEN_BY_NAME:
		if (arg == NULL || arg->name == NULL) {
			/* XXX: Work out what to do. */
			break;
		}
		m = monitor_by_name(arg->name);
		break;
	default:
		/* XXX: Possible error condition here? */
		break;
	}

	if (m == NULL)
		m = TAILQ_FIRST(&monitor_q);

	return (m);
}

/* Given pointer coordinates, return the screen the pointer is on.
 *
 * Perhaps most useful with $[pointer.screen]
 */
const char *
FScreenOfPointerXY(int x, int y)
{
	struct monitor	*m;

	m = FindScreenOfXY(x, y);

	return (m != NULL) ? m->name : "unknown";
}

/* Returns the specified screens geometry rectangle.  screen can be a screen
 * number or any of the values FSCREEN_GLOBAL, FSCREEN_CURRENT,
 * FSCREEN_PRIMARY or FSCREEN_XYPOS.  The arg union only has a meaning for
 * FSCREEN_CURRENT and FSCREEN_XYARG.  For FSCREEN_CURRENT its mouse_ev member
 * may be given.  It is tried to find out the pointer position from the event
 * first before querying the pointer.  For FSCREEN_XYPOS the xpos member is used
 * to fetch the x/y position of the point on the screen.  If arg is NULL, the
 * position 0 0 is assumed instead.
 *
 * Any of the arguments arg, x, y, w and h may be NULL.
 *
 * FSCREEN_GLOBAL:  return the global screen dimensions
 * FSCREEN_CURRENT: return dimensions of the screen with the pointer
 * FSCREEN_PRIMARY: return the primary screen dimensions
 * FSCREEN_XYPOS:   return dimensions of the screen with the given coordinates
 * FSCREEN_BY_NAME: return dimensions of the screen with the given name
 *
 * The function returns False if the global screen was returned and more than
 * one screen is configured.  Otherwise it returns True.
 */
Bool FScreenGetScrRect(fscreen_scr_arg *arg, fscreen_scr_t screen,
			int *x, int *y, int *w, int *h)
{
	struct monitor	*m = FindScreen(arg, screen);
	if (m == NULL)
		return (True);

	if (x)
		*x = m->coord.x;
	if (y)
		*y = m->coord.y;
	if (w)
		*w = m->coord.w;
	if (h)
		*h = m->coord.h;

	return !((no_of_screens > 1) &&
		(strcmp(m->name, GLOBAL_SCREEN_NAME) == 0));
}

/* Translates the coodinates *x *y from the screen specified by arg_src and
 * screen_src to coordinates on the screen specified by arg_dest and
 * screen_dest. (see FScreenGetScrRect for more details). */
void FScreenTranslateCoordinates(
	fscreen_scr_arg *arg_src, fscreen_scr_t screen_src,
	fscreen_scr_arg *arg_dest, fscreen_scr_t screen_dest,
	int *x, int *y)
{
	int x_src;
	int y_src;
	int x_dest;
	int y_dest;

	FScreenGetScrRect(arg_src, screen_src, &x_src, &y_src, NULL, NULL);
	FScreenGetScrRect(arg_dest, screen_dest, &x_dest, &y_dest, NULL, NULL);

	if (x)
	{
		*x = *x + x_src - x_dest;
	}
	if (y)
	{
		*y = *y + y_src - y_dest;
	}

	return;
}

/* Arguments work exactly like for FScreenGetScrRect() */
int FScreenClipToScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
			int *x, int *y, int w, int h)
{
	int sx;
	int sy;
	int sw;
	int sh;
	int lx = (x) ? *x : 0;
	int ly = (y) ? *y : 0;
	int x_grav = GRAV_POS;
	int y_grav = GRAV_POS;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
	if (lx + w > sx + sw)
	{
		lx = sx + sw  - w;
		x_grav = GRAV_NEG;
	}
	if (ly + h > sy + sh)
	{
		ly = sy + sh - h;
		y_grav = GRAV_NEG;
	}
	if (lx < sx)
	{
		lx = sx;
		x_grav = GRAV_POS;
	}
	if (ly < sy)
	{
		ly = sy;
		y_grav = GRAV_POS;
	}
	if (x)
	{
		*x = lx;
	}
	if (y)
	{
		*y = ly;
	}

	return grav_matrix[y_grav][x_grav];
}

/* Arguments work exactly like for FScreenGetScrRect() */
void FScreenCenterOnScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
			   int *x, int *y, int w, int h)
{
	int sx;
	int sy;
	int sw;
	int sh;
	int lx;
	int ly;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
	lx = (sw  - w) / 2;
	ly = (sh - h) / 2;
	if (lx < 0)
		lx = 0;
	if (ly < 0)
		ly = 0;
	lx += sx;
	ly += sy;
	if (x)
	{
		*x = lx;
	}
	if (y)
	{
		*y = ly;
	}
}

void FScreenGetResistanceRect(
	int wx, int wy, unsigned int ww, unsigned int wh, int *x0, int *y0,
	int *x1, int *y1)
{
	fscreen_scr_arg arg;

	arg.xypos.x = wx + ww / 2;
	arg.xypos.y = wy + wh / 2;
	FScreenGetScrRect(&arg, FSCREEN_XYPOS, x0, y0, x1, y1);
	*x1 += *x0;
	*y1 += *y0;
}

/* Arguments work exactly like for FScreenGetScrRect() */
Bool FScreenIsRectangleOnScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
				rectangle *rec)
{
	int sx;
	int sy;
	int sw;
	int sh;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);

	return (rec->x + rec->width > sx && rec->x < sx + sw &&
		rec->y + rec->height > sy && rec->y < sy + sh) ? True : False;
}

/*
 * FScreenParseGeometry
 *     Does the same as XParseGeometry, but handles additional "@scr".
 *     Since it isn't safe to define "ScreenValue" constant (actual values
 *     of other "XXXValue" are specified in Xutil.h, not by us, so there can
 *     be a clash), the screen value is always returned, even if it wasn't
 *     present in `parse_string' (set to default in that case).
 *
 */
int FScreenParseGeometryWithScreen(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	char **screen_return)
{
	char *copy, *geom_str = NULL;
	int   ret;

	/* Safety net */
	if (parsestring == NULL || *parsestring == '\0')
		return 0;

	/* If the geometry specification contains an '@' symbol, assume the
	 * screen is specified.  This must be the name of the monitor in
	 * question!
	 */
	if (strchr(parsestring, '@') == NULL) {
		copy = fxstrdup(parsestring);
		goto parse_geometry;
	}

	copy = fxstrdup(parsestring);
	copy = strsep(&parsestring, "@");
	*screen_return = parsestring;
	geom_str = strsep(&copy, "@");
	copy = geom_str;

parse_geometry:
	/* Do the parsing */
	ret = XParseGeometry(
		copy, x_return, y_return, width_return, height_return);

	return ret;
}

/* Same as above, but dump screen return value to keep compatible with the X
 * function. */
int FScreenParseGeometry(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return)
{
	struct monitor	*m;
	char		*scr = NULL;
	int		 rc;

	rc = FScreenParseGeometryWithScreen(
		parsestring, x_return, y_return, width_return, height_return,
		&scr);

	if (scr != NULL)
		m = monitor_by_name(scr);
	else
		m = monitor_get_current();

	/* adapt geometry to selected screen */
	if (rc & XValue)
	{
		if (rc & XNegative)
			*x_return -= (m->coord.w - m->coord.x);
		else
			*x_return += m->coord.x;
	}
	if (rc & YValue)
	{
		if (rc & YNegative)
			*y_return -= (m->coord.h - m->coord.y);
		else
			*y_return += m->coord.y;
	}
	return rc;
}


/*  FScreenGetGeometry
 *      Parses the geometry in a form: XGeometry[@screen], i.e.
 *          [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>][@<screen>]
 *          where <screen> is either a number or "G" (global) "C" (current)
 *          or "P" (primary)
 *
 *  Args:
 *      parsestring, x_r, y_r, w_r, h_r  the same as in XParseGeometry
 *      hints  window hints structure, may be NULL
 *      flags  bitmask of allowed flags (XValue, WidthValue, XNegative...)
 *
 *  Note1:
 *      hints->width and hints->height will be used to calc negative geometry
 *      if width/height isn't specified in the geometry itself.
 *
 *  Note2:
 *      This function's behaviour is crafted to sutisfy/emulate the
 *      FvwmWinList::MakeMeWindow()'s behaviour.
 *
 *  Note3:
 *      A special value of `flags' when [XY]Value are there but [XY]Negative
 *      aren't, means that in case of negative geometry specification
 *      x_r/y_r values will be promoted to the screen border, but w/h
 *      wouldn't be subtracted, so that the program can do x-=w later
 *      ([XY]Negative *will* be returned, albeit absent in `flags').
 *          This option is supposed for proggies like FvwmButtons, which
 *      receive geometry specification long before they are able to actually
 *      use it (and which calculate w/h themselves).
 *          (The same effect can't be obtained with omitting {Width,Height}Value
 *      in the flags, since the app may wish to get the dimensions but apply
 *      some constraints later (as FvwmButtons do, BTW...).)
 *          This option can be also useful in cases where dimensions are
 *      specified not in pixels but in some other units (e.g., charcells).
 */
int FScreenGetGeometry(
	char *parsestring, int *x_return, int *y_return,
	int *width_return, int *height_return, XSizeHints *hints, int flags)
{
	fscreen_scr_arg	 arg;
	char		*scr = NULL;
	int		 ret;
	int		 saved;
	int		 x, y;
	unsigned int	 w = 0, h = 0;
	int		 grav, x_grav, y_grav;
	int		 scr_x, scr_y;
	int		 scr_w, scr_h;

	/* I. Do the parsing and strip off extra bits */
	ret = FScreenParseGeometryWithScreen(parsestring, &x, &y, &w, &h, &scr);
	saved = ret & (XNegative | YNegative);
	ret  &= flags;

	arg.mouse_ev = NULL;
	arg.name = scr;
	FScreenGetScrRect(&arg, FSCREEN_BY_NAME, &scr_x, &scr_y, &scr_w, &scr_h);

	/* II. Interpret and fill in the values */

	/* Fill in dimensions for future negative calculations if
	 * omitted/forbidden */
	/* Maybe should use *x_return,*y_return if hints==NULL?
	 * Unreliable... */
	if (hints != NULL  &&  hints->flags & PSize)
	{
		if ((ret & WidthValue)  == 0)
		{
			w = hints->width;
		}
		if ((ret & HeightValue) == 0)
		{
			h = hints->height;
		}
	}
	else
	{
		/* This branch is required for case when size *is* specified,
		 * but masked off */
		if ((ret & WidthValue)  == 0)
		{
			w = 0;
		}
		if ((ret & HeightValue) == 0)
		{
			h = 0;
		}
	}

	/* Advance coords to the screen... */
	x += scr_x;
	y += scr_y;

	/* ...and process negative geometries */
	if (saved & XNegative)
	{
		x += scr_w;
	}
	if (saved & YNegative)
	{
		y += scr_h;
	}
	if (ret & XNegative)
	{
		x -= w;
	}
	if (ret & YNegative)
	{
		y -= h;
	}

	/* Restore negative bits */
	ret |= saved;

	/* Guess orientation */
	x_grav = (ret & XNegative)? GRAV_NEG : GRAV_POS;
	y_grav = (ret & YNegative)? GRAV_NEG : GRAV_POS;
	grav   = grav_matrix[y_grav][x_grav];

	/* Return the values */
	if (ret & XValue)
	{
		*x_return = x;
		if (hints != NULL)
		{
			hints->x = x;
		}
	}
	if (ret & YValue)
	{
		*y_return = y;
		if (hints != NULL)
		{
			hints->y = y;
		}
	}
	if (ret & WidthValue)
	{
		*width_return = w;
		if (hints != NULL)
		{
			hints->width = w;
		}
	}
	if (ret & HeightValue)
	{
		*height_return = h;
		if (hints != NULL)
		{
			hints->height = h;
		}
	}
	if (1 /*flags & GravityValue*/  &&  grav != DEFAULT_GRAVITY)
	{
		if (hints != NULL  &&  hints->flags & PWinGravity)
		{
			hints->win_gravity = grav;
		}
	}
	if (hints != NULL  &&  ret & XValue  &&  ret & YValue)
		hints->flags |= USPosition;

	return ret;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen number.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
void FScreenMangleScreenIntoUSPosHints(fscreen_scr_t screen, XSizeHints *hints)
{
	if (hints->flags & USPosition)
	{
		hints->x = FSCREEN_MANGLE_USPOS_HINTS_MAGIC;
		hints->y = (short)screen;
	}
	else
	{
		hints->x = 0;
		hints->y = 0;
	}

	return;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen spec.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
int FScreenFetchMangledScreenFromUSPosHints(XSizeHints *hints)
{
	int screen;

	if ((hints->flags & USPosition) &&
	    hints->x == FSCREEN_MANGLE_USPOS_HINTS_MAGIC)
	{
		screen = hints->y;
	} else
		screen = 0;

	return screen;
}
