/* FvwmTaskBar Module for Fvwm.
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

/* Structure definitions */
typedef struct item {
  long id;
  char *name;
  /*
     caveat - these flags do not even resemble the old FvwmWindow flags.
     They are strictly internal to this module.
  */
  long tb_flags;
  /* The new, post-gsfr flags - for possible future use.  */
  window_flags flags;
  long Desk;
  int  count;
  Picture p;
  struct item *next;
} Item;

typedef struct {
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
void InitList(List *list);
void AddItem(List *list, long id, long flags, ConfigWinPacket *cfgpacket,
             long Desk, int count);
void AddItemName(List *list, char *string, long flags);
int FindItem(List *list, long id);
int FindNameItem(List *list, char *string);
int UpdateItemName(List *list, long id, char *string);
int UpdateItemFlags(List *list, long id, long flags);
int UpdateItemFlagsDesk(List *list, long id, long flags, long desk);
int UpdateNameItem(List *list, char *string, long id, long flags);
void FreeItem(Item *ptr);
int DeleteItem(List *list,long id);
void FreeList(List *list);
void PrintList(List *list);
char *ItemName(List *list, int n);
long ItemFlags(List *list, long id );
long ItemIndexFlags(List *list, int i);
/* long XorFlags(List *list, int n, long value); */
int ItemCount(List *list);
long ItemID(List *list, int n);
void CopyItem(List *dest,List *source,int n);
void UpdateItemPicture(List *list, int n, Picture *p);
int GetDeskNumber(List *list, int n, long *Desk);
Picture *GetItemPicture(List *list, int n);
