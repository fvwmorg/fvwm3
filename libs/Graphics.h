#ifndef LIB_GRAPHICS_H
#define LIB_GRAPHICS_H

void do_relieve_rectangle(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading);

void do_relieve_rectangle_with_rotation(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading,
	int rotation);

#define RelieveRectangle(dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width) \
	do_relieve_rectangle( \
		dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width, False)

#define RelieveRectangle2(dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width) \
	do_relieve_rectangle( \
		dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width, True)

Pixmap CreateStretchXPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_width, GC gc);

Pixmap CreateStretchYPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_height, GC gc);

Pixmap CreateStretchPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_width, int dest_height, GC gc);

Pixmap CreateTiledPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height,
	int dest_width, int dest_height, int depth, GC gc);

Pixmap CreateRotatedPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int depth,
	GC gc, int rotation);

GC fvwmlib_XCreateGC(
	Display *display, Drawable drawable, unsigned long valuemask,
	XGCValues *values);

/**** gradient stuff ****/

/* gradient types */
#define H_GRADIENT 'H'
#define V_GRADIENT 'V'
#define D_GRADIENT 'D'
#define B_GRADIENT 'B'
#define S_GRADIENT 'S'
#define C_GRADIENT 'C'
#define R_GRADIENT 'R'
#define Y_GRADIENT 'Y'

Bool IsGradientTypeSupported(char type);

/* Convenience function. Calls AllocNonLinearGradient to fetch all colors and
 * then frees the color names and the perc and color_name arrays. */
XColor *AllocAllGradientColors(
	char *color_names[], int perc[], int nsegs, int ncolors, int dither);

int ParseGradient(char *gradient, char **rest, char ***colors_return,
		  int **perc_return, int *nsegs_return);

Bool CalculateGradientDimensions(Display *dpy, Drawable d, int ncolors,
				 char type, int dither, int *width_ret,
				 int *height_ret);
Drawable CreateGradientPixmap(
	Display *dpy, Drawable d, GC gc, int type, int g_width,
	int g_height, int ncolors, XColor *xcs, int dither, Pixel **d_pixels,
	int *d_npixels, Drawable in_drawable, int d_x, int d_y,
	int d_width, int d_height, XRectangle *rclip);
Pixmap CreateGradientPixmapFromString(
	Display *dpy, Drawable d, GC gc, int type, char *action,
	int *width_return, int *height_return,
	Pixel **alloc_pixels, int *nalloc_pixels, int dither);

void DrawTrianglePattern(
	Display *dpy, Drawable d, GC ReliefGC, GC ShadowGC, GC FillGC,
	int x, int y, int width, int height, int bw, char orientation,
	Bool draw_relief, Bool do_fill, Bool is_pressed);

#endif /* LIB_GRAPHICS_H */
