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
 * Fonction pour Rectangle
 */
void InitRectangle(struct XObj *xobj)
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

	xobj->gc=fvwmlib_XCreateGC(dpy,*xobj->ParentWin,0,NULL);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
	XSetBackground(dpy,xobj->gc,xobj->TabColor[back]);
	XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

	mask=0;
	xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
				-1000,-1000,xobj->width,xobj->height,0,
				CopyFromParent,InputOutput,CopyFromParent,
				mask,&Attr);
}

void DestroyRectangle(struct XObj *xobj)
{
	XFreeGC(dpy,xobj->gc);
	XDestroyWindow(dpy,xobj->win);
}

void DrawRectangle(struct XObj *xobj, XEvent *evp)
{
	XSegment segm[4];

	segm[0].x1=xobj->x;
	segm[0].y1=xobj->y;
	segm[0].x2=xobj->width+xobj->x-1;
	segm[0].y2=xobj->y;

	segm[1].x1=xobj->x;
	segm[1].y1=xobj->y;
	segm[1].x2=xobj->x;
	segm[1].y2=xobj->height+xobj->y-1;

	segm[2].x1=2+xobj->x;
	segm[2].y1=xobj->height-2+xobj->y;
	segm[2].x2=xobj->width-2+xobj->x;
	segm[2].y2=xobj->height-2+xobj->y;

	segm[3].x1=xobj->width-2+xobj->x;
	segm[3].y1=2+xobj->y;
	segm[3].x2=xobj->width-2+xobj->x;
	segm[3].y2=xobj->height-2+xobj->y;

	XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
	XDrawSegments(dpy,*xobj->ParentWin,xobj->gc,segm,4);


	segm[0].x1=1+xobj->x;
	segm[0].y1=1+xobj->y;
	segm[0].x2=xobj->width-1+xobj->x;
	segm[0].y2=1+xobj->y;

	segm[1].x1=1+xobj->x;
	segm[1].y1=1+xobj->y;
	segm[1].x2=1+xobj->x;
	segm[1].y2=xobj->height-1+xobj->y;

	segm[2].x1=1+xobj->x;
	segm[2].y1=xobj->height-1+xobj->y;
	segm[2].x2=xobj->width-1+xobj->x;
	segm[2].y2=xobj->height-1+xobj->y;

	segm[3].x1=xobj->width-1+xobj->x;
	segm[3].y1=1+xobj->y;
	segm[3].x2=xobj->width-1+xobj->x;
	segm[3].y2=xobj->height-1+xobj->y;

	XSetForeground(dpy,xobj->gc,xobj->TabColor[hili]);
	XDrawSegments(dpy,*xobj->ParentWin,xobj->gc,segm,4);

}

void EvtMouseRectangle(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyRectangle(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgRectangle(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
