/* Fvwm colorset technology is Copyright (C) 1999 Joey Shutup
     http://www.streetmap.co.uk/streetmap.dll?Postcode2Map?BS24+9TZ
     You may use this code for any purpose, as long as the original copyright
     and this notice remains in the source code and all documentation
*/
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

#ifndef LIBS_COLORSETS_H
#define LIBS_COLORSETS_H

typedef struct {
  /* AllocColorset copies the first four pixels from colorset 0, don't move */
  Pixel fg;
  Pixel bg;
  Pixel hilite;
  Pixel shadow;
  Pixmap pixmap;
  Pixmap shape_mask;
  unsigned int width : 12;
  unsigned int height : 12;
  unsigned int pixmap_type: 3;
  unsigned int shape_width : 12;
  unsigned int shape_height : 12;
  unsigned int shape_type : 2;
#ifdef FVWMTHEME_PRIVATE
  /* FvwmTheme use only. */
  Pixmap mask;
  unsigned int color_flags : 6;
  Picture *picture;
  Pixel *pixels;
  int nalloc_pixels;
#endif
} colorset_struct;

#define PIXMAP_TILED 0
#define PIXMAP_STRETCH_X 1
#define PIXMAP_STRETCH_Y 2
#define PIXMAP_STRETCH 3
#define PIXMAP_STRETCH_ASPECT 4

#define SHAPE_TILED 0
#define SHAPE_STRETCH 1
#define SHAPE_STRETCH_ASPECT 2

#ifdef FVWMTHEME_PRIVATE
#define FG_SUPPLIED 0x1
#define BG_SUPPLIED 0x2
#define HI_SUPPLIED 0x4
#define SH_SUPPLIED 0x8
#define FG_CONTRAST 0x10
#define BG_AVERAGE  0x20
#endif

/* colorsets are stored as an array of structs to permit fast dereferencing */
extern colorset_struct *Colorset;
/* this records the size of the array */
extern int nColorsets;

#ifndef FVWMTHEME_PRIVATE
/* Create n new colorsets, FvwmTheme does it's own thing (different size) */
void AllocColorset(int n);
#endif

/* dump one */
char *DumpColorset(int n, colorset_struct *colorset);
/* load one */
int LoadColorset(char *line);

Pixmap CreateBackgroundPixmap(Display *dpy, Window win, int width, int height,
			      colorset_struct *colorset, unsigned int depth,
			      GC gc, Bool is_mask);
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 colorset_struct *colorset, unsigned int depth, GC gc,
			 Bool clear_area);
void SetRectangleBackground(
  Display *dpy, Window win, int x, int y, int width, int height,
  colorset_struct *colorset, unsigned int depth, GC gc);

#endif
