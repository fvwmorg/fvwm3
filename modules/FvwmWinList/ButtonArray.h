/* FvwmWinList Module for Fvwm. 
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The functions in this header file that are the original work of Mike Finger.
 * 
 * No guarantees or warantees or anything are provided or implied in any way
 * whatsoever. Use this program at your own risk. Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 *
 *  Things to do:  use graphics contexts from fvwm
 */

#include <libs/Picture.h>

#define MAX_COLOUR_SETS 4

/* Struct definitions */
typedef struct button
{
  char *title;
  char *truncate_title;
  int up, needsupdate, tw, set, truncatewidth;
  struct button *next;
  Picture p;
  int reliefwidth;
  long desk;
} Button;

typedef struct
{
  int count;
  Button *head,*tail;
  int x,y,w,h,rw;
} ButtonArray;

/* Function Prototypes */
extern Button *ButtonNew(char *title, Picture *p, int up);
extern void InitArray(ButtonArray *array,int x,int y,int w,int h,int rw);
extern void UpdateArray(ButtonArray *array,int x,int y,int w, int h);
extern int AddButton(ButtonArray *array, char *title, Picture *p,int up);
extern int UpdateButton(ButtonArray *array, int butnum, char *title, int up);
extern void RadioButton(ButtonArray *array, int butnum);
extern void ReorderButtons(ButtonArray *array, int butnum, int FlipFocus);
extern int UpdateButtonDesk(ButtonArray *array, int butnum, long desk );
extern int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p);
extern int UpdateButtonSet(ButtonArray *array, int butnum, int set);
extern void RemoveButton(ButtonArray *array, int butnum);
extern Button *find_n(ButtonArray *array, int n);
extern void FreeButton(Button *ptr);
extern void FreeAllButtons(ButtonArray *array);
extern void DoButton(Button *ptr, int x, int y, int w, int h);
extern void DrawButtonArray(ButtonArray *array, int all);
extern void SwitchButton(ButtonArray *array,int butnum);
extern int WhichButton(ButtonArray *array,int x, int y);
extern void PrintButtons(ButtonArray *array);
