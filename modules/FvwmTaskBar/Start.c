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

#include "libs/fvwmlib.h"
#include "libs/Flocale.h"
#include "libs/Picture.h"
#include "ButtonArray.h"
#include "Mallocs.h"

extern Display *dpy;
extern Window Root, win;
extern FlocaleFont *FButtonFont;
extern int Clength;
extern char *ImagePath;
extern int ColorLimit;

Button *StartButton;
int StartButtonWidth, StartButtonHeight;
char *StartName     = NULL,
     *StartCommand  = NULL,
     *StartPopup    = NULL,
     *StartIconName = NULL;


static char *startopts[] =
{
  "StartName",
  "StartMenu",
  "StartIcon",
  "StartCommand",
  NULL
};

Bool StartButtonParseConfig(char *tline)
{
  char *rest;
  char *option;
  int i;

  option = tline + Clength;
  i = GetTokenIndex(option, startopts, -1, &rest);
  while (*rest && *rest != '\n' && isspace(*rest))
    rest++;
  switch(i)
  {
  case 0: /* StartName */
    CopyString(&StartName, rest);
    break;
  case 1: /* StartMenu */
    CopyString(&StartPopup, rest);
    break;
  case 2: /* StartIcon */
    CopyString(&StartIconName, rest);
    break;
  case 3: /* StartCommand */
    CopyString(&StartCommand, rest);
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

  fpa.mask = FPAM_NO_ALLOC_PIXELS;
  /* some defaults */
  if (StartName  == NULL)
    UpdateString(&StartName, "Start");
  if (StartIconName == NULL)
    UpdateString(&StartIconName, "mini-start.xpm");

  p = PGetFvwmPicture(dpy, win, ImagePath, StartIconName, fpa);

  StartButton = (Button *)ButtonNew(StartName, p, BUTTON_UP,0);
  if (p != NULL) pw = p->width+3; else pw = 0;
  StartButtonWidth = FlocaleTextWidth(FButtonFont, StartName, strlen(StartName))
    + pw + 14;
  StartButtonHeight = height;
}

int StartButtonUpdate(const char *title, int state)
{
#if 0
  if (title != NULL)
    ConsoleMessage("Updating StartTitle not supported yet...\n");
  ButtonUpdate(StartButton, title, state);
#else
  ButtonUpdate(StartButton, NULL, state);
#endif
  return StartButton->needsupdate;
}

void StartButtonDraw(int force)
{
  if (StartButton->needsupdate || force)
    ButtonDraw(StartButton, 0, 0, StartButtonWidth, StartButtonHeight);
}

int MouseInStartButton(int x, int y)
{
  return (x > 0 && x < StartButtonWidth &&
	  y > 0 && y < StartButtonHeight);
}
