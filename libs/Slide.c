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
** Slide.c:
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
