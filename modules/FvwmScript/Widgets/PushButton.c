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

/* left, center and right offsets */
#define PUSH_BUTTON_LCR_OFFSETS 4,0,4

/*
 * Fonction pour PushButton
 */
void InitPushButton(struct XObj *xobj)
{
	unsigned long mask;
	XSetWindowAttributes Attr;
	int i;
	char *str;

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
	Attr.cursor = XCreateFontCursor(dpy, XC_hand2);
	mask |= CWCursor;
	Attr.background_pixel = xobj->TabColor[back];
	mask |= CWBackPixel;


	/* Epaisseur de la fenetre = 0 */
	xobj->win = XCreateWindow(
		dpy, *xobj->ParentWin, xobj->x, xobj->y, xobj->width,
		xobj->height, 0, CopyFromParent, InputOutput, CopyFromParent,
		mask, &Attr);
	xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
	XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

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

	XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

	/* Redimensionnement du widget */
	str = (char*)GetMenuTitle(xobj->title, 1);
	if (xobj->icon == NULL)
	{
		i = xobj->Ffont->height + 12;
		if (xobj->height < i)
			xobj->height = i;
		i = FlocaleTextWidth(xobj->Ffont, str, strlen(str)) + 16;
		if (xobj->width < i)
			xobj->width = i;
	}
	else if (strlen(str) == 0)
	{
		if (xobj->height < xobj->icon_h + 10)
			xobj->height = xobj->icon_h + 10;
		if (xobj->width < xobj->icon_w + 10)
			xobj->width = xobj->icon_w + 10;
	}
	else
	{
		if (xobj->icon_w + 10 > FlocaleTextWidth(
			    xobj->Ffont, str, strlen(str)) + 16)
			i = xobj->icon_w + 10;
		else
			i = FlocaleTextWidth(
				xobj->Ffont, str, strlen(str)) + 16;
		if (xobj->width < i)
			xobj->width = i;
		i = xobj->icon_h+ 2 * (xobj->Ffont->height + 10);
		if (xobj->height < i)
			xobj->height = i;
	}
	XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
	if (xobj->colorset >= 0)
		SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
				    &Colorset[xobj->colorset], Pdepth,
				    xobj->gc, True);
	xobj->value3 = CountOption(xobj->title);
	XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyPushButton(struct XObj *xobj)
{
	FlocaleUnloadFont(dpy, xobj->Ffont);
	XFreeGC(dpy, xobj->gc);
	XDestroyWindow(dpy, xobj->win);
}


void DrawPushButton(struct XObj *xobj, XEvent *evp)
{
	DrawReliefRect(0, 0, xobj->width, xobj->height, xobj, hili, shad);
	DrawIconStr(0, xobj, True, PUSH_BUTTON_LCR_OFFSETS, NULL, NULL, evp);
}

void EvtMousePushButton(struct XObj *xobj, XButtonEvent *EvtButton)
{
	static XEvent event;
	int End = 1;
	unsigned int modif;
	int x1,x2,y1,y2,i;
	Window Win1,Win2,WinPop;
	Window WinBut = 0;
	int In = 0;
	char *str;
	int x,y,hOpt,yMenu,hMenu,wMenu;
	int oldvalue = 0,newvalue;
	unsigned long mask;
	XSetWindowAttributes Attr;

	if (EvtButton->button == Button1)
	{
		i = (xobj->width -
		     FlocaleTextWidth(
			     xobj->Ffont, xobj->title, strlen(xobj->title)))/2;

		while (End)
		{
			FNextEvent(dpy, &event);
			switch (event.type)
			{
			case EnterNotify:
				FQueryPointer(
					dpy, *xobj->ParentWin,
					&Win1, &Win2, &x1, &y1, &x2, &y2,
					&modif);
				if (WinBut == 0)
				{
					WinBut = Win2;
					DrawReliefRect(
						0, 0, xobj->width,
						xobj->height, xobj, shad,
						hili);
					DrawIconStr(
						0,xobj, True,
						PUSH_BUTTON_LCR_OFFSETS, NULL,
						NULL, NULL);
					In = 1;
				}
				else
				{
					if (Win2 == WinBut)
					{
						DrawReliefRect(
							0, 0, xobj->width,
							xobj->height, xobj,
							shad, hili);
						DrawIconStr(
							1, xobj, True,
							PUSH_BUTTON_LCR_OFFSETS,
							NULL, NULL, NULL);
						In = 1;
					}
					else if (In)
					{
						In = 0;
						DrawReliefRect(
							0, 0, xobj->width,
							xobj->height, xobj,
							hili, shad);
						DrawIconStr(
							0, xobj, True,
							PUSH_BUTTON_LCR_OFFSETS,
							NULL, NULL, NULL);
					}
				}
				break;
			case LeaveNotify:
				FQueryPointer(
					dpy, *xobj->ParentWin,
					&Win1, &Win2, &x1, &y1, &x2, &y2,
					&modif);
				if (Win2 == WinBut)
				{
					In = 1;
					DrawReliefRect(
						0, 0, xobj->width,
						xobj->height, xobj, shad,
						hili);
					DrawIconStr(
						1, xobj, True,
						PUSH_BUTTON_LCR_OFFSETS, NULL,
						NULL, NULL);
				}
				else if (In)
				{
					DrawReliefRect(
						0, 0, xobj->width,
						xobj->height, xobj, hili,
						shad);
					DrawIconStr(
						0, xobj, True,
						PUSH_BUTTON_LCR_OFFSETS, NULL,
						NULL, NULL);
					In = 0;
				}
				break;
			case ButtonRelease:
				End = 0;
				DrawReliefRect(
					0, 0, xobj->width, xobj->height, xobj,
					hili, shad);
				DrawIconStr(
					0, xobj, True,
					PUSH_BUTTON_LCR_OFFSETS, NULL, NULL,
					NULL);
				if (In)
				{
					/* Envoie d'un message vide de type
					 * SingleClic pour un clique souris */
					xobj->value = 1;
					SendMsg(xobj, SingleClic);
					xobj->value = 0;
				}
				break;
			}
		}
	}
	/* affichage du popup menu / Drawing the popup menu */
	else if (EvtButton->button == Button3)
	{
		if (xobj->value3 > 1)
		{
			hOpt = xobj->Ffont->height + 10;
			/* Hauteur totale du menu / Total height of the menu */
			hMenu = (xobj->value3 - 1) * hOpt;
			yMenu = xobj->y + xobj->height;
			wMenu = 0;
			for (i = 2 ; i <= xobj->value3; i++)
			{
				str = (char*)GetMenuTitle(xobj->title, i);
				if (wMenu < FlocaleTextWidth(
					    xobj->Ffont, str, strlen(str))+34)
					wMenu = FlocaleTextWidth(
						xobj->Ffont, str,
						strlen(str))+34;
				free(str);
			}

			/* Creation de la fenetre menu / create the menu
			 * window */
			XTranslateCoordinates(dpy, *xobj->ParentWin, Root,
					      xobj->x, yMenu, &x, &y, &Win1);
			if (x<0) x = 0;
			if (y<0) y = 0;
			if (x + wMenu > XDisplayWidth(dpy, screen))
			{
				x = XDisplayWidth(dpy, screen) - wMenu;
			}
			if (y + hMenu > XDisplayHeight(dpy, screen))
			{
				y = y-hMenu-xobj->height;
			}

			mask = 0;
			Attr.background_pixel = xobj->TabColor[back];
			mask |= CWBackPixel;
			Attr.border_pixel  =  0;
			mask |= CWBorderPixel;
			Attr.colormap = Pcmap;
			mask |= CWColormap;
			Attr.cursor = XCreateFontCursor(dpy, XC_hand2);
			mask |= CWCursor;         /* Curseur pour la fenetre */
			Attr.override_redirect = True;
			mask |= CWOverrideRedirect;
			WinPop = XCreateWindow(
				dpy, Root, x, y, wMenu-5, hMenu, 0,
				Pdepth, InputOutput, Pvisual, mask, &Attr);
			if (xobj->colorset >= 0)
				SetWindowBackground(
					dpy, WinPop, wMenu - 5, hMenu,
					&Colorset[xobj->colorset], Pdepth,
					xobj->gc, True);
			XMapRaised(dpy, WinPop);

			/* Dessin du menu */
			DrawPMenu(xobj, WinPop, hOpt, 1);
			do
			{
				FQueryPointer(
					dpy, Root, &Win1, &Win2, &x1, &y1,
					&x2, &y2, &modif);
				/* Determiner l'option courante / Current
				 * option */
				y2 = y2 - y;
				x2 = x2 - x;
				{
					/* calcule de xobj->value / Compute
					 * xobj->value */
					if ((x2 > 0) && (x2 < wMenu) &&
					    (y2 > 0) && (y2 < hMenu))
						newvalue = y2 / hOpt+1;
					else
						newvalue = 0;
					if (newvalue!=oldvalue)
					{
						UnselectMenu(
							xobj, WinPop, hOpt,
							oldvalue, wMenu-5,
							xobj->Ffont->ascent,
							1);
						SelectMenu(
							xobj, WinPop, hOpt,
							newvalue);
						oldvalue = newvalue;
					}
				}
			}
			while (!FCheckTypedEvent(dpy, ButtonRelease, &event));
			XDestroyWindow(dpy, WinPop);
			if (newvalue != 0)
			{
				xobj->value = newvalue;
				SendMsg(xobj, SingleClic);
				xobj->value = 0;
			}
			xobj->DrawObj(xobj, NULL);
		} /* xobj->value3 > 1 */
	} /* EvtButton->button == Button3 */
}

void EvtKeyPushButton(struct XObj *xobj, XKeyEvent *EvtKey)
{
	char car[11];
	char *carks2;
	char *carks;
	KeySym ks;
	int Size;

	Size = XLookupString(EvtKey, car, 10, &ks, NULL);
	car[Size] = '\0';
	carks2 = XKeysymToString(ks);
	if (carks2 != NULL)
	{
		carks = (char*)calloc(sizeof(char), 30);
		sprintf(carks, "%s", carks2);
		if (strcmp(carks, "Return") == 0)
		{
			xobj->value = 1;
			SendMsg(xobj, SingleClic);
			xobj->value = 0;
		}
		free(carks);
	}
}

void ProcessMsgPushButton(
	struct XObj *xobj, unsigned long type, unsigned long *body)
{
}
