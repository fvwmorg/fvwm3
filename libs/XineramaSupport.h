#ifndef __XINERAMA_SUPPORT_H
#define __XINERAMA_SUPPORT_H

#include <X11/Xlib.h>

void XineramaSupportInit(Display *dpy);
void XineramaSupportDisable(void);
void XineramaSupportEnable(void);

void XineramaSupportClipToScreen(
  int src_x, int src_y, int *dest_x, int *dest_y, int w, int h);
void XineramaSupportCenter(int ms_x, int ms_y, int *x, int *y, int w, int h);
void XineramaSupportCenterCurrent(int *x, int *y, int w, int h);
void XineramaSupportGetCurrent00(int *x, int *y);
Bool XineramaSupportGetScrRect(
  int l_x, int l_y, int *x, int *y, int *w, int *h);
void XineramaSupportGetCurrentScrRect(int *x, int *y, int *w, int *h);
void XineramaSupportGetResistanceRect(
  int wx, int wy, int ww, int wh, int *x0, int *y0, int *x1, int *y1);

#endif /* __XINERAMA_SUPPORT_H */
