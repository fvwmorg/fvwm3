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

/*
 * Fichier contenant les fonctions attachees aux objets graphiques
 */

#include "config.h"

#include "Tools.h"

extern void InitPushButton(struct XObj *xobj);
extern void DestroyPushButton(struct XObj *xobj);
extern void DrawPushButton(struct XObj *xobj, XEvent *evp);
extern void EvtMousePushButton(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyPushButton(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgPushButton(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitRadioButton(struct XObj *xobj);
extern void DestroyRadioButton(struct XObj *xobj);
extern void DrawRadioButton(struct XObj *xobj, XEvent *evp);
extern void EvtMouseRadioButton(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyRadioButton(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgRadioButton(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitItemDraw(struct XObj *xobj);
extern void DestroyItemDraw(struct XObj *xobj);
extern void DrawItemDraw(struct XObj *xobj, XEvent *evp);
extern void EvtMouseItemDraw(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyItemDraw(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgItemDraw(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitCheckBox(struct XObj *xobj);
extern void DestroyCheckBox(struct XObj *xobj);
extern void DrawCheckBox(struct XObj *xobj, XEvent *evp);
extern void EvtMouseCheckBox(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyCheckBox(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgCheckBox(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitTextField(struct XObj *xobj);
extern void DestroyTextField(struct XObj *xobj);
extern void DrawTextField(struct XObj *xobj, XEvent *evp);
extern void EvtMouseTextField(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyTextField(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgTextField(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitHScrollBar(struct XObj *xobj);
extern void DestroyHScrollBar(struct XObj *xobj);
extern void DrawHScrollBar(struct XObj *xobj, XEvent *evp);
extern void EvtMouseHScrollBar(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyHScrollBar(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgHScrollBar(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitVScrollBar(struct XObj *xobj);
extern void DestroyVScrollBar(struct XObj *xobj);
extern void DrawVScrollBar(struct XObj *xobj, XEvent *evp);
extern void EvtMouseVScrollBar(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyVScrollBar(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgVScrollBar(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitPopupMenu(struct XObj *xobj);
extern void DestroyPopupMenu(struct XObj *xobj);
extern void DrawPopupMenu(struct XObj *xobj, XEvent *evp);
extern void EvtMousePopupMenu(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyPopupMenu(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgPopupMenu(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitRectangle(struct XObj *xobj);
extern void DestroyRectangle(struct XObj *xobj);
extern void DrawRectangle(struct XObj *xobj, XEvent *evp);
extern void EvtMouseRectangle(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyRectangle(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgRectangle(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitMiniScroll(struct XObj *xobj);
extern void DestroyMiniScroll(struct XObj *xobj);
extern void DrawMiniScroll(struct XObj *xobj, XEvent *evp);
extern void EvtMouseMiniScroll(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyMiniScroll(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgMiniScroll(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitSwallow(struct XObj *xobj);
extern void DestroySwallow(struct XObj *xobj);
extern void DrawSwallow(struct XObj *xobj, XEvent *evp);
extern void EvtMouseSwallow(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeySwallow(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgSwallow(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitHDipstick(struct XObj *xobj);
extern void DestroyHDipstick(struct XObj *xobj);
extern void DrawHDipstick(struct XObj *xobj, XEvent *evp);
extern void EvtMouseHDipstick(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyHDipstick(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgHDipstick(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitVDipstick(struct XObj *xobj);
extern void DestroyVDipstick(struct XObj *xobj);
extern void DrawVDipstick(struct XObj *xobj, XEvent *evp);
extern void EvtMouseVDipstick(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyVDipstick(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgVDipstick(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitList(struct XObj *xobj);
extern void DestroyList(struct XObj *xobj);
extern void DrawList(struct XObj *xobj, XEvent *evp);
extern void EvtMouseList(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyList(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgList(
	struct XObj *xobj,unsigned long type,unsigned long *body);

extern void InitMenu(struct XObj *xobj);
extern void DestroyMenu(struct XObj *xobj);
extern void DrawMenu(struct XObj *xobj, XEvent *evp);
extern void EvtMouseMenu(struct XObj *xobj,XButtonEvent *EvtButton);
extern void EvtKeyMenu(struct XObj *xobj,XKeyEvent *EvtKey);
extern void ProcessMsgMenu(
	struct XObj *xobj,unsigned long type,unsigned long *body);

/*
 * Choix des fonctions suivant le type de bouton
 */

void ChooseFunction(struct XObj *xobj,char *type)
{
	if (strcmp(type,"PushButton")==0)
	{
		xobj->TypeWidget=PushButton;
		xobj->InitObj=InitPushButton;
		xobj->DestroyObj=DestroyPushButton;
		xobj->DrawObj=DrawPushButton;
		xobj->EvtMouse=EvtMousePushButton;
		xobj->EvtKey=EvtKeyPushButton;
		xobj->ProcessMsg=ProcessMsgPushButton;
	}
	else if (strcmp(type,"RadioButton")==0)
	{
		xobj->TypeWidget=RadioButton;
		xobj->InitObj=InitRadioButton;
		xobj->DestroyObj=DestroyRadioButton;
		xobj->DrawObj=DrawRadioButton;
		xobj->EvtMouse=EvtMouseRadioButton;
		xobj->EvtKey=EvtKeyRadioButton;
		xobj->ProcessMsg=ProcessMsgRadioButton;
	}
	else if (strcmp(type,"ItemDraw")==0)
	{
		xobj->TypeWidget=ItemDraw;
		xobj->InitObj=InitItemDraw;
		xobj->DestroyObj=DestroyItemDraw;
		xobj->DrawObj=DrawItemDraw;
		xobj->EvtMouse=EvtMouseItemDraw;
		xobj->EvtKey=EvtKeyItemDraw;
		xobj->ProcessMsg=ProcessMsgItemDraw;
	}
	else if (strcmp(type,"CheckBox")==0)
	{
		xobj->TypeWidget=CheckBox;
		xobj->InitObj=InitCheckBox;
		xobj->DestroyObj=DestroyCheckBox;
		xobj->DrawObj=DrawCheckBox;
		xobj->EvtMouse=EvtMouseCheckBox;
		xobj->EvtKey=EvtKeyCheckBox;
		xobj->ProcessMsg=ProcessMsgCheckBox;
	}
	else if (strcmp(type,"TextField")==0)
	{
		xobj->TypeWidget=TextField;
		xobj->InitObj=InitTextField;
		xobj->DestroyObj=DestroyTextField;
		xobj->DrawObj=DrawTextField;
		xobj->EvtMouse=EvtMouseTextField;
		xobj->EvtKey=EvtKeyTextField;
		xobj->ProcessMsg=ProcessMsgTextField;
	}
	else if (strcmp(type,"HScrollBar")==0)
	{
		xobj->TypeWidget=HScrollBar;
		xobj->InitObj=InitHScrollBar;
		xobj->DestroyObj=DestroyHScrollBar;
		xobj->DrawObj=DrawHScrollBar;
		xobj->EvtMouse=EvtMouseHScrollBar;
		xobj->EvtKey=EvtKeyHScrollBar;
		xobj->ProcessMsg=ProcessMsgHScrollBar;
	}
	else if (strcmp(type,"VScrollBar")==0)
	{
		xobj->TypeWidget=VScrollBar;
		xobj->InitObj=InitVScrollBar;
		xobj->DestroyObj=DestroyVScrollBar;
		xobj->DrawObj=DrawVScrollBar;
		xobj->EvtMouse=EvtMouseVScrollBar;
		xobj->EvtKey=EvtKeyVScrollBar;
		xobj->ProcessMsg=ProcessMsgVScrollBar;
	}
	else if (strcmp(type,"PopupMenu")==0)
	{
		xobj->TypeWidget=PopupMenu;
		xobj->InitObj=InitPopupMenu;
		xobj->DestroyObj=DestroyPopupMenu;
		xobj->DrawObj=DrawPopupMenu;
		xobj->EvtMouse=EvtMousePopupMenu;
		xobj->EvtKey=EvtKeyPopupMenu;
		xobj->ProcessMsg=ProcessMsgPopupMenu;
	}
	else if (strcmp(type,"Rectangle")==0)
	{
		xobj->TypeWidget=Rectangle;
		xobj->InitObj=InitRectangle;
		xobj->DestroyObj=DestroyRectangle;
		xobj->DrawObj=DrawRectangle;
		xobj->EvtMouse=EvtMouseRectangle;
		xobj->EvtKey=EvtKeyRectangle;
		xobj->ProcessMsg=ProcessMsgRectangle;
	}
	else if (strcmp(type,"MiniScroll")==0)
	{
		xobj->TypeWidget=MiniScroll;
		xobj->InitObj=InitMiniScroll;
		xobj->DestroyObj=DestroyMiniScroll;
		xobj->DrawObj=DrawMiniScroll;
		xobj->EvtMouse=EvtMouseMiniScroll;
		xobj->EvtKey=EvtKeyMiniScroll;
		xobj->ProcessMsg=ProcessMsgMiniScroll;
	}
	else if (strcmp(type,"SwallowExec")==0)
	{
		xobj->TypeWidget=SwallowExec;
		xobj->InitObj=InitSwallow;
		xobj->DestroyObj=DestroySwallow;
		xobj->DrawObj=DrawSwallow;
		xobj->EvtMouse=EvtMouseSwallow;
		xobj->EvtKey=EvtKeySwallow;
		xobj->ProcessMsg=ProcessMsgSwallow;
	}
	else if (strcmp(type,"HDipstick")==0)
	{
		xobj->TypeWidget=HDipstick;
		xobj->InitObj=InitHDipstick;
		xobj->DestroyObj=DestroyHDipstick;
		xobj->DrawObj=DrawHDipstick;
		xobj->EvtMouse=EvtMouseHDipstick;
		xobj->EvtKey=EvtKeyHDipstick;
		xobj->ProcessMsg=ProcessMsgHDipstick;
	}
	else if (strcmp(type,"VDipstick")==0)
	{
		xobj->TypeWidget=VDipstick;
		xobj->InitObj=InitVDipstick;
		xobj->DestroyObj=DestroyVDipstick;
		xobj->DrawObj=DrawVDipstick;
		xobj->EvtMouse=EvtMouseVDipstick;
		xobj->EvtKey=EvtKeyVDipstick;
		xobj->ProcessMsg=ProcessMsgVDipstick;
	}
	else if (strcmp(type,"List")==0)
	{
		xobj->TypeWidget=List;
		xobj->InitObj=InitList;
		xobj->DestroyObj=DestroyList;
		xobj->DrawObj=DrawList;
		xobj->EvtMouse=EvtMouseList;
		xobj->EvtKey=EvtKeyList;
		xobj->ProcessMsg=ProcessMsgList;
	}
	else if (strcmp(type,"Menu")==0)
	{
		xobj->TypeWidget=Menu;
		xobj->InitObj=InitMenu;
		xobj->DestroyObj=DestroyMenu;
		xobj->DrawObj=DrawMenu;
		xobj->EvtMouse=EvtMouseMenu;
		xobj->EvtKey=EvtKeyMenu;
		xobj->ProcessMsg=ProcessMsgMenu;
	}
	else
	{
		fprintf(stderr,"Item %s unknown\n",type);
		exit(1);
	}
}
