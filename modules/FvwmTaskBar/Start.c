/* -*-c-*- */
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

#include "config.h"

#include <X11/Xlib.h>
#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/Flocale.h"
#include "libs/Picture.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "ButtonArray.h"
#include "Mallocs.h"
#include "Start.h"

extern Display *dpy;
extern Window Root, win;
extern FlocaleFont *FButtonFont, *FSelButtonFont;
extern ModuleArgs *module;
extern char *ImagePath;

Button *StartButton;
Bool NoDefaultStartButton = False;
int StartAndLaunchButtonsWidth = 0;
int StartAndLaunchButtonsHeight = 0;
int WindowButtonsLeftMargin = 4;    /* default value is 4 */
int WindowButtonsRightMargin = 2;   /* default value is 2 */
int StartButtonRightMargin = 0;     /* default value is 0 */
int has_wb_left_margin = 0;
int has_wb_right_margin = 0;
Bool StartButtonOpensAboveTaskBar = False;
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
  "NoDefaultStartButton",
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
	int titleRecorded = 0, iconRecorded = 0;
	char *tokens[100];  /* This seems really big */
	StartAndLaunchButtonItem *tempPtr;
	int mouseButton;
	char **tmpStrPtr;

	option = tline + module->namelen+1;
	i = GetTokenIndex(option, startopts, -1, &rest);
	while (*rest && *rest != '\n' && isspace(*rest))
	{
		rest++;
	}
	switch(i)
	{
	case 0: /* StartName */
		if (First_Start_Button == NULL)
		{
			First_Start_Button = Last_Start_Button =
				(StartAndLaunchButtonItem*)safemalloc(
					sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(First_Start_Button);
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->isStartButton == False)
		{
			/* shortcut button has been declared before start
			 * button */
			tempPtr = (StartAndLaunchButtonItem*)safemalloc(
				sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(tempPtr);
			tempPtr->tail = First_Start_Button;
			First_Start_Button = tempPtr;
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->buttonCaption != NULL)
		{
			/* declarin caption twice, ignore */
			break;
		}
		CopyString(&(First_Start_Button->buttonCaption), rest);
		break;
	case 1: /* StartMenu */
		rest = ParseButtonOptions(rest, &mouseButton);
		if (First_Start_Button == NULL)
		{
			First_Start_Button = Last_Start_Button =
				(StartAndLaunchButtonItem*)safemalloc(
					sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(First_Start_Button);
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->isStartButton == False)
		{
			/* shortcut button has been declared before start
			 * button */
			tempPtr = (StartAndLaunchButtonItem*) safemalloc(
				sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(tempPtr);
			tempPtr->tail = First_Start_Button;
			First_Start_Button = tempPtr;
			First_Start_Button->isStartButton = True;
		}
		tmpStrPtr = (mouseButton ?
			     &(First_Start_Button->buttonCommands[mouseButton-1])
			     :
			     &(First_Start_Button->buttonCommand));
		if (*tmpStrPtr)
		{
			/* declaring command twice, ignore */
			break;
		}
		CopyString(tmpStrPtr, rest);
		break;
	case 2: /* StartIcon */
		if (First_Start_Button == NULL)
		{
			First_Start_Button = Last_Start_Button =
				(StartAndLaunchButtonItem*)safemalloc(
					sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(First_Start_Button);
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->isStartButton == False)
		{
			/* shortcut button has been declared before start
			 * button */
			tempPtr = (StartAndLaunchButtonItem*) safemalloc(
				sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(tempPtr);
			tempPtr->tail = First_Start_Button;
			First_Start_Button = tempPtr;
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->buttonIconFileName != NULL)
		{
			/* declaring icon twice, ignore */
			break;
		}
		CopyString(&(First_Start_Button->buttonIconFileName), rest);
		break;
	case 3: /* StartCommand */
		rest = ParseButtonOptions(rest, &mouseButton);
		if (First_Start_Button == NULL)
		{
			First_Start_Button = Last_Start_Button =
				(StartAndLaunchButtonItem*)safemalloc(
					sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(First_Start_Button);
			First_Start_Button->isStartButton = True;
		}
		else if (First_Start_Button->isStartButton == False)
		{
			/* shortcut button has been declared before start
			 * button */
			tempPtr = (StartAndLaunchButtonItem*)safemalloc(
				sizeof(StartAndLaunchButtonItem));
			StartAndLaunchButtonItemInit(tempPtr);
			tempPtr->tail = First_Start_Button;
			First_Start_Button = tempPtr;
			First_Start_Button->isStartButton = True;
		}
		tmpStrPtr =
			(mouseButton ?
			 &(First_Start_Button->buttonStartCommands[mouseButton-1]) :
			 &(First_Start_Button->buttonStartCommand));
		if (*tmpStrPtr)
		{
			/* declaring command twice, ignore */
			break;
		}
		CopyString(tmpStrPtr, rest);
		break;
	case 4: /* NoDefaultStartButton */
		NoDefaultStartButton = True;
		break;
	case 5: /* Button */
		if (Last_Start_Button == NULL)
		{
			First_Start_Button = Last_Start_Button =
				(StartAndLaunchButtonItem*)safemalloc(
					sizeof(StartAndLaunchButtonItem));
		}
		else
		{
			Last_Start_Button->tail =
				(StartAndLaunchButtonItem*) safemalloc(
					sizeof(StartAndLaunchButtonItem));
			Last_Start_Button = Last_Start_Button->tail;
		}

		StartAndLaunchButtonItemInit(Last_Start_Button);
		j=0;
		titleRecorded = iconRecorded = 0;
		{
			char *strtok_ptr = 0;

			tokens[j++] = strtok_r(rest, ",", &strtok_ptr);
			while((tokens[j++] = strtok_r(NULL, ",", &strtok_ptr)))
			{
				while(isspace(*(tokens[j-1])))
				{
					tokens[j-1]+=sizeof(char);
				}
			}
		}
		j--;

		for(k=0;k<j;k++)
		{
			if (strncasecmp(tokens[k], "Title", 5)==0)
			{
				tokens[j+1] = tokens[k] + ((sizeof(char))*5);
				while(*(tokens[j+1])==' ')
					tokens[j+1]+=sizeof(char);
				CopyString(
					&(Last_Start_Button->buttonCaption),
					tokens[j+1]);
				titleRecorded=1;
			}
			else if (strncasecmp(tokens[k], "Icon", 4)==0)
			{
				tokens[j+1] = tokens[k] + ((sizeof(char))*4);
				while(*(tokens[j+1])==' ')
					tokens[j+1]+=sizeof(char);
				CopyString(
					&(Last_Start_Button->buttonIconFileName),
					tokens[j+1] );
				iconRecorded = 1;
			}
			else if (strncasecmp(tokens[k], "Action", 6)==0)
			{
				rest = tokens[k] + ((sizeof(char))*6);
				rest = ParseButtonOptions(rest, &mouseButton);
				tokens[j+1] = rest;
				if (mouseButton)
				{
					tmpStrPtr =
						&(Last_Start_Button->
						  buttonStartCommands
						  [mouseButton-1]);
				}
				else
				{
					tmpStrPtr =
						&(Last_Start_Button->
						  buttonStartCommand);
				}
				if (!(*tmpStrPtr))
				{
					/* don't let them set the same action
					 * twice */
					CopyString(tmpStrPtr, tokens[j+1]);
				}
			}
		}
		if (titleRecorded==0)
		{
			CopyString(&(Last_Start_Button->buttonCaption), "\0");
		}
		if (iconRecorded==0)
		{
			CopyString(
				&(Last_Start_Button->buttonIconFileName),
				"\0");
		}
		break;

	case 6: /* WindowButtonsLeftMargin */
		if(atoi(rest)>=0)
		{
			WindowButtonsLeftMargin = atoi(rest);
			has_wb_left_margin = 1;
		}
		break;
	case 7: /* WindowButtonsRightMargin */
		if(atoi(rest)>=0)
		{
			WindowButtonsRightMargin = atoi(rest);
			has_wb_right_margin = 1;
		}
		break;
	case 8: /* StartButtonRightMargin */
		if(atoi(rest)>=0)
		{
			StartButtonRightMargin = atoi(rest);
		}
		break;
	default:
		/* unknown option */
		return False;
	} /* switch */

	return True;
}

/* Parse and set options for this taskbar button (start or launcher). This
 * will check for a string of the form (<opt1>,<opt2>...), similar to options
 * for an FvwmButtons button. The return value is a pointer to the rest of the
 * config line.
 *
 * Currently this sets just one option, mouseButton. If no "Mouse <n>" option
 * is found, mouseButton will be set to 0.
 */
char *ParseButtonOptions(char *pos, int *mouseButton)
{
  char *token = NULL;
  char *rest;
  int i;
  static char *buttonOptions[] = {"Mouse", NULL};

  *mouseButton = 0;
  while (*pos && isspace(*pos))
    pos++;
  if (*pos != '(')
    return pos;
  pos++;
  while (*pos && isspace(*pos))
    pos++;

  while (*pos && *pos != ')')
  {
    pos = GetNextToken(pos, &token);
    if (!token)
        break;
    i = GetTokenIndex(token, buttonOptions, 0, NULL);
    switch (i)
    {
    case 0:   /* Mouse */
      *mouseButton = strtol(pos, &rest, 10);
      pos = rest;
      if (*mouseButton < 1 || *mouseButton > NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
      {
        fprintf(stderr,"%s: Invalid mouse button %d", module->name,
                *mouseButton);
        *mouseButton = 0;
      }
      break;

    default:
      fprintf(stderr,"%s: Invalid taskbar button option '%s'", module->name,
              token);
    }
    while (*pos && *pos != ',' && *pos != ')')
      pos++;
    if (*pos == ',') {
      pos++;
      while (*pos && *pos != ',' && *pos != ')')
        pos++;
    }
    free(token);
  }

  if (*pos)
    pos++;
  while (*pos && isspace(*pos))
    pos++;
  return pos;
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
  if (First_Start_Button == NULL && !NoDefaultStartButton)
  {
    StartButtonParseConfig("*FvwmTaskBarStartName Start");
    StartButtonParseConfig("*FvwmTaskBarStartMenu RootMenu");
    StartButtonParseConfig("*FvwmTaskBarStartIcon mini.start.xpm");
  }
  /* some defaults */
  if (First_Start_Button && First_Start_Button->isStartButton == True)
  {
    if (First_Start_Button->buttonCaption == NULL)
      UpdateString(&(First_Start_Button->buttonCaption), "Start");
    if (First_Start_Button->buttonIconFileName == NULL)
      UpdateString(&(First_Start_Button->buttonIconFileName), "mini-start.xpm");
  }

  tempPtr = First_Start_Button;

  while(tempPtr != NULL)
  {
	  p = PGetFvwmPicture(
		  dpy, win, ImagePath, tempPtr->buttonIconFileName, fpa);
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

    tempPtr->buttonItem = (Button *)ButtonNew(
	    tempPtr->buttonCaption, p, BUTTON_UP,0);
    if (tempPtr->isStartButton)
    {
	    StartButton = tempPtr->buttonItem;
	    tempPtr->width = FlocaleTextWidth(
		    FSelButtonFont, tempPtr->buttonCaption,
		    strlen(tempPtr->buttonCaption)) + pw;
    }
    else
    {
	    tempPtr->width = FlocaleTextWidth(
		    FButtonFont, tempPtr->buttonCaption,
		    strlen(tempPtr->buttonCaption)) + pw;
    }
    tempPtr->height = height;
    StartAndLaunchButtonsWidth += tempPtr->width;
    tempPtr=tempPtr->tail;
    PFreeFvwmPictureData(p); /* should not destroy of course */
  }
  if (First_Start_Button)
  {
	  StartAndLaunchButtonsWidth += StartButtonRightMargin;
	  First_Start_Button->height = height;
	  StartAndLaunchButtonsHeight = First_Start_Button->height;
  }
  else
  {
    StartAndLaunchButtonsWidth = 0;
    StartButtonRightMargin = 0;
    if (has_wb_left_margin == 0)
    {
      WindowButtonsLeftMargin = 0;
    }
    if (has_wb_right_margin == 0)
    {
      WindowButtonsRightMargin = 0;
    }
  }
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

  if (!First_Start_Button)
  {
	  return 0;
  }

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


void StartButtonDraw(int force, XEvent *evp)
{
	int tempsum, j, i = 0;
	StartAndLaunchButtonItem *tempPtr = First_Start_Button;
	StartAndLaunchButtonItem *tempPtr2 = First_Start_Button;

	if (!First_Start_Button)
	{
		return;
	}

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
				ButtonDraw(
					tempPtr->buttonItem,
					tempsum+StartButtonRightMargin, 0,
					tempPtr->width,
					First_Start_Button->height, evp);
			else
				ButtonDraw(
					tempPtr->buttonItem,
					tempsum, 0, tempPtr->width,
					First_Start_Button->height, evp);
		}
		tempPtr = tempPtr->tail;
		i++;
	}
}

/* Returns 1 if in the start or a minibutton, 0 if not; index of button
 * pressed put in startButtonPressed */
int MouseInStartButton(int x, int y, int *whichButton,
		       Bool *startButtonPressed, int *button_x)
{
  int i = 0;
  int tempsum = 0;

  StartAndLaunchButtonItem *tempPtr = First_Start_Button;
  *startButtonPressed = False;

  while(tempPtr != NULL)
    {
      if (x >= tempsum && x < tempsum+tempPtr->width && y > 0 && y < First_Start_Button->height)
      {
	*whichButton = i;
	if (button_x)
	{
		*button_x = tempsum;
	}
	if(tempPtr->isStartButton)
	  *startButtonPressed = True;
	return 1;
      }
      tempsum += tempPtr->width;
      tempPtr = tempPtr->tail;
      i++;
    }
  *whichButton = -1;
  return 0;
}

void getButtonCommand(int whichButton, char *tmp, int mouseButton)
{
  int i=0;
  StartAndLaunchButtonItem *tempPtr = First_Start_Button;

  if (!First_Start_Button)
  {
	  return;
  }

  for(i=0; i<whichButton; i++)
    tempPtr = tempPtr->tail;
  mouseButton--;

  if (mouseButton < NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
      tempPtr->buttonCommands[mouseButton])
    sprintf(tmp, "Popup %s rectangle $widthx$height+$left+$top 0 -100m",
            tempPtr->buttonCommands[mouseButton]);
  else if (mouseButton < NUMBER_OF_EXTENDED_MOUSE_BUTTONS &&
           tempPtr->buttonStartCommands[mouseButton])
    sprintf(tmp, "%s", tempPtr->buttonStartCommands[mouseButton]);
  else if (tempPtr->buttonCommand)
    sprintf(tmp, "Popup %s rectangle $widthx$height+$left+$top 0 -100m",
            tempPtr->buttonCommand);
  else if (tempPtr->buttonStartCommand)
    sprintf(tmp, "%s", tempPtr->buttonStartCommand);
  else if (tempPtr->isStartButton)
    sprintf(tmp, "Popup StartMenu");
  else
    sprintf(tmp, "Nop");
}

void StartAndLaunchButtonItemInit(StartAndLaunchButtonItem *item)
{
  int i;

  item->head = NULL;
  item->tail = NULL;
  item->index = 0;
  item->width = 0;
  item->height = 0;
  item->isStartButton = False;
  item->buttonItem = NULL;
  item->buttonCommand = NULL;
  item->buttonStartCommand = NULL;
  item->buttonCaption = NULL;
  item->buttonIconFileName = NULL;
  item->buttonToolTip = NULL;
  for (i=0; i < NUMBER_OF_EXTENDED_MOUSE_BUTTONS; i++)
  {
    item->buttonCommands[i] = NULL;
    item->buttonStartCommands[i] = NULL;
  }
}
