/*
 * Goodies.c structures and exported functions
 */

#ifndef _H_Goodies
#define _H_Goodies

#define DEFAULT_MAIL_PATH  "/var/spool/mail/"
#define DEFAULT_BELL_VOLUME 20

/* Tip window types */
#define NO_TIP    (-1)
#define DATE_TIP  (-2)
#define MAIL_TIP  (-3)
#define START_TIP (-4)

typedef struct {
  int  x, y, w, h, tw, th, open, type;
  char *text;
  Window win;
} TipStruct;

void GoodiesParseConfig(char *tline, char *Module);
void InitGoodies();
void DrawGoodies();
int MouseInClock(int x, int y);
int MouseInMail(int x, int y);
void CreateDateWindow();
void CreateMailTipWindow();
void PopupTipWindow(int px, int py, char *text);
void CreateTipWindow(int x, int y, int w, int h);
void RedrawTipWindow();
void DestroyTipWindow();
void ShowTipWindow(int open);
void HandleMouseClick(XEvent event);
void HandleMailClick(XEvent event);

#endif
