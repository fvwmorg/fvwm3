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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include "FvwmButtons.h"
#include "button.h"

extern char *MyName;


int buttonXPos(button_info *b, int i)
{
  int column = i % b->parent->c->num_columns;

  return b->parent->c->xpos +
    (b->parent->c->width * column / b->parent->c->num_columns);
}

int buttonYPos(button_info *b, int i)
{
  int row = i / b->parent->c->num_columns;

  return b->parent->c->ypos +
    (b->parent->c->height * row / b->parent->c->num_rows);
}

int buttonWidth(button_info *b)
{
  int column = b->n % b->parent->c->num_columns;
  int column2 = column + b->BWidth;

  return (b->parent->c->width * column2 / b->parent->c->num_columns) -
    (b->parent->c->width * column / b->parent->c->num_columns);
}

int buttonHeight(button_info *b)
{
  int row = b->n / b->parent->c->num_columns;
  int row2 = row + b->BHeight;

  return (b->parent->c->height * row2 / b->parent->c->num_rows) -
    (b->parent->c->height * row / b->parent->c->num_rows);
}

int buttonSwallowCount(button_info *b)
{
  return (b->flags & (b_Swallow | b_Panel)) ? (b->swallow & b_Count) : 0;
}


/**
*** buttonInfo()
*** Give lots of info for this button: XPos, YPos, XPad, YPad, Frame(signed)
**/
void buttonInfo(button_info *b,int *x,int *y,int *px,int *py,int *f)
{
  ushort w=b_Padding|b_Frame;
  *x=buttonXPos(b,b->n);
  *y=buttonYPos(b,b->n);
  *px=b->xpad;
  *py=b->ypad;
  *f=b->framew;
  w&=~(b->flags&(b_Frame|b_Padding));

  if(b->flags&b_Container && w&b_Frame)
  {
    *f=0;
    w&=~b_Frame;
  }
  if((b->flags&b_Container || b->flags&b_Swallow) && w&b_Padding)
  {
    *px=*py=0;
    w&=~b_Padding;
  }

  while(w && (b=b->parent))
  {
    if(w&b_Frame && b->c->flags&b_Frame)
    {
      *f=b->c->framew;
      w&=~b_Frame;
    }
    if(w&b_Padding && b->c->flags&b_Padding)
    {
      *px=b->c->xpad;
      *py=b->c->ypad;
      w&=~b_Padding;
    }
  }
}

/**
*** GetInternalSize()
**/
void GetInternalSize(button_info *b,int *x,int *y,int *w,int *h)
{
  int f;
  int px,py;
  buttonInfo(b,x,y,&px,&py,&f);
  f=abs(f);

  *w=buttonWidth(b)-2*(px+f);
  *h=buttonHeight(b)-2*(py+f);

  *x+=f+px;
  *y+=f+py;

  if (*w < 1)
    *w = 1;
  if (*h < 1)
    *h = 1;

  return;
}

/**
*** buttonFrameSigned()
*** Give the signed framewidth for this button.
**/
int buttonFrameSigned(button_info *b)
{
  if(b->flags&b_Frame)
    return b->framew;
  if(b->flags&b_Container)              /* Containers usually gets 0 relief  */
    return 0;
  while((b=b->parent))
    if(b->c->flags&b_Frame)
      return b->c->framew;
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No relief width definition?\n",MyName);
#endif
  return 0;
}

/**
*** buttonXPad()
*** Give the x padding for this button
**/
int buttonXPad(button_info *b)
{
  if(b->flags&b_Padding)
    return b->xpad;
  if(b->flags&(b_Container|b_Swallow))  /* Normally no padding for these     */
    return 0;
  while((b=b->parent))
    if(b->c->flags&b_Padding)
      return b->c->xpad;
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No padding definition?\n",MyName);
#endif
  return 0;
}

/**
*** buttonYPad()
*** Give the y padding for this button
**/
int buttonYPad(button_info *b)
{
  if(b->flags&b_Padding)
    return b->ypad;
  if(b->flags&(b_Container|b_Swallow))  /* Normally no padding for these     */
    return 0;
  while((b=b->parent))
    if(b->c->flags&b_Padding)
      return b->c->ypad;
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No padding definition?\n",MyName);
#endif
  return 0;
}

/**
*** buttonFont()
*** Give the font pointer for this button
**/
XFontStruct *buttonFont(button_info *b)
{
  if(b->flags&b_Font)
    return b->font;
  while((b=b->parent))
    if(b->c->flags&b_Font)
      return b->c->font;
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No font definition?\n",MyName);
#endif
  return None;
}

#ifdef I18N_MB
/**
*** buttonFontSet()
*** Give the font pointer for this button
**/
XFontSet buttonFontSet(button_info *b)
{
  if(b->flags&b_Font)
    return b->fontset;
  while((b=b->parent))
    if(b->c->flags&b_Font)
      return b->c->fontset;
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No fontset definition?\n",MyName);
#endif
  return None;
}
#endif

/**
*** buttonFore()
*** Give the foreground pixel of this button
**/
Pixel buttonFore(button_info *b)
{
  if(b->flags&b_Fore)
    return b->fc;
  while ((b=b->parent))
  {
    if(b->c->flags&b_Fore)
      return b->c->fc;
  }
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No foreground definition?\n",MyName);
#endif
  return None;
}

/**
*** buttonBack()
*** Give the background pixel of this button
**/
Pixel buttonBack(button_info *b)
{
  if(b->flags&b_Back)
    return b->bc;
  while((b=b->parent))
  {
    if(b->c->flags&b_Back)
      return b->c->bc;
  }
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No background definition?\n",MyName);
#endif
  return None;
}

/**
*** buttonHilite()
*** Give the relief pixel of this button
**/
Pixel buttonHilite(button_info *b)
{
  if(b->flags&b_Back)
    return b->hc;
  while((b=b->parent))
  {
    if(b->c->flags&b_Back)
      return b->c->hc;
  }
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No background definition?\n",MyName);
#endif
  return None;
}

/**
*** buttonShadow()
*** Give the shadow pixel of this button
**/
Pixel buttonShadow(button_info *b)
{
  if(b->flags&b_Back)
    return b->sc;
  while((b=b->parent))
  {
    if(b->c->flags&b_Back)
      return b->c->sc;
  }
#ifdef DEBUG
  fprintf(stderr,"%s: BUG: No background definition?\n",MyName);
#endif
  return None;
}

int buttonColorset(button_info *b)
{
  if (b->flags & b_Colorset)
    return b->colorset;
  while ((b = b->parent))
  {
    if (b->c->flags & b_Colorset)
      return b->c->colorset;
  }
  return -1;
}


/**
*** buttonSwallow()
*** Give the swallowing flags for this button
**/
byte buttonSwallow(button_info *b)
{
  byte s=0,t=0;
  if(b->flags & (b_Swallow | b_Panel))
  {
    s=b->swallow;
    t=b->swallow_mask;
  }
  while((b=b->parent))
  {
    if(b->c->flags & (b_Swallow | b_Panel))
    {
      s&=~(b->c->swallow_mask&~t);
      s|=(b->c->swallow&b->c->swallow_mask&~t);
      t|=b->c->swallow_mask;
    }
  }
  return s;
}

/**
*** buttonJustify()
*** Give the justify flags for this button
**/
byte buttonJustify(button_info *b)
{
  byte j=1,i=0;
  if(b->flags&b_Justify)
  {
    i=b->justify_mask;
    j=b->justify;
  }
  while((b=b->parent))
  {
    if(b->c->flags&b_Justify)
    {
      j&=~(b->c->justify_mask&~i);
      j|=(b->c->justify&b->c->justify_mask&~i);
      i|=b->c->justify_mask;
    }
  }
  return j;
}

/* ---------------------------- button creation ---------------------------- */

/**
*** alloc_buttonlist()
*** Makes sure the list of butten_info's is long enough, if not it reallocates
*** a longer one. This happens in steps of 32. Inital length is 0.
**/
void alloc_buttonlist(button_info *ub,int num)
{
  button_info **bb;
  int i,old;

  if(num>=ub->c->allocated_buttons)
  {
    old=ub->c->allocated_buttons;
    if(num<old || old+32<old)
    {
      fprintf(stderr,"%s: Too many buttons, integer overflow\n",MyName);
      exit(1);
    }
    while(ub->c->allocated_buttons<=num)
      ub->c->allocated_buttons+=32;
    bb=(button_info**)
      mymalloc(ub->c->allocated_buttons*sizeof(button_info*));
    for(i=old;i<ub->c->allocated_buttons;i++)
      bb[i]=NULL;
    if(ub->c->buttons)
    {
      for(i=0;i<old;i++) bb[i]=ub->c->buttons[i];
      free(ub->c->buttons);
    }
    ub->c->buttons=bb;
  }
}

/**
*** alloc_button()
*** Allocates memory for a new button struct. Calles alloc_buttonlist to
*** assure enough space is present. Also initiates most elements of the struct.
**/
button_info *alloc_button(button_info *ub,int num)
{
  button_info *b;
  if(num>=ub->c->allocated_buttons)
    alloc_buttonlist(ub,num);
  if(ub->c->buttons[num])
  {
    fprintf(stderr,"%s: Allocated button twice, report bug twice\n",MyName);
    exit(2);
  }

  b=(button_info*)mymalloc(sizeof(button_info));
  ub->c->buttons[num]=b;

  memset((void *)b, 0, sizeof(*b));
  b->flags = 0;
  b->swallow = 0;
  b->BWidth = b->BHeight = 1;
  b->BPosX = b->BPosY = 0;
  b->parent = ub;
  b->n = -1;
  b->IconWin = 0;
  b->PanelWin = 0;

  b->framew = 1;
  b->xpad = 2;
  b->ypad = 4;
  b->w=1;
  b->h=1;
  b->bw=1;

  if(b->parent != NULL)
  {
    if (b->parent->c->flags & b_Colorset)
      b->flags |= b_ColorsetParent;
  }

  return(b);
}

/**
*** MakeContainer()
*** Allocs and sets the container-specific fields of a button.
**/
void MakeContainer(button_info *b)
{
  b->c=(container_info*)mymalloc(sizeof(container_info));
  memset((void *)b->c, 0, sizeof(container_info));
  b->flags|=b_Container;
  if(b->parent != NULL)
  {
    if (b->parent->c->flags & b_IconBack || b->parent->c->flags & b_IconParent)
      b->c->flags |= b_IconParent;
    if (b->parent->c->flags & b_Colorset)
      b->c->flags |= b_ColorsetParent;
  }
  else /* This applies to the UberButton */
  {
    b->c->flags=b_Font|b_Padding|b_Frame|b_Back|b_Fore;
    b->c->font_string=safestrdup("fixed");
    b->c->xpad=2;
    b->c->ypad=4;
    b->c->back=safestrdup("rgb:90/80/90");
    b->c->fore=safestrdup("black");
    b->c->framew=2;
  }

}

/* -------------------------- button administration ------------------------ */

/**
*** NumberButtons()
*** Prepare the n fields in each button
**/
void NumberButtons(button_info *b)
{
  int i=-1;
  while(++i<b->c->num_buttons)
    if(b->c->buttons[i])
    {
      b->c->buttons[i]->n=i;
      if(b->c->buttons[i]->flags&b_Container)
	NumberButtons(b->c->buttons[i]);
    }
}

/**
*** PlaceAndExpandButton()
*** Places a button in it's container and claims all needed slots.
**/
char PlaceAndExpandButton(int x, int y, button_info *b, button_info *ub)
{
  int i,j,k;
  container_info *c=ub->c;

  i = x+y*c->num_columns;
  if (x>=c->num_columns || x<0)
  {
    fprintf(stderr,"%s: Button out of horizontal range. Quitting.\n",MyName);
    fprintf(stderr,"Button=%d num_columns=%d BPosX=%d\n",
	    i,c->num_columns,b->BPosX);
    exit(1);
  }
  if (y>=c->num_rows || y<0)
  {
    if (b->flags&b_PosFixed || !(ub->c->flags&b_SizeSmart) || y<0)
    {
      fprintf(stderr,"%s: Button out of vertical range. Quitting.\n",
	      MyName);
      fprintf(stderr,"Button=%d num_rows=%d BPosY=%d\n",
	      i,c->num_rows,b->BPosY);
      exit(1);
    }
    c->num_rows=y+b->BHeight;
    c->num_buttons=c->num_columns*c->num_rows;
    alloc_buttonlist(ub,c->num_buttons);
  }
  if(x+b->BWidth>c->num_columns)
  {
    fprintf(stderr,"%s: Button too wide. giving up\n",MyName);
    fprintf(stderr,"Button=%d num_columns=%d bwidth=%d w=%d\n",
	    i,c->num_columns,b->BWidth,x);
    b->BWidth = c->num_columns-x;
  }
  if(y+b->BHeight>c->num_rows)
  {
    if (c->flags&b_SizeSmart)
    {
      c->num_rows=y+b->BHeight;
      c->num_buttons=c->num_columns*c->num_rows;
      alloc_buttonlist(ub,c->num_buttons);
    }
    else
    {
      fprintf(stderr,"%s: Button too tall. Giving up\n",MyName);
      fprintf(stderr,"Button=%d num_rows=%d bheight=%d h=%d\n",
	      i,c->num_rows,b->BHeight,y);
      b->BHeight = c->num_rows-y;
    }
  }

  /* check if buttons are free */
  for(k=0;k<b->BHeight;k++)
    for(j=0;j<b->BWidth;j++)
      if (c->buttons[i+j+k*c->num_columns])
	return 1;
  /* claim all buttons */
  for(k=0;k<b->BHeight;k++)
    for(j=0;j<b->BWidth;j++)
      c->buttons[i+j+k*c->num_columns] = b;
  b->BPosX = x;
  b->BPosY = y;
  return 0;
}

/**
*** ShrinkButton()
*** Frees all but the upper left slot a button uses in it's container.
**/
void ShrinkButton(button_info *b, container_info *c)
{
  int i,j,k,l;

  if (!b)
  {
    fprintf(stderr,"error: shrink1: button is empty but shouldn't\n");
    exit(1);
  }
  i = b->BPosX+b->BPosY*c->num_columns;
  /* free all buttons but the upper left corner */
  for(k=0;k<b->BHeight;k++)
  {
    for(j=0;j<b->BWidth;j++)
    {
      if(j||k)
      {
	l = i+j+k*c->num_columns;
	if (c->buttons[l] != b)
	{
	  fprintf(stderr,"error: shrink2: button was stolen\n");
	  exit(1);
	}
	c->buttons[l] = NULL;
      }
    }
  }
}

/**
*** ShuffleButtons()
*** Orders and sizes the buttons in the UberButton, corrects num_rows and
*** num_columns in containers.
**/
void ShuffleButtons(button_info *ub)
{
  int i,actual_buttons_used;
  int next_button_x, next_button_y, num_items;
  button_info *b;
  button_info **local_buttons;
  container_info *c=ub->c;

  /* make local copy of buttons in ub */
  num_items = c->num_buttons;
  local_buttons=(button_info**)mymalloc(sizeof(button_info)*num_items);
  for(i=0;i<num_items;i++)
  {
    local_buttons[i] = c->buttons[i];
    c->buttons[i] = NULL;
  }

  /* Allow for multi-width/height buttons */
  actual_buttons_used = 0;
  for(i=0;i<num_items;i++)
    actual_buttons_used+=local_buttons[i]->BWidth*local_buttons[i]->BHeight;

  if (!(c->flags&b_SizeFixed)||!(c->num_rows)||!(c->num_columns))
  {
    /* Size and create the window */
    if(c->num_rows==0 && c->num_columns==0)
      c->num_rows=2;
    if(c->num_columns==0)
      c->num_columns=1+(actual_buttons_used-1)/c->num_rows;
    if(c->num_rows==0)
      c->num_rows=1+(actual_buttons_used-1)/c->num_columns;
    while(c->num_rows * c->num_columns < actual_buttons_used)
      c->num_columns++;
    if (!(c->flags&b_SizeFixed))
    {
      while(c->num_rows*c->num_columns >=
	    actual_buttons_used + c->num_columns)
	c->num_rows--;
    }
  }

  if (c->flags&b_SizeSmart)
  {
    /* Set rows/columns to at least the height/width of largest button */
    for(i=0;i<num_items;i++)
    {
      b=local_buttons[i];
      if (c->num_rows<b->BHeight)
	c->num_rows=b->BHeight;
      if (c->num_columns<b->BWidth)
	c->num_columns=b->BWidth;
      if (b->flags&b_PosFixed && c->num_columns<b->BWidth+b->BPosX)
	c->num_columns=b->BWidth+b->BPosX;
      if (b->flags&b_PosFixed && c->num_columns<b->BWidth-b->BPosX)
	c->num_columns=b->BWidth-b->BPosX;
      if (b->flags&b_PosFixed && c->num_rows<b->BHeight+b->BPosY)
	c->num_rows=b->BHeight+b->BPosY;
      if (b->flags&b_PosFixed && c->num_rows<b->BHeight-b->BPosY)
	c->num_rows=b->BHeight-b->BPosY;
    }
  }

  /* this was buggy before */
  c->num_buttons = c->num_rows*c->num_columns;
  alloc_buttonlist(ub,c->num_buttons);

  /* Shuffle subcontainers */
  for(i=0;i<num_items;i++)
  {
    b=local_buttons[i];
    /* Shuffle subcontainers recursively */
    if(b && b->flags&b_Container)
      ShuffleButtons(b);
  }

  /* Place fixed buttons as given in BPosX and BPosY */
  for(i=0;i<num_items;i++)
  {
    b=local_buttons[i];
    if (!(b->flags&b_PosFixed)) continue;
    /* recalculate position for negative offsets */
    if (b->BPosX<0) b->BPosX=b->BPosX+c->num_columns-b->BWidth+1;
    if (b->BPosY<0) b->BPosY=b->BPosY+c->num_rows-b->BHeight+1;
    /* Move button if position given by user */
    if (PlaceAndExpandButton(b->BPosX,b->BPosY,b,ub))
    {
      fprintf(stderr, "%s: Overlapping fixed buttons. Quitting.\n",MyName);
      fprintf(stderr, "Button=%d, x=%d, y=%d\n", i,b->BPosX,b->BPosY);
      exit(1);
    }
  }

  /* place floating buttons dynamically */
  next_button_x = next_button_y = 0;
  for(i=0;i<num_items;i++)
  {
    b=local_buttons[i];
    if (b->flags&b_PosFixed) continue;

    if (next_button_x+b->BWidth>c->num_columns)
    {
      next_button_y++;
      next_button_x=0;
    }
    /* Search for next free position to accomodate button */
    while (PlaceAndExpandButton(next_button_x,next_button_y,b,ub))
    {
      next_button_x++;
      if (next_button_x+b->BWidth>c->num_columns)
      {
	next_button_y++;
	next_button_x=0;
	if (next_button_y>=c->num_rows)
	{
	  /* could not place button */
	  fprintf(stderr,"%s: Button confusion! Quitting\n", MyName);
	  exit(1);
	}
      }
    }
  }

  /* shrink buttons in Container */
  for(i=0;i<num_items;i++)
    ShrinkButton(local_buttons[i], c);
  free(local_buttons);
}

/* ----------------------------- button iterator --------------------------- */

/**
*** NextButton()
*** Iterator to traverse buttontree. Start it with first argument a pointer
*** to the root UberButton, and the index int set to -1. Each subsequent call
*** gives the pointer to uberbutton, button and button index within uberbutton.
*** If all, also returns containers, apart from the UberButton.
**/
button_info *NextButton(button_info **ub,button_info **b,int *i,int all)
{
  /* Get next button */
  (*i)++;
  /* Skip fake buttons */
  while((*i)<(*ub)->c->num_buttons && !(*ub)->c->buttons[*i])
    (*i)++;
  /* End of contained buttons */
  if((*i)>=(*ub)->c->num_buttons)
  {
    *b=*ub;
    *ub=(*b)->parent;
    /* End of the world as we know it */
    if(!(*ub))
    {
      *b=NULL;
      return *b;
    }
    *i=(*b)->n;
    if((*i)>=(*ub)->c->num_buttons)
    {
      fprintf(stderr,"%s: BUG: Couldn't return to uberbutton\n",MyName);
      exit(2);
    }
    NextButton(ub,b,i,all);
    return *b;
  }
  *b=(*ub)->c->buttons[*i];

  /* Found new container */
  if((*b)->flags & b_Container)
  {
    *i=-1;
    *ub=*b;
    if(!all)
      NextButton(ub,b,i,all);
    return *b;
  }
  return *b;
}

/* --------------------------- button navigation --------------------------- */

/**
*** button_belongs_to()
*** Function that finds out which button a given position belongs to.
*** Returns -1 is not part of any, button if a proper button.
**/
int button_belongs_to(button_info *ub,int button)
{
  int x,y,xx,yy;
  button_info *b;
  if(!ub || button<0 || button>ub->c->num_buttons)
    return -1;
  if(ub->c->buttons[button])
    return button;
  yy=button/ub->c->num_columns;
  xx=button%ub->c->num_columns;
  for(y=yy;y>=0;y--)
  {
    for(x=xx;x>=0;x--)
    {
      b=ub->c->buttons[x+y*ub->c->num_columns];
      if(b && (x+b->BWidth > xx) && (y+b->BHeight > yy))
      {
	return x+y*ub->c->num_columns;
      }
    }
  }
  return -1;
}

/**
*** select_button()
*** Given (x,y) and uberbutton, returns pointer to referred button, or NULL
**/
button_info *select_button(button_info *ub,int x,int y)
{
  int i;
  int row;
  int column;
  button_info *b;

  if (!(ub->flags&b_Container))
    return ub;

  x -= buttonXPad(ub) + buttonFrame(ub);
  y -= buttonYPad(ub) + buttonFrame(ub);

  if(x >= ub->c->width || x < 0 || y >= ub->c->height || y < 0)
    return ub;

  column = x * ub->c->num_columns / ub->c->width;
  row = y * ub->c->num_rows / ub->c->height;
  i = button_belongs_to(ub, column + row * ub->c->num_columns);
  if (i == -1)
    return ub;
  b = ub->c->buttons[i];

  return select_button(
      b, x + ub->c->xpos - buttonXPos(b, i),
      y + ub->c->ypos - buttonYPos(b, i));
}
