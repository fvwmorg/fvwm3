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

#ifndef _EWMH_
#define _EWMH_

/* ewmh_conf.c */
Bool EWMH_BugOpts(char *opt, Bool toggle);
void CMD_EwmhNumberOfDesktops(F_CMD_ARGS);
void CMD_EwmhBaseStrut(F_CMD_ARGS);
Bool EWMH_CMD_Style(char *token, window_style *ptmpstyle);

/* for maximize and placement ewmh style */
#define EWMH_IGNORE_WORKING_AREA      0
#define EWMH_USE_WORKING_AREA         1
#define EWMH_USE_DYNAMIC_WORKING_AREA 2
#define EWMH_WORKING_AREA_MASK        3

#ifdef HAVE_EWMH
/* Extended window manager hints support */

#include <X11/Xmd.h>


void EWMH_SetCurrentDesktop(void);
void EWMH_SetNumberOfDesktops(void);

void EWMH_SetActiveWindow(Window w);
void EWMH_SetWMDesktop(FvwmWindow *fwin);
void EWMH_SetWMState(FvwmWindow *fwin, Bool do_restore);

int EWMH_IsKdeSysTrayWindow(Window w);
void EWMH_ManageKdeSysTray(Window w, int type);
void EWMH_SetClientList(void);
void EWMH_SetClientListStacking(void);
void EWMH_UpdateWorkArea(void);
void EWMH_GetWorkAreaIntersection(FvwmWindow *fwin,
				  int *x, int *y, int *h, int *w, int func);
float EWMH_GetStrutIntersection(int x11, int y11, int x12, int y12,
				Bool use_percent);
void EWMH_SetFrameStrut(FvwmWindow *fwin);
void EWMH_SetAllowedActions(FvwmWindow *fwin);

int EWMH_GetIconGeometry(FvwmWindow *fwin, rectangle *icon_rect);

void EWMH_GetStyle(FvwmWindow *fwin, window_style *style);
void EWMH_WindowInit(FvwmWindow *fwin);
void EWMH_RestoreInitialStates(FvwmWindow *fwin, int event_type);
void EWMH_DestroyWindow(FvwmWindow *fwin);
void EWMH_WindowDestroyed(void);

void EWMH_Init(void);
void EWMH_ExitStuff(void);

/* ewmh_conf.c */

/* ewmh_events.c */
Bool EWMH_ProcessClientMessage(FvwmWindow *fwin, XEvent *ev);
void EWMH_ProcessPropertyNotify(FvwmWindow *fwin, XEvent *ev);

/* ewmh_icon.c */
void EWMH_DeleteWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon);
int EWMH_SetIconFromWMIcon(FvwmWindow *fwin, CARD32 *list, unsigned int size, 
			   Bool is_mini_icon);
void EWMH_DoUpdateWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon);

/* ewmh_name.c */
#ifdef HAVE_ICONV
void EWMH_SetVisibleName(FvwmWindow *fwin, Bool is_icon_name);
int EWMH_WMName(FvwmWindow *fwin, XEvent *ev, window_style *style,
		unsigned long any);
#define EWMH_WMName_func EWMH_WMName
int EWMH_WMIconName(FvwmWindow *fwin, XEvent *ev, window_style *style,
		    unsigned long any);
#define EWMH_WMIconName_func EWMH_WMIconName
void EWMH_SetDesktopNames(void);
#else /* HAVE_ICONV */
#define EWMH_SetVisibleName(x,y)
#define EWMH_WMName(x,y,z,t)     0
#define EWMH_WMName_func         0
#define EWMH_WMIconName(x,y,z,t) 0
#define EWMH_WMIconName_func     0
#define EWMH_SetDesktopNames()
#endif /* HAVE_ICONV */

#else /* HAVE_EWMH */


#define EWMH_SetCurrentDesktop()
#define EWMH_SetNumberOfDesktops()

#define EWMH_SetActiveWindow(x)
#define EWMH_SetWMDesktop(x)
#define EWMH_SetWMState(x,y)

#define EWMH_IsKdeSysTrayWindow(x) 0
#define EWMH_ManageKdeSysTray(x,y);
#define EWMH_SetClientList()
#define EWMH_SetClientListStacking()
#define EWMH_UpdateWorkArea()
#define EWMH_GetWorkAreaIntersection(x,y,z,t,u,v)
#define EWMH_GetStrutIntersection(x,y,z,t,u) 0
#define EWMH_SetFrameStrut(x)
#define EWMH_SetAllowedActions(x)
#define EWMH_GetIconGeometry(x,y) 0

#define EWMH_GetStyle(x,y)
#define EWMH_WindowInit(x)
#define EWMH_RestoreInitialStates(x,y)
#define EWMH_DestroyWindow(x)
#define EWMH_WindowDestroyed()
#define EWMH_Init()
#define EWMH_ExitStuff()

/* ewmh_conf.c */

/* ewmh_events.c */
#define EWMH_ProcessClientMessage(x, y) 0
#define EWMH_ProcessPropertyNotify(x, y)

/* ewmh_icon.c */
#define EWMH_DoUpdateWmIcon(x,y,z)
#define EWMH_DeleteWmIcon(x,y,z)
#define EWMH_SetIconFromWMIcon(x,y,z,t) 0

/* ewmh_name.c */
#define EWMH_SetVisibleName(x,y)
#define EWMH_WMName(x,y,z)     0
#define EWMH_WMIconName(x,y,z) 0
#define EWMH_SetDesktopNames() 
#endif

#endif /* _EWMH_ */
