/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IN_X_H
#define IN_X_H

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

extern Display *theDisplay;
extern Window theRoot;
extern int theDepth, theScreen;

extern void unmap_manager (WinManager *man);
extern void map_manager (WinManager *man);

extern Window find_frame_window (Window win, int *off_x, int *off_y);

extern void init_display (void);
extern void xevent_loop (void);
extern void create_manager_window (int man_id);
extern void X_init_manager (int man_id);

#endif
