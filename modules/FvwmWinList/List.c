/* FvwmWinList Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The functions in this source file are the original work of Mike Finger.
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
#include <stdio.h>
#include <stdlib.h>

#include "FvwmWinList.h"
#include "List.h"
#include "Mallocs.h"
#include "libs/Module.h"
#include "fvwm/fvwm.h"


/******************************************************************************
  InitList - Initialize the list
******************************************************************************/
void InitList(List *list)
{
  list->head=list->tail=NULL;
  list->count=0;
}

/******************************************************************************
  AddItem - Allocates spaces for and appends an item to the list
******************************************************************************/
/*void AddItem(List *list, long id,long flags, long desk)*/
void AddItem(List *list, ConfigWinPacket *cfgpacket)
{
Item *new;
  new=(Item *)safemalloc(sizeof(Item));
  new->id=cfgpacket->w;
  new->name=NULL;
  new->flags=cfgpacket->flags;
  new->desk=cfgpacket->desk;
  new->next=NULL;

  if (list->tail==NULL) list->head=list->tail=new;
  else {
   list->tail->next=new;
   list->tail=new;
  }
  list->count++;
}

/******************************************************************************
  ReorderList - Make the list order matcht the internal fvwm winlist
  FlipFocus is False when the Focus command has been used, True for all other
  cases of Focus change.
  If true the item is plucked from the list and placed at the start
  If false the list is closed into a loop, rotated around to bring id to
  the top and then opened up into a terminated line again.
******************************************************************************/
void ReorderList(List *list, long id, long FlipFocus)
{
  Item *temp = list->head, *prev = NULL;
  int i = 0;

  if (!id) return; /* this is a NOP if id == 0 */

  /* find the item */
  while (temp && i != id) {
    prev = temp;
    temp = temp->next;
    i++;
  }

  if (!temp) return; /* we fell off the list */

  /* prev is guaranteed to be !NULL */
  if (FlipFocus) {
    /* take care of the tail of the list */
    if (list->tail == temp) list->tail = prev;
    /* pluck it */
    prev->next = temp->next;
    /* shove it */
    temp->next = list->head;
    list->head = temp;
  } else {
    /* close the end */
    list->tail->next = list->head;
    /* rotate around by changing the list pointers */
    list->head = temp;
    list->tail = prev;
    /* unclose the end */
    prev->next = NULL;
  }
}

/******************************************************************************
  FindItem - Find the item in the list matching the id
******************************************************************************/
int FindItem(List *list, long id)
{
  Item *temp;
  int i;

  for(i=0,temp=list->head;temp!=NULL && temp->id!=id;i++,temp=temp->next);
  if (temp==NULL) return -1;
  return i;
}

/******************************************************************************
  FindItemDesk - Find the item in the list matching the id, and desk id
******************************************************************************/
int FindItemDesk(List *list, long id, long desk)
{
  Item *temp;
  int i;

  for(i=0,temp=list->head;temp!=NULL && temp->id!=id;temp=temp->next) 
    if (temp->desk == desk) i++;
  if (temp==NULL) return -1;
  return i;
}


/******************************************************************************
  UpdateItem - Update the item in the list, setting name & flags as necessary.
******************************************************************************/
int UpdateItemName(List *list, long id, char *string)
{
  Item *temp;
  int i;

  for(i=0,temp=list->head;temp!=NULL && id!=temp->id;i++,temp=temp->next);
  if (temp==NULL) return -1;
  UpdateString(&temp->name, string);
  return i;
}

/******************************************************************************
  UpdateItemDesk - Update the item in the list, setting desk as necessary.
  returns 1 if desk was updated,
  returns 0, if not changed
  returns -1 if not found
******************************************************************************/
int UpdateItemDesk(List *list, ConfigWinPacket *cfgpacket)
{
  Item *temp;

  for(temp=list->head;temp != NULL && cfgpacket->w!=temp->id;temp=temp->next);
  if (temp ==NULL ) return -1;
  if(temp->desk != cfgpacket->desk)
  {
    temp->desk = cfgpacket->desk;
    return 1;
  }

  return 0;
}

/******************************************************************************
  UpdateItemGSFRFlags - Update the GSFR flags
  returns -1 if not found
******************************************************************************/
int UpdateItemGSFRFlags(List *list, ConfigWinPacket *cfgpacket)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && cfgpacket->w!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  temp->flags = cfgpacket->flags; 
  return 0;
}

/******************************************************************************
  FreeItem - Frees allocated space for an Item
******************************************************************************/
void FreeItem(Item *ptr)
{
  if (ptr != NULL) {
    if (ptr->name!=NULL) free(ptr->name);
    free(ptr);
  }
}

/******************************************************************************
  DeleteItem - Deletes an item from the list
******************************************************************************/
int DeleteItem(List *list,long id)
{
  Item *temp,*temp2;
  int i;

  if (list->head==NULL) return -1;
  if (list->head->id==id)
  {
    temp2=list->head;
    temp=list->head=list->head->next;
    i=-1;
  }
  else
  {
    for(i=0,temp=list->head;temp->next!=NULL && temp->next->id!=id;
      i++,temp=temp->next);
    if (temp->next==NULL) return -1;
    temp2=temp->next;
    temp->next=temp2->next;
  }

  if (temp2==list->tail) list->tail=temp;

  FreeItem(temp2);
  list->count--;
  return i+1;
}

/******************************************************************************
  FreeList - Free the entire list of Items
******************************************************************************/
void FreeList(List *list)
{
  Item *temp,*temp2;

  for(temp=list->head;temp!=NULL;)
  {
    temp2=temp;
    temp=temp->next;
    FreeItem(temp2);
  }
  list->count=0;
}

/******************************************************************************
  PrintList - Print the list of item on the console. (Debugging)
******************************************************************************/
void PrintList(List *list)
{
    Item *temp;
    ConsoleMessage("List of Items:\n");
    ConsoleMessage("   %10s %-15s %-15s %-15s %-15s Flgs\n",
		   "ID","Name","I-Name", "R-Name","R-Class");
    ConsoleMessage("   ---------- --------------- --------------- --------------- --------------- ----\n");
    for(temp=list->head;temp!=NULL;temp=temp->next) {
	int i;
	unsigned char* p = (unsigned char*)&temp->flags;
	ConsoleMessage("   %10ld %-15.15s ",
		       temp->id,
		       (temp->name==NULL) ? "<null>" : temp->name);
	for( i = 0; i < sizeof(temp->flags); ++i ) {
	    ConsoleMessage( "%x2", *p++ );
	}
	ConsoleMessage( "\n" );
    }
}

/******************************************************************************
  ItemName - Return the name of an Item
******************************************************************************/
char *ItemName(List *list, int n)
{
  Item *temp;
  int i;

  for(i=0,temp=list->head;temp!=NULL && i<n;i++,temp=temp->next);
  if (temp==NULL) return NULL;
  return temp->name;
}

/******************************************************************************
  ItemFlags - Return the flags for an item
  RBW - this is no longer appropriate since the Great Style Flag Rewrite, so
  this function will just return the Item pointer. The GSFR macros know how
  deal with that.
  Function returns NULL if the item is not found.
******************************************************************************/
Item *ItemFlags(List *list, long id)
{
  Item *temp;

  for(temp=list->head; temp != NULL && id!=temp->id; temp=temp->next);

  return temp;
}

/******************************************************************************
  IsItemSticky - Say if an item is sticky
******************************************************************************/
int IsItemSticky(List *list, long id)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  return IS_STICKY(temp);
}

/******************************************************************************
  IsItemIndexIconSuppressed - Say if an item has a no icon style
******************************************************************************/
int IsItemIconSuppressed(List *list, long id)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  return IS_ICON_SUPPRESSED(temp);
}

/******************************************************************************
  IsItemIndexSkipWindowList - Say if an item is in the skip list
******************************************************************************/
int IsItemSkipWindowList(List *list, long id)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  return DO_SKIP_WINDOW_LIST(temp);
}

/******************************************************************************
  ItemDesk - Return the desk for an item
******************************************************************************/
long ItemDesk(List *list, long id)
{
  Item *temp;

  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);

  if (temp==NULL) return -1;
  else return temp->desk;
}



/******************************************************************************
  ItemCount - Return the number of items inthe list
******************************************************************************/
int ItemCount(List *list)
{
  return list->count;
}

/******************************************************************************
  ItemCountDesk - Return the number of items inthe list, with desk desk
******************************************************************************/

int ItemCountDesk(List *list, long desk)
{
  Item *temp;
  int count=0;

/*return list->count;*/

  for(temp=list->head;
	temp != NULL;
	temp = temp->next
  )
  {
    if(temp->desk == desk)
      count++;
  }

  return count;
}

/******************************************************************************
  ItemID - Return the ID of the item in the list.
******************************************************************************/
long ItemID(List *list, int n)
{
  Item *temp;
  int i;

  for(i=0,temp=list->head;temp!=NULL && i<n;i++,temp=temp->next);
  if (temp==NULL) return -1;
  return temp->id;
}

/******************************************************************************
  CopyItem - Copy an item from one list to another
******************************************************************************/
void CopyItem(List *dest, List *source, int n)
{
  Item *temp;
  int i;
  ConfigWinPacket cfgpkt;

  for(i=0,temp=source->head;temp!=NULL && i<n;i++,temp=temp->next);
  if (temp==NULL) return;
  memset(&cfgpkt, '\0', sizeof(cfgpkt));
  AddItem(dest, &cfgpkt);
  UpdateItemName(dest,temp->id,temp->name);
  DeleteItem(source,temp->id);
}

