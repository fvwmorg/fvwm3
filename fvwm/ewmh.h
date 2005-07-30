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
void EWMH_SetWMDesktop(FvwmWindow *fw);
void EWMH_SetWMState(FvwmWindow *fw, Bool do_restore);

int EWMH_IsKdeSysTrayWindow(Window w);
void EWMH_ManageKdeSysTray(Window w, int type);
void EWMH_SetClientList(void);
void EWMH_SetClientListStacking(void);
void EWMH_UpdateWorkArea(void);
void EWMH_GetWorkAreaIntersection(
	FvwmWindow *fw, int *x, int *y, int *h, int *w, int func);
float EWMH_GetBaseStrutIntersection(
	int x11, int y11, int x12, int y12, Bool use_percent);
float EWMH_GetStrutIntersection(
	int x11, int y11, int x12, int y12, Bool use_percent);
void EWMH_SetFrameStrut(FvwmWindow *fw);
void EWMH_SetAllowedActions(FvwmWindow *fw);

void EWMH_GetIconGeometry(FvwmWindow *fw, rectangle *icon_rect);

void EWMH_GetStyle(FvwmWindow *fw, window_style *style);
void EWMH_WindowInit(FvwmWindow *fw);
void EWMH_RestoreInitialStates(FvwmWindow *fw, int event_type);
void EWMH_DestroyWindow(FvwmWindow *fw);
void EWMH_WindowDestroyed(void);

void EWMH_Init(void);
void EWMH_ExitStuff(void);

/* ewmh_conf.c */

/* ewmh_events.c */
Bool EWMH_ProcessClientMessage(const exec_context_t *exc);
void EWMH_ProcessPropertyNotify(const exec_context_t *exc);

/* ewmh_icon.c */
void EWMH_DeleteWmIcon(FvwmWindow *fw, Bool mini_icon, Bool icon);
int EWMH_SetIconFromWMIcon(
	FvwmWindow *fw, CARD32 *list, unsigned int size, Bool is_mini_icon);
void EWMH_DoUpdateWmIcon(FvwmWindow *fw, Bool mini_icon, Bool icon);

/* ewmh_name.c */
void EWMH_SetVisibleName(FvwmWindow *fw, Bool is_icon_name);
int EWMH_WMName(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
int EWMH_WMIconName(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any);
void EWMH_SetDesktopNames(void);
void EWMH_fullscreen(FvwmWindow *fw);

#endif /* _EWMH_ */
