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
  /* do not reorder this without changing FvwmTheme */
  Pixel fg;
  Pixel bg;
  Pixel hilite;
  Pixel shadow;
  Pixmap pixmap;
  unsigned int width : 12;
  unsigned int height : 12;
  unsigned int stretch_x : 1;
  unsigned int stretch_y : 1;
  unsigned int keep_aspect : 1;
} colorset_struct;

/* colorsets are stored as an array of structs to permit fast dereferencing */
extern colorset_struct *Colorset;
/* this records the size of the array */
extern unsigned int nColorsets;

/* Create n new colorsets */
colorset_struct *AllocColorset(unsigned int n);
/* dump one */
char *DumpColorset(unsigned int n);
/* load one */
inline int LoadColorset(char *line);
inline int LoadColorsetAndFree(char *line);

void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 colorset_struct *colorset, unsigned int depth, GC gc);

#endif
