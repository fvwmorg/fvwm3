/* This program is free software; you can redistribute it and/or modify
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

#include<stdio.h>
#include "config.h"
#include "libs/fvwmlib.h"

/***************************************************************************
 * create and initialize the structure used to stash graphics things
 **************************************************************************/
Graphics *CreateGraphics(Display *dpy) {
  int screen = DefaultScreen(dpy);
  Graphics *G = (Graphics *)safemalloc(sizeof(Graphics));

  memset(G, 0, sizeof(Graphics));
  G->viz = DefaultVisual(dpy, screen);
  G->cmap = DefaultColormap(dpy, screen);
  G->depth = DefaultDepth(dpy, screen);
  G->usingDefaultVisual = True;
  return G;
}

/***************************************************************************
 * Initialises graphics stuff from a graphics config line sent by fvwm
 **************************************************************************/
void ParseGraphics(Display *dpy, char *line, Graphics *G) {
  int viscount;
  XVisualInfo vizinfo, *xvi;
  VisualID vid;
  Colormap cmap;

  /* ignore it if not the first time */
  if (!G->usingDefaultVisual)
    return;

  /* bail out on short lines */
  if (strlen(line) < (size_t)(DEFGRAPHLEN + 2 * DEFGRAPHNUM))
    return;

  if (sscanf(line + DEFGRAPHLEN + 1, "%lx %lx\n", &vid, &cmap) != DEFGRAPHNUM)
    return;

  /* if this is the first one grab a visual to use */
  vizinfo.visualid = vid;
  xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
  if (viscount != 1)
    return;
  G->viz = xvi->visual;
  G->depth = xvi->depth;
  G->cmap = cmap;
  G->usingDefaultVisual = False;
}
