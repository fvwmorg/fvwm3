/* -*-c-*- */
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

#ifndef CURSOR_H
#define CURSOR_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* Cursor types */
typedef enum
{
	CRS_NONE = 0,
	CRS_POSITION,          /* upper Left corner cursor */
	CRS_TITLE,             /* title-bar cursor */
	CRS_DEFAULT,           /* cursor for apps to inherit */
	CRS_SYS,               /* sys-menu and iconify boxes cursor */
	CRS_MOVE,              /* move cursor */
	CRS_RESIZE,            /* resize cursor */
	CRS_WAIT,              /* wait a while cursor */
	CRS_MENU,              /* menu cursor */
	CRS_SELECT,            /* dot cursor for selecting windows */
	CRS_DESTROY,           /* skull and cross bones */
	CRS_TOP,
	CRS_RIGHT,
	CRS_BOTTOM,
	CRS_LEFT,
	CRS_TOP_LEFT,
	CRS_TOP_RIGHT,
	CRS_BOTTOM_LEFT,
	CRS_BOTTOM_RIGHT,
	CRS_TOP_EDGE,
	CRS_RIGHT_EDGE,
	CRS_BOTTOM_EDGE,
	CRS_LEFT_EDGE,
	CRS_ROOT,
	CRS_STROKE,
	CRS_MAX
} cursor_t;

typedef enum
{
	/* busy cursor bits */
	BUSY_NONE = 0,
	BUSY_READ = (1<<0),
	BUSY_WAIT = (1<<1),
	BUSY_MODULESYNCHRONOUS = (1<<2),
	BUSY_DYNAMICMENU = (1<<3),
	BUSY_ALL = (BUSY_READ|BUSY_WAIT|BUSY_MODULESYNCHRONOUS|BUSY_DYNAMICMENU)
} busy_cursor_t;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

Cursor *CreateCursors(Display *dpy);

#endif /* CURSOR_H */
