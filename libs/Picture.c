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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/
/*
  Changed 02/12/97 by Dan Espen:
  - added routines to determine color closeness, for color use reduction.
  Some of the logic comes from pixy2, so the copyright is below.
*/
/*
 * Copyright 1996, Romano Giannetti. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 * Romano Giannetti - Dipartimento di Ingegneria dell'Informazione
 *                    via Diotisalvi, 2  PISA
 * mailto:romano@iet.unipi.it
 * http://www.iet.unipi.it/~romano
 *
 */

/****************************************************************************
 *
 * Routines to handle initialization, loading, and removing of xpm's or mono-
 * icon images.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

#include "fvwmlib.h"
#include "Colorset.h"
#include "Picture.h"
#include "FImageLoader.h"

static Picture *PictureList=NULL;

Picture *LoadPicture(Display *dpy, Window Root, char *path, int color_limit)
{
	return FImageLoadPictureFromFile(dpy, Root, path, color_limit);
}


Picture *GetPicture(Display *dpy, Window Root, char *ImagePath, char *name,
		    int color_limit)
{
	char *path = findImageFile( name, ImagePath, R_OK );
	Picture *p;

	if ( path == NULL )
	{
		return NULL;
	}
	p = LoadPicture(dpy, Root, path, color_limit);
	if ( p == NULL )
	{
		free(path);
	}

	return p;
}

Picture *CachePicture(Display *dpy, Window Root,
		      char *ImagePath, char *name, int color_limit)
{
	char *path;
	Picture *p=PictureList;

	/* First find the full pathname */
	if ( !(path=findImageFile(name,ImagePath,R_OK)) )
	{
		return NULL;
	}

	/* See if the picture is already cached */
	while (p)
	{
		register char *p1, *p2;

		for (p1=path, p2=p->name; *p1 && *p2; ++p1, ++p2)
		{
			if (*p1 != *p2)
			{
				break;
			}
		}

		/* If we have found a picture with the wanted name and stamp */
		if (!*p1 && !*p2 && !isFileStampChanged(&p->stamp, p->name))
		{
			p->count++; /* Put another weight on the picture */
			free(path);
			return p;
		}
		p=p->next;
	}

	/* Not previously cached, have to load it ourself. Put it first in list
	 */
	p=LoadPicture(dpy, Root, path, color_limit);
	if(p)
	{
		p->next=PictureList;
		PictureList=p;
	}
	else
	{
		free(path);
	}

	return p;
}

void DestroyPicture(Display *dpy, Picture *p)
{
	Picture *q=PictureList;

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
			XFreeColors(
				dpy, Pcmap, p->alloc_pixels, p->nalloc_pixels,
				0);
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

	/* Link it out of the list (it might not be there) */
	if(p==q) /* in head? simple */
	{
		PictureList=p->next;
	}
	else
	{
		while(q && q->next!=p) /* fast forward until end or found */
		{
			q=q->next;
		}
		if(q) /* not end? means we found it in there, possibly at end */
		{
			q->next=p->next; /* link around it */
		}
	}
	free(p);

	return;
}

Picture *LoadPictureFromPixmap(
	Display *dpy, Window Root, char *name, Pixmap pixmap, Pixmap mask,
	int width, int height)
{
	Picture *q;

	q = (Picture*)safemalloc(sizeof(Picture));
	memset(q, 0, sizeof(Picture));
	q->count = 1;
	q->name = name;
	q->next = NULL;
	q->stamp = pixmap;
	q->picture = pixmap;
	q->mask = mask;
	q->width = width;
	q->height = height;
	q->depth = Pdepth;
	q->alloc_pixels = 0;
	q->nalloc_pixels = 0;

	return q;
}

Picture *CachePictureFromPixmap(Display *dpy, Window Root,char *name,
				Pixmap pixmap, Pixmap mask,
				int width, int height)
{
	Picture *p=PictureList;

	/* See if the picture is already cached */
	for(;p!=NULL;p=p->next)
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
	p = LoadPictureFromPixmap(dpy, Root, name, pixmap, mask, width, height);
	if(p)
	{
		p->next=PictureList;
		PictureList=p;
	}

	return p;
}

Picture *fvwmlib_clone_picture(Picture *pic)
{
	if (pic != NULL)
	{
		pic->count++;
	}

	return pic;
}

