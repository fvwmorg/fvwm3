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

#include <X11/keysym.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <fvwmlib.h>
#include <Picture.h>


#ifdef XPM
static void c100_init_base_table (void);
static void c200_substitute_color(char **,int);
static void c300_color_to_rgb(char *, XColor *);
static double c400_distance(XColor *, XColor *);
#endif


static Picture *PictureList=NULL;
Bool Pdefault;
Visual *Pvisual;
Colormap Pcmap;
unsigned int Pdepth;
Display *Pdpy;            /* Save area for display pointer */

void InitPictureCMap(Display *dpy) {
  char *envp;

  Pdpy = dpy;
  /* if fvwm has not set this env-var it is using the default visual */
  envp = getenv("FVWM_VISUALID");
  if ((envp != NULL) && (strlen(envp) > 0)) {
    /* convert the env-vars to a visual and colormap */
    int viscount;
    XVisualInfo vizinfo, *xvi;

    sscanf(envp, "%lx", &vizinfo.visualid);
    xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
    Pvisual = xvi->visual;
    Pdepth = xvi->depth;
    sscanf(getenv("FVWM_COLORMAP"), "%lx", &Pcmap);
    Pdefault = False;
  } else {
    int screen = DefaultScreen(dpy);

    Pvisual = DefaultVisual(dpy, screen);
    Pdepth = DefaultDepth(dpy, screen);
    Pcmap = DefaultColormap(dpy, screen);
    Pdefault = True;
  }
}


static char* imagePath = FVWM_IMAGEPATH;

void SetImagePath( const char* newpath )
{
    static int need_to_free = 0;
    setPath( &imagePath, newpath, need_to_free );
    need_to_free = 1;
}

char* GetImagePath()
{
    return imagePath;
}

/****************************************************************************
 *
 * Find the specified image file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 ****************************************************************************/
char* findImageFile( const char* icon, const char* pathlist, int type )
{
    if ( pathlist == NULL )
	pathlist = imagePath;

    return searchPath( pathlist, icon, ".gz", type );
}


Picture *LoadPicture(Display *dpy,Window Root,char *path, int color_limit)
{
  int l;
  Picture *p;
#ifdef XPM
  XpmAttributes xpm_attributes;
  int rc;
  XpmImage	my_image = {0};
#endif

  p = (Picture*)safemalloc(sizeof(Picture));
  p->count = 1;
  p->name = path;
  p->next = NULL;

#ifdef XPM
  /* Try to load it as an X Pixmap first */
  xpm_attributes.visual = Pvisual;
  xpm_attributes.colormap = Pcmap;
  xpm_attributes.depth = Pdepth;
  xpm_attributes.closeness=40000; /* Allow for "similar" colors */
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmCloseness
			     | XpmVisual | XpmColormap | XpmDepth;

  rc = XpmReadFileToXpmImage(path, &my_image, NULL);
  if (rc == XpmSuccess) {
    color_reduce_pixmap(&my_image, color_limit);
    rc = XpmCreatePixmapFromXpmImage(dpy, Root, &my_image, &p->picture,
				     &p->mask, &xpm_attributes);
    if (rc == XpmSuccess) {
      p->width = my_image.width;
      p->height = my_image.height;
      p->depth = Pdepth;
      XpmFreeXpmImage(&my_image);
      return p;
    }
    XpmFreeXpmImage(&my_image);
  }
#endif

  /* If no XPM support, or XPM loading failed, try bitmap */
  if(XReadBitmapFile(dpy,Root,path,&p->width,&p->height,&p->picture,&l,&l)
     == BitmapSuccess)
    {
      p->depth = 0;
      p->mask = None;
      return p;
    }

  free(p);
  return NULL;
}


Picture *GetPicture(Display *dpy, Window Root,
		    char *ImagePath, char *name, int color_limit)
{
    char *path = findImageFile( name, ImagePath, R_OK );
    Picture *p;

    if ( path == NULL )
	return NULL;

    p = LoadPicture( dpy, Root, path, color_limit );
    if ( p == NULL )
	free(path);

    return p;
}


Picture *CachePicture(Display *dpy, Window Root,
		      char *ImagePath, char *name, int color_limit)
{
    char *path;
    Picture *p=PictureList;

    /* First find the full pathname */
    if( !(path=findImageFile(name,ImagePath,R_OK)) )
	return NULL;

    /* See if the picture is already cached */
    while(p)
    {
	register char *p1, *p2;

	for (p1=path, p2=p->name; *p1 && *p2; ++p1, ++p2)
	    if (*p1 != *p2)
		break;

	if(!*p1 && !*p2) /* We have found a picture with the wanted name */
	{
	    p->count++; /* Put another weight on the picture */
	    free(path);
	    return p;
	}
	p=p->next;
    }

    /* Not previously cached, have to load it ourself. Put it first in list */
    p=LoadPicture(dpy, Root, path, color_limit);
    if(p)
    {
	p->next=PictureList;
	PictureList=p;
    }
    else
	free(path);
    return p;
}


void DestroyPicture(Display *dpy, Picture *p)
{
  Picture *q=PictureList;

  if (!p) /* bag out if NULL */
    return;
  if(--(p->count)>0) /* Remove a weight, still too heavy? */
    return;

  /* Let it fly */
  if(p->name!=NULL)
    free(p->name);
  if(p->picture!=None)
    XFreePixmap(dpy,p->picture);
  if(p->mask!=None)
    XFreePixmap(dpy,p->mask);

  /* Link it out of the list (it might not be there) */
  if(p==q) /* in head? simple */
    PictureList=p->next;
  else
    {
      while(q && q->next!=p) /* fast forward until end or found */
	q=q->next;
      if(q) /* not end? means we found it in there, possibly at end */
	q->next=p->next; /* link around it */
    }
  free(p);
}


#ifdef XPM
/* This structure is used to quickly access the RGB values of the colors */
/* without repeatedly having to transform them.   */
typedef struct {
  char * c_color;	/* Pointer to the name of the color */
  XColor rgb_space;                     /* rgb color info */
} Color_Info;

/* First thing in base array are colors probably already in the color map
   because they have familiar names.
   I pasted them into a xpm and spread them out so that similar colors are
   spread out.
   Toward the end are some colors to fill in the gaps.
   Currently 61 colors in this list.
   */
static Color_Info base_array[] = {
  {"white"},
  {"black"},
  {"grey"},
  {"green"},
  {"blue"},
  {"red"},
  {"cyan"},
  {"yellow"},
  {"magenta"},
  {"DodgerBlue"},
  {"SteelBlue"},
  {"chartreuse"},
  {"wheat"},
  {"turquoise"},
  {"CadetBlue"},
  {"gray87"},
  {"CornflowerBlue"},
  {"YellowGreen"},
  {"NavyBlue"},
  {"MediumBlue"},
  {"plum"},
  {"aquamarine"},
  {"orchid"},
  {"ForestGreen"},
  {"lightyellow"},
  {"brown"},
  {"orange"},
  {"red3"},
  {"HotPink"},
  {"LightBlue"},
  {"gray47"},
  {"pink"},
  {"red4"},
  {"violet"},
  {"purple"},
  {"gray63"},
  {"gray94"},
  {"plum1"},
  {"PeachPuff"},
  {"maroon"},
  {"lavender"},
  {"salmon"},                           /* for peachpuff, orange gap */
  {"blue4"},                            /* for navyblue/mediumblue gap */
  {"PaleGreen4"},                       /* for forestgreen, yellowgreen gap */
  {"#AA7700"},                          /* brick, no close named color */
  {"#11EE88"},                          /* light green, no close named color */
  {"#884466"},                          /* dark brown, no close named color */
  {"#CC8888"},                          /* light brick, no close named color */
  {"#EECC44"},                          /* gold, no close named color */
  {"#AAAA44"},                          /* dull green, no close named color */
  {"#FF1188"},                          /* pinkish red */
  {"#992299"},                          /* purple */
  {"#CCFFAA"},                          /* light green */
  {"#664400"},                          /* dark brown*/
  {"#AADD99"},                          /* light green */
  {"#66CCFF"},                          /* light blue */
  {"#CC2299"},                          /* dark red */
  {"#FF11CC"},                          /* bright pink */
  {"#11CC99"},                          /* grey/green */
  {"#AA77AA"},                          /* purple/red */
  {"#EEBB77"}                           /* orange/yellow */
};

#define NColors (sizeof(base_array) / sizeof(Color_Info))

/* given an xpm, change colors to colors close to the
   subset above. */
void color_reduce_pixmap(XpmImage *image,int color_limit)
{
  int i;
  XpmColor *color_table_ptr;
  static char base_init = 'n';
  if (color_limit > 0) {                /* If colors to be limited */
    if (base_init == 'n') {             /* if base table not created yet */
      c100_init_base_table();           /* init the base table */
      base_init = 'y';                  /* remember that its set now. */
    }                                   /* end base table init */
    color_table_ptr = image->colorTable; /* start of xpm color table */
    for(i=0; i<image->ncolors; i++) {   /* all colors in the xpm */
      c200_substitute_color(&color_table_ptr->c_color,
                            color_limit); /* fix each one */
      color_table_ptr +=1;              /* counter for loop */
    }                                   /* end all colors in xpm */
  }                                     /* end colors limited */
  return;                               /* return, no rc! */
}

/* from the color names in the base table, calc rgbs */
static void c100_init_base_table ()
{
  int i;
  for (i=0; i<NColors; i++) {           /* change all base colors to numbers */
    c300_color_to_rgb(base_array[i].c_color, &base_array[i].rgb_space);
  }
}


/* Replace the color in my_color by the closest matching color
   from base_table */
static void c200_substitute_color(char **my_color, int color_limit)
{
  int i, limit, minind;
  double mindst=1e20;
  double dst;
  XColor rgb;          /* place to calc rgb for each color in xpm */

  if (!strcasecmp(*my_color,"none")) {
    return ;                        /* do not substitute the "none" color */
  }

  c300_color_to_rgb(*my_color, &rgb);  /* get rgb for a color in xpm */
  /* Loop over all base_array colors; find out which one is closest
     to my_color */
  minind = 0;                           /* Its going to find something... */
  limit = NColors;                      /* init to max */
  if (color_limit < NColors) {          /* can't do more than I have */
    limit = color_limit;                /* Do reduction using subset */
  }                                     /* end reducing limit */
  for(i=0; i < limit; i++) {            /* loop over base array */
    dst = c400_distance (&rgb, &base_array[i].rgb_space); /* distance */
    if (dst < mindst ) {              /* less than min and better than last */
      mindst=dst;                     /* new minimum */
      minind=i;                       /* save loc of new winner */
      if (dst <= 100) {               /* if close enough */
        break;                        /* done */
      }                               /* end close enough */
    }                                 /* end new low distance */
  }                                   /* end all base colors */
  /* Finally: replace the color string by the newly determined color string */
  free(*my_color);                      /* free old color */
  *my_color = safemalloc(strlen(base_array[minind].c_color) + 1); /* area for new color */
  strcpy(*my_color,base_array[minind].c_color); /* put it there */
  return;                             /* all done */
 }

static void c300_color_to_rgb(char *c_color, XColor *rgb_space)
{
  int rc;
  rc=XParseColor(Pdpy, Pcmap, c_color, rgb_space);
  if (rc==0) {
    fprintf(stderr,"color_to_rgb: can't parse color %s, rc %d\n", c_color, rc);
    return;
  }
}

/* A macro for squaring things */
#define SQUARE(X) ((X)*(X))
/* RGB Color distance sum of square of differences */
static double c400_distance(XColor *target_ptr, XColor *base_ptr)
{
  register double dst;
  dst = SQUARE((double)(base_ptr->red   - target_ptr->red  )/655.35)
    +   SQUARE((double)(base_ptr->green - target_ptr->green)/655.35)
    +   SQUARE((double)(base_ptr->blue  - target_ptr->blue )/655.35);
  return dst;
}
#endif /* XPM */

Pixel GetColor(char *name)
{
  XColor color;
  color.pixel = 0;
   if (!XParseColor (Pdpy, Pcmap, name, &color))
     fprintf(stderr, "Cannot parse color %s\n", name);
   else if(!XAllocColor (Pdpy, Pcmap, &color))
     fprintf(stderr, "Cannot alloc color %s\n", name);
  return color.pixel;
}
