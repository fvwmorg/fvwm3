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

#ifndef Picture_Utils_H
#define Picture_Utils_H

#define PICTURE_CALLED_BY_FVWM   0
#define PICTURE_CALLED_BY_MODULE 1

void PictureReduceColorName(char **my_color);
void PictureAllocColors(
	Display *dpy, Colormap cmap, XColor *colors, int size, Bool no_limit);
int PictureAllocColor(Display *dpy, Colormap cmap, XColor *c, int no_limit);
int PictureAllocColorAllProp(
	Display *dpy, Colormap cmap, XColor *c, int x, int y,
	Bool no_limit, Bool is_8, Bool do_dither);
int PictureAllocColorImage(
	Display *dpy, PictureImageColorAllocator *pica, XColor *c, int x, int y);
PictureImageColorAllocator *PictureOpenImageColorAllocator(
	Display *dpy, Colormap cmap, int x, int y, Bool no_limit,
	Bool save_pixels, int dither, Bool is_8);
void PictureCloseImageColorAllocator(
	Display *dpy, PictureImageColorAllocator *pica, int *nalloc_pixels,
	Pixel **alloc_pixels, int *no_limit);
void PictureFreeColors(
	Display *dpy, Colormap cmap, Pixel *pixels, int n, unsigned long planes,
	Bool no_limit);
Pixel PictureGetNextColor(Pixel p, int n);
Bool PictureDitherByDefault(void);
Bool PictureUseBWOnly(void);
int PictureInitColors(
	int call_type, Bool init_color_limit, char *opt,
	Bool use_my_color_limit, Bool init_dither);
void PicturePrintColorInfo(int verbose);

#endif
