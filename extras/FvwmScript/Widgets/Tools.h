#include "../types.h"

extern char *Scrapt;
extern Atom propriete;
extern Atom type;
extern char *pixmapPath;
extern X11base *x11base;
extern int nbobj;
extern struct XObj *tabxobj[100];
extern int x_fd;

/* Constante pour les type de message envoie entre objets */
/* <0 valeur reserve pour les messages internes */
/* >0 message envoie par l'utilisateur */
#define SingleClic -1
#define DoubleClic -2

void DrawString(Display *dpy,GC gc,Window win,int x,int y,char *str,
		int strl,unsigned long ForeC,unsigned long HiC,
		unsigned long BackC,int WithRelief);

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
