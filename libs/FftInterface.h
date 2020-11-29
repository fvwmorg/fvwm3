/* -*-c-*- */

#ifndef FVWMLIB_FFT_INTERFACE_H
#define FVWMLIB_FFT_INTERFACE_H
#include "Fft.h"
#include "Flocale.h"

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent);
void FftGetFontWidths(
	FlocaleFont *flf, int *max_char_width);
FftFontType *FftGetFont(Display *dpy, char *fontname, char *module);
void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len,
	unsigned long flags);
int FftTextWidth(FlocaleFont *flf, char *str, int len);
void FftPrintPatternInfo(FftFont *f, Bool vertical);
#endif
