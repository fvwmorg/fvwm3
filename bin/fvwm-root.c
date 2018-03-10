/* -*-c-*- */
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
#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Graphics.h"
#include "libs/Fsvg.h"

int save_colors = 0;
Display *dpy;
int screen;
Window root;
char *display_name = NULL;
Pixmap rootImage = None;
Bool NoDither = False;
Bool Dither = False;
Bool NoColorLimit = False;
int opt_color_limit = -1;
Bool use_our_color_limit = False;

void usage(int verbose)
{
	FILE *output = verbose ? stdout : stderr;
	fprintf(
		output, "fvwm-root version %s with support for: XBM"
#ifdef XPM
		", XPM"
#endif
#ifdef HAVE_PNG
		", PNG"
#endif
#ifdef HAVE_RSVG
		", SVG"
#endif
		"\n", VERSION);
	fprintf(output,
		"\nUsage: fvwm-root [ options ] file\n");
	if (verbose)
	{
		fprintf(output,
			"Options:\n"
			"\t--dither\n"
			"\t--no-dither\n"
			"\t--retain-pixmap\n"
			"\t--no-retain-pixmap\n"
			"\t--color-limit l\n"
			"\t--no-color-limit\n"
			"\t--dummy\n"
			"\t--no-dummy\n"
			"\t--help\n"
			"\t--version\n");
	}
}

int SetRootWindow(char *tline)
{
	Pixmap shapeMask = None, temp_pix = None, alpha = None;
	int w, h;
	int depth;
	int nalloc_pixels = 0;
	Pixel *alloc_pixels = NULL;
	char *file_path;
	FvwmPictureAttributes fpa;

	if (use_our_color_limit)
	{
		PictureColorLimitOption colorLimitop = {-1, -1, -1, -1, -1};
		colorLimitop.color_limit = opt_color_limit;
		PictureInitCMapRoot(
			dpy, !NoColorLimit, &colorLimitop, True, True);
	}
	else
	{
		/* this use the default visual (not the fvwm one) as
		 * getenv("FVWM_VISUALID") is NULL in any case. But this use
		 * the same color limit than fvwm.
		 * This is "broken" when fvwm use depth <= 8 and a private
		 * color map (i.e., fvwm is started with the -visual{ID}
		 * option), because when fvwm use a private color map the
		 * default color limit is 244. There is no way to know here if
		 * getenv("FVWM_VISUALID") !=NULL.
		 * So, in this unfortunate case the user should use the
		 * --color-limit option */
		PictureInitCMap(dpy);
	}
	flib_init_graphics(dpy);
	/* try built-in image path first, but not before pwd */
	PictureSetImagePath(".:+");
	file_path = PictureFindImageFile(tline, NULL, R_OK);

	if (file_path == NULL)
	{
		file_path = tline;
	}
	fpa.mask = FPAM_NO_ALLOC_PIXELS | FPAM_NO_ALPHA;
	if (Pdepth <= 8 && !NoDither)
	{
		fpa.mask |= FPAM_DITHER;
	}
	else if (Pdepth <= 16 && Dither)
	{
		fpa.mask |= FPAM_DITHER;
	}
	if (NoColorLimit)
	{
		fpa.mask |= FPAM_NO_COLOR_LIMIT;
	}
	if (!PImageLoadPixmapFromFile(
		dpy, root, file_path, &temp_pix, &shapeMask, &alpha,
		&w, &h, &depth, &nalloc_pixels, &alloc_pixels, 0, fpa))
	{
		fprintf(
			stderr, "[fvwm-root] failed to load image file '%s'\n",
			tline);
		return -1;
	}
	if (depth == Pdepth)
	{
		rootImage = temp_pix;
	}
	else
	{
		XGCValues gcv;
		GC gc;

		gcv.background= WhitePixel(dpy, screen);
		gcv.foreground= BlackPixel(dpy, screen);
		gc = fvwmlib_XCreateGC(
			dpy, root, GCForeground | GCBackground, &gcv);
		rootImage = XCreatePixmap(dpy, root, w, h, Pdepth);
		XCopyPlane(dpy, temp_pix, rootImage, gc, 0, 0, w, h, 0, 0, 1);
		XFreePixmap(dpy, temp_pix);
		XFreeGC(dpy, gc);
	}
	XSetWindowBackgroundPixmap(dpy, root, rootImage);
	save_colors = 1;
	XClearWindow(dpy, root);

	return 0;
}

int main(int argc, char **argv)
{
	Atom prop = None;
	Atom e_prop = None;
	Atom m_prop = None;
	Atom type;
	int format;
	unsigned long length, after;
	unsigned char *data;
	int i = 1;
	Bool e_killed = False;
	Bool Dummy = False;
	Bool RetainPixmap = False;

	if (argc < 2)
	{
		usage(0);
		fprintf(stderr, "Nothing to do, try again.\n");
		exit(1);
	}
	dpy = XOpenDisplay(display_name);
	if (!dpy)
	{
		fprintf(
			stderr, "fvwm-root: unable to open display '%s'\n",
			XDisplayName (display_name));
		exit(2);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	for (i = 1; i < argc - 1; i++)
	{
		if (
			strcasecmp(argv[i], "-r") == 0 ||
			strcasecmp(argv[i], "--retain-pixmap") == 0)
		{
			RetainPixmap = True;
		}
		else if (
			strcasecmp(argv[i], "--no-retain-pixmap") == 0)
		{
			RetainPixmap = False;
		}
		else if (
			strcasecmp(argv[i], "-d") == 0 ||
			strcasecmp(argv[i], "--dummy") == 0)
		{
			Dummy = True;
		}
		else if (
			strcasecmp(argv[i], "--no-dummy") == 0)
		{
			Dummy = False;
		}
		else if (
			strcasecmp(argv[i], "--dither") == 0)
		{
			Dither = True;
		}
		else if (
			strcasecmp(argv[i], "--no-dither") == 0)
		{
			NoDither = True;
		}
		else if (
			strcasecmp(argv[i], "--color-limit") == 0)
		{
			use_our_color_limit = True;
			if (i+1 < argc)
			{
				i++;
				opt_color_limit = atoi(argv[i]);
			}
		}
		else if (
			strcasecmp(argv[i], "--no-color-limit") == 0)
		{
			NoColorLimit = True;
		}
		else if (
			strcasecmp(argv[i], "-h") == 0 ||
			strcasecmp(argv[i], "-?") == 0 ||
			strcasecmp(argv[i], "--help") == 0)
		{
			usage(1);
			exit(0);
		}
		else if (
			strcasecmp(argv[i], "-V") == 0 ||
			strcasecmp(argv[i], "--version") == 0)
		{
			fprintf(stdout, "%s\n", VERSION);
			exit(0);
		}
		else
		{
			fprintf(
				stderr, "fvwm-root: unknown option '%s'\n",
				argv[i]);
			fprintf(
				stderr, "Run '%s --help' to get the usage.\n",
				argv[0]);
			exit(1);
		}
	}

	if (
		Dummy ||
		strcasecmp(argv[argc-1], "-d") == 0 ||
		strcasecmp(argv[argc-1], "--dummy") == 0)
	{
		Dummy = True;
	}
	else if (
		strcasecmp(argv[argc-1], "-h") == 0 ||
		strcasecmp(argv[argc-1], "-?") == 0 ||
		strcasecmp(argv[argc-1], "--help") == 0)
	{
		usage(1);
		exit(0);
	}
	else if (
		strcasecmp(argv[argc-1], "-V") == 0 ||
		strcasecmp(argv[argc-1], "--version") == 0)
	{
		fprintf(stdout, "%s\n", VERSION);
		exit(0);
	}
	else
	{
		int rc;

		rc = SetRootWindow(argv[argc-1]);
		if (rc == -1)
		{
			exit(1);
		}
	}

	prop = XInternAtom(dpy, "_XSETROOT_ID", False);
	(void)XGetWindowProperty(
		dpy, root, prop, 0L, 1L, True, AnyPropertyType,
		&type, &format, &length, &after, &data);
	if (type == XA_PIXMAP && format == 32 && length == 1 && after == 0 &&
	    data != NULL && (Pixmap)(*(long *)data) != None)
	{
		XKillClient(dpy, *((Pixmap *)data));
	}

	if (data != NULL)
		XFree(data);
	e_prop = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
	(void)XGetWindowProperty(
		dpy, root, e_prop, 0L, 1L, True, AnyPropertyType,
		&type, &format, &length, &after, &data);
	if (type == XA_PIXMAP && format == 32 && length == 1 && after == 0 &&
	    data != NULL && (Pixmap)(*(long *)data) != None)
	{
		e_killed = True;
		XKillClient(dpy, *((Pixmap *)data));
	}
	if (e_killed && !Dummy)
	{
		m_prop = XInternAtom(dpy, "_XROOTPMAP_ID", False);
		XDeleteProperty(dpy, root, m_prop);
	}

	if (RetainPixmap && !Dummy)
	{
		long prop;

		prop = rootImage;
		if (data != NULL)
			XFree(data);
		XSetCloseDownMode(dpy, RetainPermanent);
		if (e_prop == None)
			e_prop = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
		if (m_prop == None)
			m_prop = XInternAtom(dpy, "_XROOTPMAP_ID", False);
		XChangeProperty(
			dpy, root, e_prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *) &prop, 1);
		XChangeProperty(
			dpy, root, m_prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *) &prop, 1);
	}
	else
	{
		long dp = (long)None;

		if (prop == None)
			prop = XInternAtom(dpy, "_XSETROOT_ID", False);
		XChangeProperty(
			dpy, root, prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *)  &dp, 1);
	}
	XCloseDisplay(dpy);

	return 0;
}
