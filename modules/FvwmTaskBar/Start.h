/* -*-c-*- */
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
  char *buttonCommands[NUMBER_OF_EXTENDED_MOUSE_BUTTONS];
  char *buttonStartCommands[NUMBER_OF_EXTENDED_MOUSE_BUTTONS];
  char *buttonCaption;
  char *buttonIconFileName;
  char *buttonToolTip;
} StartAndLaunchButtonItem;

extern Bool StartButtonParseConfig(char *tline);
extern char *ParseButtonOptions(char *pos, int *mouseButton);
extern void StartButtonInit(int height);
extern void StartAndLaunchButtonItemInit(StartAndLaunchButtonItem *item);
extern void AddStartAndLaunchButtonItem(StartAndLaunchButtonItem *item);
extern int StartButtonUpdate(const char *title, int index, int state);
extern void StartButtonDraw(int force, XEvent *evp);
extern int  MouseInStartButton(int x, int y, int *whichButton, Bool *startButtonPressed);
extern void getButtonCommand(int whichButton, char *tmp, int mouseButton);
#endif
