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

/*
 * Start.c exported functions
 */

#ifndef START_H
#define START_H

typedef struct startAndLaunchButtonItem {
  struct startAndLaunchButtonItem *head, *tail;
  int index;
  Button *buttonItem;
  int width, height;
  Bool isStartButton; 
  char *buttonCommand;
  char *buttonStartCommand;
  char *buttonCaption;
  char *buttonIconFileName;
  char *buttonToolTip;
} StartAndLaunchButtonItem;

extern Bool StartButtonParseConfig(char *tline);
extern void StartButtonInit(int height);
extern void StartAndLaunchButtonItemInit(StartAndLaunchButtonItem *item);
extern void AddStartAndLaunchButtonItem(StartAndLaunchButtonItem *item);
extern int StartButtonUpdate(const char *title, int index, int state);
extern void StartButtonDraw(int force);
extern int  MouseInStartButton(int x, int y, int *whichButton, Bool *startButtonPressed);
extern void getButtonCommand(int whichButton, char *tmp);
#endif
