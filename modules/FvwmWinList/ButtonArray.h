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
  Bool is_sticky;
  Bool skip;
  Bool is_iconified;
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
extern void UpdateArray(ButtonArray *array,int w);
extern int AddButton(ButtonArray *array, char *title, Picture *p,int up);
extern int UpdateButton(ButtonArray *array, int butnum, char *title, int up);
extern void UpdateButtonIconified(
    ButtonArray *array, int butnum, int iconified);
extern void RadioButton(ButtonArray *array, int butnum, int butnumpressed);
extern void ReorderButtons(ButtonArray *array, int butnum, int FlipFocus);
extern int UpdateButtonDeskFlags(ButtonArray *array, int butnum, long desk,
				 int is_sticky, int skip);
extern int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p);
extern int UpdateButtonSet(ButtonArray *array, int butnum, int set);
extern void RemoveButton(ButtonArray *array, int butnum);
extern Button *find_n(ButtonArray *array, int n);
extern void FreeButton(Button *ptr);
extern void FreeAllButtons(ButtonArray *array);
extern void DoButton(Button *ptr, int x, int y, int w, int h, Bool clear_bg);
extern void DrawButtonArray(ButtonArray *array, Bool all, Bool clear_bg);
extern void ExposeAllButtons(ButtonArray *array, XEvent *eventp);
extern void SwitchButton(ButtonArray *array,int butnum);
extern int WhichButton(ButtonArray *array,int x, int y);
extern char *ButtonName(ButtonArray *array, int butnum);
extern void PrintButtons(ButtonArray *array);
#ifdef MINI_ICONS
extern Picture *ButtonPicture(ButtonArray *array, int butnum);
#endif
extern int IsButtonVisible(Button *btn);
extern int IsButtonIndexVisible(ButtonArray *array, int butnum);
