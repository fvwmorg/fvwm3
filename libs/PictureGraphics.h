/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef F_PICTURE_GRAPHICS_H
#define F_PICTURE_GRAPHICS_H

/*
 * <pubfunc>PGraphicsCopyPixmaps
 * <description>
 * PGraphicsCopyPixmaps copies a rectangle constituted by a pixmap with its
 * mask and alpha channel to a drawable at a given position.
 * </description>
 * <arg>
 * dpy:           Specifies the connection to the X server.
 * pixmap:        Source pixmap.
 * mask:          Source mask (can be None).
 * alpha:         Source alpha chanel (can be None).
 * depth:         depth of the pixmap (1 or Pdepth).
 * d:             Destination drawable which should be of depth Pdepth.
 * GC gc:         Specifies the GC.
 * src_x,src_y:   Specify the x and y coordinates, which are relative to the
 *                origin of the source pixmap, mask and alpha of the rectangle
 *                which is copied.
 * src_w,src_h:   Width and height of the source rectangle relatively to the
 *                src_x and src_y.
 * dest_x,dest_y: Specify the x and y coordinates of the destination rectangle,
 *                which are relative to the origin of the drawable d.
 * </arg>
 * <note>
 * The clip_mask, clip_x_origin and clip_y_origin value of the gc are
 * modified in this function: the clip_mask is set to mask and reseted to None,
 * clip_x_origin and clip_y_origin are set to src_x and src_y.
 * The background and foreground value of the gc is important when you render
 * a bitmap. If the gc is set to None the function create and destroy a gc for
 * its use.
 * </note>
 */
void PGraphicsCopyPixmaps(Display *dpy, Pixmap pixmap, Pixmap mask, Pixmap alpha,
			  int depth, Drawable d, GC gc,
			  int src_x, int src_y, int src_w, int src_h,
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
void PGraphicsCopyFvwmPicture(Display *dpy, FvwmPicture *p, Drawable d, GC gc,
			      int src_x, int src_y, int src_w, int src_h,
			      int dest_x, int dest_y);

/*
 * <pubfunc>PGraphicsTileRectangle
 * <description>
 * This function is similar to PGraphicsCopyPixmaps. It tiles the rectangle of
 * the drawable d defined with coordinates dest_x, dest_y, dest_w, dest_h with
 * the rectangle composed by the pixmap, its mask and its alpha channel.
 * </description>
 */
void PGraphicsTileRectangle(Display *dpy, Window win,
			    Pixmap pixmap, Pixmap shape, Pixmap alpha,
			    int depth,
			    int src_x, int src_y,
			    Drawable d, GC gc, GC mono_gc,
			    int dest_x, int dest_y, int dest_w, int dest_h);

/*
 * <pubfunc>PGraphicsTintDrawable
 * <description>
 * PGraphicsTintDrawable tint a rectangle of a dawable.
 * </description>
 * <arg>
 * dpy:            Specifies the connection to the X server.
 * win:            A Window, says the Root window.
 * tint_percent:   Rate in percent for the tinting, a value of 0 will do
		   nothing while a value of 100 will totally tint.
 * tint:           Colour with which the tint is performed.
 * mask:           Mask for the tint.
 * d:              Destination drawable.
 * dest_x, dest_y: Specify the x and y coordinates of the destination rectangle,
 *                 which are relative to the origin of the drawable.
 * dest_w, dest_h: Specify the height and the width of the destination rectangle.
 * </arg>
 */
void PGraphicsTintRectangle(
	Display *dpy, Window win, int tint_percent, Pixel tint,
	Drawable d, Pixmap mask, int depth, GC gc, int color_limit,
	int dest_x, int dest_y, int dest_w, int dest_h);

Pixmap PGraphicsCreateDitherPixmap(
	Display *dpy, Window win, Drawable src, Pixmap mask, int depth, GC gc,
	int in_width, int in_height, int out_width, int out_height);
#endif
