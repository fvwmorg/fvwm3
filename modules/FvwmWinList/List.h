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

/* Structure definitions */
typedef struct item
{
  long id;
  char *name;
  window_flags flags;
  long desk;
  struct item *next;
} Item;

typedef struct
{
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
extern void InitList(List *list);
extern void ReorderList(List *list, long id, long FlipFocus);

extern void AddItem(List *list, ConfigWinPacket *cfgpacket);
extern int FindItem(List *list, long id);
extern int FindItemVisible(List *list, long id);
extern long ItemDesk(List *list, long id);
extern int UpdateItemName(List *list, long id, char *string);
extern int UpdateItemDesk(List *list, ConfigWinPacket *cfgpacket);
extern int UpdateItemGSFRFlags(List *list, ConfigWinPacket *cfgpacket);
extern void FreeItem(Item *ptr);
extern int DeleteItem(List *list,long id);
extern void FreeList(List *list);
extern void PrintList(List *list);
extern char *ItemName(List *list, int n);

extern Item *ItemFlags(List *list, long id );

extern int ItemCount(List *list);
extern int ItemCountVisible(List *list);
extern long ItemID(List *list, int n);
extern void CopyItem(List *dest,List *source,int n);
extern int IsItemVisible(Item *temp);
extern int IsItemIndexVisible(List *list,int n);
