/* Copyright (C) 1999  Dominik Vogt
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

/*
** WinMagic.c:
** This file supplied routines for moving and resizing windows in an animated
** fashion.
*/

#include <X11/Xlib.h>
#include <fvwmlib.h>
#include <stdio.h>

/* Continuously moves and resized window win from start geometry (s_?) to end
 * geometry (e_?). Waits for delay_ms milliseconds after each step except the
 * last (max. 10 seconds per step). The number of steps is determined by the
 * steps argument (min. 1 and max. 10000). If the pointer ppctMovement is NULL
 * the steps are all the same width, if it is given it is interpreted as a
 * pointer to an array of float percent values. These are used to determine the
 * distance of each step counted from the start position, i.e. 0.0 means the
 * start position itself, 50.0 is halfway between start and end position and
 * 100.0 is the end position. Values smaller than 0.0 or bigger than 100.0 are
 * allowed too. The do_flush flag determines of all requests are sent to the
 * X server immediately (True) or not (False). */
void SlideWindow(
  Display *dpy, Window win,
  int s_x, int s_y, unsigned int s_w, unsigned int s_h,
  int e_x, int e_y, unsigned int e_w, unsigned int e_h,
  int steps, int delay_ms, float *ppctMovement, Bool do_flush)
{
  int x;
  int y;
  int w;
  int h;
  int i = 0;
  unsigned int us;
  Bool is_mapped;
  Bool keep_x1 = False;
  Bool keep_x2 = False;
  Bool keep_y1 = False;
  Bool keep_y2 = False;

  /* check limits */
  if (delay_ms > 10000)
  {
    /* max. 10 seconds per step */
    us = 10000000;
  }
  else if (delay_ms < 0)
  {
    us = 0;
  }
  else
  {
    us = 1000 * delay_ms;
  }

  if (steps > 10000)
  {
    /* max. 10000 steps */
    steps = 10000;
  }
  if (steps <= 0)
  {
    /* no steps, no animation */
    if (e_w == 0 || e_h == 0)
    {
      XUnmapWindow(dpy, win);
    }
    else
    {
      XMoveResizeWindow(dpy, win, e_x, e_y, e_w, e_h);
      XMapWindow(dpy, win);
      XMapSubwindows(dpy, win);
    }
    if (do_flush)
      XFlush(dpy);
    return;
  }

  is_mapped = False;

  if (s_x == e_x)
    keep_x1 = True;
  if (s_y == e_y)
    keep_y1 = True;
  if (s_x + s_w == e_x + e_w)
    keep_x2 = True;
  if (s_y + s_h == e_y + e_h)
    keep_y2 = True;

  /* animate the window */
  for (i = 0; i <= steps; i++)
  {
    if (ppctMovement == NULL || i == steps)
    {
      x = s_x + (int)(e_x - s_x) * i / steps;
      y = s_y + (int)(e_y - s_y) * i / steps;
    }
    else
    {
      float f = ppctMovement[i] / (float)steps;

      x = (int)((float)(s_x + (e_x - s_x)) * f);
      y = (int)((float)(s_y + (e_y - s_y)) * f);
    }
    w = s_w + (int)(e_w - s_w) * i / steps;
    h = s_h + (int)(e_h - s_h) * i / steps;

    /* prevent annoying flickering */
    if (keep_x1)
      x = s_x;
    if (keep_y1)
      y = s_y;
    if (keep_x2)
      w = s_x + s_w - x;
    if (keep_y2)
      h = s_y + s_h - y;

    if (w == 0 || h == 0)
    {
      /* don't show zero width/height windows */
      if (is_mapped)
      {
	XUnmapWindow(dpy, win);
	is_mapped = False;
      }
    }
    else
    {
      XMoveResizeWindow(dpy, win, x, y, w, h);
      if (!is_mapped)
      {
	XMapWindow(dpy, win);
	XMapSubwindows(dpy, win);
	is_mapped = True;
      }
    }
    /* make sure everything is updated */
    if (do_flush)
      XFlush(dpy);
    if (us && i < steps)
    {
      /* don't sleep after the last step */
      usleep(us);
    }
  }

  return;
}

/* This function returns the top level ancestor of the window 'child'. It
 * returns None if an error occurs or if the window is a top level window. */
Window GetTopAncestorWindow(Display *dpy, Window child)
{
  Window root = None;
  Window ancestor = child;
  Window last_child = child;
  Window *children;
  int nchildren;

  if (child == None)
    return None;

  while (ancestor != root)
  {
    last_child = ancestor;
    children = NULL;
    if (!XQueryTree(dpy, last_child, &root, &ancestor, &children, &nchildren))
      return None;
    if (children)
      XFree(children);
  }

  return (last_child == child) ? None : last_child;
}

/* Given a parent window this function returns a list of children of the
 * parent window that have the same size, depth, visual and colormap as the
 * parent window and that have position +0+0 within the parent. If the 'depth'
 * argument is non-zero it must match the depth of the window. The 'visualid'
 * and 'colormap' arguments work just the same. The number of matching
 * children is returned. The list of children is returned in *children. If this
 * list is non-NULL, it must be free'd with XFree. If an error occurs or the
 * parent window does not match the required depth, colormap or visualid, the
 * function returns -1 and NULL in *children. */
int GetEqualSizeChildren(
  Display *dpy, Window parent, int depth, VisualID visualid, Colormap colormap,
  Window **ret_children)
{
  XWindowAttributes pxwa;
  XWindowAttributes cxwa;
  Window JunkW;
  Window *children;
  int nchildren;
  int i;
  int j;

  if (!XGetWindowAttributes(dpy, parent, &pxwa))
    return -1;
  if (!XQueryTree(dpy, parent, &JunkW, &JunkW, &children, &nchildren))
    return -1;
  if (depth && pxwa.depth != depth)
    return -1;
  if (visualid && XVisualIDFromVisual(pxwa.visual) != visualid)
    return -1;
  if (colormap && pxwa.colormap != colormap)
    return -1;

  for (i = 0, j = 0; i < nchildren; i++)
  {
    if (XGetWindowAttributes(dpy, children[i], &cxwa) &&
	cxwa.x == 0 &&
	cxwa.y == 0 &&
	cxwa.width == pxwa.width &&
	cxwa.height == pxwa.height &&
	(!depth || cxwa.depth == depth) &&
	(!visualid || XVisualIDFromVisual(cxwa.visual) == visualid) &&
	cxwa.class == InputOutput &&
	(!colormap || cxwa.colormap == colormap))
    {
      children[j++] = children[i];
    }
  } /* for */

  if (j == 0)
  {
    if (children)
    {
      XFree(children);
      children = NULL;
    }
  }
  *ret_children = children;

  return j;
}
