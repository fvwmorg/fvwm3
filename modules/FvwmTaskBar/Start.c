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

/* Start ;-) button handling */

#include <X11/Xlib.h>
#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/Flocale.h"
#include "libs/Picture.h"
#include "ButtonArray.h"
#include "Mallocs.h"
#include "Start.h"

extern Display *dpy;
extern Window Root, win;
extern FlocaleFont *FButtonFont;
extern int Clength;
extern char *ImagePath;
extern int ColorLimit;

Button *StartButton;
int StartAndLaunchButtonsWidth = 0;
int StartAndLaunchButtonsHeight = 0;
int WindowButtonsLeftMargin = 4;    /* default value is 4 */
int WindowButtonsRightMargin = 2;   /* default value is 2 */
int StartButtonRightMargin = 0;     /* default value is 0 */
Bool StartButtonOpensAboveTaskBar = FALSE;
char *StartName     = NULL,
     *StartCommand  = NULL,
     *StartPopup    = NULL,
     *StartIconName = NULL;

StartAndLaunchButtonItem *First_Start_Button = NULL;
StartAndLaunchButtonItem *Last_Start_Button = NULL;

static char *startopts[] =
{
  "StartName",
  "StartMenu",
  "StartIcon",
  "StartCommand",
  "Button",
  "WindowButtonsLeftMargin",
  "WindowButtonsRightMargin",
  "StartButtonRightMargin",
  NULL
};

Bool StartButtonParseConfig(char *tline)
{
  char *rest;
  char *option;
  int i, j, k;
  int titleRecorded = 0, iconRecorded = 0, actionRecorded = 0;
  char *tokens[100];  /* This seems really big */
  char *strtok_ptr;
  StartAndLaunchButtonItem *tempPtr;

  option = tline + Clength;
  i = GetTokenIndex(option, startopts, -1, &rest);
  while (*rest && *rest != '\n' && isspace(*rest))
    rest++;
  switch(i)
  {
  case 0: /* StartName */
    if (First_Start_Button == NULL)
    {
      First_Start_Button = Last_Start_Button = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;
    }
    else if (First_Start_Button->isStartButton == FALSE)
    {
      /* shortcut button has been declared before start button */
      tempPtr = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      tempPtr->tail = First_Start_Button;
      First_Start_Button = tempPtr;
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;      
    }
    else if (First_Start_Button->buttonCaption != NULL)
    {
      /* declarin caption twice, ignore */
      break;
    }
    First_Start_Button->buttonCaption = safemalloc(strlen(rest) * sizeof(char));
    CopyString(&(First_Start_Button->buttonCaption), rest);    
    break;
  case 1: /* StartMenu */
    if (First_Start_Button == NULL)
    {
      First_Start_Button = Last_Start_Button = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;
    }
    else if (First_Start_Button->isStartButton == FALSE)
    {
      /* shortcut button has been declared before start button */
      tempPtr = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      tempPtr->tail = First_Start_Button;
      First_Start_Button = tempPtr;
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;      
    }
    else if (First_Start_Button->buttonCommand  != NULL)
    {
      /* declaring command twice, ignore */
      break;
    }
    First_Start_Button->buttonCommand = safemalloc(strlen(rest) * sizeof(char));
    CopyString(&(First_Start_Button->buttonCommand), rest);   
    break;
  case 2: /* StartIcon */
    if (First_Start_Button == NULL)
    {
      First_Start_Button = Last_Start_Button = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;
    }
    else if (First_Start_Button->isStartButton == FALSE)
    {
      /* shortcut button has been declared before start button */
      tempPtr = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      tempPtr->tail = First_Start_Button;
      First_Start_Button = tempPtr;
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;      
    }
    else if (First_Start_Button->buttonIconFileName != NULL)
    {
      /* declaring icon twice, ignore */
      break;
    }
    First_Start_Button->buttonIconFileName = safemalloc(strlen(rest) * sizeof(char));
    CopyString(&(First_Start_Button->buttonIconFileName), rest);   
   break;
  case 3: /* StartCommand */
    if (First_Start_Button == NULL)
    {
      First_Start_Button = Last_Start_Button = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;
    }
    else if (First_Start_Button->isStartButton == FALSE)
    {
      /* shortcut button has been declared before start button */
      tempPtr = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      tempPtr->tail = First_Start_Button;
      First_Start_Button = tempPtr;
      StartAndLaunchButtonItemInit(First_Start_Button);
      First_Start_Button->isStartButton = TRUE;      
    }
    else if (First_Start_Button->buttonIconFileName != NULL)
    {
      /* declaring icon twice, ignore */
      break;
    }
    First_Start_Button->buttonStartCommand = safemalloc(strlen(rest) * sizeof(char));
    CopyString(&(First_Start_Button->buttonStartCommand), rest);   
    break;
  case 4:
    if (Last_Start_Button == NULL) 
      First_Start_Button = Last_Start_Button = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
    else
    {  
      Last_Start_Button->tail = (StartAndLaunchButtonItem*) safemalloc(sizeof(StartAndLaunchButtonItem));
      Last_Start_Button = Last_Start_Button->tail;
    }

    StartAndLaunchButtonItemInit(Last_Start_Button);
    j=0;
    titleRecorded = iconRecorded = actionRecorded = 0;
    tokens[j++] = strtok_r(rest, ",", &strtok_ptr);
    while((tokens[j++] = strtok_r(NULL, ",", &strtok_ptr)))
      while(*(tokens[j-1])==' ')
	tokens[j-1]+=sizeof(char);
    j--;

    for(k=0;k<j;k++)
    {
      if (strncmp(tokens[k], "Title", 5)==0)
      {
	tokens[j+1] = tokens[k] + ((sizeof(char))*5);
	while(*(tokens[j+1])==' ')
	  tokens[j+1]+=sizeof(char);
	Last_Start_Button->buttonCaption = (char *) safemalloc(sizeof(char) * (strlen(tokens[j+1])));
        CopyString(&(Last_Start_Button->buttonCaption), tokens[j+1]);
	titleRecorded=1;
      }	
      else if (strncmp(tokens[k], "Icon", 4)==0)
      {
	tokens[j+1] = tokens[k] + ((sizeof(char))*4);
	while(*(tokens[j+1])==' ')
	  tokens[j+1]+=sizeof(char);
	Last_Start_Button->buttonIconFileName = (char *) safemalloc(sizeof(char) * (strlen(tokens[j+1])));
	CopyString(&(Last_Start_Button->buttonIconFileName),tokens[j+1] );
	iconRecorded = 1;
      }
      else if (strncmp(tokens[k], "Action", 6)==0)
      {
	tokens[j+1] = tokens[k] + ((sizeof(char))*6);
	while(*(tokens[j+1])==' ')
	  tokens[j+1]+=sizeof(char);
	Last_Start_Button->buttonCommand = (char *) safemalloc(sizeof(char) * (strlen(tokens[j+1])));
	CopyString(&(Last_Start_Button->buttonCommand), tokens[j+1]);
	actionRecorded = 1;
      }
    }
    if (titleRecorded==0)
      CopyString(&(Last_Start_Button->buttonCaption), "\0");
    if (iconRecorded==0)
      CopyString(&(Last_Start_Button->buttonIconFileName), "\0");
    if (actionRecorded==0)
      CopyString(&(Last_Start_Button->buttonCommand), "\0");
    break;

  case 5: /* WindowButtonsLeftMargin */
    if(atoi(rest)>=0)
      WindowButtonsLeftMargin = atoi(rest);
    break;
  case 6: /* WindowButtonsRightMargin */
    if(atoi(rest)>=0)
      WindowButtonsRightMargin = atoi(rest);
    break;
  case 7: /* StartButtonRightMargin */
    if(atoi(rest)>=0)
      StartButtonRightMargin = atoi(rest);
    break;
  default:
    /* unknown option */
    return False;
  } /* switch */

  return True;
}

void StartButtonInit(int height)
{
  FvwmPicture *p = NULL;
  int pw;
  FvwmPictureAttributes fpa;
  StartAndLaunchButtonItem *tempPtr;

  fpa.mask = FPAM_NO_ALLOC_PIXELS;

  /* if no start button params were specified, trick the program into
   * thinking that they were */
  if (First_Start_Button == NULL)
  {
    StartButtonParseConfig("*FvwmTaskBarStartName Start");
    StartButtonParseConfig("*FvwmTaskBarStartMenu RootMenu");
    StartButtonParseConfig("*FvwmTaskBarStartIcon mini.start.xpm");
  }
  /* some defaults */
  if (First_Start_Button->isStartButton == TRUE)
  {
    if (First_Start_Button->buttonCaption == NULL)
      UpdateString(&(First_Start_Button->buttonCaption), "Start");
    if (First_Start_Button->buttonIconFileName == NULL)
      UpdateString(&(First_Start_Button->buttonIconFileName), "mini-start.xpm");
  }

  tempPtr = First_Start_Button;

  while(tempPtr != NULL)
  {
    p = PGetFvwmPicture(dpy, win, ImagePath, tempPtr->buttonIconFileName, fpa);

    if (p != NULL && strlen(tempPtr->buttonCaption) != 0)
    {
      /* icon and title */
      pw = p->width + 12;
    }
    else if (p != NULL)
    {
      /* just icon */
      pw = p->width + 8; 
    }
    else
    {
      /* just title */
      pw = 10;
    }

    tempPtr->buttonItem = (Button *)ButtonNew(tempPtr->buttonCaption, p, BUTTON_UP,0);
    tempPtr->width = FlocaleTextWidth(FButtonFont, tempPtr->buttonCaption, strlen(tempPtr->buttonCaption)) + pw; 
    tempPtr->height = height;
    StartAndLaunchButtonsWidth += tempPtr->width;
    tempPtr=tempPtr->tail;
  }
  StartAndLaunchButtonsWidth += StartButtonRightMargin;
  First_Start_Button->height = height;
  StartAndLaunchButtonsHeight = First_Start_Button->height;
}

int StartButtonUpdate(const char *title, int index, int state)
{
  int i=0;
  StartAndLaunchButtonItem *tempPtr = First_Start_Button;
#if 0
  if (title != NULL)
    ConsoleMessage("Updating StartTitle not supported yet...\n");
  if(index != -1)
  {
    for(i=0; i<index; i++)
      tempPtr = tempPtr->tail;      
    ButtonUpdate(tempPtr->buttonItem, title, state);
  }
  else
    while(tempPtr != NULL)
    {
      ButtonUpdate(tempPtr->buttonItem, title, state);
      tempPtr = tempPtr->tail;
    }

#else
  if(index != -1)
  {
    for(i=0; i<index; i++)
      tempPtr = tempPtr->tail;      
    ButtonUpdate(tempPtr->buttonItem, title, state);
  }
  else
    while(tempPtr != NULL)
    {
      ButtonUpdate(tempPtr->buttonItem, title, state);
      tempPtr = tempPtr->tail;
    }

#endif
  tempPtr = First_Start_Button;
  while(tempPtr != NULL)
  {
    if (tempPtr->buttonItem->needsupdate)
      return 1;
    tempPtr = tempPtr->tail;    
  }
  return 0;
}


void StartButtonDraw(int force)
{
  int tempsum, j, i = 0;
  StartAndLaunchButtonItem *tempPtr = First_Start_Button;
  StartAndLaunchButtonItem *tempPtr2 = First_Start_Button;

  while(tempPtr != NULL)
  {
    if(tempPtr->buttonItem->needsupdate || force)
    {
      tempsum = 0;
      j=0;
      tempPtr2 = First_Start_Button;
      while((tempPtr2 != NULL) && (j<i))
      {
	tempsum+=tempPtr2->width;
	tempPtr2 = tempPtr2->tail;	
	j++;
      }
      if (!(tempPtr->isStartButton))
	ButtonDraw(tempPtr->buttonItem, tempsum+StartButtonRightMargin, 0, tempPtr->width, First_Start_Button->height);
      else
	ButtonDraw(tempPtr->buttonItem, tempsum, 0, tempPtr->width, First_Start_Button->height);
    }
    tempPtr = tempPtr->tail;
    i++;
  }
}

/* Returns 1 if in the start or a minibutton, 0 if not; index of button
 * pressed put in startButtonPressed */
int MouseInStartButton(int x, int y, int *whichButton, Bool *startButtonPressed)
{
  int i=0, j=0;
  int tempsum = 0;

  StartAndLaunchButtonItem *tempPtr = First_Start_Button;
  StartAndLaunchButtonItem *tempPtr2 = First_Start_Button;
  *startButtonPressed = FALSE;

  while(tempPtr != NULL)
    {
      tempsum = 0;
      j = 0;
      tempPtr2 = First_Start_Button;
      while((tempPtr2 != NULL) && (j<i))
      {
	tempsum+=tempPtr2->width;
	tempPtr2 = tempPtr2->tail;	
	j++;
      }     
      if (x >= tempsum && x < tempsum+tempPtr->width && y > 0 && y < First_Start_Button->height)
      {
	*whichButton = i;
	if(tempPtr->isStartButton)
	  *startButtonPressed = TRUE;	  
	return 1;
      }
      tempPtr = tempPtr->tail;
      i++;
    }
  *whichButton = -1;
  return 0;
}

void getButtonCommand(int whichButton, char *tmp)
{
  int i=0;
  StartAndLaunchButtonItem *tempPtr = First_Start_Button;

  for(i=0; i<whichButton; i++)
    tempPtr = tempPtr->tail;

  if(tempPtr->isStartButton)
      if (tempPtr->buttonCommand != NULL)
	sprintf(tmp,"Popup %s rectangle $widthx$height+$left+$top 0 -100m", tempPtr->buttonCommand);
      else if (tempPtr->buttonStartCommand != NULL)
	sprintf(tmp,"%s", tempPtr->buttonStartCommand);
      else
	sprintf(tmp,"Popup StartMenu");
  else
    sprintf(tmp,"%s", tempPtr->buttonCommand);
}

void StartAndLaunchButtonItemInit(StartAndLaunchButtonItem *item)
{
  item->head = NULL;
  item->tail = NULL;
  item->index = 0;
  item->width = 0;
  item->height = 0;
  item->isStartButton = FALSE;
  item->buttonItem = NULL;
  item->buttonCommand = NULL;
  item->buttonStartCommand = NULL;
  item->buttonCaption = NULL;
  item->buttonIconFileName = NULL;  
  item->buttonToolTip = NULL;
}
