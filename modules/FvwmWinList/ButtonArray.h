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
 *  Things to do:  Convert to C++  (In Progress)
 */

/* Struct definitions */
typedef struct button
{
  char *title;
  char *truncate_title; /* valid only if truncatewidth > 0 */
  int up, needsupdate, tw, set, truncatewidth;
  struct button *next;
  Picture p;
  long desk;
} Button;

typedef struct
{
  int count;
  Button *head,*tail;
  int x,y,w,h;
} ButtonArray;

#define MAX_COLOUR_SETS 3

/* Function Prototypes */
Button *ButtonNew(char *title, Picture *p, int up);
void InitArray(ButtonArray *array,int x,int y,int w,int h);
void UpdateArray(ButtonArray *array,int x,int y,int w, int h);
int AddButton(ButtonArray *array, char *title, Picture *p,int up);
int UpdateButton(ButtonArray *array, int butnum, char *title, int up);
int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p);
int UpdateButtonSet(ButtonArray *array, int butnum, int set);
void RemoveButton(ButtonArray *array, int butnum);
Button *find_n(ButtonArray *array, int n);
void FreeButton(Button *ptr);
void FreeAllButtons(ButtonArray *array);
void DoButton(Button *ptr, int x, int y, int w, int h);
void DrawButtonArray(ButtonArray *array, int all);
void SwitchButton(ButtonArray *array,int butnum);
int WhichButton(ButtonArray *array,int x, int y);
void PrintButtons(ButtonArray *array);
