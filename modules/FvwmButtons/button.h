/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

/* --------------------------- button information -------------------------- */

void buttonInfo(button_info*,int *x,int *y,int *padx,int *pady,int *frame);
void GetInternalSize(button_info*,int*,int*,int*,int*);
#define buttonFrame(b) abs(buttonFrameSigned(b))
int buttonFrameSigned(button_info*);
int buttonXPad(button_info*);
int buttonYPad(button_info*);
FlocaleFont *buttonFont(button_info*);
Pixel buttonFore(button_info*);
Pixel buttonBack(button_info*);
Pixel buttonHilite(button_info*);
Pixel buttonShadow(button_info*);
int buttonColorset(button_info *b);
byte buttonSwallow(button_info*);
byte buttonJustify(button_info*);
#define buttonNum(b) ((b)->n)

/* ---------------------------- button creation ---------------------------- */

void alloc_buttonlist(button_info*,int);
button_info *alloc_button(button_info*,int);
void MakeContainer(button_info*);

/* ------------------------- button administration ------------------------- */

void NumberButtons(button_info*);
void ShuffleButtons(button_info*);

/* ---------------------------- button iterator ---------------------------- */

button_info *NextButton(button_info**,button_info**,int*,int);

/* --------------------------- button navigation --------------------------- */

int button_belongs_to(button_info*,int);
button_info *get_xy_button(button_info *ub, int row, int column);
button_info *select_button(button_info*,int,int);

/* --------------------------- button geometry ----------------------------- */

int buttonXPos(button_info *b, int i);
int buttonYPos(button_info *b, int i);
int buttonWidth(button_info *b);
int buttonHeight(button_info *b);
void get_button_root_geometry(rectangle *r, button_info *b);

/* --------------------------- swallowing ---------------------------------- */

int buttonSwallowCount(button_info *b);
