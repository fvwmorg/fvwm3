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

#ifndef F_RENDER_INTERFACE_H
#define F_RENDER_INTERFACE_H

int FRenderGetErrorCodeBase(void);
int FRenderGetMajorOpCode(void);
Bool FRenderGetErrorText(int code, char *msg);
Bool FRenderGetExtensionSupported(Display *dpy);

void FRenderCopyArea(Display *dpy, Pixmap pixmap, Pixmap mask, Pixmap alpha,
		     Drawable d,
		     int src_x, int src_y,
		     int dest_x, int dest_y, int dest_w, int dest_h,
		     Bool do_repeat);
void FRenderTintRectangle(Display *dpy, Window win, int shade_percent,
			  Pixel tint, Pixmap mask, Drawable d,
			 int dest_x, int dest_y, int dest_w, int dest_h);

#endif
