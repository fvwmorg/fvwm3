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
#include <X11/xpm.h> /* Has to be after Intrinsic.h gets included */

int save_colors = 0;
Display *dpy;
int screen;
Window root;
char *display_name = NULL;
void SetRootWindow(char *tline);
Pixmap rootImage = None;

int main(int argc, char **argv)
{
	Atom prop, type, e_prop;
	int format;
	unsigned long length, after;
	unsigned char *data;
	int i = 1;
	Bool FreeEsetroot = False;
	Bool Dummy = False;

	if (argc < 2)
	{
		fprintf(
			stderr, "fvwm-root version %s with support for: XBM "
#ifdef XPM
			"XPM "
#endif
#ifdef HAVE_PNG
			"PNG"
#endif
			"\n", VERSION);
		fprintf(stderr, "Usage: fvwm-root [-fe -np -d] file\n");
		fprintf(stderr, "Try Again\n");
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
		if (strcasecmp(argv[i], "-fe") == 0)
		{
			FreeEsetroot = True;
		}
		else if (strcasecmp(argv[i], "-d") == 0)
		{
			Dummy = True;
		}
		else
		{
			fprintf(
				stderr, "fvwm-root: unknown option '%s'\n",
				argv[i]);
		}
	}

	if (Dummy || strcasecmp(argv[argc-1], "-d") == 0)
	{
		Dummy = True;
	}
	else
	{
		SetRootWindow(argv[argc-1]);
	}

	prop = XInternAtom(dpy, "_XSETROOT_ID", False);
	(void)XGetWindowProperty(
		dpy, root, prop, 0L, 1L, True, AnyPropertyType,
		&type, &format, &length, &after, &data);
	if (type == XA_PIXMAP && format == 32 && length == 1 && after == 0)
	{
		XKillClient(dpy, *((Pixmap *)data));
	}

	if (FreeEsetroot)
	{
		if (data != NULL)
			XFree(data);
		e_prop = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
		(void)XGetWindowProperty(
			dpy, root, e_prop, 0L, 1L, True, AnyPropertyType,
			&type, &format, &length, &after, &data);
		if (type == XA_PIXMAP && format == 32 &&
			length == 1 && after == 0)
		{
			XKillClient(dpy, *((Pixmap *)data));
		}
		XDeleteProperty(dpy, root, e_prop);
	}

	if (!Dummy)
	{
		if (data != NULL)
			XFree(data);
		XSetCloseDownMode(dpy, RetainPermanent);
	}
	XChangeProperty(
		dpy, root, prop, XA_PIXMAP, 32, PropModeReplace,
		(unsigned char *) &rootImage, 1);
	XCloseDisplay(dpy);
	return 0;
}


void SetRootWindow(char *tline)
{

	Pixmap shapeMask, temp_pix;
	int w, h, depth;
	FvwmPictureFlags fpf;

	fpf.alloc_pixels = 0;
	fpf.alpha = 0;
	PictureInitCMap(dpy);
	if (!PImageLoadPixmapFromFile(
		dpy, root, tline, 0, &temp_pix, &shapeMask, NULL,
		&w, &h, &depth, 0, NULL, fpf))
	{
		fprintf(
			stderr,"[fvwm-root] failed to load image file '%s'\n",
			tline);
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
