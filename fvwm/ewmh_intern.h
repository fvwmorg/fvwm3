/* -*-c-*- */
/* Copyright (C) 2001  Olivier Chapuis */

#ifndef _EWMH_INTERN_
#define _EWMH_INTERN_

/* Extended window manager hints support */

/* #define EWMH_DEBUG */
#ifdef EWMH_DEBUG
#include <stdarg.h>
#include <sys/times.h>
#include "ftime.h"
#endif

typedef struct ewmh_atom
{
	char *name;
	Atom atom;
	Atom atom_type;
#ifdef __STDC__
	int (*action)(FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
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
void ewmh_ChangeProperty(
	Window w, const char *atom_name, ewmh_atom_list_name list,
	unsigned char *data, int length);
void ewmh_DeleteProperty(
	Window w, const char *atom_name, ewmh_atom_list_name list);
void *ewmh_AtomGetByName(
	Window win, const char *atom_name, ewmh_atom_list_name list,
	int *size);

int ewmh_HandleDesktop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleDialog(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleDock(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleMenu(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleNormal(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleToolBar(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_HandleNotification(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

void ewmh_AddToKdeSysTray(FvwmWindow *fw);
void ewmh_SetWorkArea(void);
void ewmh_ComputeAndSetWorkArea(void);
void ewmh_HandleDynamicWorkArea(void);
void ewmh_HandleWindowType(FvwmWindow *fw, window_style *style);

int ewmh_CurrentDesktop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_DesktopGeometry(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_DesktopViewPort(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_NumberOfDesktops(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

int ewmh_ActiveWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_CloseWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_MoveResizeWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_RestackWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMDesktop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_MoveResize(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

int ewmh_WMState(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateFullScreen(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateHidden(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateMaxHoriz(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateMaxVert(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateModal(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateShaded(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateSkipPager(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateSkipTaskBar(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateStaysOnTop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateStaysOnBottom(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStateSticky(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

int ewmh_WMIconGeometry(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int ewmh_WMStrut(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

Bool ewmh_AllowsYes(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsClose(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsFullScreen(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsMinimize(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsMaximize(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsMove(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
Bool ewmh_AllowsResize(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);

/* ewmh_icon */
int ewmh_WMIcon(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
CARD32 *ewmh_SetWmIconFromPixmap(
	FvwmWindow *fw, CARD32 *orig_icon, int *orig_size,
	Bool is_mini_icon);

/* debugging */
#ifdef EWMH_DEBUG
void  EWMH_DLOG(char *msg, ...);
#else

#endif

#endif /* _EWMH_INTERN_ */
