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
  long flags;
  long desk;
  struct item *next;
} Item;

typedef struct
{
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
void InitList(List *list);
void AddItem(List *list, long id, long flags, long desk );
int FindItem(List *list, long id);
int FindItemDesk(List *list, long id, long desk);

int UpdateItemName(List *list, long id, char *string);
int UpdateItemDesk(List *list, long id, long desk);
int UpdateItemFlags(List *list, long id, long flags);
void FreeItem(Item *ptr);
int DeleteItem(List *list,long id);
void FreeList(List *list);
void PrintList(List *list);
char *ItemName(List *list, int n);
long ItemFlags(List *list, long id );
long ItemFlags(List *list, long id );
long XorFlags(List *list, int n, long value);
int ItemCount(List *list);
int ItemCountDesk(List *list, long desk);
long ItemID(List *list, int n);
void CopyItem(List *dest,List *source,int n);
