/* Copyright (C) 1999 Joey Shutup.
 *
 * http://www.streetmap.co.uk/streetmap.dll?postcode2map?BS24+9TZ
 *
 * No guarantees or warranties or anything are provided or implied in any way
 * whatsoever.  Use this program at your own risk.  Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 */
/*
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Intrinsic.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"

/* Globals */
static Display *dpy;
static Window win;			/* need a window to create pixmaps */
static GC gc;				/* ditto */
static char *name;
static int namelen;
static int color_limit = 0;
static int fd[2];			/* communication pipes */
static Bool privateCells = False;	/* set when read/write colors used */
static Bool sharedCells = False;	/* set after a shared color is used */
static int nSets = 0;			/* how many read/write sets allocated */

/* forward declarations */
static RETSIGTYPE signal_handler(int signal);
static void set_signals(void);
static int error_handler(Display *dpy, XErrorEvent *e);
static void parse_config(void);
static void parse_config_line(char *line);
static void parse_message_line(char *line);
static void main_loop(void) __attribute__((__noreturn__));
static void parse_colorset(char *line);

int main(int argc, char **argv)
{
  XSetWindowAttributes xswa;
  XGCValues xgcv;

  name = GetFileNameFromPath(argv[0]);
  namelen = strlen(name);

  set_signals();

  /* try to make sure it is fvwm that spawned this module */
  if (argc != 6) {
    fprintf(stderr, "%s "VERSION" should only be executed by fvwm!\n", name);
    exit(1);
  }

  /* note the communication pipes */
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* open the display and initialize the picture library */
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr,"%s: can't open display %s", name, XDisplayName(NULL));
    exit (1);
  }
  InitPictureCMap(dpy);

  /* This module allocates resouces that othe rmodules may rely on.
   * Set the closedown mode so that the pixmaps and colors are not freed
   * by the server if this module dies
   */
  XSetCloseDownMode(dpy, RetainTemporary);

  /* create a window to work in */
  xswa.background_pixmap = None;
  xswa.border_pixel = 0;
  xswa.colormap = Pcmap;
  win = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)), -2, -2, 2, 2, 0,
		      Pdepth, InputOutput, Pvisual,
		      CWBackPixmap | CWBorderPixel | CWColormap, &xswa);

  /* create a GC */
  gc = XCreateGC(dpy, win, 0, &xgcv);

  /* die horribly if either win or gc could not be created */
  XSync(dpy, False);

  /* install a non-fatal error handler */
  XSetErrorHandler(error_handler);

  /* get the initial configuration options */
  parse_config();

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  /* garbage collect */
  alloca(0);

  /* just in case any previous FvwmTheme has died and left pixmaps dangling.
   * This might be overkill but any other method must ensure that fvwm doesn't
   * get killed (it can be the owner of the pixels in colorset 0)
   */
  XKillClient(dpy, AllTemporary);

  /* sit around waiting for something to do */
  main_loop();
}

static void main_loop(void)
{
  FvwmPacket *packet;

  while (True) {
    /* garbage collect */
    alloca(0);

    /* wait for an instruction from fvwm */
    packet = ReadFvwmPacket(fd[1]);
    if (!packet)
      exit(0);

    /* handle dynamic config lines and text messages */
    switch (packet->type) {
    case M_CONFIG_INFO:
      parse_config_line((char *)&packet->body[3]);
      break;
    case M_STRING:
      parse_message_line((char *)&packet->body[3]);
      break;
    }
  }
}

/* config options, the first NULL is replaced with *FvwmThemeColorset */
/* the second NULL is replaced with *FvwmThemeReadWriteColors */
/* the "Colorset" config option is what gets reflected by fvwm */
static char *config_options[] =
{
  "ImagePath",
  "ColorLimit",
  "Colorset",
  NULL,
  NULL,
  NULL
};

static void parse_config_line(char *line)
{
  char *rest;

  switch(GetTokenIndex(line, config_options, -1, &rest))
  {
  case 0: /* ImagePath */
    SetImagePath(rest);
    break;
  case 1: /* ColorLimit */
    sscanf(rest, "%d", &color_limit);
    break;
  case 2: /* Colorset */
    /* got a colorset config line from fvwm, this may be due to FvwmTheme
     * sending one in or it may be another module (or fvwm itself)
     * if using read/write cells ignore it so the pixels are not forgotten
     * if not load the colorset and free any pixels/pixmaps that differ */
    if (!privateCells)
      LoadColorsetAndFree(rest);
    break;
  case 3: /* *FvwmThemColorset */
    parse_colorset(rest);
    break;
  case 4: /* *FvwmThemeReadWriteColors */
    if (sharedCells)
      fprintf(stderr, "%s: must come first, already allocated shared pixels\n",
	      config_options[4]);
    else if (Pvisual->class != PseudoColor)
      fprintf(stderr, "%s: only works with PseudoColor visuals\n",
	      config_options[4]);
    else
      privateCells = True;
  }
}

static char *get_simple_color(
  char *string, char **color, colorset_struct *cs, int supplied_color,
  int special_flag, char *special_string)
{
  char *rest;

  if (*color)
  {
    free(*color);
    *color = NULL;
  }
  rest = GetNextToken(string, color);
  if (*color)
  {
    if (special_string && StrEquals(*color, special_string))
    {
      free(*color);
      *color = NULL;
      cs->color_flags |= special_flag;
      cs->color_flags &= ~supplied_color;
    }
    else
    {
      cs->color_flags |= supplied_color;
      cs->color_flags &= ~special_flag;
    }
  }
  else
    cs->color_flags &= ~(supplied_color | special_flag);

  return rest;
}

static char *csetopts[] =
{
  "Foreground",
  "Fore",
  "fg",

  "Background",
  "Back",
  "bg",

  "Hilight",
  "Hilite",
  "hi",

  "Shadow",
  "Shade",
  "sh",

  /* these strings are used inside the cases in the switch below! */
  "Pixmap",
  "TiledPixmap",
  "AspectPixmap",

  /* these strings are used inside the cases in the switch below! */
  "Shape",
  "TiledShape",
  "AspectShape",

  NULL
};

/* translate a colorset spec into a colorset structure */
static void parse_colorset(char *line)
{
  int n;
  int i;
  int w;
  int h;
  colorset_struct *cs;
  char *token;
  char *optstring;
  char *args;
  char *option;
  char type;
  char *fg = NULL;
  char *bg = NULL;
  char *hi = NULL;
  char *sh = NULL;
  Picture *picture;
  Bool have_pixels_changed = False;
  Bool has_fg_changed = False;
  Bool has_bg_changed = False;
  Bool has_sh_changed = False;
  Bool has_hi_changed = False;
  Bool has_pixmap_changed = False;
  Bool has_shape_changed = False;
  XColor color;
  XGCValues xgcv;
  static GC mono_gc = None;

  /* find out which colorset and make sure it exists */
  if (GetIntegerArguments(line, &line, &n, 1) != 1)
    return;
  if (n < 0)
  {
    fprintf(stderr, "%s: colorset number must be zero or positive\n", name);
    return;
  }
  cs = AllocColorset(n);

  /* grab cells */
  if (privateCells) {
    while (nSets <= n) {
      have_pixels_changed = True;
      XAllocColorCells(dpy, Pcmap, False, NULL, 0, &Colorset[nSets].fg, 4);
      nSets++;
    }
  }

  /* ---------- Parse the options ---------- */
  while (line && *line)
  {
    /* Read next option specification (delimited by a comma or \0). */
    line = GetQuotedString(line, &optstring, ",", NULL, NULL, NULL);
    if (!optstring)
      break;
    args = GetNextToken(optstring, &option);
    if (!option)
    {
      free(optstring);
      break;
    }

    switch((i = GetTokenIndex(option, csetopts, 0, NULL)))
    {
    case 0: /* Foreground */
    case 1: /* Fore */
    case 2: /* fg */
      get_simple_color(args, &fg, cs, FG_SUPPLIED, FG_CONTRAST, "contrast");
      has_fg_changed = True;
      break;
    case 3: /* Background */
    case 4: /* Back */
    case 5: /* bg */
      get_simple_color(args, &bg, cs, BG_SUPPLIED, BG_AVERAGE, "average");
      has_bg_changed = True;
      break;
    case 6: /* Hilight */
    case 7: /* Hilite */
    case 8: /* hi */
      get_simple_color(args, &hi, cs, HI_SUPPLIED, 0, NULL);
      has_hi_changed = True;
      break;
    case 9: /* Shadow */
    case 10: /* Shade */
    case 11: /* sh */
      get_simple_color(args, &sh, cs, SH_SUPPLIED, 0, NULL);
      has_sh_changed = True;
      break;
    case 12: /* TiledPixmap */
    case 13: /* Pixmap */
    case 14: /* AspectPixmap */
      has_pixmap_changed = True;
      if (cs->pixmap)
      {
	XFreePixmap(dpy, cs->pixmap);
	cs->pixmap = None;
      }
      if (cs->mask)
      {
	XFreePixmap(dpy, cs->mask);
	cs->mask = None;
      }
      /* set the flags */
      if (csetopts[i][0] == 'T')
	cs->pixmap_type = PIXMAP_TILED;
      else if (csetopts[i][0] == 'A')
	cs->pixmap_type = PIXMAP_STRETCH_ASPECT;
      else
	cs->pixmap_type = PIXMAP_STRETCH;

      /* read filename */
      token = PeekToken(args, &args);
      if (!token)
	break;
      /* load the file using the color reduction routines in Picture.c */
      picture = GetPicture(dpy, win, NULL, token, color_limit);
      if (!picture)
	fprintf(stderr, "%s: can't load picture %s\n", name, token);
      if (!picture)
	break;
      /* don't try to be smart with bitmaps */
      if (picture->depth != Pdepth) {
	fprintf(stderr, "%s: bitmaps not supported\n", name);
	DestroyPicture(dpy, picture);
	break;
      }
      /* copy the picture pixmap, then destroy the picture
       * could cache picture but it would require an extension to the struct
       * and I don't expect the same pixmap to be re-used very often */
      cs->width = picture->width;
      cs->height = picture->height;
      cs->pixmap = XCreatePixmap(dpy, win, cs->width, cs->height, Pdepth);

      if (cs->pixmap)
      {
	/* we can't handle the mask here because we don't know the background
	 * colour yet. */
	XCopyArea(dpy, picture->picture, cs->pixmap, gc, 0, 0, cs->width,
		  cs->height, 0, 0);
	if (picture->mask != None)
	{
	  cs->mask = XCreatePixmap(dpy, win, cs->width, cs->height, 1);
	  if (cs->mask)
	  {
	    if (mono_gc == None)
	    {
	      xgcv.foreground = 1;
	      xgcv.background = 0;
	      /* create a gc for 1 bit depth */
	      mono_gc = XCreateGC(
		dpy, picture->mask, GCForeground|GCBackground, &xgcv);
	    }
	    XCopyArea(dpy, picture->mask, cs->mask, mono_gc, 0, 0,
		      cs->width, cs->height, 0, 0);
	    /* Invert the mask. We use it to draw the background. */
	    XSetFunction(dpy, mono_gc, GXinvert);
	    XFillRectangle(
	      dpy, cs->mask, mono_gc, 0, 0, cs->width, cs->height);
	    XSetFunction(dpy, mono_gc, GXcopy);
	  }
	}
      }
      DestroyPicture(dpy, picture);
      picture = None;
      break;
    case 15: /* Shape */
    case 16: /* TiledShape */
    case 17: /* AspectShape */
#ifdef SHAPE
      /* read filename */
      token = PeekToken(args, &args);
      has_shape_changed = True;
      if (cs->shape_mask)
      {
	XFreePixmap(dpy, cs->shape_mask);
	cs->shape_mask = None;
      }
      /* set the flags */
      if (csetopts[i][0] == 'T')
	cs->shape_type = SHAPE_TILED;
      else if (csetopts[i][0] == 'A')
	cs->shape_type = SHAPE_STRETCH_ASPECT;
      else
	cs->shape_type = SHAPE_STRETCH;

      /* try to load the shape mask */
      if (token)
      {
	/* load the shape mask */
	picture = GetPicture(dpy, win, NULL, token, color_limit);
	if (!picture)
	  fprintf(stderr, "%s: can't load picture %s\n", name, token);
	else if (picture->depth != 1 && picture->mask == None)
	{
	  fprintf(stderr, "%s: shape pixmap must be of depth 1\n", name);
	  DestroyPicture(dpy, picture);
	}
	else
	{
	  Pixmap mask;

	  /* okay, we have what we want */
	  if (picture->mask != None)
	    mask = picture->mask;
	  else
	    mask = picture->picture;
	  cs->shape_width = picture->width;
	  cs->shape_height = picture->height;

	  if (mask != None)
	  {
	    cs->shape_mask = XCreatePixmap(
	      dpy, mask, picture->width, picture->height, 1);
	    if (cs->shape_mask != None)
	    {
	      if (mono_gc == None)
	      {
		xgcv.foreground = 1;
		xgcv.background = 0;
		/* create a gc for 1 bit depth */
		mono_gc = XCreateGC(
		  dpy, picture->mask, GCForeground|GCBackground, &xgcv);
	      }
	      XCopyPlane(dpy, mask, cs->shape_mask, mono_gc, 0, 0,
			 picture->width, picture->height, 0, 0, 1);
	    }
	  }
	}
	if (picture)
	{
	  DestroyPicture(dpy, picture);
	  picture = None;
	}
      }
#else
      cs->shape_mask = None;
#endif /* SHAPE */
      break;
    default:
      /* test for ?Gradient */
      if (option[0] && StrEquals(&option[1], "Gradient"))
      {
	type = toupper(option[0]);
	if (!IsGradientTypeSupported(type))
	  break;
	has_pixmap_changed = True;
	if (cs->pixmap)
	{
	  XFreePixmap(dpy, cs->pixmap);
	  cs->pixmap = None;
	}
	if (cs->mask)
	{
	  XFreePixmap(dpy, cs->mask);
	  cs->mask = None;
	}
	/* create a pixmap of the gradient type */
	cs->pixmap =
	  CreateGradientPixmapFromString(dpy, win, gc, type, args, &w, &h);
	cs->width = w;
	cs->height = h;
	if (type == V_GRADIENT)
	  cs->pixmap_type = PIXMAP_STRETCH_Y;
	else if (type == H_GRADIENT)
	  cs->pixmap_type = PIXMAP_STRETCH_X;
	else
	  cs->pixmap_type = PIXMAP_STRETCH;
      }
      else
      {
	fprintf(stderr, "%s: bad colorset pixmap specifier %s %s\n", name,
		option, line);
      }
      break;
    } /* switch */

    if (option)
    {
      free(option);
      option = NULL;
    }
    free(optstring);
    optstring = NULL;
  } /* while (line && *line) */

  /*
   * ---------- change the background colour ----------
   */
  if (has_bg_changed || has_pixmap_changed)
  {
    Bool do_set_default_background = False;

    has_bg_changed = 1;
    if ((cs->color_flags & BG_AVERAGE) && cs->pixmap != None)
    {
      /* calculate average background color */
      XColor *colors;
      XImage *image;
      XImage *mask_image = None;
      unsigned int i, j, k = 0;
      unsigned long red = 0, blue = 0, green = 0;

      /* create an array to store all the pixmap colors in */
      colors = (XColor *)safemalloc(cs->width * cs->height * sizeof(XColor));
      /* get the pixmap into an image */
      image = XGetImage(dpy, cs->pixmap, 0, 0, cs->width, cs->height,
			AllPlanes, ZPixmap);
      /* get each pixel */
      if (cs->mask == None)
      {
	for (i = 0; i < cs->width; i++)
	  for (j = 0; j < cs->height; j++)
	    colors[k++].pixel = XGetPixel(image, i, j);
      }
      else
      {
	mask_image = XGetImage(dpy, cs->mask, 0, 0, cs->width, cs->height,
			       AllPlanes, ZPixmap);
	/* only fetch the pixels that are not transparent */
	for (i = 0; i < cs->width; i++)
	  for (j = 0; j < cs->height; j++)
	    if (XGetPixel(mask_image, i, j) == 0)
	      colors[k++].pixel = XGetPixel(image, i, j);
      }
      XDestroyImage(image);
      if (mask_image != None)
	XDestroyImage(mask_image);
      if (k == 0)
      {
	do_set_default_background = True;
      }
      else
      {
	/* look them all up, XQueryColors() can't handle more than 256 */
	for (i = 0; i < k; i += 256)
	  XQueryColors(dpy, Pcmap, &colors[i], min(k - i, 256));
	/* calculate average, ignore overflow: .red is a short, red is a
	 * long */
	for (i = 0; i < k; i++) {
	  red += colors[i].red;
	  green += colors[i].green;
	  blue += colors[i].blue;
	}
	free(colors);
	/* get it */
	color.red = red / k;
	color.green = green / k;
	color.blue = blue / k;
	if (privateCells) {
	  color.pixel = cs->bg;
	  XStoreColor(dpy, Pcmap, &color);
	} else {
	  XFreeColors(dpy, Pcmap, &cs->bg, 1, 0);
	  XAllocColor(dpy, Pcmap, &color);
	  cs->bg = color.pixel;
	  have_pixels_changed = True;
	}
      }
    } /* average */
    else if ((cs->color_flags & BG_SUPPLIED) && bg != NULL)
    {
      /* user specified colour */
      if (privateCells) {
	unsigned short red, green, blue;

	XParseColor(dpy, Pcmap, bg, &color);
	red = color.red;
	green = color.green;
	blue = color.blue;
	color.pixel = cs->bg;
	XStoreColor(dpy, Pcmap, &color);
      } else {
	XFreeColors(dpy, Pcmap, &cs->bg, 1, 0);
	cs->bg = GetColor(bg);
	have_pixels_changed = True;
      }
    } /* user specified */
    else if (bg == NULL)
    {
      /* default */
      do_set_default_background = True;
    } /* default */
    if (do_set_default_background)
    {
      /* Have to guess a default color. The only certainty is that colorset 0
       * is set up by fvwm so copy its background */
      if (privateCells) {
        /* query it */
        color.pixel = Colorset[0].bg;
        XQueryColor(dpy, Pcmap, &color);
        color.pixel = cs->bg;
        XStoreColor(dpy, Pcmap, &color);
      } else {
        /* copy the pixel */
        cs->bg = Colorset[0].bg;
        have_pixels_changed = True;
      }
    }
  } /* has_bg_changed */

  /*
   * ---------- change the foreground colour ----------
   */
  if (has_fg_changed || (has_bg_changed && (cs->color_flags & FG_CONTRAST)))
  {
    has_fg_changed = 1;
    if (cs->color_flags & FG_CONTRAST)
    {
      /* calculate contrasting foreground color */
      color.pixel = cs->bg;
      XQueryColor(dpy, Pcmap, &color);
      color.red = (color.red > 32767) ? 0 : 65535;
      color.green = (color.green > 32767) ? 0 : 65535;
      color.blue = (color.blue > 32767) ? 0 : 65535;
      if (privateCells) {
	color.pixel = cs->fg;
	XStoreColor(dpy, Pcmap, &color);
      } else {
	XFreeColors(dpy, Pcmap, &cs->fg, 1, 0);
	XAllocColor(dpy, Pcmap, &color);
	cs->fg = color.pixel;
	have_pixels_changed = True;
      }
    } /* contrast */
    else if ((cs->color_flags & FG_SUPPLIED) && fg != NULL)
    {
      /* user specified colour */
      if (privateCells) {
	XParseColor(dpy, Pcmap, fg, &color);
	color.pixel = cs->fg;
	XStoreColor(dpy, Pcmap, &color);
      } else {
	XFreeColors(dpy, Pcmap, &cs->fg, 1, 0);
	cs->fg = GetColor(fg);
	have_pixels_changed = True;
      }
    } /* user specified */
    else if (fg == NULL)
    {
      /* default, copy colorset 0 as it is the only thing guaranteed to exist */
      if (privateCells) {
        /* query it */
        color.pixel = Colorset[0].fg;
        XQueryColor(dpy, Pcmap, &color);
        color.pixel = cs->fg;
        XStoreColor(dpy, Pcmap, &color);
      } else {
        /* copy the pixel */
        cs->fg = Colorset[0].fg;
        have_pixels_changed = True;
      }
    }
  } /* has_fg_changed */

  /*
   * ---------- change the hilight colour ----------
   */
  if (has_hi_changed || has_bg_changed)
  {
    has_hi_changed = 1;
    if ((cs->color_flags & HI_SUPPLIED) && hi != NULL)
    {
      /* user specified colour */
      if (privateCells) {
	XParseColor(dpy, Pcmap, hi, &color);
	color.pixel = cs->hilite;
	XStoreColor(dpy, Pcmap, &color);
      } else {
	XFreeColors(dpy, Pcmap, &cs->hilite, 1, 0);
	cs->hilite = GetColor(hi);
	have_pixels_changed = True;
      }
    } /* user specified */
    else if (hi == NULL)
    {
      if (privateCells) {
        /* calculate from the background, Can't use GetHilite() because
         * it uses XAllocColor */
	color.pixel = cs->bg;
	XQueryColor(dpy, Pcmap, &color);
	color_mult(&color.red, &color.green, &color.blue, BRIGHTNESS_FACTOR);
	color.pixel = cs->hilite;
	XStoreColor(dpy, Pcmap, &color);
      } else {
        cs->hilite = GetHilite(cs->bg);
	have_pixels_changed = True;
      }
    }
  } /* has_hi_changed */

  /*
   * ---------- change the shadow colour ----------
   */
  if (has_sh_changed || has_bg_changed)
  {
    has_sh_changed = 1;
    if ((cs->color_flags & HI_SUPPLIED) && sh != NULL)
    {
      /* user specified colour */
      if (privateCells) {
	XParseColor(dpy, Pcmap, hi, &color);
	color.pixel = cs->shadow;
	XStoreColor(dpy, Pcmap, &color);
      } else {
	XFreeColors(dpy, Pcmap, &cs->shadow, 1, 0);
	cs->shadow = GetColor(hi);
	have_pixels_changed = True;
      }
    } /* user specified */
    else if (sh == NULL)
    {
      if (privateCells) {
	color.pixel = cs->bg;
	XQueryColor(dpy, Pcmap, &color);
	color_mult(&color.red, &color.green, &color.blue, DARKNESS_FACTOR);
	color.pixel = cs->shadow;
	XStoreColor(dpy, Pcmap, &color);
      } else {
        cs->shadow = GetShadow(cs->bg);
	have_pixels_changed = True;
      }
    }
  } /* has_sh_changed */

  /*
   * ---------- change the transparent parts of the background pixmap ----------
   */
  if ((cs->mask != None) && (has_pixmap_changed || has_bg_changed))
  {
    /* Now that we know the background colour we can update the pixmap
     * background. */
    /* default to background colour where pixmap is transparent */
    XSetForeground(dpy, gc, cs->bg);
    XSetClipMask(dpy, gc, cs->mask);
    XFillRectangle(dpy, cs->pixmap, gc, 0, 0, cs->width, cs->height);
    XSetClipMask(dpy, gc, None);
    has_pixmap_changed = True;
  } /* has_pixmap_changed */

  /*
   * ---------- send new colorset to fvwm and clean up ----------
   */
  /* make sure the server has this to avoid races */
  XSync(dpy, False);

  /* inform fvwm of the change */
  if (have_pixels_changed || has_pixmap_changed || has_shape_changed)
    SendText(fd, DumpColorset(n), 0);

  if (fg)
    free(fg);
  if (bg)
    free(bg);
  if (hi)
    free(hi);
  if (sh)
    free(sh);

  /* if privateCells are not being used and XAllocColor has been used
   * we are stuck in sharedCells behaviour forever */
  /* have_pixels_changed will be set if a new colorset has been made */
  if (!privateCells && have_pixels_changed)
    sharedCells = True;
}

/* SendToMessage options */
static char *message_options[] = {"Colorset", NULL};

static void parse_message_line(char *line)
{
  char *rest;

  switch(GetTokenIndex(line, message_options, -1, &rest)) {
  case 0:
    parse_colorset(rest);
    break;
  }
}

static void parse_config(void)
{
  char *line;

  /* prepare the tokenizer array, [0,1] are ImagePath and ColorLimit */
  config_options[3] = safemalloc(namelen + 10);
  sprintf(config_options[3], "*%sColorset", name);
  config_options[4] = safemalloc(namelen + 17);
  sprintf(config_options[4], "*%sReadWriteColors", name);

  /* set a filter on the config lines sent */
  line = safemalloc(namelen + 2);
  sprintf(line, "*%s", name);
  InitGetConfigLine(fd, line);
  free(line);

  /* tell fvwm what we want to receive */
  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO
		 | M_SENDCONFIG | M_STRING);

  /* loop on config lines, a bit strange looking because GetConfigLine
   * is a void(), have to test $line */
  while (GetConfigLine(fd, &line), line)
    parse_config_line(line);
}

static int error_handler(Display *d, XErrorEvent *e)
{
  /* Attempting to free colors or pixmaps that were not allocated by this
   * module or were never allocated at all does not cause problems */
  if (((X_FreeColors == e->request_code) && (BadAccess == e->error_code))
      || ((X_FreePixmap == e->request_code) && (BadPixmap == e->error_code)))
    return 0;

  /* other errors cause diagnostic output and stop execution */
  PrintXErrorAndCoredump(d, e, name);
  return 0;
}

static void set_signals(void) {
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;

  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGPIPE);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigaddset(&sigact.sa_mask, SIGHUP);
  sigaddset(&sigact.sa_mask, SIGTERM);
# ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
# else
  sigact.sa_flags = 0;
# endif
  sigact.sa_handler = signal_handler;
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask(sigmask(SIGPIPE)
		    | sigmask(SIGINT)
		    | sigmask(SIGHUP)
		    | sigmask(SIGTERM));
#endif
  signal(SIGPIPE, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif
}

static RETSIGTYPE signal_handler(int signal) {
  fprintf(stderr, "%s quiting on signal %d\n", name, signal);
  exit(signal);
}
