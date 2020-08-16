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
 * Function for VDipstick
 * Create a vertical slider
 * several options
 */
void InitVDipstick(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;

	/* Save colors and font */
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

	/* Minimum size */
	if (xobj->width<11)
		xobj->width=11;
	if (xobj->height<30)
		xobj->height=30;

	mask=0;
	Attr.background_pixel=x11base->TabColor[back];
	mask|=CWBackPixel;
	xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
				xobj->x,xobj->y,xobj->width,xobj->height,0,
				CopyFromParent,InputOutput,CopyFromParent,
				mask,&Attr);
	xobj->gc=fvwmlib_XCreateGC(dpy,xobj->win,0,NULL);
	if (xobj->colorset >= 0)
		SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
				    &Colorset[xobj->colorset], Pdepth,
				    xobj->gc, True);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
	XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

	if (xobj->value2>xobj->value3)
		xobj->value3=xobj->value2+50;
	if (xobj->value<xobj->value2)
		xobj->value=xobj->value2;
	if (xobj->value>xobj->value3)
		xobj->value=xobj->value3;
	XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyVDipstick(struct XObj *xobj)
{
	XFreeGC(dpy,xobj->gc);
	XDestroyWindow(dpy,xobj->win);
}

void DrawVDipstick(struct XObj *xobj, XEvent *evp)
{
	int  i;

	i=(xobj->height-4)*
		(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);

	DrawReliefRect(0,0,xobj->width,xobj->height,xobj,shad,hili);
	if (i!=0)
	{
		DrawReliefRect(
			2,xobj->height-i-2,xobj->width-4,i,xobj,hili,shad);
		XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
		XFillRectangle(
			dpy,xobj->win,xobj->gc,4,xobj->height-i,xobj->width-8,
			i-4);
	}

}

void EvtMouseVDipstick(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyVDipstick(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgVDipstick(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
