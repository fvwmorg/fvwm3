/* FvwmTaskBar Module for Fvwm.
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
#include "libs/Module.h"
#include "libs/safemalloc.h"

#include <stdio.h>
#include <stdlib.h>

#include "FvwmTaskBar.h"
#include "List.h"
#include "Mallocs.h"

extern char *Module;

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
void AddItem(List *list, long id, ConfigWinPacket *cfgpacket,
             long Desk, int count)
{
  Item *new;
  new=(Item *)safemalloc(sizeof(Item));
  new->id=id;
  new->name=NULL;
  memcpy(&new->flags, &cfgpacket->flags, sizeof(new->flags));
  new->Desk=Desk;
  new->count=count;
  memset((void *)&(new->p), 0, sizeof(Picture));
  new->next=NULL;
  if (list->tail==NULL) list->head=list->tail=new;
  else {
   list->tail->next=new;
   list->tail=new;
  }
  list->count++;
}

/******************************************************************************
  AddItemName - Allocates spaces for and appends an item to the list
******************************************************************************/
void AddItemName(List *list, char *string, int iconified)
{
  Item *new;

  new = (Item *)safemalloc(sizeof(Item));
  new->id = 0L;
  new->name = NULL;
  UpdateString(&new->name, string);
  SET_ICONIFIED(new,iconified);
  new->next = NULL;

  if (list->tail == NULL)
    list->head = list->tail = new;
  else
  {
    list->tail->next = new;
    list->tail = new;
  }
  list->count++;
}


/******************************************************************************
  FindItem - Find the item in the list matching the id
******************************************************************************/
int FindItem(List *list, long id)
{
  Item *temp;

  for(temp=list->head;temp!=NULL && temp->id!=id;temp=temp->next);
  if (temp==NULL)
    return -1;
  return temp->count;
}

/******************************************************************************
  FindNameItem - Find the item in the list matching the string
******************************************************************************/
int FindNameItem(List *list, char *string)
{
  Item *temp;

  for(temp=list->head;
      temp!=NULL && strcmp(temp->name,string) != 0;
      temp=temp->next);
  if (temp==NULL) return -1;
  return temp->count;
}

/******************************************************************************
  UpdateItem* - Update the item in the list
******************************************************************************/

int UpdateItemName(List *list, long id, char *string)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  UpdateString(&temp->name,string);
  return temp->count;
}

int UpdateItemIconifiedFlag(List *list, long id, int iconified)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && id!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  SET_ICONIFIED(temp,iconified);
  return temp->count;
}

int UpdateItemGSFRFlags(List *list, ConfigWinPacket *cfgpacket)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && cfgpacket->w!=temp->id;temp=temp->next);
  if (temp==NULL) return -1;
  temp->flags = cfgpacket->flags;
  return temp->count;
}

int UpdateItemIndexDesk(List *list, int i, long desk)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  temp->Desk=desk;
  return temp->count;
}

int UpdateNameItem(List *list, char *string, long id, int iconified)
{
  Item *temp;

  for(temp=list->head;
      temp!=NULL && strcmp(temp->name,string) != 0;
      temp=temp->next);
  if (temp==NULL) return -1;
  else {
    if (id != -1) temp->id = id;
    if (iconified != -1) SET_ICONIFIED(temp,iconified);
    return temp->count;
  }
}

int UpdateItemIndexGeometry(List *list, int i, rectangle *new_g)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  temp->win_g = *new_g;
  return temp->count;
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
  if (list->head->id==id) {
    temp2=list->head;
    temp=list->head=list->head->next;
    i=temp2->count;
  } else {
     for(temp=list->head,temp2=temp->next; temp2; temp2=temp2->next) {
      if (temp2->id == id)
        break;
      temp = temp2;
    }
    if (temp2 == NULL) return -1;
    temp->next=temp2->next;
  }

  if (temp2==list->tail) list->tail=temp;

  i=temp2->count;
  FreeItem(temp2);
  list->count--;
  return i;
/*    return i+1; */
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
  PrintList - Print the list of item to stderr. (Debugging)
******************************************************************************/
void PrintList(List *list)
{
  Item *temp;
  fprintf(stderr,"%s List of Items:\n", Module);
  fprintf(stderr,"   %10s %-15s %-15s %-15s %-15s Flgs\n","ID","Name","I-Name",
		 "R-Name","R-Class");
  fprintf(stderr,"   ---------- --------------- --------------- --------------- --------------- ----\n");
  for(temp=list->head;temp!=NULL;temp=temp->next) {
    fprintf(stderr,"   %10ld %-15.15s\n",temp->id,
		   (temp->name==NULL) ? "<null>" : temp->name);
  }
}

/******************************************************************************
  ItemName - Return the name of an Item
******************************************************************************/
char *ItemName(List *list, int n)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=n;temp=temp->next);
  if (temp==NULL) return NULL;
  return temp->name;
}

/******************************************************************************
  IsItemIndexIconified - Say if an item is iconified
******************************************************************************/
int IsItemIconified(List *list, long id)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->id!=id;temp=temp->next);
  if (temp==NULL) return -1;
  return IS_ICONIFIED(temp);
}

/******************************************************************************
  IsItemIndexIconified - Say if an item of index i is iconified
******************************************************************************/
int IsItemIndexIconified(List *list, int i)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  return IS_ICONIFIED(temp);
}

/******************************************************************************
  IsItemIndexSticky - Say if an item of index i is sticky
******************************************************************************/
int IsItemIndexSticky(List *list, int i)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  return (IS_STICKY(temp) || (IS_ICONIFIED(temp) && IS_ICON_STICKY(temp)));
}

/******************************************************************************
  IsItemIndexSkipWindowList - Say if an item of index i is in the skip list
******************************************************************************/
int IsItemIndexSkipWindowList(List *list, int i)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  return DO_SKIP_WINDOW_LIST(temp);
}

/******************************************************************************
  IsItemIndexIconSuppressed - Say if an item has a no icon style
******************************************************************************/
int IsItemIndexIconSuppressed(List *list, int i)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=i;temp=temp->next);
  if (temp==NULL) return -1;
  return IS_ICON_SUPPRESSED(temp);
}

/******************************************************************************
  ItemCount - Return the number of items inthe list
******************************************************************************/
int ItemCount(List *list)
{
  return list->count;
}

/******************************************************************************
  ItemID - Return the ID of the item in the list.
******************************************************************************/
long ItemID(List *list, int n)
{
  Item *temp;
  for(temp=list->head;temp!=NULL && temp->count!=n;temp=temp->next);
  if (temp==NULL) return -1;
  return temp->id;
}

/******************************************************************************
  CopyItem - Copy an item from one list to another
******************************************************************************/
void CopyItem(List *dest, List *source, int n)
{
  Item *temp;
  ConfigWinPacket cfgpkt;

  for(temp=source->head;temp!=NULL && temp->count!=n;temp=temp->next);
  if (temp==NULL) return;
  memcpy(&cfgpkt.flags, &temp->flags, sizeof(cfgpkt.flags));
  AddItem(dest, temp->id, &cfgpkt,temp->Desk,temp->count);
  UpdateItemName(dest,temp->id,temp->name);
  DeleteItem(source,temp->id);
}

/******************************************************************************
  UpdateItemPicture - Adds the picture information in the list
******************************************************************************/
void UpdateItemPicture(List *list, int n, Picture *p)
{
  Item *temp;

  for (temp=list->head;temp && temp->count!=n;temp=temp->next);
  if (temp==NULL) return;
  if (p != NULL)
  {
    temp->p.picture = p->picture;
    temp->p.mask    = p->mask;
    temp->p.width   = p->width;
    temp->p.height  = p->height;
    temp->p.depth   = p->depth;
  }
}

/******************************************************************************
  GetDeskNumber - Returns the desknumber of the item
******************************************************************************/
int GetDeskNumber(List *list, int n, long *Desk)
{
  Item *temp;

  for (temp=list->head;temp && temp->count!=n;temp=temp->next);
  if (temp==NULL) return 0;
  *Desk=temp->Desk;
  return 1;
}

/******************************************************************************
  GetItemPicture - Returns the picture
******************************************************************************/
Picture *GetItemPicture(List *list, int n)
{
  Item *temp;

  for (temp=list->head;temp && temp->count!=n;temp=temp->next);
  if (temp==NULL) return 0;
  return &(temp->p);
}

/******************************************************************************
  GetItemGeometry - returns a pointer to the internal geometry rectangle
******************************************************************************/
int GetItemGeometry(List *list, int n, rectangle **r)
{
  Item *temp;

  for (temp=list->head;temp && temp->count!=n;temp=temp->next);
  if (temp==NULL) return 0;
  *r=&temp->win_g;
  return 1;
}

