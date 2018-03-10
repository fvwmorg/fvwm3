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
 * Fonction pour RadioButton / Function for RadioButton
 */
void InitRadioButton(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;

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

	mask=0;
	Attr.cursor=XCreateFontCursor(dpy,XC_hand2);
	mask|=CWCursor;                /* Curseur pour la fenetre */
	Attr.background_pixel=xobj->TabColor[back];
	mask|=CWBackPixel;

	xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
				xobj->x,xobj->y,xobj->width,xobj->height,0,
				CopyFromParent,InputOutput,CopyFromParent,
				mask,&Attr);

	xobj->gc=fvwmlib_XCreateGC(dpy,xobj->win,0,NULL);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);

	if ((xobj->Ffont = FlocaleLoadFont(
		     dpy, xobj->font, ScriptName)) == NULL)
	{
		fprintf(
			stderr, "%s: Couldn't load font. Exiting!\n",
			ScriptName);
		exit(1);
	}
	if (xobj->Ffont->font != NULL)
		XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

	XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

	/* Redimensionnement du widget */
	xobj->height= xobj->Ffont->height +5;
	xobj->width=FlocaleTextWidth(
		xobj->Ffont,xobj->title,strlen(xobj->title))+20;
	XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
	if (xobj->colorset >= 0)
		SetWindowBackground(
			dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth, xobj->gc, True);

	XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyRadioButton(struct XObj *xobj)
{
	FlocaleUnloadFont(dpy,xobj->Ffont);
	XFreeGC(dpy,xobj->gc);
	XDestroyWindow(dpy,xobj->win);
}

void DrawRadioButton(struct XObj *xobj, XEvent *evp)
{
	int i,j;

	j=xobj->height/2+3;
	i=16;
	/* Dessin du cercle arrondi */
	XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
	XDrawArc(dpy,xobj->win,xobj->gc,1,j-11,11,11,45*64,180*64);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[hili]);
	XDrawArc(dpy,xobj->win,xobj->gc,1,j-11,11,11,225*64,180*64);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
	XDrawArc(dpy,xobj->win,xobj->gc,2,j-10,9,9,0*64,360*64);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
	if (xobj->value)
		XFillArc(dpy,xobj->win,xobj->gc,2,j-10,9,9,0*64,360*64);

	/* Calcul de la position de la chaine de charactere */
	MyDrawString(dpy,xobj,xobj->win,i,j,xobj->title,fore,hili,
		     back,!xobj->flags[1], NULL, evp);
}

void EvtMouseRadioButton(struct XObj *xobj,XButtonEvent *EvtButton)
{
	static XEvent event;
	int End=1;
	unsigned int modif;
	int x1,x2,y1,y2,j;
	Window Win1,Win2;
	Window WinBut=0;
	int In = 0;

	j=xobj->height/2+3;

	while (End)
	{
		FNextEvent(dpy, &event);
		switch (event.type)
		{
		case EnterNotify:
			FQueryPointer(dpy,*xobj->ParentWin,
				      &Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
			if (WinBut==0)
			{
				WinBut=Win2;
				/* Mouse on button */
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[back]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,9,9,
					0*64,360*64);
				In=1;
			}
			else
			{
				if (Win2==WinBut)
				{
					/* Mouse on button */
					XSetForeground(
						dpy,xobj->gc,
						xobj->TabColor[back]);
					XFillArc(
						dpy,xobj->win,xobj->gc,2,j-10,
						9,9,0*64,360*64);
					In=1;
				}
				else if (In)
				{
					In=0;
					/* Mouse not on button */
					XSetForeground(
						dpy,xobj->gc,
						xobj->TabColor[fore]);
					XFillArc(
						dpy,xobj->win,xobj->gc,2,j-10,
						9,9,0*64,360*64);
				}
			}
			break;
		case LeaveNotify:
			FQueryPointer(dpy,*xobj->ParentWin,
				      &Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
			if (Win2==WinBut)
			{
				In=1;
				/* Mouse on button */
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[back]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,8,8,
					0*64,360*64);
			}
			else if (In)
			{
				/* Mouse not on button */
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[back]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,8,8,
					0*64,360*64);
				In=0;
			}
			break;
		case ButtonRelease:
			End=0;
			/* Mouse not on button */
			if (In)
			{
				/* Envoie d'un message vide de type SingleClic
				 * pour un clique souris */
				xobj->value=1;
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[fore]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,8,8,
					0*64,360*64);
				SendMsg(xobj,SingleClic);
			}
			else if (xobj->value)
			{
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[fore]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,8,8,
					0*64,360*64);
			}
			else
			{
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[back]);
				XFillArc(
					dpy,xobj->win,xobj->gc,2,j-10,8,8,
					0*64,360*64);
			}
			break;
		}
	}
}

void EvtKeyRadioButton(struct XObj *xobj,XKeyEvent *EvtKey)
{
	KeySym ks;
	unsigned char buf[10];
	int j=xobj->height/2+3;

	XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
	if (ks == XK_Return) {
		xobj->value=1;
		XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
		XFillArc(
			dpy, xobj->win, xobj->gc, 2, j-10, 8, 8, 0*64, 360*64);
		SendMsg(xobj,SingleClic);
	}
}

void ProcessMsgRadioButton(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
