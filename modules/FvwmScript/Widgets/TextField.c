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

#include "libs/Flocale.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/FEvent.h"
#include "libs/log.h"
#include "Tools.h"

/*
 * Function for TextField
 */

void InitTextField(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;
	int i;
	int num_chars;

	/* Save colors and fonts */
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
	Attr.cursor=XCreateFontCursor(dpy,XC_xterm);
	mask|=CWCursor; /* window cursor */
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
		fvwm_debug(__func__, "%s: Couldn't load font. Exiting!\n",
			   ScriptName);
		exit(1);
	}
	if (xobj->Ffont->font != NULL)
		XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

	XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);
	/* value2 gives the end of the selected zone */
	/* calculate number of characters in title */
	num_chars = FlocaleStringCharLength(xobj->Ffont, xobj->title);
	if (xobj->value > num_chars)
		xobj->value = num_chars;
	xobj->value2=xobj->value;
	/* left position of the visible title */
	xobj->value3=0;

	/* widget resizing */
	xobj->height= xobj->Ffont->height + 10;
	i = FlocaleTextWidth(xobj->Ffont,xobj->title,strlen(xobj->title))+40;
	if (xobj->width<i)
		xobj->width=i;
	XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
	if (xobj->colorset >= 0)
		SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
				    &Colorset[xobj->colorset], Pdepth,
				    xobj->gc, True);
	XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyTextField(struct XObj *xobj)
{
	FlocaleUnloadFont(dpy,xobj->Ffont);
	XFreeGC(dpy,xobj->gc);
	XDestroyWindow(dpy,xobj->win);
}

/* text cursor drawing */
void DrawPointTxt(struct XObj *xobj,unsigned int pixel)
{
#define dec 2
	int x,y;
	int offset_abs,offset_rel;
	XSegment segm[2];

	/* calculate byte offsets */
	offset_abs = FlocaleStringCharToByteOffset(xobj->Ffont,xobj->title,
						   xobj->value);
	offset_rel = FlocaleStringCharToByteOffset(xobj->Ffont,xobj->title,
						   xobj->value3);
	x = FlocaleTextWidth(xobj->Ffont, xobj->title + offset_rel,
			     offset_abs - offset_rel)+5;
	y = xobj->Ffont->ascent + 5;

	segm[0].x1=x;
	segm[0].y1=y;
	segm[0].x2=x-dec;
	segm[0].y2=y+dec;
	segm[1].x1=x;
	segm[1].y1=y;
	segm[1].x2=x+dec;
	segm[1].y2=y+dec;
	XSetForeground(dpy,xobj->gc,pixel);
	XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);
}

/* text field drawing */

void DrawTextField(struct XObj *xobj, XEvent *evp)
{
	int x1,y1;
	int x2,l;
	int nl=0;
	int right=0;
	int offset,offset2,offset3;
	int end_value;

	fvwm_debug(__func__, "DrawTextField: value %d value2 %d value3 %d\n",
		   xobj->value, xobj->value2, xobj->value3);

	y1 = xobj->Ffont->ascent;
	l=strlen(xobj->title);
	/* calculate byte offsets corresponding to value,value2,value3 */
	offset = getByteOffsetBoundsCheck(xobj->Ffont,xobj->title,xobj->value);
	offset2 = getByteOffsetBoundsCheck(xobj->Ffont,xobj->title,
					   xobj->value2);
	offset3 = getByteOffsetBoundsCheck(xobj->Ffont,xobj->title,
					   xobj->value3);
	/* value corresponding to behind last character */
	end_value = FlocaleStringCharLength(xobj->Ffont, xobj->title );
	if(offset == l)
		xobj->value = end_value;
	if(offset2 == l)
		xobj->value2 = end_value;
	if(offset3 == l)
		xobj->value3 = end_value;

	fvwm_debug(__func__,
		   "DrawTextField: offset: %d offset2 %d, offset3: %d\n",
		   offset, offset2, offset3);

	/*if (offset > l)
		xobj->value = FlocaleStringByteToCharOffset(xobj->Ffont,
							    xobj->title,
							    l-1) + 1;
	if (offset2 > l)
		xobj->value2 = FlocaleStringByteToCharOffset(xobj->Ffont,
							     xobj->title,
							     l-1) + 1; */
	DrawReliefRect(0,0,xobj->width,xobj->height,xobj,shad,hili);
	XClearArea(dpy,xobj->win,2,2,xobj->width-4,xobj->height-4,False);
	XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
	/* computation of the first visible character */
	while (l-nl >= 1 &&
	       FlocaleTextWidth(xobj->Ffont,xobj->title + nl,
				offset - nl) > (xobj->width-10))
	{
		nl += FlocaleStringNumberOfBytes(xobj->Ffont,xobj->title + nl);
		fvwm_debug(__func__, "nl: %d\n", nl);
	}
	/* now nl is the byte offset of the first the first visible
	   character */
	if (nl > offset3) /* the first visible character needs to be updated */
	{
		xobj->value3 = getCharOffsetBoundsCheck(xobj->Ffont,
							xobj->title, nl);
		/*FlocaleStringByteToCharOffset(xobj->Ffont,
		  xobj->title, nl);*/
		offset3 = nl;
	}
	else if (xobj->value3 > xobj->value)
	{
		xobj->value3--;
		offset3 = FlocaleStringCharToByteOffset(xobj->Ffont,
							xobj->title,
							xobj->value3);
	}
	fvwm_debug(__func__, "Got visible offset; %d char %d\n", offset3,
		   xobj->value3);
	/* computation of the length of the visible title */
	/* increase string until it won't fit anymore into into the textbox */
	right = offset3;
	while(right < l && FlocaleTextWidth(xobj->Ffont, xobj->title + offset3,
					    right - offset3) <=
	      xobj->width - 10)
	{
		right += FlocaleStringNumberOfBytes(xobj->Ffont,
						    xobj->title + right);
	}
	/* the string didn't fit? */
	if(FlocaleTextWidth(xobj->Ffont, xobj->title + offset3,
			    right - offset3) >
	   xobj->width - 10)
	{
		right -= FlocaleStringNumberOfBytes(
				xobj->Ffont,
				xobj->title +
				FlocaleStringCharToByteOffset(
				       xobj->Ffont, xobj->title,
				       FlocaleStringByteToCharOffset(
					      xobj->Ffont, xobj->title,
					      right) - 1));
	}
	/* unless we had an empty string, we would have already been at the
	   beginning */
	if(right < offset3)
		right = offset3;

	/*while ( l - offset3 - right >= 1 &&
		FlocaleTextWidth(xobj->Ffont,xobj->title + offset3,
				 l - offset3 - right) > (xobj->width-10))
	{
	right += FlocaleStringNumberOfBytes(xobj->Ffont,
	xobj->title + right);
		fprintf(stderr, "sl: %d\n", l - offset3 - right);
		} */
	fvwm_debug(__func__, "computed length of visible string\n");
	FwinString->win = xobj->win;
	FwinString->gc = xobj->gc;
	FwinString->x = 5;
	FwinString->y = y1+5;
	FwinString->str = xobj->title + offset3;
	FwinString->len = right - offset3;
	  /*strlen(xobj->title) - offset3 - right;*/
	FlocaleDrawString(dpy, xobj->Ffont, FwinString, FWS_HAVE_LENGTH);
#if 0
	XmbDrawString(dpy,xobj->win,xobj->xfontset,xobj->gc,5,y1+5,
		      xobj->title + xobj->value3,
		      strlen(xobj->title) - xobj->value3-right);
#endif

	/* Dessin de la zone selectionnee */
	/* selected zone drawing */
	XSetFunction(dpy,xobj->gc,GXinvert);
	if (xobj->value2>xobj->value)          /* Curseur avant la souris */
	{
		x1=FlocaleTextWidth(xobj->Ffont,&xobj->title[offset3],
				    offset - offset3);
		x2=FlocaleTextWidth(xobj->Ffont,&xobj->title[offset],
				    offset2 - offset);
	}
	else           /* Curseur apres la souris / cursor after the mouse */
	{
		x1=FlocaleTextWidth(xobj->Ffont,&xobj->title[offset3],
				    offset2 - offset3);
		x2=FlocaleTextWidth(xobj->Ffont,&xobj->title[offset2],
				    offset - offset2);
	}
	XFillRectangle(
		dpy,xobj->win,xobj->gc,x1+5,7,x2,y1+xobj->Ffont->descent-2);
	XSetFunction(dpy,xobj->gc,GXcopy);

	/* insertion point drawing */
	DrawPointTxt(xobj,xobj->TabColor[fore]);
}

void EvtMouseTextField(struct XObj *xobj,XButtonEvent *EvtButton)
{
	unsigned int modif;
	int x1,x2,y1,y2,i;
	Window Win1,Win2;
	int PosCurs=0;
	int SizeBuf;
	char *str;
	int NewPos;
	Atom type;
	XEvent event;
	int ButPress=1;
	int format;
	unsigned long longueur,octets_restant;
	unsigned char *donnees=(unsigned char *)"";
	XRectangle rect;
	int start_pos, selection_pos, curs_pos;

	/* We move the cursor at mouse position and we get the mouse position
	 */
	switch (EvtButton->button)
	{
	case Button1:
		FQueryPointer(
			dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,
			&modif);
		x2=x2-xobj->x;
		/* see where we clicked */
		PosCurs=0;
		/* byte position of first visible character */
		start_pos = getByteOffsetBoundsCheck(xobj->Ffont, xobj->title,
						     xobj->value3);
		/* cursor offset in bytes */
		curs_pos = 0;
		while ((curs_pos < strlen(xobj->title + start_pos)) &&
		       (x2 > FlocaleTextWidth(
				    xobj->Ffont,xobj->title + start_pos,
				    curs_pos) + 8))
		{
			curs_pos += FlocaleStringNumberOfBytes(
					  xobj->Ffont,
					  xobj->title + start_pos + curs_pos);
			PosCurs++;
		}
		DrawPointTxt(xobj,xobj->TabColor[back]);
		/* set selection start and end where clicked */
		/* first visible char + position clicked */
		xobj->value = PosCurs + xobj->value3;
		xobj->value2 = PosCurs + xobj->value3;
		/* byte offset corresponding to the above */
		start_pos = getByteOffsetBoundsCheck(xobj->Ffont, xobj->title,
						     xobj->value3);
		selection_pos = getByteOffsetBoundsCheck(xobj->Ffont,
							 xobj->title,
							 xobj->value3);
		DrawPointTxt(xobj,xobj->TabColor[fore]);
		DrawTextField(xobj,NULL);

		while (ButPress)
		{
			FNextEvent(dpy, &event);
			switch (event.type)
			{
			case MotionNotify:
				FQueryPointer(
					dpy,*xobj->ParentWin,&Win1,&Win2,&x1,
					&y1,&x2,&y2,&modif);
				x2=x2-xobj->x;
				PosCurs=0;
				curs_pos = 0;
				/* determine how far in the mouse is now */
				while ((curs_pos < strlen(
						xobj->title + start_pos)) &&
				       (x2 >
					FlocaleTextWidth(
						xobj->Ffont,xobj->title+
						start_pos, curs_pos) + 8))
				{
					curs_pos += FlocaleStringNumberOfBytes(
							   xobj->Ffont,
							   xobj->title +
							   start_pos +
							   curs_pos);
					PosCurs++;
				}
				/* limitation of the drawing zone */
				/* these 2 if-statements updates current
				   cursor position of the widget if needed */
				if (PosCurs > (xobj->value2 - xobj->value3))
				{
					/* select made "forward" */
					rect.x=
						FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+
							start_pos,
							selection_pos -
							start_pos);
					rect.y=0;
					rect.width=
						FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+
							start_pos,
							curs_pos)
						-rect.x+1;
					rect.height=xobj->height;
					xobj->value2 = PosCurs + xobj->value3;
					DrawTextField(xobj,NULL);
				}
				else if (PosCurs <
					 (xobj->value2 - xobj->value3))
				{
					/* selection made "backwards" */
					rect.x=FlocaleTextWidth(
						xobj->Ffont,
						xobj->title+
						start_pos,
						curs_pos) - 1;
					rect.y=0;
					rect.width=FlocaleTextWidth(
						xobj->Ffont,
						xobj->title+
						start_pos,
						selection_pos - start_pos) -
						rect.x+2;
					rect.height=xobj->height;
					xobj->value2= PosCurs + xobj->value3;
					DrawTextField(xobj,NULL);
				}

				break;
			case ButtonRelease:
				ButPress=0;
				break;
			}
		}

		/* Save selection zone in clipboard */
		/* Program becomes the selection owner */
		/* selection stuff: get the selection */
		if (xobj->value != xobj->value2)
		{
			str=(char*)GetText(xobj, xobj->value2);
			for (i=0;i<=7;i++)
				XStoreBuffer(dpy,str,strlen(str),i);
			Scrapt = (char*)realloc(
					(void*)Scrapt,
					(strlen(str)+2)*sizeof(char));
			Scrapt = strcpy(Scrapt,str);
			free(str);
			x11base->HaveXSelection=True;
			XSetSelectionOwner(
				dpy,XA_PRIMARY,x11base->win,EvtButton->time);
			SelectOneTextField(xobj);
		}
		break;

	case Button2:                        /* Paste text */
		/* If application has no selection, it asks for one
		 */
		/* else it reads its clipboard */
		/* read the selection */
		if (!x11base->HaveXSelection)
		{
			/* ask for the selection */
			XConvertSelection(
				dpy,XA_PRIMARY,XA_STRING,propriete,
				*xobj->ParentWin, EvtButton->time);
			while (!(FCheckTypedEvent(dpy,SelectionNotify,&event)))
				;
			if (event.xselection.property!=None)
				if (event.xselection.selection==XA_PRIMARY)
				{
					XGetWindowProperty(
						dpy,event.xselection.requestor,
						event.xselection.property,0L,
						8192L,False,
						event.xselection.target,&type,
						&format, &longueur,
						&octets_restant,&donnees);
					if (longueur>0)
					{
						Scrapt=(char*)realloc(
							(void*)Scrapt,
							(longueur+1)*
							sizeof(char));
						Scrapt=strcpy(
							Scrapt,(char *)donnees);
						XDeleteProperty(
							dpy, event.xselection.
							requestor,
							event.xselection.
							property);
						XFree(donnees);
					}
				}
		}
		SizeBuf=strlen(Scrapt);
		if (SizeBuf>0)
		{
			NewPos=InsertText(xobj,Scrapt,SizeBuf);
			DrawPointTxt(xobj,xobj->TabColor[back]);
			xobj->value=NewPos;
			xobj->value2=NewPos;
			DrawPointTxt(xobj,xobj->TabColor[fore]);
			DrawTextField(xobj,NULL);
			SendMsg(xobj,SingleClic);
		}
		break;

	case Button3:                /* Push on third button */
		FQueryPointer(
			dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,
			&modif);
		x2=x2-xobj->x;
		PosCurs=0;
		while ((PosCurs<strlen(xobj->title))&&
		       (x2>FlocaleTextWidth(
			       xobj->Ffont,xobj->title+xobj->value3,
			       PosCurs)+8))
			PosCurs++;
		if ((PosCurs<xobj->value) && (xobj->value<xobj->value2))
			xobj->value=xobj->value2;
		if ((PosCurs>xobj->value) && (xobj->value>xobj->value2))
			xobj->value=xobj->value2;
		xobj->value2=PosCurs+xobj->value3;
		DrawTextField(xobj,NULL);

		while (ButPress)
		{
			FNextEvent(dpy, &event);
			switch (event.type)
			{
			case MotionNotify:
				FQueryPointer(
					dpy,*xobj->ParentWin,&Win1,&Win2,&x1,
					&y1,&x2,&y2,&modif);
				x2=x2-xobj->x;
				while ((PosCurs<strlen(xobj->title))&&
				       (x2 >
					FlocaleTextWidth(
						xobj->Ffont,
						xobj->title+xobj->value3,
						PosCurs)+8))
					PosCurs++;
				if (PosCurs>xobj->value2)
				{
					rect.x=
						FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+
							xobj->value3,
							xobj->value2);
					rect.y=0;
					rect.width=
						FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+
							xobj->value3,PosCurs+1)
						-rect.x+1;
					rect.height=xobj->height;
					xobj->value2=PosCurs;
					DrawTextField(xobj,NULL);
				}
				else
					if (PosCurs<xobj->value2)
					{
						rect.x=FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+
							xobj->value3,
							PosCurs)-1;
						rect.y=0;
						rect.width=FlocaleTextWidth(
							xobj->Ffont,
							xobj->title+xobj->
							value3,
							xobj->value2+1)-
							rect.x+2;
						rect.height=xobj->height;
						xobj->value2=
							PosCurs+xobj->value3;
						DrawTextField(xobj,NULL);
					}
				PosCurs=0;
				break;
			case ButtonRelease:
				ButPress=0;
				break;
			}
		}
		if (xobj->value!=xobj->value2)
		{
			str=(char*)GetText(xobj,xobj->value2);
			for (i=0;i<=7;i++)
				XStoreBuffer(dpy,str,strlen(str),i);
			Scrapt=(char*)realloc(
				(void*)Scrapt,(strlen(str)+2)*sizeof(char));
			Scrapt=strcpy(Scrapt,str);
			free(str);
			x11base->HaveXSelection=True;
			XSetSelectionOwner(
				dpy,XA_PRIMARY,x11base->win,EvtButton->time);
		}
		break;
	}
}

void EvtKeyTextField(struct XObj *xobj,XKeyEvent *EvtKey)
{
	int i,x2;
	char car[11];
	char* carks2;
	char* carks = NULL;
	KeySym ks;
	int Size;
	int NewPos;
	int new_value;

	/* Find character */
	i=XLookupString(EvtKey,car,10,&ks,NULL);
	/* calculate byte offset from current position */
	NewPos = getByteOffsetBoundsCheck(xobj->Ffont, xobj->title,
					  xobj->value);
	car[i]='\0';
	carks2=XKeysymToString(ks);

	if (carks2!=NULL)
	{
		carks=fxstrdup(carks2);
		if (strcmp(carks,"Right")==0)
		{
			/* if we are at the end, don't go further */
			if (NewPos >= strlen(xobj->title))
				NewPos = strlen(xobj->title);
			/* otherwise step forward as many bytes as the
			   next charcter at the insertion point is wide */
			else
				NewPos +=
					FlocaleStringNumberOfBytes(
					       xobj->Ffont,
					       xobj->title + NewPos);
		}
		else if (strcmp(carks,"Left")==0)
		{
			/* if we are at the beginning, don't go further */
			if (NewPos <= 0)
				NewPos=0;
			/* otherwise step back as many bytes as the chacter
			   behind the insertion point */
			else
				NewPos =
					FlocaleStringCharToByteOffset(
						   xobj->Ffont,
						   xobj->title,
						   xobj->value - 1);
		}
		else if (strcmp(carks,"Return")==0)
		{
			;
		}
		else if ((strcmp(carks,"Delete")==0)||
			 (strcmp(carks,"BackSpace")==0))
		{
			if (NewPos>0)
			{
				/* need to delete the "right" amount of
				   bytes from the string, we might be using
				   multi-byte strings */
				int PrevPos;
				Size=strlen(xobj->title);
				PrevPos =
					FlocaleStringCharToByteOffset(
					       xobj->Ffont,
					       xobj->title,
					       xobj->value - 1);
				memmove(
					&xobj->title[PrevPos],
					&xobj->title[NewPos],
					Size-NewPos+1);
				xobj->title=(char*)realloc(
					xobj->title,
					(Size - (NewPos - PrevPos) + 1) *
					sizeof(char));
				NewPos = PrevPos;
				SendMsg(xobj,SingleClic);
			}
		}
		else if (i!=0)        /* Normal character */
		{
			/* a normal character: insertion in the title */
			/* here "i" is the number of bytes returned
			   need not be 1 incase of MB locale */
			Size=strlen(xobj->title);
			fvwm_debug(__func__, "Current size: %d\n", Size);
			/* make room for more the new text */
			/* shouldn't fxrealloc be used here? */
			xobj->title=(char*)realloc(
				xobj->title,(Size+i+1)*sizeof(char));
			/* new text will be inserted at NewPos */
			/* move string currently in wiget's buffer to make
			   room for new text */
			memmove(&xobj->title[NewPos+i],&xobj->title[NewPos],
				Size - NewPos + 1);
			/* and insert new text */
			memcpy(&xobj->title[NewPos],car,i);
			NewPos += i;
			SendMsg(xobj,SingleClic);
		}

	}

	new_value = getCharOffsetBoundsCheck(xobj->Ffont, xobj->title, NewPos);
	if ((xobj->value != new_value) || (xobj->value2 != new_value))
	{
		XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
		x2=FlocaleTextWidth(
			xobj->Ffont,xobj->title,strlen(xobj->title));
		XFillRectangle(dpy,xobj->win,xobj->gc,x2+4,4,xobj->width-x2-8,
			       xobj->height-8);
		/* convert back to char offsets */
		xobj->value = new_value;
		/*FlocaleStringByteToCharOffset(xobj->Ffont,
							  xobj->title,
							  NewPos);*/
		xobj->value2 = new_value; /*FlocaleStringByteToCharOffset(xobj->Ffont,
							  xobj->title,
							  NewPos);*/
		DrawPointTxt(xobj,xobj->TabColor[fore]);
		DrawTextField(xobj,NULL);
	}
	free(carks);
}

void ProcessMsgTextField(
	struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
