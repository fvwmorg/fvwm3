#include "config.h"

#ifdef HAVE_XFT

#include <stdio.h>

#include <X11/Xlib.h>
#include "safemalloc.h"
#include "Strings.h"
#include "Flocale.h"

static Display *xftdpy = NULL;
static int xftscreen;
static int xft_initialized = False;

/* we cannot include Picture.h */
extern Colormap Pcmap;
extern Visual *Pvisual;

static
void init_xft(Display *dpy)
{
  xftdpy = dpy;
  xftscreen = DefaultScreen(dpy);
  xft_initialized = True;
}

/* FIXME: a better method? */
static
Bool is_utf8_encoding(XftFont *f)
{
  int i = 0;
  XftPatternElt *e;

  while(i < f->pattern->num)
  {
    e = &f->pattern->elts[i];
    if (StrEquals(e->object, XFT_ENCODING))
    {
      if (StrEquals(e->values->value.u.s, "iso10646-1"))
      {
	return True;
      }
      else
	return False;
    }
    i++;
  }
  return False;
}

FlocaleFont *get_FlocaleXftFont(Display *dpy, char *fontname)
{
  XftFont *xftfont = NULL;
  FlocaleFont *flf = NULL;
  XGlyphInfo	extents;

  if (!fontname)
    return NULL;

  if (!xft_initialized)
    init_xft(dpy);

  xftfont = XftFontOpenName (dpy, xftscreen, fontname);

  if (!xftfont)
    return NULL;

  flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
  flf->count = 1;
  flf->xftfont = xftfont;
  flf->utf8 = is_utf8_encoding(xftfont);
  MULTIBYTE_CODE(flf->fontset = None);
  flf->font = NULL;
  flf->height = xftfont->ascent + xftfont->descent;
  flf->ascent = xftfont->ascent;
  flf->descent = xftfont->descent;
  /* FIXME */
  if (flf->utf8)
    XftTextExtentsUtf8(xftdpy, xftfont, "W", 1, &extents);
  else
    XftTextExtents8(xftdpy, xftfont, "W", 1, &extents);
  flf->max_char_width = extents.xOff;
  return flf;
}


void FftDrawString(Display *dpy, Window win, FlocaleFont *flf, GC gc,
		   int x, int y, char *str, int len)
{
  XftDraw *xftdraw = NULL;
  XGCValues vr;
  XColor xfg;
  XftColor xft_fg;

  xftdraw = XftDrawCreate(dpy, (Drawable) win, Pvisual, Pcmap);
  if (XGetGCValues(dpy, gc, GCForeground, &vr))
  {
    xfg.pixel = vr.foreground; 
  }
  else
  {
    fprintf(stderr, "[fvwmlibs][FftDrawString]: ERROR -- cannot found color\n");
    xfg.pixel = WhitePixel(dpy, xftscreen);
  }

  XQueryColor(dpy, Pcmap, &xfg);
  xft_fg.color.red = xfg.red;
  xft_fg.color.green = xfg.green;
  xft_fg.color.blue = xfg.blue;
  xft_fg.color.alpha = 0xffff;
  xft_fg.pixel = xfg.pixel;
  
  if (flf->utf8)
    XftDrawStringUtf8(xftdraw,
		      &xft_fg, flf->xftfont, x, y, (unsigned char *) str, len);
  else
    XftDrawString8(xftdraw,
		   &xft_fg, flf->xftfont, x, y, (unsigned char *) str, len);
  XftDrawDestroy (xftdraw);
}

int FftTextWidth(FlocaleFont *flf, char *str, int len)
{
  XGlyphInfo	extents;

  if (flf->utf8)
    XftTextExtentsUtf8(xftdpy, flf->xftfont, str, len, &extents);
  else
    XftTextExtents8(xftdpy, flf->xftfont, str, len, &extents);
  return extents.xOff;
}

#endif /* HAVE_XFT */
