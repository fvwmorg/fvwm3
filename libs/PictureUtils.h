/* -*-c-*- */

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
	Display *dpy, PictureImageColorAllocator *pica, XColor *c, int x,
	int y);
PictureImageColorAllocator *PictureOpenImageColorAllocator(
	Display *dpy, Colormap cmap, int x, int y, Bool no_limit,
	Bool save_pixels, int dither, Bool is_8);
void PictureCloseImageColorAllocator(
	Display *dpy, PictureImageColorAllocator *pica, int *nalloc_pixels,
	Pixel **alloc_pixels, int *no_limit);
void PictureFreeColors(
	Display *dpy, Colormap cmap, Pixel *pixels, int n,
	unsigned long planes, Bool no_limit);
Pixel PictureGetNextColor(Pixel p, int n);
Bool PictureDitherByDefault(void);
Bool PictureUseBWOnly(void);
int PictureInitColors(
	int call_type, Bool init_color_limit, PictureColorLimitOption *opt,
	Bool use_my_color_limit, Bool init_dither);
void PicturePrintColorInfo(int verbose);

#endif
