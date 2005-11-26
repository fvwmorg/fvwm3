/* -*-c-*- */
/*
 * x selection/cut buffer utility for Fvwm
 *
 * Copyright (c) 1999 Matthias Clasen <clasen@mathematik.uni-freiburg.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

void paste_primary(Display *dpy, int window, int property, int delete)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after, nread;
  unsigned char *data, *d, *h, buf[256];

  if (property == None)
    {
      window = DefaultRootWindow (dpy);
      property = XA_CUT_BUFFER0;
      delete = False;
    }

  nread = 0;
  h = buf;
  do
    {
      if (XGetWindowProperty (dpy, window, property, nread/4, 1024,
			      delete, AnyPropertyType, &actual_type,
			      &actual_format, &nitems, &bytes_after,
			      (unsigned char **)&data) != Success)
	return;
      if (actual_type != XA_STRING)
	return;

      /* send the text line-by-line, recognize continuation lines */
      d = data;
      while (1) {
	switch (*d) {
	case '\n':
	  *h = 0;
	  break;
	case '\\':
	    if (d[1] == '\n')
	      {
		*h = ' ';
		d++;
		break;
	      }
	    /* fall through */
	default:
	  *h = *d;
	}
	if (h - buf == 255)
	  {
	    fprintf(stderr, "xselection: line too long\n");
	    *h = 0;
	  }
	if (*h == 0)
	  {
	    h = buf;
	    while (*h == ' ') h++;
	    printf ("%s\n", h);
	    h = buf;
	  }
	else h++;
	if (*d == 0) break;
	d++;
      }

      nread += nitems;
      XFree (data);
    } while (bytes_after > 0);
}

int main (int argc, char **argv)
{
  XEvent event;
  Atom sel_property;
  char *display_name = NULL;
  Display *dpy;
  Window window;

  if (!(dpy = XOpenDisplay (display_name)))
    {
      fprintf (stderr, "xselection: can't open display %s\n",
	       XDisplayName (display_name));
      exit (1);
    }

  window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy), 0,0,10,10, 0,0,0);
  sel_property = XInternAtom (dpy, "VT_SELECTION", False);

  XConvertSelection (dpy, XA_PRIMARY, XA_STRING, sel_property, window,
		     CurrentTime);

  while (1)
    {
      XNextEvent (dpy, &event);
      if (event.type == SelectionNotify)
	{
	  paste_primary (dpy, event.xselection.requestor,
			 event.xselection.property, True);
	  exit (0);
	}
    }
}
