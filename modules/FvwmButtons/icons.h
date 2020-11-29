/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */
#ifndef FVWMBUTTONS_ICONS_H
#define FVWMBUTTONS_ICONS_H


#include "FvwmButtons.h"

Bool GetIconPosition(
	button_info *b,	FvwmPicture *pic,
	int *r_x, int *r_y, int *r_w, int *r_h);
void DrawForegroundIcon(button_info *b, XEvent *pev);

#endif /* FVWMBUTTONS_ICONS_H */
