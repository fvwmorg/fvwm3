/****************************************************************************
 * This is an all new program to set the root window to an Xpm pixmap.
 * Copyright 1993, Rob Nation
 * You may use this file for anything you want, as long as the copyright
 * is kept intact. No guarantees of any sort are made in any way regarding
 * this program or anything related to it.
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <libs/fvwmlib.h>
#include <libs/Picture.h>

int save_colors = 0;
Display *dpy;
int screen;
Window root;
char *display_name = NULL;
void SetRootWindow(char *tline);
Pixmap rootImage = None;
Bool NoDither = False;
Bool Dither = False;
Bool NoColorLimit = False;
char *opt_color_limit = NULL;
Bool use_our_color_limit = False;

void usage(int verbose)
{
	fprintf(
		stderr, "fvwm-root version %s with support for: XBM"
#ifdef XPM
		", XPM"
#endif
#ifdef HAVE_PNG
		", PNG"
#endif
		"\n", VERSION);
	fprintf(stderr,
		"Usage: fvwm-root [ options ] file\n");
	if (verbose)
	{
		fprintf(stderr,
			"Options:\n"
			"\t--dither\n"
			"\t--no-dither\n"
			"\t--retain-pixmap\n"
			"\t--no-retain-pixmap\n"
			"\t--color-limit l\n"
			"\t--no-color-limit\n"
			"\t--dummy\n"
			"\t--no-dummy\n"
			"\t--help\n");
	}
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
			strcasecmp(argv[i], "--retain-pixmap") == 0 ||
			strcasecmp(argv[i], "-retain-pixmap") == 0)
		{
			RetainPixmap = True;
		}
		else if (
			strcasecmp(argv[i], "--no-retain-pixmap") == 0 ||
			strcasecmp(argv[i], "-no-retain-pixmap") == 0)
		{
			RetainPixmap = False;
		}
		else if (
			strcasecmp(argv[i], "-d") == 0 ||
			strcasecmp(argv[i], "--dummy") == 0 ||
			strcasecmp(argv[i], "-dummy") == 0)
		{
			Dummy = True;
		}
		else if (
			strcasecmp(argv[i], "--no-dummy") == 0 ||
			strcasecmp(argv[i], "-no-dummy") == 0)
		{
			Dummy = False;
		}
		else if (
			strcasecmp(argv[i], "--dither") == 0 ||
			strcasecmp(argv[i], "-dither") == 0)
		{
			Dither = True;
		}
		else if (
			strcasecmp(argv[i], "--no-dither") == 0 ||
			strcasecmp(argv[i], "-no-dither") == 0)
		{
			NoDither = True;
		}
		else if (
			strcasecmp(argv[i], "--color-limit") == 0 ||
			strcasecmp(argv[i], "-color-limit") == 0)
		{
			use_our_color_limit = True;
			if (i+1 < argc)
			{
				i++;
				CopyString(&opt_color_limit,argv[i]);
			}
		}
		else if (
			strcasecmp(argv[i], "--no-color-limit") == 0 ||
			strcasecmp(argv[i], "-no-color-limit") == 0)
		{
			NoColorLimit = True;
		}
		else if (
			strcasecmp(argv[i], "-h") == 0 ||
			strcasecmp(argv[i], "--help") == 0 ||
			strcasecmp(argv[i], "-help") == 0)
		{
			usage(1);
			exit(0);
		}
		else
		{
			fprintf(
				stderr, "fvwm-root: unknown option '%s'\n",
				argv[i]);
		}
	}

	if (Dummy ||
		strcasecmp(argv[argc-1], "-d") == 0 ||
		strcasecmp(argv[argc-1], "--dummy") == 0 ||
		strcasecmp(argv[argc-1], "-dummy") == 0)
	{
		Dummy = True;
	}
	else if (
		strcasecmp(argv[argc-1], "-h") == 0 ||
		strcasecmp(argv[argc-1], "--help") == 0 ||
		strcasecmp(argv[argc-1], "-help") == 0)
	{
		usage(1);
		exit(0);
	}
	else
	{
		SetRootWindow(argv[argc-1]);
	}

	prop = XInternAtom(dpy, "_XSETROOT_ID", False);
	(void)XGetWindowProperty(
		dpy, root, prop, 0L, 1L, True, AnyPropertyType,
		&type, &format, &length, &after, &data);
	if (type == XA_PIXMAP && format == 32 && length == 1 && after == 0 &&
	    *((Pixmap *)data) != None)
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
	    *((Pixmap *)data) != None)
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
		if (data != NULL)
			XFree(data);
		XSetCloseDownMode(dpy, RetainPermanent);
		if (e_prop == None)
			e_prop = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
		if (m_prop == None)
			m_prop = XInternAtom(dpy, "_XROOTPMAP_ID", False);
		XChangeProperty(
			dpy, root, e_prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *) &rootImage, 1);
		XChangeProperty(
			dpy, root, m_prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *) &rootImage, 1);
	}
	else
	{
		Pixmap dp = None;

		if (prop == None)
			prop = XInternAtom(dpy, "_XSETROOT_ID", False);
		XChangeProperty(
			dpy, root, prop, XA_PIXMAP, 32, PropModeReplace,
			(unsigned char *)  &dp, 1);
	}
	XCloseDisplay(dpy);
	return 0;
}


void SetRootWindow(char *tline)
{
	Pixmap shapeMask = None, temp_pix = None, alpha = None;
	int w, h, depth;
	int nalloc_pixels = 0;
	Pixel *alloc_pixels = NULL;
	char *file_path;
	FvwmPictureAttributes fpa;

	if (use_our_color_limit)
	{
		PictureInitCMapRoot(
			dpy, !NoColorLimit, opt_color_limit, True, True);
	}
	else
	{
		/* this use the default visual (not the fvwm one) as
		 * getenv("FVWM_VISUALID") is NULL in any case. But this use
		 * the same color limit than fvwm.
		 * This is "broken" when fvwm use depth <= 8 and a private
		 * color map (i.e., fvwm is started with the -visual{ID} option),
		 * because when fvwm use a private color map the default color
		 * limit is 244. There is no way to know here if
		 * getenv("FVWM_VISUALID") !=NULL.
		 * So, in this unfortunate case the user should use the
		 * --color-limit option */
		PictureInitCMap(dpy);
	}
	/* try built-in image path first */
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
		return;
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
}
