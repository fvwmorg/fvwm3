#ifndef LIB_COLORUTILS_H
#define LIB_COLORUTILS_H

/*
 * Stuff for dealing w/ bitmaps & pixmaps:
 */

XColor *GetShadowColor(Pixel);
XColor *GetHiliteColor(Pixel);
Pixel GetShadow(Pixel);
Pixel GetHilite(Pixel);
XColor *GetForeShadowColor(Pixel foreground, Pixel background);
Pixel GetForeShadow(Pixel foreground, Pixel background);
XColor *GetTintedColor(Pixel in, Pixel tint, int percent);
Pixel GetTintedPixel(Pixel in, Pixel tint, int percent);

/* This function converts the colour stored in a colorcell (pixel) into the
 * string representation of a colour.  The output is printed at the
 * address 'output'.  It is either in rgb format ("rgb:rrrr/gggg/bbbb") if
 * use_hash is False or in hash notation ("#rrrrggggbbbb") if use_hash is true.
 * The return value is the number of characters used by the string.  The
 * rgb values of the output are undefined if the colorcell is invalid.  The
 * memory area pointed at by 'output' must be at least 64 bytes (in case of
 * future extensions and multibyte characters).*/
int pixel_to_color_string(
	Display *dpy, Colormap cmap, Pixel pixel, char *output, Bool use_hash);

Pixel GetSimpleColor(char *name);
/* handles colorset color names too */
Pixel GetColor(char *name);

/* duplicate an already allocated color */
Pixel fvwmlib_clone_color(Pixel p);
void fvwmlib_free_colors(Display *dpy, Pixel *pixels, int n, Bool no_limit);
void fvwmlib_copy_color(
	Display *dpy, Pixel *dst_color, Pixel *src_color, Bool do_free_dest,
	Bool do_copy_src);

#endif /* LIB_COLORUTILS_H */
