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

#include "Tools.h"
#include <time.h>

/******************************************************/
/******************************************************/
/* Fonction pour PopupMenu / function for PopupMenu   */
/******************************************************/
/******************************************************/

void InitPopupMenu(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int i;
  char *str;
  int asc,desc,dir;
  XCharStruct struc;
#ifdef I18N_MB
  char **ml;
  XFontStruct **fs_list;
#endif

  /* Enregistrement des couleurs et de la police / set the colors and the font */
  if (xobj->colorset >= 0) {
    xobj->TabColor[fore] = Colorset[xobj->colorset].fg;
    xobj->TabColor[back] = Colorset[xobj->colorset].bg;
    xobj->TabColor[hili] = Colorset[xobj->colorset].hilite;
    xobj->TabColor[shad] = Colorset[xobj->colorset].shadow;
  } else {
    xobj->TabColor[fore] = GetColor(xobj->forecolor);
    xobj->TabColor[back] = GetColor(xobj->backcolor);
    xobj->TabColor[hili] = GetColor(xobj->hilicolor);
    xobj->TabColor[shad] = GetColor(xobj->shadcolor);
  }

  mask = 0;
  Attr.background_pixel = xobj->TabColor[back];
  mask |= CWBackPixel;
  Attr.cursor = XCreateFontCursor(dpy, XC_hand2);
  mask |= CWCursor;		/* Curseur pour la fenetre / window cursor */

  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
		xobj->x, xobj->y, xobj->width, xobj->height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		mask, &Attr);
  xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

#ifdef I18N_MB
  if ((xobj->xfontset = GetFontSetOrFixed(dpy, xobj->font)) == NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
  XFontsOfFontSet(xobj->xfontset, &fs_list, &ml);
  xobj->xfont = fs_list[0];
#else
  if ((xobj->xfont = GetFontOrFixed(dpy, xobj->font)) == NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
#endif
  XSetFont(dpy, xobj->gc, xobj->xfont->fid);

  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);
  xobj->value3 = CountOption(xobj->title);
  if (xobj->value > xobj->value3)
    xobj->value = xobj->value3;
  if (xobj->value < 1)
    xobj->value = 1;

  /* Redimensionnement du widget / Widget resizing */
  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  xobj->height = asc +desc +12;
  xobj->width = 30;
  for (i = 1; i <= xobj->value3; i++)
  {
    str = (char*)GetMenuTitle(xobj->title, i);
    if (xobj->width<XTextWidth(xobj->xfont, str, strlen(str))+34)
      xobj->width = XTextWidth(xobj->xfont, str, strlen(str))+34;
    free(str);
  }
  XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyPopupMenu(struct XObj *xobj)
{
  XFreeFont(dpy, xobj->xfont);
  XFreeGC(dpy, xobj->gc);
  XDestroyWindow(dpy, xobj->win);
}

void DrawPopupMenu(struct XObj *xobj)
{
  XSegment segm[4];
  char* str;
  int x,y;
  int asc,desc,dir;
  XCharStruct struc;

  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  DrawReliefRect(0, 0, xobj->width, xobj->height, xobj, hili, shad);

  /* Dessin de la fleche / arrow drawing */
  segm[0].x1 = 7;
  segm[0].y1 = asc;
  segm[0].x2 = 19;
  segm[0].y2 = asc;
  segm[1].x1 = 8;
  segm[1].y1 = asc;
  segm[1].x2 = 13;
  segm[1].y2 = 5 + asc;
  segm[2].x1 = 6;
  segm[2].y1 = asc - 1;
  segm[2].x2 = 19;
  segm[2].y2 = 0 + asc -1;
  segm[3].x1 = 7;
  segm[3].y1 = asc;
  segm[3].x2 = 12;
  segm[3].y2 = 5 +asc;
  XSetForeground(dpy, xobj->gc, xobj->TabColor[shad]);
  XDrawSegments(dpy, xobj->win, xobj->gc, segm, 4);
  segm[0].x1 = 17;
  segm[0].y1 = asc + 1;
  segm[0].x2 = 13;
  segm[0].y2 = 5 + asc;
  segm[1].x1 = 19;
  segm[1].y1 = asc;
  segm[1].x2 = 14;
  segm[1].y2 = 5 + asc;
  XSetForeground(dpy, xobj->gc, xobj->TabColor[hili]);
  XDrawSegments(dpy, xobj->win, xobj->gc, segm, 2);

  /* Dessin du titre du popup menu */
  str = (char*)GetMenuTitle(xobj->title, xobj->value);
  y = asc + 5;
  x = GetXTextPosition(xobj,xobj->width,
		       XTextWidth(xobj->xfont,str,strlen(str)),
		       25,8,8);
  DrawString(dpy, xobj, xobj->win, x, y, str, strlen(str), fore, hili, back,
	     !xobj->flags[1]);

  free(str);
}

void EvtMousePopupMenu(struct XObj *xobj, XButtonEvent *EvtButton)
{
  static XEvent event;
  int x,y,hOpt,yMenu,hMenu;
  int x1,y1,x2,y2,oldy;
  int oldvalue = 0;
  int newvalue = 0;
  Window Win1,Win2,WinPop;
  unsigned int modif;
  unsigned long mask;
  unsigned long while_mask;
  XSetWindowAttributes Attr;
  int asc,desc,dir;
  XCharStruct struc;
  Time start_time = 0;
  KeySym ks;
  unsigned char buf[10];
  Bool End = 1;

  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  hOpt = asc + desc + 10;
  xobj->value3 = CountOption(xobj->title);
  yMenu = xobj->y - ((xobj->value-1) * hOpt);
  hMenu = xobj->value3 * hOpt;

  /* Creation de la fenetre menu */
  XTranslateCoordinates(dpy, *xobj->ParentWin, Root,
			xobj->x, yMenu, &x, &y, &Win1);
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x+xobj->width > XDisplayWidth(dpy, screen))
    x = XDisplayWidth(dpy, screen) - xobj->width;
  if (y+hMenu > XDisplayHeight(dpy, screen))
    y = XDisplayHeight(dpy, screen) - hMenu;
  mask = 0;
  Attr.background_pixel = xobj->TabColor[back];
  mask |= CWBackPixel;
  Attr.border_pixel = 0;
  mask |= CWBorderPixel;
  Attr.colormap = Pcmap;
  mask |= CWColormap;
  Attr.cursor = XCreateFontCursor(dpy, XC_hand2);
  mask |= CWCursor;		/* Curseur pour la fenetre / Window cursor */
  Attr.override_redirect = True;
  mask |= CWOverrideRedirect;
  WinPop = XCreateWindow(dpy, Root, x, y, xobj->width, hMenu, 0,
			 Pdepth, InputOutput, Pvisual, mask, &Attr);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, WinPop, xobj->width, hMenu,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XMapRaised(dpy, WinPop);

  /* Dessin du menu / Drawing the menu */
  DrawPMenu(xobj, WinPop, hOpt, 0);
  XGrabPointer(dpy, WinPop, True, GRAB_EVMASK, GrabModeAsync, GrabModeAsync,
	       None, None, CurrentTime);
  if (EvtButton == NULL) {
    while_mask = ButtonPress;
  }
  else {
    start_time = EvtButton->time;
    while_mask = ButtonRelease;
  }

  while (End)
  {
    XQueryPointer(dpy, Root, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
    /* Determiner l'option courante / Current option */
    y2 = y2 - y;
    x2 = x2 - x;
    oldy = y2;
    /* calcule de xobj->value / compute xobj->value */
    if ((x2 > 0) && (x2 < xobj->width) && (y2 > 0) && (y2 < hMenu))
      newvalue = y2 / hOpt+1;
    else
      newvalue = 0;
    if (newvalue != oldvalue)
    {
      UnselectMenu(xobj, WinPop, hOpt, oldvalue, xobj->width, asc, 0);
      SelectMenu(xobj, WinPop, hOpt, newvalue);
      oldvalue = newvalue;
    }

    XNextEvent(dpy, &event);
    switch (event.type)
    {
    case KeyPress:
      XLookupString(&event.xkey, (char *)buf, sizeof(buf), &ks, NULL);
      if (ks == XK_Escape) {
	newvalue = 0;
	End = 0;
      }
      else if (ks == XK_Return) {
	End = 0;
      }
      else if (ks == XK_Up && y2 >= hOpt) {
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -hOpt);
      }
      else if (ks == XK_Down && y2 + hOpt <= hMenu) {
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, hOpt);
      }
      break;
    case ButtonPress:
      if (while_mask == ButtonPress)
	End = 0;
      break;
    case ButtonRelease:
      if (start_time != 0 && event.xbutton.time - start_time < MENU_DRAG_TIME) {
	while_mask = ButtonPress;
	start_time = 0;
      }
      if (while_mask == ButtonRelease)
	End = 0;
      break;
    }
  }
#if 0
  do
  {
    XQueryPointer(dpy, Root, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
    /* Determiner l'option courante / Current option */
    y2 = y2 - y;
    x2 = x2 - x;
    {
      oldy = y2;
      /* calcule de xobj->value / compute xobj->value */
      if ((x2 > 0) && (x2 < xobj->width) && (y2 > 0) && (y2 < hMenu))
	newvalue = y2 / hOpt+1;
      else
	newvalue = 0;
      if (newvalue != oldvalue)
      {
	UnselectMenu(xobj, WinPop, hOpt, oldvalue, xobj->width, asc, 0);
	SelectMenu(xobj, WinPop, hOpt, newvalue);
	oldvalue = newvalue;
      }
    }
    FD_ZERO(&in_fdset);
    FD_SET(x_fd, &in_fdset);
    select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
  }
  while (!XCheckTypedEvent(dpy, while_mask, &event));
#endif
 XUngrabPointer(dpy, CurrentTime);
 XSync(dpy,0);

 XDestroyWindow(dpy, WinPop);
 if (newvalue != 0)
 {
   xobj->value = newvalue;
   SendMsg(xobj, SingleClic);
 }
}


void EvtKeyPopupMenu(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];

  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Return) {
    EvtMousePopupMenu(xobj, NULL);
  }
}

void ProcessMsgPopupMenu(struct XObj *xobj, unsigned long type, unsigned long *body)
{
}
