/* Start ;-) button handling */

#include <X11/Xlib.h>

#include "../../libs/fvwmlib.h"
#include "ButtonArray.h"

extern Display *dpy;
extern Window Root, win;
extern XFontStruct *ButtonFont;
extern int Clength;
extern char *PixmapPath;
extern char *IconPath;
Button *StartButton;
int StartButtonWidth, StartButtonHeight;
char *StartName     = NULL,
     *StartPopup    = NULL,
     *StartIconName = NULL;


void StartButtonParseConfig(char *tline, char *Module)
{
  if(strncasecmp(tline,CatString3(Module,"StartName",""), Clength+9)==0)
    CopyString(&StartName,&tline[Clength+9]);
  else if(strncasecmp(tline,CatString3(Module,"StartMenu",""), Clength+9)==0)
    CopyString(&StartPopup,&tline[Clength+9]);
  else if(strncasecmp(tline,CatString3(Module,"StartIcon",""), Clength+9)==0)
    CopyString(&StartIconName,&tline[Clength+9]);
}

void StartButtonInit(int height)
{
  Picture *p = NULL;
  int pw;

  /* some defaults */
  if (StartName  == NULL)
    UpdateString(&StartName, "Start");
  if (StartPopup == NULL)
    UpdateString(&StartPopup, "StartMenu");
  if (StartIconName == NULL)
    UpdateString(&StartIconName, "mini-start.xpm");

  /** FIXME: what should the colour limit be?
      I put in -1, which apparently imposes NO limit.
  **/
  p = GetPicture(dpy, Root, IconPath, PixmapPath, StartIconName, -1);

  StartButton = (Button *)ButtonNew(StartName, p, BUTTON_UP);
  if (p != NULL) pw = p->width+3; else pw = 0;
  StartButtonWidth = XTextWidth(ButtonFont, StartName, strlen(StartName)) +
    pw + 14;
  StartButtonHeight = height;
}

void StartButtonUpdate(char *title, int state)
{
  if (title != NULL)
    ConsoleMessage("Updating StartTitle not supported yet...\n");
  ButtonUpdate(StartButton, title, state);
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
