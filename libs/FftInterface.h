/* -*-c-*- */

#ifndef FVWMLIB_FFT_INTERFACE_H
#define FVWMLIB_FFT_INTERFACE_H

#include "fvwm_x11.h"

#ifdef HAVE_XFT
#define FftSupport 1
#else
#define FftSupport 0
#endif

typedef struct _FlocaleFont FlocaleFont;
typedef struct _FlocaleWinString FlocaleWinString;

/*
 * Fvwm Xft font structure
 */

typedef struct
{
	XftFont *fftfont[4]; /* Four rotations */
	char *encoding;
} FftFontType;

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent);

FftFontType *FftGetFont(Display *dpy, char *fontname, char *module);
void FftCloseFont(Display *dpy, FftFontType *fftf);

void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len,
	unsigned long flags);

int FftTextWidth(Display *dpy, FlocaleFont *flf, char *str, int len);
void FftPrintPatternInfo(FftFontType *ft);

#endif