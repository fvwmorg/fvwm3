/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "libs/fvwmlib.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "Tools.h"

/*
 * Fonction pour VScrollBar
 */
void DrawThumbV(struct XObj *xobj, XEvent *evp)
{
	int x,y,w,h;
	XSegment segm;
	char str[20];

	x = xobj->width/2 - 10;
	y = 2 +
		(xobj->height - 36)*(xobj->value - xobj->value3) /
		(xobj->value2 - xobj->value3);
	w = 20;
	h = 32;
	DrawReliefRect(x, y, w, h, xobj, hili, shad);
	segm.x1 = x + 3;
	segm.y1 = y + 15;
	segm.x2 = x + w - 3;
	segm.y2 = y + 15;
	XSetForeground(dpy, xobj->gc, xobj->TabColor[shad]);
	XDrawSegments(dpy, xobj->win, xobj->gc, &segm, 1);
	segm.x1 = x + 3;
	segm.y1 = y + 16;
	segm.x2 = x + w - 3;
	segm.y2 = y + 16;
	XSetForeground(dpy, xobj->gc, xobj->TabColor[hili]);
	XDrawSegments(dpy, xobj->win, xobj->gc, &segm, 1);
	XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

	sprintf(str, "%d", xobj->value);
	x = x-FlocaleTextWidth(xobj->Ffont, str, strlen(str))-6;
	y = y + 13 + xobj->Ffont->ascent/2;
	MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
		     !xobj->flags[1], NULL, evp);
}

void HideThumbV(struct XObj *xobj)
{
	int x,y;
	char str[20];

	x = xobj->width/2 - 10;
	y = 2 +
		(xobj->height - 36) * (xobj->value - xobj->value3) /
		(xobj->value2 - xobj->value3);
	XClearArea(dpy, xobj->win, x, y, 20, 32, False);
	sprintf(str, "%d", xobj->value);
	XClearArea(
		dpy, xobj->win, 0, 0,xobj->width/2 - 14,xobj->height, False);
}

void InitVScrollBar(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;
	int i,j;
	char str[20];

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

	mask = 0;
	Attr.background_pixel = xobj->TabColor[back];
	mask |= CWBackPixel;
	Attr.cursor = XCreateFontCursor(dpy,XC_hand2);
	mask |= CWCursor;             /* Curseur pour la fenetre */

	xobj->win = XCreateWindow(
		dpy, *xobj->ParentWin, xobj->x, xobj->y, xobj->width,
		xobj->height, 0, CopyFromParent, InputOutput, CopyFromParent,
		mask, &Attr);
	xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
	XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);


	if ((xobj->Ffont = FlocaleLoadFont(dpy, xobj->font, ScriptName)) ==
	    NULL)
	{
		fprintf(
			stderr, "%s: Couldn't load font. Exiting!\n",
			ScriptName);
		exit(1);
	}
	if (xobj->Ffont->font != NULL)
		XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

	XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

	if ((xobj->value3 - xobj->value2) <= 0)
		xobj->value3 = xobj->value2 + 10;
	if (!((xobj->value >= xobj->value2) && (xobj->value <= xobj->value3)))
		xobj->value = xobj->value2;

	i = (xobj->Ffont->height)*2+30;
	if (xobj->height < i)
		xobj->height = i;
	sprintf(str, "%d", xobj->value2);
	i = FlocaleTextWidth(xobj->Ffont, str, strlen(str));
	sprintf(str, "%d", xobj->value3);
	j = FlocaleTextWidth(xobj->Ffont, str, strlen(str));
	if (i<j)
		i = j*2+30;
	else
		i = i*2+30;
	xobj->width = i;
	XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
	if (xobj->colorset >= 0)
		SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
				    &Colorset[xobj->colorset], Pdepth,
				    xobj->gc, True);
	XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyVScrollBar(struct XObj *xobj)
{
	FlocaleUnloadFont(dpy,xobj->Ffont);
	XFreeGC(dpy,xobj->gc);
	XDestroyWindow(dpy,xobj->win);
}

void DrawVScrollBar(struct XObj *xobj, XEvent *evp)
{
	int x,y,w,h;
	char str[20];

	/* Calcul de la taille de l'ascenseur */
	x = xobj->width/2 - 12;
	y = 0;
	w = 24;
	h = xobj->height;
	DrawThumbV(xobj, evp);
	DrawReliefRect(x, y, w, h, xobj, shad, hili);
	/* Ecriture des valeurs */
	x = x + 26;
	y = xobj->Ffont->ascent + 2;
	sprintf(str, "%d", xobj->value3);
	MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
		     !xobj->flags[1], NULL, evp);
	sprintf(str, "%d", xobj->value2);
	y = h - xobj->Ffont->descent - 2;
	MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
		     !xobj->flags[1], NULL, evp);
}

void EvtMouseVScrollBar(struct XObj *xobj, XButtonEvent *EvtButton)
{
	static XEvent event;
	int oldy = 0;
	int oldvalue = -1;
	int newvalue;
	int x1,y1,x2,y2;
	Window Win1,Win2;
	unsigned int modif;
	fd_set in_fdset;

	do
	{
		/* On suit les mouvements de la souris */
		FQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2,
			      &x1, &y1, &x2, &y2, &modif);
		y2 = y2 - xobj->y;
		if (y2 < 15)
			y2 = 15;
		if (y2 > xobj->height - 21)
			y2 = xobj->height - 21;
		if (oldy != y2)
		{
			oldy = y2;
			/* calcule de xobj->value */
			newvalue = (y2-15)*xobj->height/(xobj->height - 36) *
				(xobj->value2 - xobj->value3) /
				(xobj->height) + xobj->value3;
			if (newvalue!=oldvalue)
			{
				HideThumbV(xobj);
				xobj->value = newvalue;
				DrawThumbV(xobj, NULL);
				oldvalue = newvalue;
				SendMsg(xobj,SingleClic);
				XSync(dpy,0);
				usleep(10000);
			}
		}
		FD_ZERO(&in_fdset);
		FD_SET(x_fd, &in_fdset);
		select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
	}
	while (!FCheckTypedEvent(dpy, ButtonRelease, &event) &&
	       EvtButton != NULL);
}

void EvtKeyVScrollBar(struct XObj *xobj, XKeyEvent *EvtKey)
{
	KeySym ks;
	unsigned char buf[10];

	XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
	if (ks == XK_Down && xobj->value > 0) {
		HideThumbV(xobj);
		xobj->value--;
		DrawThumbV(xobj, NULL);
		SendMsg(xobj,SingleClic);
	}
	else if (ks == XK_Up &&
		 xobj->value <
		 xobj->width*(xobj->value3-xobj->value2) /
		 (xobj->width)+xobj->value2) {
		HideThumbV(xobj);
		xobj->value++;
		DrawThumbV(xobj, NULL);
		SendMsg(xobj,SingleClic);
	}
	else if (ks == XK_Return) {
		EvtMouseVScrollBar(xobj, NULL);
	}
}

void ProcessMsgVScrollBar(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
