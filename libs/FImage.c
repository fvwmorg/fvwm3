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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include "libs/fvwmlib.h"
#include "libs/safemalloc.h"
#include "libs/PictureBase.h"
#include "libs/FImage.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

int FShmMajorOpCode = -10000;
int FShmEventBase = -10000;
int FShmErrorBase = -10000;
Bool FShmInitialized = False;
Bool FShmImagesSupported = False;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void FShmInit(Display *dpy)
{
	if (FShmInitialized)
	{
		return;
	}

	FShmInitialized = True;

	if (1 || !XShmSupport)
	{
		return;
	}
	FShmImagesSupported = XQueryExtension(
		dpy, "MIT-SHM", &FShmMajorOpCode, &FShmEventBase,
		&FShmErrorBase);
}

static void FShmSafeCreateImage(
	Display *dpy, FImage *fim, Visual *visual, unsigned int depth,
	int format, unsigned int width, unsigned int height)
{
	Bool error = False;

	if (!XShmSupport)
	{
		return;
	}
	fim->shminfo = (FShmSegmentInfo *)safecalloc(
		1, sizeof(FShmSegmentInfo));
	if (!(fim->im = FShmCreateImage(
		dpy, visual, depth, format, NULL, fim->shminfo,
		width, height)))
	{
		error = True;
		goto bail;
	}
	fim->shminfo->shmid = Fshmget(
		IPC_PRIVATE,
		fim->im->bytes_per_line * fim->im->height,
		IPC_CREAT|0777);
	if (fim->shminfo->shmid == 0)
	{
		error = True;
		goto bail;
	}
	fim->shminfo->shmaddr = fim->im->data = Fshmat(
		fim->shminfo->shmid, 0, 0);
	if (fim->shminfo->shmaddr == NULL)
	{
		error = True;
		goto bail;
	}
	fim->shminfo->readOnly = False;
	if (!FShmAttach (dpy, fim->shminfo))
	{
		error = True;
		goto bail;;
	}
	XSync(dpy, False);

 bail:
	if (error)
	{
		if (fim->im)
		{
			XDestroyImage (fim->im);
			fim->im = NULL;
		}
		if (fim->shminfo->shmaddr)
		{
			Fshmdt(fim->shminfo->shmaddr);
		}
		if (fim->shminfo->shmid)
		{
			Fshmctl(fim->shminfo->shmid, IPC_RMID, 0);
		}
		free(fim->shminfo);
		fim->shminfo = NULL;
	}
}

/* ---------------------------- interface functions ------------------------ */

FImage *FCreateFImage (
	Display *dpy, Visual *visual, unsigned int depth, int format,
	unsigned int width, unsigned int height)
{
	FImage *fim;

	FShmInit(dpy);

	fim = (FImage *)safemalloc(sizeof(FImage));
	fim->im = NULL;
	fim->shminfo = NULL;

	if (XShmSupport && FShmImagesSupported)
	{
		FShmSafeCreateImage(
			dpy, fim, visual, depth, format, width, height);
	}

	if(!fim->im)
	{
		if ((fim->im = XCreateImage(
			dpy, visual, depth, ZPixmap, 0, 0, width, height,
			Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0)))
		{
			fim->im->data = safemalloc(
				fim->im->bytes_per_line * height);
		}
		else
		{
			free(fim);
		}
	}

	return fim;
}

FImage *FGetFImage(
	Display *dpy, Drawable d, Visual *visual,
	unsigned int depth, int x, int y, unsigned int width,
	unsigned int height, unsigned long plane_mask, int format)
{
	FImage *fim;

	FShmInit(dpy);

	fim = (FImage *)safemalloc(sizeof(FImage));
	fim->im = NULL;
	fim->shminfo = NULL;

	if (XShmSupport && FShmImagesSupported)
	{
		FShmSafeCreateImage(
			dpy, fim, visual, depth, format, width, height);
		if (fim->im)
		{
			FShmGetImage(
				dpy, d, fim->im, x, y, plane_mask);
		}
	}

	if (!fim->im)
	{
		fim->im = XGetImage(
			dpy, d, x, y, width, height, plane_mask, format);
	}

	return fim;
}

void FPutFImage(
	Display *dpy, Drawable d, GC gc, FImage *fim, int src_x, int src_y,
	int dest_x, int dest_y, unsigned int width, unsigned int height)
{

	if (fim->shminfo)
	{
		if (FShmPutImage(
			dpy, d, gc, fim->im, src_x, src_y, dest_x, dest_y, width,
			height, False))
		{
		}
	}
	else
	{
		XPutImage(
			dpy, d, gc, fim->im, src_x, src_y, dest_x, dest_y, width,
			height);
	}
}

void FDestroyFImage(Display *dpy, FImage *fim)
{
	if (fim->shminfo)
	{
		if (FShmDetach(dpy, fim->shminfo))
		{
		}
	}
	XDestroyImage (fim->im);
	if (fim->shminfo)
	{
		Fshmdt(fim->shminfo->shmaddr);
		Fshmctl(fim->shminfo->shmid, IPC_RMID, 0);
		free(fim->shminfo);
	}
	free(fim);
}

