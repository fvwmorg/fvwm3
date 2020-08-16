/* -*-c-*- */
#include "../types.h"
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#include "libs/Flocale.h"

extern Display *dpy;
extern int screen;
extern Window Root;
extern char *Scrapt;
extern Atom propriete;
extern Atom type;
extern char *imagePath;
extern X11base *x11base;
extern int nbobj;
extern struct XObj *tabxobj[1000];
extern int x_fd;
extern char *ScriptName;
extern FlocaleWinString *FwinString;

/* Constant for messages types sent between objects */
/* <0 value for internal messages */
/* >0 messages sent by user */
#define SingleClic -1
#define DoubleClic -2

/* if the time in X ms between a Button Press and ButtonRelease is < to *
 * MENU_DRAG_TIME, then we consider the mouse click as simple click and
 * not "menu drag" (use in Menu.c and PopupMenu.c */
#define MENU_DRAG_TIME 300
#define GRAB_EVMASK (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask)

/* helper functions */
/* get byte offset corresponding to character offset, including 
   bounds check to represent end of text */
int getByteOffsetBoundsCheck(FlocaleFont *flf, char *str, int offset);
/* opposite of the above, return character offset */
int getCharOffsetBoundsCheck(FlocaleFont *flf, char *str, int offset);

void MyDrawString(
	Display *dpy, struct XObj *xobj, Window win, int x, int y,
	char *str, unsigned long ForeC,unsigned long HiC,
	unsigned long BackC, int WithRelief, XRectangle *clip, XEvent *evp);

int GetXTextPosition(struct XObj *xobj, int obj_width, int str_len,
		     int left_offset, int center_offset, int right_offset);

char* GetMenuTitle(char *str,int id);

void DrawPMenu(struct XObj *xobj,Window WinPop,int h,int StrtOpt);

void UnselectMenu(struct XObj *xobj,Window WinPop,int hOpt,int value,
		  unsigned int width, int asc, int start);
void SelectMenu(struct XObj *xobj,Window WinPop,int hOpt,int value);

int CountOption(char *str);

void DrawIconStr(int offset, struct XObj *xobj, int DoRedraw,
		 int l_offset, int c_offset, int r_offset,
		 XRectangle *str_clip, XRectangle *icon_clip, XEvent *evp);

void DrawReliefRect(int x,int y,int width,int height,struct XObj *xobj,
		unsigned int LiC, unsigned int ShadC);

int InsertText(struct XObj *xobj,char *str,int SizeStr);

char *GetText(struct XObj *xobj,int End);

void SelectOneTextField(struct XObj *xobj);

void DrawArrowN(struct XObj *xobj,int x,int y,int Press);

void DrawArrowS(struct XObj *xobj,int x,int y,int Press);

void DrawArrowE(struct XObj *xobj,int x,int y,int Press);

void DrawArrowW(struct XObj *xobj,int x,int y,int Press);

int PtInRect(XPoint pt,XRectangle rect);

void Wait(int t);

int IsItDoubleClic(struct XObj *xobj);
