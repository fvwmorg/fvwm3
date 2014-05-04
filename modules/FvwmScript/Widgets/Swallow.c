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
#include "libs/Strings.h"
#include "Tools.h"

extern int fd[2];

/*
 * Fonction pour Swallow
 */

void DrawRelief(struct XObj *xobj)
{
	XSegment segm[2];
	int i;

	if (xobj->value!=0)
	{
		for (i=1;i<2;i++)
		{
			segm[0].x1=xobj->x-i;
			segm[0].y1=xobj->y-i;
			segm[0].x2=xobj->x+xobj->width+i-2;
			segm[0].y2=xobj->y-i;
			segm[1].x1=xobj->x-i;
			segm[1].y1=xobj->y-i;
			segm[1].x2=xobj->x-i;
			segm[1].y2=xobj->y+xobj->height+i-2;
			if (xobj->value==-1)
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[shad]);
			else
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[hili]);
			XDrawSegments(dpy,x11base->win,xobj->gc,segm,2);

			segm[0].x1=xobj->x-i;
			segm[0].y1=xobj->y+xobj->height+i-1;
			segm[0].x2=xobj->x+xobj->width+i-1;
			segm[0].y2=xobj->y+xobj->height+i-1;
			segm[1].x1=xobj->x+xobj->width+i-1;
			segm[1].y1=xobj->y-i;
			segm[1].x2=xobj->x+xobj->width+i-1;
			segm[1].y2=xobj->y+xobj->height+i-1;
			if (xobj->value==-1)
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[hili]);
			else
				XSetForeground(
					dpy,xobj->gc,xobj->TabColor[shad]);
			XDrawSegments(dpy,x11base->win,xobj->gc,segm,2);
		}
	}

}

void InitSwallow(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;
	static char *my_sm_env = NULL;
	static char *orig_sm_env = NULL;
	static int len = 0;
	static Bool sm_initialized = False;
	static Bool session_manager = False;
	char *cmd;

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
	xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
				-1000,-1000,xobj->width,xobj->height,0,
				CopyFromParent,InputOutput,CopyFromParent,
				mask,&Attr);

	/* Redimensionnement du widget */
	if (xobj->height<30)
		xobj->height=30;
	if (xobj->width<30)
		xobj->width=30;

	if (xobj->swallow == NULL)
	{
		fprintf(stderr,"Error\n");
		return;
	}

	if (!sm_initialized)
	{
		/* use sm only when needed */
		sm_initialized = True;
		orig_sm_env = getenv("SESSION_MANAGER");
		if (orig_sm_env && !StrEquals("", orig_sm_env))
		{
			/* this set the new SESSION_MANAGER env */
			session_manager = fsm_init(x11base->TabArg[0]);
		}
	}

	if (!session_manager)
	{
		SendText(fd, xobj->swallow, 0);
		return;
	}

	if (my_sm_env == NULL)
	{
		my_sm_env = getenv("SESSION_MANAGER");
		len = 45 + strlen(my_sm_env) + strlen(orig_sm_env);
	}

	cmd = xmalloc(len + strlen(xobj->swallow));
	sprintf(
		cmd,
		"FSMExecFuncWithSessionManagment \"%s\" \"%s\" \"%s\"",
		my_sm_env, xobj->swallow, orig_sm_env);
	SendText(fd, cmd, 0);
	free (cmd);
}

void DestroySwallow(struct XObj *xobj)
{
	/* Arrete le programme swallow */
	if (xobj->win!=None)
		XKillClient(dpy, xobj->win);
	xobj->win = None;
}

void DrawSwallow(struct XObj *xobj, XEvent *evp)
{
	DrawRelief(xobj);
}

void EvtMouseSwallow(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeySwallow(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

/* Recupere le pointeur de la fenetre Swallow */
void CheckForHangon(struct XObj *xobj,unsigned long *body)
{
	char *cbody;

	cbody=(char*)calloc(strlen((char *)&body[3]) + 1,sizeof(char));
	sprintf(cbody,"%s",(char *)&body[3]);
	if(strcmp(cbody,xobj->title)==0)
	{
		xobj->win = (Window)body[0];
		free(xobj->title);
		xobj->title=(char*)calloc(sizeof(char),20);
		sprintf(xobj->title,"No window");
		XUnmapWindow(dpy,xobj->win);
		XSetWindowBorderWidth(dpy,xobj->win,0);
	}
	free(cbody);
}

void swallow(struct XObj *xobj,unsigned long *body)
{
	char cmd[256];

	if(xobj->win == (Window)body[0])
	{
		XReparentWindow(
			dpy,xobj->win,*xobj->ParentWin,xobj->x,xobj->y);
		XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
		XMapWindow(dpy,xobj->win);
		sprintf(
			cmd,"PropertyChange %u %u %lu %lu",
			MX_PROPERTY_CHANGE_SWALLOW, 1, xobj->win,
			(x11base->swallower_win)?
			x11base->swallower_win:x11base->win);
		SendText(fd,cmd,0);
		fsm_proxy(dpy, xobj->win, getenv("SESSION_MANAGER"));
	}
}

void ProcessMsgSwallow(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
	switch(type)
	{
	case M_MAP:
		swallow(xobj,body);
		break;
	case M_WINDOW_NAME:
	case M_RES_NAME:
	case M_RES_CLASS:
		CheckForHangon(xobj,body);
		break;
	}
}
