#ifndef __XINERAMA_SUPPORT_H
#define __XINERAMA_SUPPORT_H

/* needs X11/Xlib.h and X11/Xutil.h */

/* Control */
Bool XineramaSupportIsEnabled(void);
void XineramaSupportInit(Display *dpy);
void XineramaSupportDisable(void);
void XineramaSupportEnable(void);
/* Intended to be called by modules.  Simply pass in the parameter from the
 * config string sent by fvwm. */
void XineramaSupportConfigureModule(char *args);
void XineramaSupportDisableRandR(void);

int XineramaSupportGetPrimaryScreen(void);
void XineramaSupportSetPrimaryScreen(int scr);

/* Clipping/positioning */
int XineramaSupportClipToScreen(
  int at_x, int at_y, int *x, int *y, int w, int h);

void XineramaSupportPositionCurrent(int *x, int *y,
                                    int px, int py, int w, int h);

void XineramaSupportCenter(int ms_x, int ms_y, int *x, int *y, int w, int h);
void XineramaSupportCenterCurrent(XEvent *eventp, int *x, int *y, int w, int h);
void XineramaSupportCenterPrimary(int *x, int *y, int w, int h);

/* Screen info */
void XineramaSupportGetCurrent00(XEvent *eventp, int *x, int *y);

Bool XineramaSupportGetScrRect(
  int l_x, int l_y, int *x, int *y, int *w, int *h);
void XineramaSupportGetCurrentScrRect(XEvent *eventp,
                                      int *x, int *y, int *w, int *h);
void XineramaSupportGetPrimaryScrRect(int *x, int *y, int *w, int *h);
void XineramaSupportGetGlobalScrRect(int *x, int *y, int *w, int *h);

void XineramaSupportGetResistanceRect(
  int wx, int wy, int ww, int wh, int *x0, int *y0, int *x1, int *y1);
Bool XineramaSupportIsRectangleOnThisScreen(
  XEvent *eventp, rectangle *rec, int screen);

/* Geometry management */
int XineramaSupportParseGeometry(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return);
int  XineramaSupportGetGeometry(
  char *parsestring, int *x_return, int *y_return,
  int *width_return, int *height_return, XSizeHints *hints, int flags);

/* RandR support */
int  XineramaSupportGetRandrEventType(void);
Bool XineramaSupportHandleRandrEvent(
  XEvent *event, int *old_w, int *old_h, int *new_w, int *new_h);

#endif /* __XINERAMA_SUPPORT_H */
