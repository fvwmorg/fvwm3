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

extern Display *dpy;
extern int screen;
extern char *Scrapt;
extern Atom propriete;
extern Atom type;
extern char *imagePath;
extern X11base *x11base;
extern int nbobj;
extern struct XObj *tabxobj[100];
extern int x_fd;

/* Constante pour les type de message envoie entre objets */
/* <0 valeur reserve pour les messages internes */
/* >0 message envoie par l'utilisateur */
#define SingleClic -1
#define DoubleClic -2

#ifdef I18N_MB
void FakeDrawString(XFontSet FONTSET, Display *dpy,GC gc,Window win,int x,int y,char *str,
		int strl,unsigned long ForeC,unsigned long HiC,
		unsigned long BackC,int WithRelief);
#else
void DrawString(Display *dpy,GC gc,Window win,int x,int y,char *str,
		int strl,unsigned long ForeC,unsigned long HiC,
		unsigned long BackC,int WithRelief);
#endif

char* GetMenuTitle(char *str,int id);

void DrawPMenu(struct XObj *xobj,Window WinPop,int h,int StrtOpt);

void SelectMenu(struct XObj *xobj,Window WinPop,int hOpt,int newvalue,int Show);

int CountOption(char *str);

void DrawIconStr(int offset,struct XObj *xobj,int DoRedraw);

void DrawReliefRect(int x,int y,int width,int height,struct XObj *xobj,
		unsigned int LiC, unsigned int ShadC,unsigned int ForeC,int RectIn);

int GetAscFont(XFontStruct *xfont);

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
