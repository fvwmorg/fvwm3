/*
** Graphics.c: misc convenience functions for drawing stuff
*/

#include <X11/Xlib.h>
/****************************************************************************
 *
 * Draws the relief pattern around a window
 * Draws a line_width wide rectangle from (x,y) to (x+w,y+h) i.e w+1 wide, h+1 high
 * Draws end points assuming CAP_NOT_LAST style in GC
 * Draws anit-clockwise in case CAP_BUTT is the style and the end points overlap
 * Top and bottom lines come out full length, the sides come out 1 pixel less
 * This is so FvwmBorder windows have a correct bottom edge and the sticky lines
 * look like just lines
 ****************************************************************************/
void RelieveWindowGC(Display *dpy, Window win, int x,int y,int w,int h,
		     GC ReliefGC, GC ShadowGC, int line_width)
{
  XSegment* seg = (XSegment*)safemalloc(sizeof(XSegment) * line_width);
  int i;

  /* left side, from 0 to the lesser of line_width & just over half w */
  for (i = 0; (i < line_width) && (i <= w / 2); i++) {
    seg[i].x1 = x+i; seg[i].y1 = y+i;
    seg[i].x2 = x+i; seg[i].y2 = y+h-i;
  }
  XDrawSegments(dpy, win, ReliefGC, seg, i);
  /* bottom */
  for (i = 0; (i < line_width) && (i <= h / 2); i++) {
    seg[i].x1 = x+i;   seg[i].y1 = y+h-i;
    seg[i].x2 = x+w-i; seg[i].y2 = y+h-i;
  }
  XDrawSegments(dpy, win, ShadowGC, seg, i);
  /* right */
  for (i = 0; (i < line_width) && (i <= w / 2); i++) {
    seg[i].x1 = x+w-i; seg[i].y1 = y+h-i;
    seg[i].x2 = x+w-i; seg[i].y2 = y+i;
  }
  XDrawSegments(dpy, win, ShadowGC, seg, i);
  /* draw top segments */
  for (i = 0; (i < line_width) && (i <= h / 2); i++) {
    seg[i].x1 = x+w-i; seg[i].y1 = y+i;
    seg[i].x2 = x+i;   seg[i].y2 = y+i;
  }
  XDrawSegments(dpy, win, ReliefGC, seg, i);
}
