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

#ifndef FVWMLIB_MODGRAPH_H
#define FVWMLIB_MODGRAPH_H

/* stuff to enable modules to use fvwm visual/colormap/GCs */
#define DEFGRAPHSTR "Default_Graphics"
#define DEFGRAPHLEN 16 /* length of above string */
#define DEFGRAPHNUM 9 /* number of items sent */

typedef struct _Background {
  union {
    unsigned long word;
    struct {
      Bool is_pixmap : 1;
      Bool stretch_h : 1;
      Bool stretch_v : 1;
      unsigned int w : 12;
      unsigned int h : 12;
    } bits;
  } type;
  Pixmap pixmap;
  unsigned int depth;
} Background;

typedef struct GraphicsThing {
  Bool create_drawGC : 1;
  Bool create_foreGC : 1;
  Bool create_reliefGC : 1;
  Bool create_shadowGC : 1;
  Bool initialised : 1;
  Bool useFvwmLook : 1;
  Visual *viz;
  Colormap cmap;
  GC drawGC;
  Background *bg;
  GC foreGC;
  GC reliefGC;
  GC shadowGC;
  XFontStruct *font;
} Graphics;

Graphics *CreateGraphics(void);
void InitGraphics(Display *dpy, Graphics *graphics);
Bool ParseGraphics(Display *dpy, char *line, Graphics *graphics);
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 Background *background, GC gc);

#endif
