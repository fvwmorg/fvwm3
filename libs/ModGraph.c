
#define XLIB_ILLEGAL_ACCESS
#include "config.h"
#include <X11/Intrinsic.h> /* needed for Pixel definition */
#include <stdio.h>

#include "libs/ModGraph.h"

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
  G->back_bits = 0; /* will be a packed struct */
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
void ParseGraphics(Display *dpy, char *line, Graphics *G) {
  long backbits, bg;
  int viscount;
  XVisualInfo vizinfo, *xvi;
  VisualID vid;
  Colormap cmap;
  GContext drawGContext, foreGContext, relGContext, shadGContext;
  Font fid;

  /* ignore it if not using fvwm's graphics */
  if (!G->useFvwmLook)
    return;
    
  /* bail out on short lines */
  if (DEFGRAPHLEN + 1 >= strlen(line)) {
    G->useFvwmLook = False;
    return;
  }

  if (sscanf(line + DEFGRAPHLEN, "%lx %lx %lx %lx %lx %lx %lx %lx %lx\n",
            &vid, &cmap, &drawGContext, &backbits, &bg, &foreGContext,
            &relGContext, &shadGContext, &fid) != DEFGRAPHNUM)
    return;
    
  /* if this is the first one grab a visual to use */
  if (!G->initialised) {
    vizinfo.visualid = vid;
    xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
    if (viscount != 1) {
      G->useFvwmLook = False;
      return;
    }
    G->initialised = True;
    G->viz = xvi->visual;
    G->depth = xvi->depth;
    G->cmap = cmap;
    XFree(xvi);
  }

  /* update the changeable entries */
  G->back_bits = backbits;
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
}
