/*
   FvwmButtons v2.0.41-plural-Z-alpha, copyright 1996, Jarl Totland

 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.

*/

/* --------------------------- button information -------------------------- */

#define buttonXPos(b,i) \
  ((b)->parent->c->xpos + \
   ((i)%(b)->parent->c->num_columns)*((b)->parent->c->ButtonWidth))
#define buttonYPos(b,i) \
  ((b)->parent->c->ypos + \
   ((i)/(b)->parent->c->num_columns)*((b)->parent->c->ButtonHeight))
#define buttonWidth(b) \
  ((b)->BWidth*(b)->parent->c->ButtonWidth)
#define buttonHeight(b) \
  ((b)->BHeight*(b)->parent->c->ButtonHeight)

#define buttonSwallowCount(b) \
  (((b)->flags&b_Swallow)?((b)->swallow&b_Count):0)

void buttonInfo(button_info*,int *x,int *y,int *padx,int *pady,int *frame);
void GetInternalSize(button_info*,int*,int*,int*,int*);
#define buttonFrame(b) abs(buttonFrameSigned(b))
int buttonFrameSigned(button_info*);
int buttonXPad(button_info*);
int buttonYPad(button_info*);
XFontStruct *buttonFont(button_info*);
Pixel buttonFore(button_info*);
Pixel buttonBack(button_info*);
Pixel buttonHilite(button_info*);
Pixel buttonShadow(button_info*);
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
button_info *select_button(button_info*,int,int);
