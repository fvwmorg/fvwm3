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

#include "../types.h"
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

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

/* Constante pour les type de message envoie entre objets */
/* <0 valeur reserve pour les messages internes */
/* >0 message envoie par l'utilisateur */
#define SingleClic -1
#define DoubleClic -2

/* if the time in X ms between a Button Press and ButtonRelease is < to *
 * MENU_DRAG_TIME, then we consider the mouse click as simple click and
 * not "menu drag" (use in Menu.c and PopupMenu.c */
#define MENU_DRAG_TIME 300
#define GRAB_EVMASK (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask)

void MyDrawString(Display *dpy, struct XObj *xobj, Window win, int x, int y,
		  char *str, unsigned long ForeC,unsigned long HiC,
		  unsigned long BackC, int WithRelief);

int GetXTextPosition(struct XObj *xobj, int obj_width, int str_len,
		     int left_offset, int center_offset, int right_offset);

char* GetMenuTitle(char *str,int id);

void DrawPMenu(struct XObj *xobj,Window WinPop,int h,int StrtOpt);

void UnselectMenu(struct XObj *xobj,Window WinPop,int hOpt,int value,
		  unsigned int width, int asc, int start);
void SelectMenu(struct XObj *xobj,Window WinPop,int hOpt,int value);

int CountOption(char *str);

void DrawIconStr(int offset, struct XObj *xobj, int DoRedraw,
		 int l_offset, int c_offset, int r_offset);

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
