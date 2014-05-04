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
/*
  Changed 02/12/97 by Dan Espen:
  - added routines to determine color closeness, for color use reduction.
  Some of the logic comes from pixy2, so the copyright is below.
*/

/*
 *
 * Routines to handle initialization, loading, and removing of xpm's or mono-
 * icon images.
 *
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#include "ftime.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

#include "fvwmlib.h"
#include "System.h"
#include "Colorset.h"
#include "Picture.h"
#include "PictureUtils.h"
#include "Fsvg.h"

static FvwmPicture *FvwmPictureList=NULL;

FvwmPicture *PGetFvwmPicture(
	Display *dpy, Window win, char *ImagePath, const char *name,
	FvwmPictureAttributes fpa)
{
	char *path = PictureFindImageFile(name, ImagePath, R_OK);
	FvwmPicture *p;

	if (path == NULL)
	{
		return NULL;
	}
	p = PImageLoadFvwmPictureFromFile(dpy, win, path, fpa);
	if (p == NULL)
	{
		free(path);
	}

	return p;
}

void PFreeFvwmPictureData(FvwmPicture *p)
{
	if (!p)
	{
		return;
	}
	if (p->alloc_pixels != NULL)
	{
		free(p->alloc_pixels);
	}
	if(p->name!=NULL)
	{
		free(p->name);
	}
	free(p);

	return;
}

FvwmPicture *PCacheFvwmPicture(
	Display *dpy, Window win, char *ImagePath, const char *name,
	FvwmPictureAttributes fpa)
{
	char *path;
	char *real_path;
	FvwmPicture *p = FvwmPictureList;

	/* First find the full pathname */
	if ((path = PictureFindImageFile(name, ImagePath, R_OK)) == NULL)
	{
		return NULL;
	}
        /* Remove any svg rendering options from real_path */
	if (USE_SVG && *path == ':' &&
	    (real_path = strchr(path + 1, ':')))
	{
		real_path++;
	}
	else
	{
		real_path = path;
	}

	/* See if the picture is already cached */
	while (p)
	{
		register char *p1, *p2;

		for (p1 = path, p2 = p->name; *p1 && *p2; ++p1, ++p2)
		{
			if (*p1 != *p2)
			{
				break;
			}
		}

		/* If we have found a picture with the wanted name and stamp */
		if (!*p1 && !*p2 && !isFileStampChanged(&p->stamp, real_path)
		    && PICTURE_FPA_AGREE(p,fpa))
		{
			p->count++; /* Put another weight on the picture */
			free(path);
			return p;
		}
		p=p->next;
	}

	/* Not previously cached, have to load it ourself. Put it first in list
	 */
	p = PImageLoadFvwmPictureFromFile(dpy, win, path, fpa);
	if(p)
	{
		p->next=FvwmPictureList;
		FvwmPictureList=p;
	}
	else
	{
		free(path);
	}

	return p;
}

void PDestroyFvwmPicture(Display *dpy, FvwmPicture *p)
{
	FvwmPicture *q = FvwmPictureList;

	if (!p)
	{
		return;
	}
	/* Remove a weight */
	if(--(p->count)>0)
	{
		/* still too heavy? */
		return;
	}

	/* Let it fly */
	if (p->alloc_pixels != NULL)
	{
		if (p->nalloc_pixels != 0)
		{
			PictureFreeColors(
				dpy, Pcmap, p->alloc_pixels, p->nalloc_pixels,
				0, p->no_limit);
		}
		free(p->alloc_pixels);
	}
	if(p->name!=NULL)
	{
		free(p->name);
	}
	if(p->picture!=None)
	{
		XFreePixmap(dpy,p->picture);
	}
	if(p->mask!=None)
	{
		XFreePixmap(dpy,p->mask);
	}
	if(p->alpha != None)
	{
		XFreePixmap(dpy, p->alpha);
	}
	/* Link it out of the list (it might not be there) */
	if(p==q) /* in head? simple */
	{
		FvwmPictureList = p->next;
	}
	else
	{
		while(q && q->next!=p) /* fast forward until end or found */
		{
			q = q->next;
		}
		if(q) /* not end? means we found it in there, possibly at end */
		{
			q->next = p->next; /* link around it */
		}
	}
	free(p);

	return;
}

FvwmPicture *PLoadFvwmPictureFromPixmap(
	Display *dpy, Window win, char *name, Pixmap pixmap,
	Pixmap mask, Pixmap alpha, int width, int height, int nalloc_pixels,
	Pixel *alloc_pixels, int no_limit)
{
	FvwmPicture *q;

	q = xcalloc(1, sizeof(FvwmPicture));
	q->count = 1;
	q->name = name;
	q->next = NULL;
	q->stamp = pixmap;
	q->picture = pixmap;
	q->mask = mask;
	q->alpha = alpha;
	q->width = width;
	q->height = height;
	q->depth = Pdepth;
	q->nalloc_pixels = nalloc_pixels;
	q->alloc_pixels = alloc_pixels;
	q->no_limit = no_limit;
	return q;
}

FvwmPicture *PCacheFvwmPictureFromPixmap(
	Display *dpy, Window win, char *name, Pixmap pixmap,
	Pixmap mask, Pixmap alpha, int width, int height, int nalloc_pixels,
	Pixel *alloc_pixels, int no_limit)
{
	FvwmPicture *p = FvwmPictureList;

	/* See if the picture is already cached */
	for(; p != NULL; p = p->next)
	{
#if 0
		/* at th present time no good way to cache a pixmap */
		if (!strcmp(p->name,name))
		{
			p->count++;
			return p;
		}
#endif
	}

	/* Not previously cached, have to load. Put it first in list */
	p = PLoadFvwmPictureFromPixmap(
		dpy, win, name, pixmap, mask, alpha, width, height,
		nalloc_pixels, alloc_pixels, no_limit);
	if(p)
	{
		p->next = FvwmPictureList;
		FvwmPictureList = p;
	}

	return p;
}

FvwmPicture *PCloneFvwmPicture(FvwmPicture *pic)
{
	if (pic != NULL)
	{
		pic->count++;
	}

	return pic;
}

void PicturePrintImageCache(int verbose)
{
	FvwmPicture *p;
	unsigned int count = 0;
	unsigned int hits = 0;
	unsigned int num_alpha = 0;
	unsigned int num_mask = 0;

	fflush(stderr);
	fflush(stdout);
	fprintf(stderr, "fvwm info on Image cache:\n");

	for (p = FvwmPictureList; p != NULL; p = p->next)
	{
		int num_pixmaps = 1;
		if (p->mask != None)
		{
			num_mask++;
			num_pixmaps++;
		}
		if (p->alpha != None)
		{
			num_alpha++;
			num_pixmaps++;
		}
		if (verbose > 0)
		{
			fprintf(stderr,
				"Image: %s (%d pixmaps; used %d times)\n",
				p->name, num_pixmaps, p->count);
		}
		count++;
		hits += p->count-1;
	}

	fprintf(stderr, "%u images in cache (%d reuses) "
		"(%u masks, %u alpha channels => %u pixmaps)\n",
		count, hits, num_mask, num_alpha, count + num_mask + num_alpha);
	fflush(stderr);
}
