/*
Copyright (C) 1996 César Crusius

This file is part of the DND Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __DragAndDropH__
#define __DragAndDropH__

#include <OffiX/DragAndDropTypes.h>

#include <X11/Intrinsic.h>

void DndInitialize(Widget shell);

void DndRegisterRootDrop(XtEventHandler handler);
void DndRegisterIconDrop(XtEventHandler handler);
void DndRegisterOtherDrop(XtEventHandler handler);

void DndRegisterDropWidget(Widget widget,XtEventHandler handler,
			   XtPointer data);
void DndRegisterDragWidget(Widget widget,XtEventHandler handler,
			   XtPointer data);

int DndHandleDragging(Widget widget,XEvent* event);

void DndMultipleShells(void);
void DndAddShell(Widget widget);

void DndSetData(int Type,unsigned char *Data,unsigned long Size);
void DndGetData(unsigned char **Data,unsigned long *Size);
		
int		DndIsIcon(Widget widget);
int		DndDataType(XEvent *event);
unsigned int	DndDragButtons(XEvent *event);
Widget		DndSourceWidget(XEvent *event);

#endif
