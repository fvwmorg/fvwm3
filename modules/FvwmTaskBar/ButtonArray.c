/* FvwmTaskBar Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *  Copyright 1995,  Pekka Pietik{inen (pp@netppl.fi)
 *
 * The functions in this source file that are the original work of Mike Finger.
 * This source file has been modified for use with fvwm95look by
 * Pekka Pietik{inen, David Barth, Hector Peraza, etc, etc...
 *
 * No guarantees or warantees or anything are provided or implied in any way
 * whatsoever. Use this program at your own risk. Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
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

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

#include "libs/fvwmlib.h"
#include "libs/Colorset.h"
#include "libs/safemalloc.h"

#include "ButtonArray.h"
#include "Mallocs.h"

#ifdef I18N_MB
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#endif

extern XFontStruct *ButtonFont, *SelButtonFont;
#ifdef I18N_MB
extern XFontSet ButtonFontset, SelButtonFontset;
#endif
extern Display *dpy;
extern Window win;
extern GC shadow, hilite, graph, whitegc, blackgc, checkered, icongraph;
extern GC iconbackgraph;
extern int button_width;
extern int iconcolorset;

extern Button *StartButton;

int w3p;         /* width of the string "..." in pixels */

extern int NRows, RowHeight;

#define MIN_BUTTON_SIZE 32 /*....*/

/*************************************************************************
 *                                                                       *
 *  Button handling functions and procedures                             *
 *                                                                       *
 *************************************************************************/

/* Draws a 3D rectangle */
void Draw3dRect(Window wn, int x, int y, int w, int h, int state,
		Bool iconified)
{
  XClearArea (dpy, wn, x, y, w, h, False);
  if (iconified)
  {
    colorset_struct *cset;

    if (iconcolorset >= 0)
      cset = &Colorset[iconcolorset];
    if (iconcolorset >= 0 && (cset->pixmap || cset->shape_mask))
    {
      /* we have a colorset background */
      SetRectangleBackground(dpy, win, x + 2, y + 2, w - 4, h - 4, cset,
			     Pdepth, icongraph);
    }
    else
    {
      XFillRectangle (dpy, wn, iconbackgraph, x + 2, y + 2, w - 4, h - 4);
    }
  }

  switch (state)
  {
  case BUTTON_UP:
    XDrawLine (dpy, wn, hilite, x, y, x+w-2, y);
    XDrawLine (dpy, wn, hilite, x, y, x, y+h-2);

    XDrawLine (dpy, wn, shadow,  x+1, y+h-2, x+w-2, y+h-2);
    XDrawLine (dpy, wn, shadow,  x+w-2, y+h-2, x+w-2, y+1);
    XDrawLine (dpy, wn, blackgc, x, y+h-1, x+w-1, y+h-1);
    XDrawLine (dpy, wn, blackgc, x+w-1, y+h-1, x+w-1, y);
    break;
  case BUTTON_BRIGHT:
    XFillRectangle (dpy, wn, checkered, x+2, y+2, w-4, h-4);
    XDrawLine (dpy, wn, hilite, x+2, y+2, x+w-3, y+2);
  case BUTTON_DOWN:
    XDrawLine (dpy, wn, blackgc, x, y, x+w-1, y);
    XDrawLine (dpy, wn, blackgc, x, y, x, y+h-1);

    XDrawLine (dpy, wn, shadow, x+1, y+1, x+w-3, y+1);
    XDrawLine (dpy, wn, shadow, x+1, y+1, x+1, y+h-3);
    XDrawLine (dpy, wn, hilite, x+1, y+h-1, x+w-1, y+h-1);
    XDrawLine (dpy, wn, hilite, x+w-1, y+h-1, x+w-1, y+1);
    break;
  }
}


/* -------------------------------------------------------------------------
   ButtonNew - Allocates and fills a new button structure
   ------------------------------------------------------------------------- */
Button *ButtonNew(const char *title, Picture *p, int state, int count)
{
  Button *new;

  new = (Button *)safemalloc(sizeof(Button));
  new->title = safemalloc(strlen(title)+1);
  strcpy(new->title, title);
  if (p != NULL) {
    new->p.picture = p->picture;
    new->p.mask    = p->mask;
    new->p.width   = p->width;
    new->p.height  = p->height;
    new->p.depth   = p->depth;
  } else {
    new->p.picture = 0;
  }

  new->state = state;
  new->count = count;
  new->next  = NULL;
  new->needsupdate = 1;
  new->iconified = 0;

  return new;
}

/* -------------------------------------------------------------------------
   ButtonDraw - Draws the specified button
   ------------------------------------------------------------------------- */
void ButtonDraw(Button *button, int x, int y, int w, int h)
{
  static char *t3p = "...";
  int state, x3p, newx;
  int search_len;
  XFontStruct *font;
#ifdef I18N_MB
  XFontSet fontset;
#endif
  XGCValues gcv;
  unsigned long gcm;
  GC *drawgc;

  if (button == NULL)
    return;
  button->needsupdate = 0;
  state = button->state;
  Draw3dRect(win, x, y, w, h, state, !!(button->iconified));
  if (button->iconified)
    drawgc = &icongraph;
  else
    drawgc = &graph;

  if (state != BUTTON_UP) { x++; y++; }

  if (state == BUTTON_BRIGHT || button == StartButton)
#ifdef I18N_MB
  {
    font = SelButtonFont;
    fontset = SelButtonFontset;
  }
#else
    font = SelButtonFont;
#endif
  else
#ifdef I18N_MB
  {
    font = ButtonFont;
    fontset = ButtonFontset;
  }
#else
    font = ButtonFont;
#endif

  gcm = GCFont;
  gcv.font = font->fid;
  XChangeGC(dpy, *drawgc, gcm, &gcv);

  newx = 4;

  w3p = XTextWidth(font, t3p, 3);

  if ((button->p.picture != 0) &&
      (w + button->p.width + w3p + 3 > MIN_BUTTON_SIZE))
  {
    gcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
    gcv.clip_mask = button->p.mask;
    gcv.clip_x_origin = x + 3;
    gcv.clip_y_origin = y + ((h-button->p.height) >> 1);
    XChangeGC(dpy, hilite, gcm, &gcv);
    XCopyArea(dpy, button->p.picture, win, hilite, 0, 0,
                   button->p.width, button->p.height,
                   gcv.clip_x_origin, gcv.clip_y_origin);
    gcm = GCClipMask;
    gcv.clip_mask = None;
    XChangeGC(dpy, hilite, gcm, &gcv);

    newx += button->p.width+2;
  }

  if (button->title == NULL)
    return;

  search_len = strlen(button->title);

  button->truncate = False;
  if (XTextWidth(font, button->title, search_len) > w-newx-3) {

    x3p = 0;
    while (search_len > 0
	   && ((x3p = newx + XTextWidth(font, button->title, search_len))
	       > w-w3p-3))
      search_len--;

    /* It seems a little bogus that we don't see if the "..." _itself_
       will fit on the button; what if it won't?  Oh well.  */
#ifdef I18N_MB
    XmbDrawString(dpy, win, fontset,
#else
    XDrawString(dpy, win,
#endif
		*drawgc, x + x3p, y+ButtonFont->ascent+4, t3p, 3);
    button->truncate = True;
  }

  /* Only print as much of the title as will fit.  */
  if (search_len)
#ifdef I18N_MB
    XmbDrawString(dpy, win, fontset,
#else
    XDrawString(dpy, win,
#endif
		*drawgc, x+newx, y+font->ascent+4,
		button->title, search_len);
}


/* -------------------------------------------------------------------------
   ButtonUpdate - Change the name/state of a button
   ------------------------------------------------------------------------- */
int ButtonUpdate(Button *button, const char *title, int state)
{
  if (button == NULL) return -1;

  if ((title != NULL) && (button->title != title)) {
    button->title = (char *)saferealloc(button->title,strlen(title)+1);
    strcpy(button->title,title);
    button->needsupdate = 1;
  }

  if (state != DONT_CARE) {
    if (state != button->state) {
      button->state = state;
      button->needsupdate = 1;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------------
   ButtonDelete - Free space allocated to a button
   ------------------------------------------------------------------------- */
void ButtonDelete(Button *ptr)
{
  if (ptr != NULL) {
    if (ptr->title != NULL) free(ptr->title);
    free(ptr);
  }
}

/* -------------------------------------------------------------------------
   ButtonName - Return the name of the button
   ------------------------------------------------------------------------- */
char *ButtonName(Button *button)
{
  if (button == NULL) return NULL;
  else return button->title;
}


/*************************************************************************
 *                                                                       *
 *  ButtonArray handling functions and procedures                        *
 *                                                                       *
 *************************************************************************/

/* -------------------------------------------------------------------------
   InitArray - Initialize the arrary of buttons
   ------------------------------------------------------------------------- */
void InitArray(ButtonArray *array, int x, int y, int w, int h, int tw)
{
   array->count = 0;
   array->head = array->tail = NULL;
   array->x  = x;
   array->y  = y;
   array->w  = w;
   array->h  = h;
   array->tw = tw;
}

/* -------------------------------------------------------------------------
   UpdateArray - Update the array specifics.  x,y, width, height
   ------------------------------------------------------------------------- */
void UpdateArray(ButtonArray *array,int x,int y,int w, int h, int tw)
{
   Button *temp;

   if (x != -1)
     array->x = x;
   if (y != -1)
     array->y = y;
   if (w != -1)
     array->w = w;
   if (h != -1)
     array->h = h;
   if (tw != -1)
     array->tw = tw;
   for(temp=array->head; temp!=NULL; temp=temp->next)
     temp->needsupdate = 1;
}

/* -------------------------------------------------------------------------
   AddButton - Allocate space for and add the button to the list
   ------------------------------------------------------------------------- */
void AddButton(ButtonArray *array, const char *title, Picture *p, int state,
               int count, int iconified)
{
  Button *new, *temp;

  new = ButtonNew(title, p, state, count);
  if (iconified)
    new->iconified = 1;
  if (array->head == NULL)
    array->head = new;
  else
  {
    for (temp = array->head; temp->next; temp = temp->next)
      ;
    temp->next = new;
  }
  array->count++;

  ArrangeButtonArray(array);
}

/* -------------------------------------------------------------------------
   ArrangeButtonArray - Rearrange the button size (called from AddButton,
                          RemoveButton, AdjustWindow)
   ------------------------------------------------------------------------- */

void ArrangeButtonArray (ButtonArray *array)
{
  int tw;
  Button *temp;

  if (!array->count)
    tw = array->w;
  else if (NRows == 1)
    tw = array->w / array->count;
  else
    tw = array->w / ((array->count / NRows)+1);

  if (tw > button_width)
    tw = button_width;
  if (tw < MIN_BUTTON_SIZE)
    tw = MIN_BUTTON_SIZE;

  if (tw != array->tw) /* update needed */
  {
    array->tw = tw;
    for(temp = array->head; temp != NULL; temp = temp->next)
      temp->needsupdate = 1;
  }
}

/* -------------------------------------------------------------------------
   UpdateButton - Change the name/state of a button
   ------------------------------------------------------------------------- */
int UpdateButton(ButtonArray *array, int butnum, const char *title, int state)
{
  Button *temp;

  for (temp = array->head; temp; temp = temp->next)
    if (temp->count == butnum)
      break;

  return ButtonUpdate(temp, title, state);
}

/* -------------------------------------------------------------------------
   UpdateButtonPicture - Change the picture of a button
   ------------------------------------------------------------------------- */
int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p)
{
  Button *temp;

  for (temp=array->head; temp; temp=temp->next)
    if (temp->count == butnum)
      break;

  if (temp == NULL) return -1;
  if (temp->p.picture != p->picture || temp->p.mask != p->mask) {
    temp->p.picture = p->picture;
    temp->p.mask    = p->mask;
    temp->p.width   = p->width;
    temp->p.height  = p->height;
    temp->p.depth   = p->depth;
    temp->needsupdate = 1;
  }
  return 1;
}

/* -------------------------------------------------------------------------
   RemoveButton - Delete a button from the list
   ------------------------------------------------------------------------- */
void RemoveButton(ButtonArray *array, int butnum)
{
  Button *temp, *to_die;

  if (array->head == NULL)
    return;

  if (array->head->count == butnum)
  {
    to_die = array->head;
    array->head = array->head->next;
  }
  else
  {
    for (temp = array->head, to_die = array->head->next; to_die != NULL;
	 to_die = to_die->next, temp = temp->next)
    {
      if (to_die->count == butnum)
	break;
    }
    if (!to_die)
      return;
    temp->next = to_die->next;
  }

  ButtonDelete(to_die);
  if (array->count > 0)
    array->count--;
  for (temp = array->head; temp; temp = temp->next)
    temp->needsupdate = 1;

  ArrangeButtonArray(array);
}

/* -------------------------------------------------------------------------
   find_n - Find the nth button in the list (Use internally)
   ------------------------------------------------------------------------- */
Button *find_n(ButtonArray *array, int n)
{
  Button *temp;

  temp = array->head;
  if (n < 0)
    return NULL;
  for ( ; temp && n != temp->count; temp=temp->next)
    ;
  return temp;
}

/* -------------------------------------------------------------------------
   FreeAllButtons - Free the whole array of buttons
   ------------------------------------------------------------------------- */
void FreeAllButtons(ButtonArray *array)
{
  Button *temp, *temp2;

  for(temp=array->head; temp!=NULL; ) {
    temp2 = temp;
    temp  = temp->next;
    ButtonDelete(temp2);
  }
  array->count = 0;
  array->head = NULL;
}

/* ------------------------------------------------------------------------
   DrawButtonArray - Draw the whole array (all=1), or only those that need.
   ------------------------------------------------------------------------ */
void DrawButtonArray(ButtonArray *array, int all)
{
  Button *temp;
  int x, y, n;

  x = 0;
  y = array->y;
  n = 1;
  for(temp=array->head; temp!=NULL; temp=temp->next) {
    if ((x + array->tw > array->w) && (n < NRows))
      { x = 0; y += RowHeight+2; ++n; }
    if (temp->needsupdate || all)
      ButtonDraw(temp, x + array->x, y, array->tw-3, array->h);
    x += array->tw;
  }
}

/* -------------------------------------------------------------------------
   RadioButton - Enable button i and verify all others are disabled
   ------------------------------------------------------------------------- */
void RadioButton(ButtonArray *array, int butnum, int state)
{
  Button *button;

  for(button=array->head; button!=NULL; button=button->next) {
    if (button->count == butnum) {
      button->state = state;
      button->needsupdate = 1;
    } else {
      if (button->state != BUTTON_UP) {
	button->state = BUTTON_UP;
	button->needsupdate = 1;
      }
    }
  }
}

/* -------------------------------------------------------------------------
   WhichButton - Based on x,y which button was pressed
   ------------------------------------------------------------------------- */
int WhichButton(ButtonArray *array, int xp, int yp)
{
  int   junkx, junky, junkz;
  char *junkt;

  return LocateButton(array, xp, yp, &junkx, &junky, &junkt, &junkz);
}

int LocateButton(ButtonArray *array, int xp,  int yp, int *xb, int *yb,
		 char **name, int *trunc)
{
  Button *temp;
  int num, x, y, n;

  if (xp < array->x || xp > array->x+array->w) return -1;

  x = 0;
  y = array->y;
  n = 1;
  for(temp=array->head, num=0; temp!=NULL; temp=temp->next, num++) {
    if((x + array->tw > array->w) && (n < NRows))
      { x = 0; y += RowHeight+2; ++n; }
    if( xp >= x+array->x && xp <= x+array->x+array->tw-3 &&
        yp >= y && yp <= y+array->h) break;
    x += array->tw;
  }

  if (num<0 || num>array->count-1) num = -1;

  *xb = x+array->x;
  *yb = y;
  if (temp) {
    *name  = temp->title;
    *trunc = temp->truncate;
  } else {
    *name = NULL;
  }

  return (num == -1) ? num : temp->count;
}

extern int StartButtonWidth;

void ButtonCoordinates(ButtonArray *array, int numbut, int *xc, int *yc)
{
  Button *temp;
  int x = 0;
  int y = 0;
  int r = 0;

  for(temp=array->head; temp->count != numbut; temp=temp->next) {
    if((x + 2*array->tw > array->w) && (r < NRows))
      { x = 0; y += RowHeight+2; ++r; }
    else
     x += array->tw;
  }

  *xc = x+StartButtonWidth+3;
  *yc = y;
}

