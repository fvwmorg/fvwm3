/* -*-c-*- */
/* Copyright (C) 2001  Olivier Chapuis */

#ifndef _EWMH_
#define _EWMH_

/* Extended window manager hints support */

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

/* Extended window manager hints support */

#include <X11/Xmd.h>

void EWMH_SetCurrentDesktop(void);
void EWMH_SetNumberOfDesktops(void);
void EWMH_SetDesktopViewPort(void);
void EWMH_SetDesktopGeometry(void);

void EWMH_SetActiveWindow(Window w);
void EWMH_SetWMDesktop(FvwmWindow *fwin);
void EWMH_SetWMState(FvwmWindow *fwin, Bool do_restore);

int EWMH_IsKdeSysTrayWindow(Window w);
void EWMH_ManageKdeSysTray(Window w, int type);
void EWMH_SetClientList(void);
void EWMH_SetClientListStacking(void);
void EWMH_UpdateWorkArea(void);
void EWMH_GetWorkAreaIntersection(
	FvwmWindow *fwin, int *x, int *y, int *h, int *w, int func);
float EWMH_GetStrutIntersection(
	int x11, int y11, int x12, int y12, Bool use_percent);
void EWMH_SetFrameStrut(FvwmWindow *fwin);
void EWMH_SetAllowedActions(FvwmWindow *fwin);

void EWMH_GetIconGeometry(FvwmWindow *fwin, rectangle *icon_rect);

void EWMH_GetStyle(FvwmWindow *fwin, window_style *style);
void EWMH_WindowInit(FvwmWindow *fwin);
void EWMH_RestoreInitialStates(FvwmWindow *fwin, int event_type);
void EWMH_DestroyWindow(FvwmWindow *fwin);
void EWMH_WindowDestroyed(void);

void EWMH_Init(void);
void EWMH_ExitStuff(void);

/* ewmh_conf.c */

/* ewmh_events.c */
Bool EWMH_ProcessClientMessage(const exec_context_t *exc);
void EWMH_ProcessPropertyNotify(const exec_context_t *exc);

/* ewmh_icon.c */
void EWMH_DeleteWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon);
int EWMH_SetIconFromWMIcon(
	FvwmWindow *fwin, CARD32 *list, unsigned int size, Bool is_mini_icon);
void EWMH_DoUpdateWmIcon(FvwmWindow *fwin, Bool mini_icon, Bool icon);

/* ewmh_name.c */
void EWMH_SetVisibleName(FvwmWindow *fwin, Bool is_icon_name);
int EWMH_WMName(
	FvwmWindow *fwin, XEvent *ev, window_style *style, unsigned long any);
int EWMH_WMIconName(
	FvwmWindow *fwin, XEvent *ev, window_style *style, unsigned long any);
void EWMH_SetDesktopNames(void);

#endif /* _EWMH_ */
