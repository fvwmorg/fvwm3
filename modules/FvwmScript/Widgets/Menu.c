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

/* left, center and right offsets for title 0 (not very usefull here) */
#define MENU_LCR_OFFSETS 0,0,0

/***********************************************/
/* Fonction pour Menu / Functions for the Menu */
/***********************************************/
void InitMenu(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int asc,desc,dir;
  XCharStruct struc;
  int i;
  char *Option;
#ifdef I18N_MB
  char **ml;
  XFontStruct **fs_list;
#endif

  /* Enregistrement des couleurs et de la police / fonts and colors */
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
  mask |= CWCursor;
  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
			    xobj->x, xobj->y, xobj->width, xobj->height, 0,
			    CopyFromParent, InputOutput, CopyFromParent,
			    mask, &Attr);
  xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

#ifdef I18N_MB
  if ((xobj->xfontset=GetFontSetOrFixed(dpy, xobj->font)) == NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
  XFontsOfFontSet(xobj->xfontset, &fs_list, &ml);
  xobj->xfont = fs_list[0];
#else
  if ((xobj->xfont=GetFontOrFixed(dpy, xobj->font))==NULL) {
    fprintf(stderr, "FvwmScript: Couldn't load font. Exiting!\n");
    exit(1);
  }
#endif
  XSetFont(dpy, xobj->gc, xobj->xfont->fid);

  /* Size */
  Option = GetMenuTitle(xobj->title, 0);
  xobj->width = XTextWidth(xobj->xfont, Option, strlen(Option))+6;
  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  xobj->height = asc + desc + 4;
  free(Option);

  /* Position */
  xobj->y = 2;
  xobj->x = 2;
  i = 0;
  while (xobj != tabxobj[i])
  {
    if (tabxobj[i]->TypeWidget == Menu)
    {
      Option = GetMenuTitle(tabxobj[i]->title, 0);
      xobj->x = xobj->x+XTextWidth(tabxobj[i]->xfont, Option, strlen(Option))+10;
      free(Option);
    }
    i++;
  }
  XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  XMoveWindow(dpy, xobj->win, xobj->x, xobj->y);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy,  xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyMenu(struct XObj *xobj)
{
  XFreeFont(dpy, xobj->xfont);
  XFreeGC(dpy, xobj->gc);
  XDestroyWindow(dpy, xobj->win);
}

void DrawMenu(struct XObj *xobj)
{
  XSegment segm[2];
  int i;

  /* Si c'est le premier menu, on dessine la bar de menu */
  /* if it is the first menu we draw the menu bar */
  if (xobj->x == 2)
  {
    for (i = 0;i<2;i++)
    {
      segm[0].x1 = i;
      segm[0].y1 = i;
      segm[0].x2 = x11base->size.width-i-1;
      segm[0].y2 = i;

      segm[1].x1 = i;
      segm[1].y1 = i;
      segm[1].x2 = i;
      segm[1].y2 = xobj->height-i+3;
      XSetForeground(dpy, xobj->gc, xobj->TabColor[hili]);
      XDrawSegments(dpy, x11base->win, xobj->gc, segm, 2);

      segm[0].x1 = 1+i;
      segm[0].y1 = xobj->height-i+3;
      segm[0].x2 = x11base->size.width-i-1;
      segm[0].y2 = xobj->height-i+3;

      segm[1].x1 = x11base->size.width-i-1;
      segm[1].y1 = i;
      segm[1].x2 = x11base->size.width - i - 1;
      segm[1].y2 = xobj->height - i + 3;
      XSetForeground(dpy, xobj->gc, xobj->TabColor[shad]);
      XDrawSegments(dpy, x11base->win, xobj->gc, segm, 2);
    }
  }
  DrawIconStr(0, xobj, True, MENU_LCR_OFFSETS);
}

void EvtMouseMenu(struct XObj *xobj, XButtonEvent *EvtButton)
{
  static XEvent event;
  unsigned int modif;
  int x1,x2,y1,y2,i,oldy;
  Window Win1,Win2,WinPop;
  char *str;
  int x,y,hOpt,yMenu,hMenu,wMenu;
  int oldvalue = 0;
  int newvalue = 0;
  unsigned long mask;
  unsigned long while_mask;
  XSetWindowAttributes Attr;
  int asc,desc,dir;
  XCharStruct struc;
  Time start_time = 0;
  KeySym ks;
  unsigned char buf[10];
  Bool End = 1;

  DrawReliefRect(0, 0, xobj->width, xobj->height, xobj, shad, hili);

  XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  hOpt = asc+desc+10;
  xobj->value3 = CountOption(xobj->title);
  /* Hauteur totale du menu / total height of the menu */
  hMenu = (xobj->value3 - 1) * hOpt;
  yMenu = xobj->y + xobj->height;
  wMenu = 0;
  for (i=2; i <= xobj->value3; i++)
  {
    str = (char*)GetMenuTitle(xobj->title, i);
    if (wMenu < XTextWidth(xobj->xfont, str, strlen(str))+20)
      wMenu = XTextWidth(xobj->xfont, str, strlen(str))+20;
    free(str);
  }

  /* Creation de la fenetre menu / Create the menu window */
  XTranslateCoordinates(dpy, *xobj->ParentWin, Root,
			xobj->x, yMenu, &x, &y, &Win1);
  if (x<0) x = 0;
  if (y<0) y = 0;
  if (x + wMenu > XDisplayWidth(dpy, screen))
    x = XDisplayWidth(dpy,screen) - wMenu;
  if (y + hMenu > XDisplayHeight(dpy, screen))
    y = y - hMenu - xobj->height;

  mask = 0;
  Attr.background_pixel = xobj->TabColor[back];
  mask |= CWBackPixel;
  Attr.border_pixel = 0;
  mask |=  CWBorderPixel;
  Attr.colormap = Pcmap;
  mask |= CWColormap;
  Attr.cursor = XCreateFontCursor(dpy, XC_hand2);
  /* Curseur pour la fenetre / cursor for the window */
  mask |= CWCursor;
  Attr.override_redirect = True;
  mask |= CWOverrideRedirect;
  WinPop = XCreateWindow(dpy, Root, x, y, wMenu-5, hMenu, 0, Pdepth,
			 InputOutput, Pvisual, mask, &Attr);
  if (xobj->colorset >=  0)
    SetWindowBackground(dpy, WinPop, wMenu - 5, hMenu,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XMapRaised(dpy, WinPop);

 /* Dessin du menu / Drawing the menu */
  DrawPMenu(xobj, WinPop, hOpt, 1);
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
    /* Determiner l'option courante */
    y2 = y2 - y;
    x2 = x2 - x;
    oldy = y2;
    /* calcule de xobj->value */
    if ((x2 > 0) && (x2 < wMenu) && (y2 > 0) && (y2 < hMenu))
      newvalue = y2 / hOpt+1;
    else
      newvalue = 0;
    if (newvalue != oldvalue)
    {
      UnselectMenu(xobj, WinPop, hOpt, oldvalue, wMenu-5, asc, 1);
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
      else if (ks == XK_Up && y2 >= 0) {
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
  XUngrabPointer(dpy, CurrentTime);
  XSync(dpy,0);
  XDestroyWindow(dpy, WinPop);

  if (newvalue != 0)
  {
    xobj->value = newvalue;
    SendMsg(xobj, SingleClic);
    xobj->value = 0;
  }
  XClearArea(dpy, xobj->win, 0, 0, xobj->width, xobj->height, True);
}

void EvtKeyMenu(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];

  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Return) {
    EvtMouseMenu(xobj, NULL);
  }
}

void ProcessMsgMenu(struct XObj *xobj, unsigned long type, unsigned long *body)
{
}
