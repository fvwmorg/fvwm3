/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef F_RENDER_INTERFACE_H
#define F_RENDER_INTERFACE_H

Bool FRenderTintRectangle(
	Display *dpy, Window win, Pixmap mask, Pixel tint, int shade_percent,
	Drawable d, int dest_x, int dest_y, int dest_w, int dest_h);

int FRenderRender(
	Display *dpy, Window win, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int depth, int shade_percent, Pixel tint, int tint_percent,
	Drawable d, GC gc, GC alpha_gc,
	int src_x, int src_y, int src_w, int src_h,
	int dest_x, int dest_y, int dest_w, int dest_h,
	Bool do_repeat);

#endif
