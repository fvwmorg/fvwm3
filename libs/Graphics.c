/*
** Graphics.c: misc convenience functions for drawing stuff
*/

#include <X11/Xlib.h>
/****************************************************************************
 *
 * Draws the relief pattern around a window
 * Draws a line_width wide rectangle from (x,y) to (x+w-1,y+h-1) i.e w wide, h high
 * Draws end points assuming CAP_NOT_LAST style in GC
 * Draws clockwise in case CAP_BUTT is the style and the end points overlap
 *
 ****************************************************************************/
void RelieveWindowGC(Display *dpy, Window win, int x,int y,int w,int h,
		     GC ReliefGC, GC ShadowGC, int line_width)
{
  XSegment seg[line_width];
  int i;

  /* draw top segments, from 0 to the lesser of line_width & just over half h */
  for (i = 0; (i < line_width) && (i < (h + 1) / 2); i++) {
    seg[i].x1 = x+i;     seg[i].y1 = y+i;
    seg[i].x2 = x+w-1-i; seg[i].y2 = y+i;
  }
  XDrawSegments(dpy, win, ReliefGC, seg, i);
  /* right */
  for (i = 0; (i < line_width) && (i < w / 2); i++) {
    seg[i].x1 = x+w-1-i; seg[i].y1 = y+i;
    seg[i].x2 = x+w-1-i; seg[i].y2 = y+h-1-i;
  }
  XDrawSegments(dpy, win, ShadowGC, seg, i);
  /* bottom */
  for (i = 0; (i < line_width) && (i < h / 2); i++) {
    seg[i].x1 = x+w-1-i; seg[i].y1 = y+h-1-i;
    seg[i].x2 = x+i;     seg[i].y2 = y+h-1-i;
  }
  XDrawSegments(dpy, win, ShadowGC, seg, i);
  /* left side */
  for (i = 0; (i < line_width) && (i < (w + 1) / 2); i++) {
    seg[i].x1 = x+i; seg[i].y1 = y+h-1-i;
    seg[i].x2 = x+i; seg[i].y2 = y-i;
  }
  XDrawSegments(dpy, win, ReliefGC, seg, i);
}
