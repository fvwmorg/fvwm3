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

#ifndef STEPGFX_H_
#define STEPGFX_H_

#define TEXTURE_SOLID		0
#define TEXTURE_GRADIENT 	1
#define TEXTURE_HGRADIENT	2
#define TEXTURE_HCGRADIENT	3
#define TEXTURE_VGRADIENT	4
#define TEXTURE_VCGRADIENT	5
#define TEXTURE_COLORSET	6

#define TEXTURE_PIXMAP		128

#define TEXTURE_BUILTIN		255

extern int DrawVGradient(Display *dpy, Drawable d, int x, int y, int w, int h,
			 int from[3], int to[3], int relief, int maxcols,
			 int type);
extern int DrawHGradient(Display *dpy, Drawable d, int x, int y, int w, int h,
			 int from[3], int to[3], int relief, int maxcols,
			 int type);
extern int DrawDegradeRelief(Display *dpy, Drawable d, int x, int y, int w,
			     int h, int from[3], int to[3], int relief,
			     int maxcols);
extern void DrawTexturedText(Display *dpy, Drawable d, XFontStruct *font,
		      int x, int y, Pixmap gradient, char *text, int chars);

extern int MakeShadowColors(Display *dpy, int from[3], int to[3],
			    unsigned long *dark, unsigned long *light);

#endif
