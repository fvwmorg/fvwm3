/* -*-c-*- */
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
#include "libs/Flocale.h"

#include "FvwmWinList.h"
#include "ButtonArray.h"
#include "Mallocs.h"
#include "libs/PictureGraphics.h"
#include "libs/Rectangles.h"

extern FlocaleFont *FButtonFont;
extern FlocaleWinString *FwinString;
extern Display *dpy;
extern Window win;
extern GC shadow[MAX_COLOUR_SETS],hilite[MAX_COLOUR_SETS];
extern GC graph[MAX_COLOUR_SETS],background[MAX_COLOUR_SETS];
extern int colorset[MAX_COLOUR_SETS];
extern Pixmap pixmap[MAX_COLOUR_SETS];
extern int LeftJustify, TruncateLeft, ShowFocus;
extern int win_width;
extern int win_height;

extern long CurrentDesk;
extern int ShowCurrentDesk;
extern int UseSkipList;

/*
 * Button handling functions and procedures
 */

/*
  ButtonNew - Allocates and fills a new button structure
*/
Button *ButtonNew(char *title, FvwmPicture *p, int up)
{
  Button *new;

  new = (Button *)safemalloc(sizeof(Button));
  new->title = safemalloc(strlen(title)+1);
  strcpy(new->title, title);
  if (p != NULL)
  {
    new->p.picture = p->picture;
    new->p.mask = p->mask;
    new->p.alpha = p->alpha;
    new->p.width = p->width;
    new->p.height = p->height;
    new->p.depth = p->depth;
  } else {
    new->p.picture = None;
    new->p.width = None;
  }

  new->up = up;
  new->next = NULL;
  new->needsupdate = 1;

  return new;
}

/*
  InitArray - Initialize the arrary of buttons
*/
void InitArray(ButtonArray *array,int x,int y,int w,int h,int rw)
{
  array->count=0;
  array->head=array->tail=NULL;
  array->x=x;
  array->y=y;
  array->w=w;
  array->h=h+2*rw;
  array->rw=rw;
}

/*
  UpdateArray - Update the array width
*/
void UpdateArray(ButtonArray *array, int w)
{
  Button *temp;

  if (w >= 0 && array->w != w)
  {
    array->w = w;
    for(temp = array->head; temp != NULL; temp = temp->next)
      temp->needsupdate = 1;
  }
}

/*
  Reorder buttons in window list order (see List.c:ReorderList()
*/
void ReorderButtons(ButtonArray *array, int ButNum, int FlipFocus)
{
  Button *temp = array->head, *prev = NULL;
  int i = 0;

  if (!ButNum) return; /* this is a NOP if ButNum == 0 */
  /* find the item, marking everything from the top as needing update */
  while (temp && i != ButNum) {
    prev = temp;
    temp->needsupdate = True;
    temp = temp->next;
    i++;
  }

  if (!temp) return; /* we fell off the list */

  /* prev is guaranteed to be !NULL */
  if (FlipFocus) {
    /* take care of the tail of the list */
    if (array->tail == temp) array->tail = prev;
    /* pluck it */
    prev->next = temp->next;
    /* shove it */
    temp->next = array->head;
    array->head = temp;
  } else {
    /* close the end */
    array->tail->next = array->head;
    /* rotate around by changing the list pointers */
    array->head = temp;
    array->tail = prev;
    /* unclose the end */
    prev->next = NULL;
  }

  /* Focus requires the entire array to be redrawn */
  if (!FlipFocus)
    for (temp = array->head; temp; temp = temp->next)
      temp->needsupdate = True;
}

/*
  AddButton - Allocate space for and add the button to the bottom
*/
int AddButton(ButtonArray *array, char *title, FvwmPicture *p, int up)
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

/* in Taskbar this replaces below  ArrangeButtonArray (array);*/

  new->tw=FlocaleTextWidth(FButtonFont,title,strlen(title));
  new->truncatewidth=0;
  new->next=NULL;
  new->needsupdate=1;
  new->set=0;
  new->reliefwidth=array->rw;

  return (array->count-1);
}

/*
  UpdateButton - Change the name/state of a button
*/
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
      temp->tw=FlocaleTextWidth(FButtonFont,title,strlen(title));
      temp->truncatewidth = 0;
    }
    if (up!=-1)
    {
      temp->up=up;
    }
  } else return -1;
  temp->needsupdate=1;
  return 1;
}

/* -------------------------------------------------------------------------
   UpdateButtonPicture - Change the picture of a button
   ------------------------------------------------------------------------- */
int UpdateButtonPicture(ButtonArray *array, int butnum, FvwmPicture *p)
{
  Button *temp;
  temp=find_n(array,butnum);
  if (temp == NULL) return -1;
  if (temp->p.picture != p->picture || temp->p.mask != p->mask)
  {
    temp->p.picture = p->picture;
    temp->p.mask = p->mask;
    temp->p.alpha = p->alpha;
    temp->p.width = p->width;
    temp->p.height = p->height;
    temp->p.depth = p->depth;
    temp->needsupdate = 1;
  }
  return 1;
}

void UpdateButtonIconified(ButtonArray *array, int butnum, int iconified)
{
  Button *temp;

  temp=find_n(array,butnum);
  if (temp!=NULL)
  {
    temp->is_iconified = !!iconified;
  }
  return;
}


/*
  UpdateButtonSet - Change colour set of a button between odd and even
*/
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

/*
  UpdateButtonDeskFlags - Change desk and flags of a button
*/
int UpdateButtonDeskFlags(ButtonArray *array, int butnum, long desk,
			  int is_sticky, int skip)
{
  Button *btn;

  btn = find_n(array, butnum);
  if (btn != NULL)
  {
    btn->desk = desk;
    btn->is_sticky = is_sticky;
    btn->skip = skip;
  } else return -1;
  return 1;
}


/*
  RemoveButton - Delete a button from the list
*/
void RemoveButton(ButtonArray *array, int butnum)
{
  Button *temp, *to_die;

  if (butnum == 0) {
    to_die = array->head;
    temp = NULL;
  } else {
    temp = find_n(array, butnum-1);
    if (temp == NULL) return;
    to_die = temp->next;
  }
  if ( !to_die ) return;

  if (array->tail == to_die)
    array->tail = temp;
  if ( !temp )
    array->head = to_die->next;
  else
    temp->next = to_die->next;

  FreeButton(to_die);
  array->count--;
  if (temp && temp != array->head)
    temp = temp->next;
  for (; temp!=NULL; temp=temp->next)
    temp->needsupdate = 1;
}

/*
  find_n - Find the nth button in the list (Use internally)
*/
Button *find_n(ButtonArray *array, int n)
{
  Button *temp;
  int i;

  temp=array->head;
  for(i=0;i<n && temp!=NULL;i++,temp=temp->next);
  return temp;
}

/*
  FreeButton - Free space allocated to a button
*/
void FreeButton(Button *ptr)
{
  if (ptr != NULL) {
    if (ptr->title!=NULL) free(ptr->title);
    free(ptr);
  }
}

/*
  FreeAllButtons - Free the whole array of buttons
*/
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

/*
  DoButton - Draw the specified button.  (Used internally)
*/
void DoButton(
	Button *button, int x, int y, int w, int h, Bool clear_bg, XEvent *evp)
{
	int up,Fontheight,newx,set,len;
	GC topgc;
	GC bottomgc;
	char *string;
	XGCValues gcv;
	unsigned long gcm;
	FlocaleFont *Ffont;
	FvwmRenderAttributes fra;
	XRectangle rect,inter;
	Region t_region;

	/* The margin we want between the relief/text/pixmaps */
#define INNER_MARGIN 2

	h += 1;
	if (evp)
	{
		if (!frect_get_intersection(
			x, y, w, h,
			evp->xexpose.x, evp->xexpose.y,
			evp->xexpose.width, evp->xexpose.height,
			&rect))
		{
			return;
		}
	}
	else
	{
		rect.x = x;
		rect.y = y;
		rect.width = w;
		rect.height = h;
	}
	up=button->up;
	set=button->set;
	topgc = up ? hilite[set] : shadow[set];
	bottomgc = up ? shadow[set] : hilite[set];
	Ffont = FButtonFont;

	if (Ffont->font != NULL)
	{
		gcm = GCFont;
		gcv.font = Ffont->font->fid;
		XChangeGC(dpy, graph[set], gcm, &gcv);
	}

	Fontheight=FButtonFont->height;

	if ((FftSupport && Ffont->fftf.fftfont != NULL) ||
	    (button->p.picture != 0 && button->p.alpha != 0) ||
	    (colorset[set] >= 0 &&
	     Colorset[colorset[set]].icon_alpha_percent < 100))
	{
		clear_bg = True;
	}

	/* handle transparency by clearing button, otherwise paint with
	 * background */
	if (clear_bg)
	{
		if (CSET_IS_TRANSPARENT_PR_PURE(colorset[set]))
		{
			XClearArea(
				dpy, win, rect.x, rect.y, rect.width,
				rect.height, False);
		}
		else if (CSET_IS_TRANSPARENT_PR_TINT(colorset[set]))
		{
			SetRectangleBackground(
				dpy, win, rect.x, rect.y, rect.width,
				rect.height, &Colorset[colorset[set]], Pdepth,
				background[set]);
		}
		else
		{
			XFillRectangle(
				dpy, win, background[set],
				rect.x, rect.y, rect.width, rect.height);
		}
	}

	if (button->p.picture != 0)
	{
		/* clip pixmap to fit inside button */
		int height = min(button->p.height, h);
		int offset = (button->p.height > h) ?
			0 : ((h - button->p.height) >> 1);

		fra.mask = FRAM_DEST_IS_A_WINDOW;
		if (colorset[set] >= 0)
		{
			fra.mask |= FRAM_HAVE_ICON_CSET;
			fra.colorset = &Colorset[colorset[set]];
		}
		if (frect_get_intersection(
			rect.x, rect.y, rect.width, rect.height,
			x + 2 + button->reliefwidth, y+offset,
			button->p.width, height, &inter))
		{
			PGraphicsRenderPicture(
				dpy, win, &(button->p), &fra,
				win, graph[set], None, None,
				inter.x - (x + 2 + button->reliefwidth),
				inter.y - (y+offset),
				inter.width, inter.height,
				inter.x, inter.y, inter.width, inter.height,
				False);
		}
		newx = button->p.width+2*INNER_MARGIN;
	}
	else
	{
		if (LeftJustify)
		{
			newx = INNER_MARGIN;
			if (!button->is_iconified)
			{
				static int icon_offset = -1;

				if (icon_offset == -1)
					icon_offset = FlocaleTextWidth(
						FButtonFont, "(", 1);
				newx += icon_offset;
			}
		}
		else
		{
			newx = max(
				(w - button->tw) / 2 - button->reliefwidth,
				INNER_MARGIN);
		}
	}
	/* check if the string needs to be truncated */
	string = button->title;
	len = strlen(string);

	if (newx + button->tw + 2 * button->reliefwidth + INNER_MARGIN > w)
	{
		if (button -> truncatewidth == w)
		{
                        /* truncated version already calculated use it */
			string = button->truncate_title;
			len = strlen(string);
		}
		else
		{
			if (TruncateLeft)
			{
				/* move the ptr up until the rest fits */
				while (*string &&
				       (newx + FlocaleTextWidth(
					       FButtonFont, string,
					       strlen(string))
					+ 2 * button->reliefwidth +
					INNER_MARGIN) > w)
				{
					string++;
					len--;
				}
				button->truncatewidth = w;
				button->truncate_title = string;
			}
			else
			{
				while ((len > 1) &&
				       (newx + FlocaleTextWidth(
					       FButtonFont, string, len)
					+ 2 * button->reliefwidth +
					INNER_MARGIN) > w)
				{
					len--;
				}
			}
		}
	}
	FwinString->str = string;
	FwinString->len = len;
	FwinString->x = x+newx+button->reliefwidth;
	FwinString->y = y+1+button->reliefwidth+FButtonFont->ascent;
	FwinString->gc = graph[set];
	t_region = XCreateRegion();
	XUnionRectWithRegion (&rect, t_region, t_region);
	FwinString->flags.has_clip_region = True;
	FwinString->clip_region = t_region;
	FwinString->flags.has_colorset = False;
	if (colorset[set] >= 0)
	{
		FwinString->colorset = &Colorset[colorset[set]] ;
		FwinString->flags.has_colorset = True;
	}
	FlocaleDrawString(dpy, FButtonFont, FwinString, FWS_HAVE_LENGTH);
	FwinString->flags.has_clip_region = False;
	XDestroyRegion(t_region);

	/* Draw relief last */
	RelieveRectangle(
	  dpy,win,x,y,w-1,h-1,topgc,bottomgc,button->reliefwidth);

	button->needsupdate=0;
}

/*
  DrawButtonArray - Draw the whole array (all=1), or only those that need.
*/
void DrawButtonArray(
	ButtonArray *barray, Bool all, Bool clear_bg, XEvent *evp)
{
  Button *btn;
  int i = 0;            /* buttons displayed */

  for(btn = barray->head; btn != NULL; btn = btn->next)
    if (IsButtonVisible(btn))
    {
      if (all || btn->needsupdate)
	DoButton(btn, barray->x, barray->y + (i * (barray->h + 1)),
		 barray->w, barray->h, clear_bg, evp);
      i++;
    }
}

/*
  Mark all the buttons in the array as dirty
*/
void DirtyButtonArray(ButtonArray *barray)
{
	Button *btn;

	for (btn = barray->head; btn != NULL; btn = btn->next)
	{
		btn->needsupdate = 1;
	}

	return;
}

/*
  DrawButtonArray - Draw the whole array (all=1), or only those that need.
*/
void DrawTransparentButtonArray(ButtonArray *barray)
{
	Button *btn;
	int i = 0;            /* buttons displayed */

	for (i = 0; i != MAX_COLOUR_SETS; i++)
	{
		int cset = colorset[i];

		if (!CSET_IS_TRANSPARENT_ROOT(cset))
		{
			continue;
		}
		if (pixmap[i])
			XFreePixmap(dpy, pixmap[i]);
		pixmap[i] = CreateBackgroundPixmap(
			dpy, win, win_width, win_height,
			&Colorset[cset], Pdepth,
			background[i], False);
		XSetTile(dpy, background[i], pixmap[i]);
	}

	i = 0;
	for(btn = barray->head; btn != NULL; btn = btn->next)
	{
		if (IsButtonVisible(btn))
		{
			if (CSET_IS_TRANSPARENT(colorset[btn->set]))
			{
				DoButton(
					btn, barray->x,
					barray->y + (i * (barray->h + 1)),
					barray->w, barray->h, True, NULL);
			}
			i++;
		}
	}
}
/*
  SwitchButton - Alternate the state of a button
*/
void SwitchButton(ButtonArray *array, int butnum)
{
  Button *btn;

  btn = find_n(array, butnum);
  if (btn != NULL)
  {
    btn->up =!btn->up;
    btn->needsupdate=1;
    DrawButtonArray(array, False, True, NULL);
  }
}

/* -------------------------------------------------------------------------
   RadioButton - Enable button i and verify all others are disabled
   ------------------------------------------------------------------------- */
void RadioButton(ButtonArray *array, int butnum, int butnumpressed)
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
    else if (i == butnumpressed)
    {
      if (temp->set & 2)
      {
	temp->set &= 1;
	temp->needsupdate = 1;
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

/*
  WhichButton - Based on x,y which button was pressed
*/
int WhichButton(ButtonArray *array,int x, int y)
{
  int num;
  Button *temp;
  int i, n;

  num=y/(array->h+1);
  if (x<array->x || x>array->x+array->w || num<0 || num>array->count-1) num=-1;

  temp=array->head;
  for(i=0, n = 0;n < (num + 1) && temp != NULL;temp=temp->next, i++)
  {
    if (IsButtonVisible(temp))
      n++;
  }
  num = i-1;

  return(num);
}

/*
  ButtonName - Return the name of the button
*/
char *ButtonName(ButtonArray *array, int butnum)
{
  Button *temp;

  temp=find_n(array,butnum);
  return temp->title;
}

/*
  PrintButtons - Print the array of button names to the console. (Debugging)
*/
void PrintButtons(ButtonArray *array)
{
  Button *temp;

  ConsoleMessage("List of Buttons:\n");
  for(temp=array->head;temp!=NULL;temp=temp->next)
    ConsoleMessage("   %s is %s\n",temp->title,(temp->up) ? "Up":"Down");
}

/*
  ButtonPicture - Return the mini icon associated with the button
*/
FvwmPicture *ButtonPicture(ButtonArray *array, int butnum)
{
  Button *temp;

  if (!FMiniIconsSupported)
  {
    return NULL;
  }
  temp=find_n(array,butnum);
  return &(temp->p);
}

/*
  IsButtonVisible - Says if the button should be in winlist
*/
int IsButtonVisible(Button *btn)
{

  if ((!ShowCurrentDesk || btn->desk == CurrentDesk || btn->is_sticky) &&
      (!btn->skip || !UseSkipList))
    return 1;
  else
    return 0;
}

/*
  IsButtonIndexVisible - Says if the button of index butnum should be
  in winlist
*/
int IsButtonIndexVisible(ButtonArray *array, int butnum)
{
  Button *temp;

  temp=find_n(array,butnum);
  if (temp == NULL) return 0;
  if (IsButtonVisible(temp))
    return 1;
  else
    return 0;
}

#if 0
/*
  ButtonArrayMaxWidth - Calculate the width needed for the widest title
*/
int ButtonArrayMaxWidth(ButtonArray *array)
{
Button *temp;
int x=0;
  for(temp=array->head;temp!=NULL;temp=temp->next)
    x=max(temp->tw,x);
  return x;
}
#endif

