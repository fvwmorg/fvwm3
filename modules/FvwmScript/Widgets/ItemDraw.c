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

/* left, center and right offsets */
#define ITEM_DRAW_LCR_OFFSETS 1,0,1

/***************************************************/
/* Fonction pour ItemDraw / functions for ItemDraw */
/***************************************************/
void InitItemDraw(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int minHeight,minWidth;
  int asc,desc,dir;
  XCharStruct struc;
#ifdef I18N_MB
  char **ml;
  XFontStruct **fs_list;
#endif

  /* Enregistrement des couleurs et de la police */
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

  Attr.background_pixel = xobj->TabColor[back];
  mask = CWBackPixel;

  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
		xobj->x, xobj->y, xobj->width, xobj->height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		mask, &Attr);
  xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

#ifdef I18N_MB
  if ((xobj->xfontset = GetFontSetOrFixed(dpy,xobj->font)) == NULL) {
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

  /* Redimensionnement du widget */
  if (strlen(xobj->title) != 0)
    XTextExtents(xobj->xfont, "lp", strlen("lp"), &dir, &asc, &desc, &struc);
  else {
    asc = 0;
    desc = 0;
  }

  if (xobj->icon == NULL)
  {
    if (strlen(xobj->title) != 0)
    {
      /* title but no icon */
      minHeight = asc+desc+2;
      minWidth = XTextWidth(xobj->xfont, xobj->title, strlen(xobj->title))+2;
      if (xobj->height < minHeight)
	xobj->height = minHeight;
      if (xobj->width < minWidth)
	xobj->width = minWidth;
    }
  }
  else if (strlen(xobj->title)==0)
  {
    /* icon but no title */
    if (xobj->height<xobj->icon_h)
      xobj->height = xobj->icon_h;
    if (xobj->width<xobj->icon_w)
      xobj->width = xobj->icon_w;
  }
  else
  {
    /* title and icon */
    if (xobj->icon_w>XTextWidth(xobj->xfont, xobj->title, strlen(xobj->title))+2)
    {
      /* icon is wider than the title */
      if (xobj->width<xobj->icon_w)
	xobj->width = xobj->icon_w;
    }
    else
    {
      /* title is wider than icon */
      if (xobj->width < 
	  XTextWidth(xobj->xfont, xobj->title, strlen(xobj->title)) + 2)
	xobj->width =
	  XTextWidth(xobj->xfont, xobj->title, strlen(xobj->title))+2;
    }
    xobj->height = xobj->icon_h+asc+desc+2;
  }
  XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSelectInput(dpy, xobj->win, ExposureMask);
  /* x and y value of a clic */
  xobj->value2 = -1;
  xobj->value3 = -1;
}

void DestroyItemDraw(struct XObj *xobj)
{
  XFreeFont(dpy,xobj->xfont);
  XFreeGC(dpy,xobj->gc);
  XDestroyWindow(dpy,xobj->win);
}

void DrawItemDraw(struct XObj *xobj)
{
  XClearArea(dpy,xobj->win,0,0,xobj->width,xobj->height,False);
  XClearWindow(dpy,xobj->win);
  DrawIconStr(0,xobj,False,ITEM_DRAW_LCR_OFFSETS);
}

void EvtMouseItemDraw(struct XObj *xobj,XButtonEvent *EvtButton)
{
  unsigned int modif;
  int x1,x2,y1,y2;
  Window Win1,Win2;

  XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
  xobj->value2 = x2-xobj->x;
  xobj->value3 = y2-xobj->y;
  SendMsg(xobj,SingleClic);
}

void EvtKeyItemDraw(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];
  unsigned int modif;
  int x1,x2,y1,y2;
  Window Win1,Win2;

  XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
  xobj->value2 = x2-xobj->x;
  xobj->value3 = y2-xobj->y;
  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Left && xobj->value2 > 0) {
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, -1, 0);
  }
  else if (ks == XK_Right && xobj->value2 < xobj->width - 1) {
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 1, 0);
  }
  else if (ks == XK_Up && xobj->value3 > 0) {
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -1);
  }
  else if (ks == XK_Down && xobj->value3 < xobj->height - 1) {
    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, 1);
  }
  else if (ks == XK_Return) {
    EvtMouseItemDraw(xobj, NULL);
  }
}

void ProcessMsgItemDraw(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
