/* FvwmWinList Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The functions in this source file that are the original work of Mike Finger.
 *
 * No guarantees or warantees or anything are provided or implied in any way
 * whatsoever. Use this program at your own risk. Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 *
 *  Things to do:  Convert to C++  (In Progress)
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

#include "../../libs/fvwmlib.h"

#include "ButtonArray.h"
#include "Mallocs.h"


extern XFontStruct *ButtonFont;
extern Display *dpy;
extern Window win;
extern GC shadow[MAX_COLOUR_SETS],hilite[MAX_COLOUR_SETS];
extern GC graph[MAX_COLOUR_SETS],background[MAX_COLOUR_SETS];
extern int LeftJustify, TruncateLeft, ShowFocus;

extern long CurrentDesk;
extern int ShowCurrentDesk;


/*************************************************************************
 *                                                                       *
 *  Button handling functions and procedures                             *
 *                                                                       *
 *************************************************************************/

/* -------------------------------------------------------------------------
   ButtonNew - Allocates and fills a new button structure
   ------------------------------------------------------------------------- */
Button *ButtonNew(char *title, Picture *p, int up)
{
  Button *new;

  new = (Button *)safemalloc(sizeof(Button));
  new->title = safemalloc(strlen(title)+1);
  strcpy(new->title, title);
  if (p != NULL)
  {
    new->p.picture = p->picture;
    new->p.mask = p->mask;
    new->p.width = p->width;
    new->p.height = p->height;
    new->p.depth = p->depth;
  }
  else

  new->p.picture = 0;

  new->up = up;
  new->next = NULL;
  new->needsupdate = 1;

  return new;
}

/******************************************************************************
  InitArray - Initialize the arrary of buttons
******************************************************************************/
void InitArray(ButtonArray *array,int x,int y,int w,int h)
{
  array->count=0;
  array->head=array->tail=NULL;
  array->x=x;
  array->y=y;
  array->w=w;
  array->h=h;
}

/******************************************************************************
  UpdateArray - Update the array specifics.  x,y, width, height
******************************************************************************/
void UpdateArray(ButtonArray *array,int x,int y,int w, int h)
{
  Button *temp;

  if (x!=-1) array->x=x;
  if (y!=-1) array->y=y;
  if (w!=-1) array->w=w;
  if (h!=-1) array->h=h;
  for(temp=array->head;temp!=NULL;temp=temp->next) temp->needsupdate=1;
}

/******************************************************************************
  AddButton - Allocate space for and add the button to the bottom
******************************************************************************/
int AddButton(ButtonArray *array, char *title, Picture *p, int up)
{
  Button *new;

  new = ButtonNew(title, p, up);
  if (array->head == NULL)
  {
    array->head = array->tail = new;
  }
  else
  {
    array->tail->next = new;
    array->tail = new;
  }
  array->count++;

/* in Taskbar this replaces below  ArrangeButtonArray (array);
*/

  new->tw=XTextWidth(ButtonFont,title,strlen(title));
  new->truncatewidth=0;
  new->next=NULL;
  new->needsupdate=1;
  new->set=0;

  return (array->count-1);
}

/******************************************************************************
  UpdateButton - Change the name/stae of a button
******************************************************************************/
int UpdateButton(ButtonArray *array, int butnum, char *title, int up)
{
  Button *temp;

  temp=find_n(array,butnum);
  if (temp!=NULL)
  {
    if (title!=NULL)
    {
      temp->title=(char *)saferealloc(temp->title,strlen(title)+1);
      strcpy(temp->title,title);
      temp->tw=XTextWidth(ButtonFont,title,strlen(title));
      temp->truncatewidth = 0;
    }
    if (up!=-1) temp->up=up;
  } else return -1;
  temp->needsupdate=1;
  return 1;
}

/* -------------------------------------------------------------------------
   UpdateButtonPicture - Change the picture of a button
   ------------------------------------------------------------------------- */
int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p)
{
  Button *temp;
  temp=find_n(array,butnum);
  if (temp == NULL) return -1;
  if (temp->p.picture != p->picture || temp->p.mask != p->mask)
  {
    temp->p.picture = p->picture;
    temp->p.mask = p->mask;
    temp->p.width = p->width;
    temp->p.height = p->height;
    temp->p.depth = p->depth;
    temp->needsupdate = 1;
  }
  return 1;
}

/******************************************************************************
  UpdateButtonSet - Change colour set of a button between odd and even
******************************************************************************/
int UpdateButtonSet(ButtonArray *array, int butnum, int set)
{
  Button *btn;

  btn=find_n(array, butnum);
  if (btn != NULL)
  {
    if ((btn->set & 1) != set)
    {
      btn->set = (btn->set & 2) | set;
      btn->needsupdate = 1;
    }
  } else return -1;
  return 1;
}

/******************************************************************************
  UpdateButtonDesk - Change desk of a button
******************************************************************************/
int UpdateButtonDesk(ButtonArray *array, int butnum, long desk )
{
  Button *btn;

  btn = find_n(array, butnum);
  if (btn != NULL)
  {
    btn->desk = desk;
  } else return -1;
  return 1;
}


/******************************************************************************
  RemoveButton - Delete a button from the list
******************************************************************************/
void RemoveButton(ButtonArray *array, int butnum)
{
  Button *temp,*temp2;

  if (butnum==0)
  {
    temp2=array->head;
    temp=array->head=array->head->next;
  }
  else
  {
    temp=find_n(array,butnum-1);
    if (temp==NULL) return;
    temp2=temp->next;
    temp->next=temp2->next;
  }

  if (array->tail==temp2) array->tail=temp;

  FreeButton(temp2);

  if (temp!=array->head) temp=temp->next;
  for(;temp!=NULL;temp=temp->next) temp->needsupdate=1;
}

/******************************************************************************
  find_n - Find the nth button in the list (Use internally)
******************************************************************************/
Button *find_n(ButtonArray *array, int n)
{
  Button *temp;
  int i;

  temp=array->head;
  for(i=0;i<n && temp!=NULL;i++,temp=temp->next);
  return temp;
}

/******************************************************************************
  FreeButton - Free space allocated to a button
******************************************************************************/
void FreeButton(Button *ptr)
{
  if (ptr != NULL) {
    if (ptr->title!=NULL) free(ptr->title);
    free(ptr);
  }
}

/******************************************************************************
  FreeAllButtons - Free the whole array of buttons
******************************************************************************/
void FreeAllButtons(ButtonArray *array)
{
Button *temp,*temp2;
  for(temp=array->head;temp!=NULL;)
  {
    temp2=temp;
    temp=temp->next;
    FreeButton(temp2);
  }
}

/******************************************************************************
  DoButton - Draw the specified button.  (Used internally)
******************************************************************************/
void DoButton(Button *button, int x, int y, int w, int h)
{
  int up,Fontheight,newx,set;
  GC topgc;
  GC bottomgc;
  char *string;
  XGCValues gcv;
  unsigned long gcm;
  XFontStruct *font;

  up=button->up;
  set=button->set;
  topgc = up ? hilite[set] : shadow[set];
  bottomgc = up ? shadow[set] : hilite[set];
  font = ButtonFont;

  gcm = GCFont;
  gcv.font = font->fid;
  XChangeGC(dpy, graph[set], gcm, &gcv);


  Fontheight=ButtonFont->ascent+ButtonFont->descent;

 /*? XClearArea(dpy,win,x,y,w,h,False);*/
  XFillRectangle(dpy,win,background[set],x,y,w,h+1);

  if ((button->p.picture != 0)/* &&
      (w + button->p.width + w3p + 3 > MIN_BUTTON_SIZE)*/) {

    gcm = GCClipMask|GCClipXOrigin|GCClipYOrigin;
    gcv.clip_mask = button->p.mask;
    gcv.clip_x_origin = x + 4;
    gcv.clip_y_origin = y + ((h-button->p.height) >> 1);
    XChangeGC(dpy, hilite[set], gcm, &gcv);
    XCopyArea(dpy, button->p.picture, win, hilite[set], 0, 0,
                   button->p.width, button->p.height,
                   gcv.clip_x_origin, gcv.clip_y_origin);
    gcm = GCClipMask;
    gcv.clip_mask = None;
    XChangeGC(dpy, hilite[set], gcm, &gcv);

    newx = button->p.width+6;
  }
  else
  {
    if (LeftJustify)
      newx=4;
    else
      newx=max((w-button->tw)/2,4);
  }

  string=button->title;

  if (!LeftJustify) {
    if (TruncateLeft && (w-button->tw)/2 < 4) {
      if (button->truncatewidth == w)
	string=button->truncate_title;
      else {
	string=button->title;
	while(*string && (w-XTextWidth(ButtonFont,string,strlen(string)))/2 < 4)
	    string++;
	button->truncatewidth = w;
	button->truncate_title=string;
      }
    }
  }
  XDrawString(dpy,win,graph[set],x+newx,y+3+ButtonFont->ascent,string,strlen(string));
  button->needsupdate=0;

  /* Draw relief last, don't forget that XDrawLine doesn't do the last pixel */
  XDrawLine(dpy,win,topgc,x,y,x+w-1,y);
  XDrawLine(dpy,win,topgc,x+1,y+1,x+w-2,y+1);
  XDrawLine(dpy,win,topgc,x,y+1,x,y+h+1);
  XDrawLine(dpy,win,topgc,x+1,y+2,x+1,y+h);
  XDrawLine(dpy,win,bottomgc,x+1,y+h,x+w,y+h);
  XDrawLine(dpy,win,bottomgc,x+2,y+h-1,x+w-1,y+h-1);
  XDrawLine(dpy,win,bottomgc,x+w-1,y,x+w-1,y+h);
  XDrawLine(dpy,win,bottomgc,x+w-2,y+1,x+w-2,y+h-1);

}

/******************************************************************************
  DrawButtonArray - Draw the whole array (all=1), or only those that need.
******************************************************************************/
void DrawButtonArray(ButtonArray *barray, int all)
{
  Button *btn;
  int i = 0;		/* buttons displayed */

  for(btn = barray->head; btn != NULL; btn = btn->next)
  {
    if((!ShowCurrentDesk) || ( btn->desk == CurrentDesk ) )
    {
      if (btn->needsupdate || all)
      {
        DoButton
        (
  		btn,barray->x,
  		barray->y+(i*(barray->h+1)),
  		barray->w,barray->h
    	);
      }
      i++;
    }
  }
}

/******************************************************************************
  SwitchButton - Alternate the state of a button
******************************************************************************/
void SwitchButton(ButtonArray *array, int butnum)
{
  Button *btn;

  btn = find_n(array, butnum);
  btn->up =!btn->up;
  btn->needsupdate=1;
  DrawButtonArray(array, 0);
}

/* -------------------------------------------------------------------------
   RadioButton - Enable button i and verify all others are disabled
   ------------------------------------------------------------------------- */
void RadioButton(ButtonArray *array, int butnum)
{
  Button *temp;
  int i;

  for(temp=array->head,i=0; temp!=NULL; temp=temp->next,i++)
  {
    if (i == butnum)
    {
      if (ShowFocus && temp->up)
      {
        temp->up = 0;
        temp->needsupdate=1;
      }
      if (!(temp->set & 2))
      {
        temp->set |= 2;
        temp->needsupdate=1;
      }
    }
    else
    {
      if (ShowFocus && !temp->up)
      {
        temp->up = 1;
        temp->needsupdate = 1;
      }
      if (temp->set & 2)
      {
        temp->set &= 1;
        temp->needsupdate = 1;
      }
    }
  }
}

/******************************************************************************
  WhichButton - Based on x,y which button was pressed
******************************************************************************/
int WhichButton(ButtonArray *array,int x, int y)
{
  int num;

  num=y/(array->h+1);
  if (x<array->x || x>array->x+array->w || num<0 || num>array->count-1) num=-1;

  /* Current Desk Hack */

  if(ShowCurrentDesk)
  {
    Button *temp;
    int i, n;

    temp=array->head;
    for(i=0, n = 0;n < (num + 1) && temp != NULL;temp=temp->next, i++)
    {
       if(temp->desk == CurrentDesk)
         n++;
    }
    num = i-1;
  }
  return(num);
}

/******************************************************************************
  ButtonName - Return the name of the button
******************************************************************************/
char *ButtonName(ButtonArray *array, int butnum)
{
  Button *temp;

  temp=find_n(array,butnum);
  return temp->title;
}

/******************************************************************************
  PrintButtons - Print the array of button names to the console. (Debugging)
******************************************************************************/
void PrintButtons(ButtonArray *array)
{
  Button *temp;

  ConsoleMessage("List of Buttons:\n");
  for(temp=array->head;temp!=NULL;temp=temp->next)
    ConsoleMessage("   %s is %s\n",temp->title,(temp->up) ? "Up":"Down");
}

#if 0
/******************************************************************************
  ButtonArrayMaxWidth - Calculate the width needed for the widest title
******************************************************************************/
int ButtonArrayMaxWidth(ButtonArray *array)
{
Button *temp;
int x=0;
  for(temp=array->head;temp!=NULL;temp=temp->next)
    x=max(temp->tw,x);
  return x;
}
#endif
