/* -*-c-*- */
/* Copyright (C) 2001  Olivier Chapuis */

#ifndef _EWMH_INTERN_
#define _EWMH_INTERN_

/* Extended window manager hints support */

/* #define EWMH_DEBUG */
#ifdef EWMH_DEBUG
#include <stdarg.h>
#include <time.h>
#include "ftime.h"
#endif

#define EWMH_CMD_ARGS FvwmWindow *fwin, XEvent *ev, window_style *style, \
	unsigned long any

typedef struct ewmh_atom
{
	char *name;
	Atom atom;
	Atom atom_type;
#ifdef __STDC__
	int (*action)(EWMH_CMD_ARGS);
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
	EWMH_ATOM_LIST_ALLOWED_ACTIONS,
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

#define EWMH_MAXIMIZE_HORIZ      0x1
#define EWMH_MAXIMIZE_VERT       0x2
#define EWMH_MAXIMIZE_FULL       0x3
#define EWMH_MAXIMIZE_REMOVE     0x4
#define EWMH_MAXIMIZE_FULLSCREEN 0x8
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
	_NET_WM_MOVERESIZE_MOVE,
	_NET_WM_MOVERESIZE_SIZE_KEYBOARD,
	_NET_WM_MOVERESIZE_MOVE_KEYBOARD
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
int ewmh_WMStateFullScreen(EWMH_CMD_ARGS);
int ewmh_WMStateHidden(EWMH_CMD_ARGS);
int ewmh_WMStateMaxHoriz(EWMH_CMD_ARGS);
int ewmh_WMStateMaxVert(EWMH_CMD_ARGS);
int ewmh_WMStateModal(EWMH_CMD_ARGS);
int ewmh_WMStateShaded(EWMH_CMD_ARGS);
int ewmh_WMStateSkipPager(EWMH_CMD_ARGS);
int ewmh_WMStateSkipTaskBar(EWMH_CMD_ARGS);
int ewmh_WMStateStaysOnTop(EWMH_CMD_ARGS);
int ewmh_WMStateStaysOnBottom(EWMH_CMD_ARGS);
int ewmh_WMStateSticky(EWMH_CMD_ARGS);

int ewmh_WMIconGeometry(EWMH_CMD_ARGS);
int ewmh_WMStrut(EWMH_CMD_ARGS);

Bool ewmh_AllowsYes(EWMH_CMD_ARGS);
Bool ewmh_AllowsClose(EWMH_CMD_ARGS);
Bool ewmh_AllowsMinimize(EWMH_CMD_ARGS);
Bool ewmh_AllowsMaximize(EWMH_CMD_ARGS);
Bool ewmh_AllowsMove(EWMH_CMD_ARGS);
Bool ewmh_AllowsResize(EWMH_CMD_ARGS);

/* ewmh_icon */
int ewmh_WMIcon(EWMH_CMD_ARGS);
CARD32 *ewmh_SetWmIconFromPixmap(
	FvwmWindow *fwin, CARD32 *orig_icon, unsigned int *orig_size,
	Bool is_mini_icon);

/* debugging */
#ifdef EWMH_DEBUG
void  EWMH_DLOG(char *msg, ...);
#else

#endif

#endif /* _EWMH_INTERN_ */
