/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef F_PICTURE_GRAPHICS_H
#define F_PICTURE_GRAPHICS_H

/*
 * <generalArgs>
 * dpy:           Specifies the connection to the X server.
 * pixmap:        Source pixmap.
 * mask:          Source mask (can be None).
 * alpha:         Source alpha chanel (can be None).
 * depth:         depth of the pixmap (1 or Pdepth).
 * d:             Destination drawable which should be of depth Pdepth.
 * GC gc:         Specifies the a visual GC.
 * src_x,src_y:   Specify the x and y coordinates, which are relative to the
 *                origin of the source pixmap, mask and alpha of the rectangle
 *                which is copied.
 * src_w,src_h:   Width and height of the source rectangle relatively to the
 *                src_x and src_y.
 * dest_x,dest_y: Specify the x and y coordinates of the destination rectangle,
 *                which are relative to the origin of the drawable d.
 * </generalArgs>
 */

/* return a pixmap of depth depth which is the copy of the src pixmap of depth
 * 1 using the gc for the bg and fg colors */
Pixmap PictureBitmapToPixmap(
	Display *dpy, Window win, Pixmap src, int depth, GC gc,
	int src_x, int src_y, int src_w, int src_h);

/*
 * <pubfunc>PGraphicsRenderPixmaps
 * <description>
 * PGraphicsRenderPixmaps copies a rectangle, the src, constituted by a pixmap
 * with its mask, alpha channel, and rendering attributes to a drawable at a
 * given position.
 * </description>
 */
void PGraphicsRenderPixmaps(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, FvwmRenderAttributes *fra, Drawable d,
	GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, int do_repeat);

/*
 * <pubfunc>PGraphicsRenderPicture
 * <description>
 * PGraphicsRenderPicture copies a rectangle, the src, constituted by a picture
 * and its rendering attributes to a drawable at a given position.
 * </description>
 * <arg>
 */
void PGraphicsRenderPicture(
	Display *dpy, Window win, FvwmPicture *p, FvwmRenderAttributes *fra,
	Drawable d, GC gc, GC mono_gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h, int do_repeat);

/*
 * <pubfunc>PGraphicsCopyPixmaps
 * <description>
 * PGraphicsCopyPixmaps copies a rectangle constituted by a pixmap with its
 * mask and alpha channel to a drawable at a given position.
 * </description>
 * <note>
 * The clip_mask, clip_x_origin and clip_y_origin value of the gc are
 * modified in this function: the clip_mask is set to mask and reseted to None,
 * clip_x_origin and clip_y_origin are set to src_x and src_y.
 * The background and foreground value of the gc is important when you render
 * a bitmap. If the gc is set to None the function create and destroy a gc for
 * its use.
 * </note>
 */
void PGraphicsCopyPixmaps(
	Display *dpy, Pixmap pixmap, Pixmap mask, Pixmap alpha, int depth,
	Drawable d, GC gc, int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y);

/*
 * <pubfunc>PGraphicsCopyFvwmPicture
 * <description>
 * PGraphicsCopyFvwmPicture copies a rectangle constituted by an FvwmPicture
 * to a drawable at a given position. It is similar to PGraphicsCopyPixmaps
 * and the same remarks are valids. The only difference is that the source
 * rectangle is given by the FvwmPicture p (which contains the pixmap, the
 * mask, the alpha and the depth).
 * </description>
 */
void PGraphicsCopyFvwmPicture(
	Display *dpy, FvwmPicture *p, Drawable d, GC gc, int src_x, int src_y,
	int src_w, int src_h, int dest_x, int dest_y);

/*
 * <pubfunc>PGraphicsTileRectangle
 * <description>
 * This function is similar to PGraphicsCopyPixmaps. It tiles the rectangle of
 * the drawable d defined with coordinates dest_x, dest_y, dest_w, dest_h with
 * the rectangle composed by the pixmap, its mask and its alpha channel.
 * </description>
 */
void PGraphicsTileRectangle(
	Display *dpy, Window win, Pixmap pixmap, Pixmap shape, Pixmap alpha,
	int depth, Drawable d, GC gc, GC mono_gc,
	int src_x, int src_y,  int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h);

FvwmPicture *PGraphicsCreateStretchPicture(
	Display *dpy, Window win, FvwmPicture *src,
	int dest_width, int dest_height, GC gc, GC mono_gc, GC alpha_gc);

FvwmPicture *PGraphicsCreateTiledPicture(
	Display *dpy, Window win, FvwmPicture *src,
	int dest_width, int dest_height, GC gc, GC mono_gc, GC alpha_gc);

Pixmap PGraphicsCreateTransparency(
	Display *dpy, Window win, FvwmRenderAttributes *fra, GC gc,
	int x, int y, int width, int height, Bool parent_relative);

void PGraphicsTintRectangle(
	Display *dpy, Window win, Pixel tint, int tint_percent,
	Drawable dest, Bool dest_is_a_window, GC gc, GC mono_gc, GC alpha_gc,
	int dest_x, int dest_y, int dest_w, int dest_h);

/* never used ! */
Pixmap PGraphicsCreateDitherPixmap(
	Display *dpy, Window win, Drawable src, Pixmap mask, int depth, GC gc,
	int in_width, int in_height, int out_width, int out_height);

#endif
