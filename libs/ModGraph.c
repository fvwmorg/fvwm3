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
  char *envp;
  int screen = DefaultScreen(dpy);
  Graphics *G = (Graphics *)safemalloc(sizeof(Graphics));

  memset(G, 0, sizeof(Graphics));
  /* if fvwm has not set this env-var it is using the default visual */
  if ((envp = getenv("FVWM_VISUALID")) == NULL) {
    G->viz = DefaultVisual(dpy, screen);
    G->cmap = DefaultColormap(dpy, screen);
    G->depth = DefaultDepth(dpy, screen);
    G->usingDefaultVisual = True;
  } else {
    /* convert the env-vars to a visual and colormap */
    int viscount;
    XVisualInfo vizinfo, *xvi;

    sscanf(envp, "%lx", &vizinfo.visualid);
    xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
    G->viz = xvi->visual;
    G->depth = xvi->depth;
    sscanf(getenv("FVWM_COLORMAP"), "%lx", &G->cmap);
    G->usingDefaultVisual = False;
  }
  return G;
}
