/* Copyright (C) 2001  Olivier Chapuis
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

#ifndef _EWMH_INTERN_
#define _EWMH_INTERN_

#ifdef HAVE_EWMH
/* Extended window manager hints support */

/*#include "fvwm.h"*/

/* #define EWMH_DEBUG */
#ifdef EWMH_DEBUG
#include <stdarg.h>
#include <time.h>
#include <sys/times.h>
#endif

typedef struct ewmh_atom
{
  char *name;
  Atom atom;
  Atom atom_type;
#ifdef __STDC__
  int (*action)(FvwmWindow *, XEvent *, window_style *);
#else
  int (*action)();
#endif
} ewmh_atom;

typedef enum
{
  EWMH_ATOM_LIST_ALL,
  EWMH_ATOM_LIST_CLIENT_ROOT,
  EWMH_ATOM_LIST_CLIENT_WIN,
  EWMH_ATOM_LIST_WM_STATE,
  EWMH_ATOM_LIST_WINDOW_TYPE,
  EWMH_ATOM_LIST_FIXED_PROPERTY,
  EWMH_ATOM_LIST_PROPERTY_NOTIFY,
  EWMH_ATOM_LIST_FVWM_ROOT,
  EWMH_ATOM_LIST_FVWM_WIN,
  EWMH_ATOM_LIST_END
} ewmh_atom_list_name;

typedef struct
{
  ewmh_atom_list_name name;
  ewmh_atom      *list;
  int size;
} ewmh_atom_list;

#define NET_WM_STATE_ADD       1
#define NET_WM_STATE_REMOVE    0
#define NET_WM_STATE_TOGGLE    2

#define EWMH_MAXIMIZE_HORIZ  0x1
#define EWMH_MAXIMIZE_VERT   0x2
#define EWMH_MAXIMIZE_FULL   0x3
#define EWMH_MAXIMIZE_REMOVE 0x4

typedef enum
{
  _NET_WM_MOVERESIZE_SIZE_TOPLEFT,
  _NET_WM_MOVERESIZE_SIZE_TOP,
  _NET_WM_MOVERESIZE_SIZE_TOPRIGHT,
  _NET_WM_MOVERESIZE_SIZE_RIGHT,
  _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT,
  _NET_WM_MOVERESIZE_SIZE_BOTTOM,
  _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT,
  _NET_WM_MOVERESIZE_SIZE_LEFT,
  _NET_WM_MOVERESIZE_MOVE
} ewmh_move_resize;

typedef struct ewmh_info
{
  unsigned NumberOfDesktops;
  unsigned MaxDesktops;
  unsigned CurrentNumberOfDesktops;
  Bool NeedsToCheckDesk;
  ewmh_strut BaseStrut;
} ewmhInfo;

extern ewmhInfo ewmhc;

#define EWMH_CMD_ARGS FvwmWindow *fwin, XEvent *ev, window_style *style

ewmh_atom *ewmh_GetEwmhAtomByAtom(Atom atom, ewmh_atom_list_name list_name);
void ewmh_ChangeProperty(Window w,
			 const char *atom_name,
			 ewmh_atom_list_name list, 
			 unsigned char *data,
			 unsigned int length);
void ewmh_DeleteProperty(Window w,
			 const char *atom_name,
			 ewmh_atom_list_name list);
void *ewmh_AtomGetByName(Window win, const char *atom_name,
			 ewmh_atom_list_name list, unsigned int *size);

int ewmh_HandleDesktop(EWMH_CMD_ARGS);
int ewmh_HandleDialog(EWMH_CMD_ARGS);
int ewmh_HandleDock(EWMH_CMD_ARGS);
int ewmh_HandleMenu(EWMH_CMD_ARGS);
int ewmh_HandleNormal(EWMH_CMD_ARGS);
int ewmh_HandleToolBar(EWMH_CMD_ARGS);

void ewmh_AddToKdeSysTray(FvwmWindow *fwin);
void ewmh_SetWorkArea(void);
void ewmh_ComputeAndSetWorkArea(void);
void ewmh_HandleDynamicWorkArea(void);
void ewmh_HandleWindowType(FvwmWindow *fwin, window_style *style);

int ewmh_CurrentDesktop(EWMH_CMD_ARGS);
int ewmh_DesktopGeometry(EWMH_CMD_ARGS);
int ewmh_DesktopViewPort(EWMH_CMD_ARGS);
int ewmh_NumberOfDesktops(EWMH_CMD_ARGS);

int ewmh_ActiveWindow(EWMH_CMD_ARGS);
int ewmh_CloseWindow(EWMH_CMD_ARGS);
int ewmh_WMDesktop(EWMH_CMD_ARGS);
int ewmh_MoveResize(EWMH_CMD_ARGS);

int ewmh_WMState(EWMH_CMD_ARGS);
int ewmh_WMStateMaxHoriz(EWMH_CMD_ARGS);
int ewmh_WMStateMaxVert(EWMH_CMD_ARGS);
int ewmh_WMStateModal(EWMH_CMD_ARGS);
int ewmh_WMStateShaded(EWMH_CMD_ARGS);
int ewmh_WMStateListSkip(EWMH_CMD_ARGS);
int ewmh_WMStateStaysOnTop(EWMH_CMD_ARGS);
int ewmh_WMStateSticky(EWMH_CMD_ARGS);

int ewmh_WMIconGeometry(EWMH_CMD_ARGS);
int ewmh_WMStrut(EWMH_CMD_ARGS);

/* ewmh_icon */
int ewmh_WMIcon(EWMH_CMD_ARGS);
CARD32 *ewmh_SetWmIconFromPixmap(FvwmWindow *fwin,
				 CARD32 *orig_icon,
				 unsigned int *orig_size,
				 Bool is_mini_icon);

/* debugging */
#ifdef EWMH_DEBUG
void  EWMH_DLOG(char *msg, ...);
#define EWMH_DEBUG_CODE(x) x

#else

#define EWMH_DLOG(x, ...)
#define EWMH_DEBUG_CODE(x)

#endif

#endif /* HAVE_EWMH */
#endif /* _EWMH_INTERN_ */
