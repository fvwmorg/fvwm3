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

#ifndef FFT_INTERFACE_H
#define FFT_INTERFACE_H

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent);
void FftGetFontWidths(
	FlocaleFont *flf, int *max_char_width);
FftFontType *FftGetFont(Display *dpy, char *fontname, char *module);
void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len, unsigned long flags);
int FftTextWidth(FlocaleFont *flf, char *str, int len);
void FftPrintPatternInfo(FftFont *f, Bool vertical);
#endif
