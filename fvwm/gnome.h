/*********************************************************/
/* GNOME WM Compliance adapted for FVWM                  */
/* Properties set on the root window (or desktop window) */
/*                                                       */
/* Even though the rest of fvwm is GPL consider this file*/
/* Public Domain - use it however you see fit to make    */
/* your WM GNOME compiant                                */
/*                                                       */
/* written by Raster                                     */
/* adapted for FVWM by Jay Painter <jpaint@gnu.org>      */
/*********************************************************/

#ifndef GNOME_H
#define GNOME_H

#ifdef GNOME
/* GNOME window manager hints support */

/* initalization */
void GNOME_Init(void);

/* client messages; setting hints on a window comes through this mechanism */
int  GNOME_ProcessClientMessage(FvwmWindow *fwin, XEvent *ev);

/* hook into .fvwm2rc functions */
void GNOME_ButtonFunc(
	 XEvent *eventp,
	 Window w,
	 FvwmWindow *fwin,
	 unsigned long context,
	 char *action,
	 int *Module);

void GNOME_ProxyButtonEvent(const XEvent *ev);

void GNOME_ShowDesks(
	 XEvent *eventp,
	 Window w,
	 FvwmWindow *fwin,
	 unsigned long context,
	 char *action,
	 int *Module);

/* get hints on a window; sets parameters in a FvwmWindow */
void GNOME_GetHints(FvwmWindow *fwin);
/* get hints on a window, set parameters in style */
void GNOME_GetStyle(FvwmWindow *fwin, window_style *style);

/* set hints on a window from parameters in FvwmWindow */
void GNOME_SetHints(FvwmWindow *fwin);

void GNOME_SetLayer(FvwmWindow *fwin);
void GNOME_SetDesk(FvwmWindow *fwin);
void GNOME_SetWinArea(FvwmWindow *w);
void GNOME_HandlePropRequest(unsigned int propm,
			     unsigned int prop,
			     Window win,
			     XEvent *ev);

/* update public window manager information */
void GNOME_SetAreaCount(void);
void GNOME_SetDeskCount(void);
void GNOME_SetCurrentArea(void);
void GNOME_SetCurrentDesk(void);
void GNOME_SetClientList(void);

#else

#define GNOME_Init()
#define GNOME_ProcessClientMessage(fwin, ev) 0
#define GNOME_ButtonFunc(eventp, w, fwin, context, action, Module)
#define GNOME_ProxyButtonEvent(ev)
#define GNOME_ShowDesks(eventp, w, fwin, context, action, Module)
#define GNOME_GetHints(fwin)
#define GNOME_GetStyle(fwin, style)
#define GNOME_SetHints(fwin)
#define GNOME_SetLayer(fwin)
#define GNOME_SetDesk(fwin)
#define GNOME_SetWinArea(w)
#define GNOME_HandlePropRequest(propm, prop, win, ev)
#define GNOME_SetAreaCount()
#define GNOME_SetDeskCount()
#define GNOME_SetCurrentArea()
#define GNOME_SetCurrentDesk()
#define GNOME_SetClientList()

#endif /* GNOME */

#endif /* GNOME_H */
