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

#include<stdio.h>
#define XLIB_ILLEGAL_ACCESS
#include "config.h"
#include "libs/fvwmlib.h"
#include "libs/ModGraph.h"

/***************************************************************************
 * create and initialize the structure used to stash graphics things
 **************************************************************************/
Graphics *CreateGraphics() {
  Graphics *G = (Graphics *)safemalloc(sizeof(Graphics));
  memset(G, 0, sizeof(Graphics));
  return G;
}

/***************************************************************************
 * Initialises graphics stuff ready for geting graphics config from fvwm
 **************************************************************************/
void InitGraphics(Display *dpy, Graphics *G) {
  int screen = DefaultScreen(dpy);
  unsigned long mask = GCFunction | GCForeground | GCLineWidth | GCCapStyle;
  XGCValues values;

  G->initialised = False; /* not filled from fvwm yet */
  G->useFvwmLook = True; /* default to using Fvwm look */
  G->viz = DefaultVisual(dpy, screen);
  G->cmap = DefaultColormap(dpy, screen);
  G->depth = DefaultDepth(dpy, screen);
  G->bgtype.word = 0; /* it's a pixel */
  G->bg.pixel = BlackPixel(dpy, screen);

  /* create a gc for rubber band lines */
  values.function = GXxor;
  values.foreground = BlackPixel(dpy, screen) ^ WhitePixel(dpy, screen);
  values.line_width = 0;
  values.cap_style = CapNotLast;
  if (G->create_drawGC)
    G->drawGC = XCreateGC(dpy, RootWindow(dpy, screen), mask, &values);

  /* create relief gc's */
  values.function = GXcopy;
  values.foreground = WhitePixel(dpy, screen);
  if (G->create_reliefGC)
    G->reliefGC = XCreateGC(dpy, RootWindow(dpy, screen), mask, &values);
  if (G->create_shadowGC)
    G->shadowGC = XCreateGC(dpy, RootWindow(dpy, screen), mask, &values);

  /* create a gc for lines and text */
  G->font = XLoadQueryFont(dpy, "fixed");
  mask |= GCFont;
  values.font = G->font->fid;
  if (G->create_foreGC)
    G->foreGC = XCreateGC(dpy, RootWindow(dpy, screen), mask, &values);

}

/***************************************************************************
 * Initialises graphics stuff from a graphics config line sent by fvwm
 **************************************************************************/
Bool ParseGraphics(Display *dpy, char *line, Graphics *G) {
  long bg, bgtype;
  int viscount;
  XVisualInfo vizinfo, *xvi;
  VisualID vid;
  Colormap cmap;
  GContext drawGContext, foreGContext, relGContext, shadGContext;
  Font fid;

  /* ignore it if not using fvwm's graphics */
  if (!G->useFvwmLook)
    return False;
    
  /* bail out on short lines */
  if (strlen(line) < (size_t)(DEFGRAPHLEN + 2 * DEFGRAPHNUM)) {
    G->useFvwmLook = False;
    return False;
  }

  if (sscanf(line + DEFGRAPHLEN + 1, "%lx %lx %lx %lx %lx %lx %lx %lx %lx\n",
	     &vid, &cmap, &drawGContext, &bgtype, &bg, &foreGContext,
	     &relGContext, &shadGContext, &fid) != DEFGRAPHNUM) {
    G->useFvwmLook = False;
    return False;
  }
    
  /* if this is the first one grab a visual to use */
  if (!G->initialised) {
    vizinfo.visualid = vid;
    xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
    if (viscount != 1) {
      G->useFvwmLook = False;
      return False;
    }
    G->initialised = True;
    G->viz = xvi->visual;
    G->depth = xvi->depth;
    G->cmap = cmap;
    XFree(xvi);
  }

  /* update the changeable entries */
  G->bgtype.word = bgtype;
  G->bg.pixel = bg; /* one day may be a pixmap */
  
  /* copy the rest into place, I know this is horrible but GC's must be alloc'd
   * by xlib before the fvwm suppied GContext can be used */
  if (G->create_drawGC)
    G->drawGC->gid = drawGContext;
  if (G->create_foreGC)
    G->foreGC->gid = foreGContext;
  if (G->create_reliefGC)
    G->reliefGC->gid = relGContext;
  if (G->create_shadowGC)
    G->shadowGC->gid = shadGContext;

  /* get the font, this isn't as dirty as the GC stuff as X provides a way to
   * share font id's */
  if (G->font)
    XFreeFontInfo(NULL, G->font, 1);
  G->font = XQueryFont(dpy, fid);

  return True;
}

/***************************************************************************
 * sets a window background according to the back_bits flags
 **************************************************************************/
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 BG *bg, BGtype *bgtype)
{
  /* only does pixel type backgrounds as yet */
  if (!bgtype->bits.is_pixmap)
    XSetWindowBackground(dpy, win, bg->pixel);
  else if (!bgtype->bits.stretch_h && !bgtype->bits.stretch_v)
    XSetWindowBackgroundPixmap(dpy, win, bg->pixmap);
  else if (!bgtype->bits.stretch_h)
    fprintf(stderr, "Can't do VGradient yet\n");
  else if (!bgtype->bits.stretch_v)
    fprintf(stderr, "Can't do HGradient yet\n");
  else
    fprintf(stderr, "Can't do stretch pixmap or B/DGradient yet\n");

  XClearArea(dpy, win, 0, 0, width, height, True);
}
