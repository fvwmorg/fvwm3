/* -*-c-*- */
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
 */

/* Structure definitions */
typedef struct item {
  long id;
  char *name;
  /* The new, post-gsfr flags - now the "only" flag  */
  window_flags flags;
  long Desk;
  rectangle win_g;
  int  count;
  FvwmPicture p;
  struct item *next;
} Item;

typedef struct {
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
void InitList(List *list);
void AddItem(List *list, long id, ConfigWinPacket *cfgpacket,
	     long Desk, int count);
void AddItemName(List *list, char *string, int iconified);
int FindItem(List *list, long id);
int FindNameItem(List *list, char *string);
int UpdateItemName(List *list, long id, char *string);
int UpdateItemIconifiedFlag(List *list, long id, int iconified);
int UpdateItemGSFRFlags(List *list, ConfigWinPacket *cfgpacket);
int UpdateItemIndexDesk(List *list, int i, long desk);
int UpdateNameItem(List *list, char *string, long id, int iconified);
int UpdateItemIndexGeometry(List *list, int i, rectangle *new_g);
void FreeItem(Item *ptr);
int DeleteItem(List *list,long id);
void FreeList(List *list);
void PrintList(List *list);
char *ItemName(List *list, int n);
int IsItemIconified(List *list, long id);
int IsItemIndexIconified(List *list, int i);
int IsItemIndexSticky(List *list, int i);
int IsItemIndexSkipWindowList(List *list, int i);
int IsItemIndexIconSupressed(List *list, int i);
int ItemCount(List *list);
long ItemID(List *list, int n);
void CopyItem(List *dest,List *source,int n);
void UpdateItemPicture(List *list, int n, FvwmPicture *p);
int GetDeskNumber(List *list, int n, long *Desk);
FvwmPicture *GetItemPicture(List *list, int n);
int GetItemGeometry(List *list, int n, rectangle **r);
