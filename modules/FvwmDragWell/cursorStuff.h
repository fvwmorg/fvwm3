/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef _CURSOR_STUFF_H
#define _CURSOR_STUFF_H

#include <X11/Xlib.h>

/* copied and modified from Paul Sheer's xdndv2 library.*/
typedef struct XdndCursorStruct {
  int w;
  int h;
  int x;
  int y;  /*width,height,x offset, y offset*/
  unsigned char *imageData, *maskData; /*Used for initialization*/
  char *actionStr; /*Action string the cursor is associated with.*/
  Atom action;	/*Action the cursor is associated with.*/
  Cursor cursor;  /*The actual cursor*/
} XdndCursor;

#endif
