#ifndef __XINERAMA_SUPPORT_H
#define __XINERAMA_SUPPORT_H

/* needs X11/Xlib.h and X11/Xutil.h */

/* Control */
Bool FScreenIsEnabled(void);
void FScreenInit(Display *dpy);
void FScreenDisable(void);
void FScreenEnable(void);
/* Intended to be called by modules.  Simply pass in the parameter from the
 * config string sent by fvwm. */
void FScreenConfigureModule(char *args);
void FScreenDisableRandR(void);

int FScreenGetPrimaryScreen(void);
void FScreenSetPrimaryScreen(int scr);

/* Clipping/positioning */
int FScreenClipToScreen(
  int at_x, int at_y, int *x, int *y, int w, int h);

void FScreenPositionCurrent(int *x, int *y,
                                    int px, int py, int w, int h);

void FScreenCenter(int ms_x, int ms_y, int *x, int *y, int w, int h);
void FScreenCenterCurrent(XEvent *eventp, int *x, int *y, int w, int h);
void FScreenCenterPrimary(int *x, int *y, int w, int h);

/* Screen info */
void FScreenGetCurrent00(XEvent *eventp, int *x, int *y);

Bool FScreenGetScrRect(
  int l_x, int l_y, int *x, int *y, int *w, int *h);
void FScreenGetCurrentScrRect(XEvent *eventp,
                                      int *x, int *y, int *w, int *h);
void FScreenGetPrimaryScrRect(int *x, int *y, int *w, int *h);
void FScreenGetGlobalScrRect(int *x, int *y, int *w, int *h);

void FScreenGetNumberedScrRect(
  int screen, int *x, int *y, int *w, int *h);
void FScreenGetResistanceRect(
  int wx, int wy, int ww, int wh, int *x0, int *y0, int *x1, int *y1);
Bool FScreenIsRectangleOnThisScreen(
  XEvent *eventp, rectangle *rec, int screen);

/* Geometry management */
int FScreenGetScreenArgument(char *arg, char default_screen);
int FScreenParseGeometryWithScreen(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return, int *screen_return);
int FScreenParseGeometry(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return);
int  FScreenGetGeometry(
  char *parsestring, int *x_return, int *y_return,
  int *width_return, int *height_return, XSizeHints *hints, int flags);

/* RandR support */
int  FScreenGetRandrEventType(void);
Bool FScreenHandleRandrEvent(
  XEvent *event, int *old_w, int *old_h, int *new_w, int *new_h);

#endif /* __XINERAMA_SUPPORT_H */
