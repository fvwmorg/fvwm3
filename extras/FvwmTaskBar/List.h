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

/* Structure definitions */
typedef struct item {
  long id;
  char *name;
  long flags;
  struct item *next;
} Item;

typedef struct {
  Item *head,*tail;
  int count;
} List;

/* Function Prototypes */
void InitList(List *list);
void AddItem(List *list, long id, long flags);
void AddItemName(List *list, char *string, long flags);
int FindItem(List *list, long id);
int FindNameItem(List *list, char *string);
int UpdateItemName(List *list, long id, char *string);
int UpdateItemFlags(List *list, long id, long flags);
int UpdateNameItem(List *list, char *string, long id, long flags);
void FreeItem(Item *ptr);
int DeleteItem(List *list,long id);
void FreeList(List *list);
void PrintList(List *list);
char *ItemName(List *list, int n);
long ItemFlags(List *list, long id );
long ItemIndexFlags(List *list, int i);
long XorFlags(List *list, int n, long value);
int ItemCount(List *list);
long ItemID(List *list, int n);
void CopyItem(List *dest,List *source,int n);
