/*************************************************************************
 *
 * Subroutine Prototypes
 *
 *************************************************************************/
#include "../../libs/fvwmlib.h"
struct icon_info;
Bool ExecIconBoxFunction(char *msg);
extern void   CreateWindow(void);
extern Pixel  GetColor(char *name);
extern Pixel  GetHilite(Pixel background);
extern Pixel  GetShadow(Pixel background);
extern void   nocolor(char *a, char *b);
extern void   RedrawWindow(void);
extern void   match_string(char *tline);
extern void   Loop(void);
extern void   ParseOptions(void);
extern char   *safemalloc(int length);
extern int    My_XNextEvent(Display *dpy, XEvent *event);
extern void   CopyString(char **dest, char *source);
extern void   RelieveWindow(Window win,int x,int y,int w,int h,GC rGC,GC sGC);
extern void   SendFvwmPipe(int *,char *text, unsigned long window);
extern void   DeadPipe(int nonsense);
extern void   CreateIconWindow(struct icon_info *item);
extern void   ConfigureIconWindow(struct icon_info *item);
extern void   DrawIconWindow(struct icon_info *item);
extern void   GetBitmapFile(struct icon_info *item);
extern void   GetXPMFile(struct icon_info *item);
extern void GetIconWindow(struct icon_info *item);
extern void GetIconBitmap(struct icon_info *item);
extern void process_message(unsigned long type,unsigned long *body);
extern Bool AddItem(unsigned long id, long desk, unsigned long flags);
extern Bool DeleteItem(unsigned long id);
extern struct icon_info *UpdateItem(unsigned long type, unsigned long id, char *item);
extern void freeitem(struct icon_info *item, int d);
extern void RedrawHScrollbar(void);
extern void RedrawVScrollbar(void);
extern void RedrawIcons(void);
extern void GetIconwinSize(int *dx, int *dy);
extern void RedrawLeftButton(GC rgc, GC sgc);
extern void RedrawRightButton(GC rgc, GC sgc);
extern void RedrawTopButton(GC rgc, GC sgc);
extern void RedrawBottomButton(GC rgc, GC sgc);
extern struct icon_info *Search(Window w);
extern void ExecuteAction(int x, int y, struct icon_info *item);
extern void HScroll(int x);
extern void VScroll(int y);
extern void RedrawLabel(struct icon_info *item);
extern void parseplacement(char *tline);
extern void parseaction(char *tline);
extern void AdjustIconWindow(struct icon_info *item, int n);
extern int AdjustIconWindows(void);
extern struct icon_info *SetFlag(unsigned long id, int t);
extern void RedrawIcon(struct icon_info *item, int f);
extern void Next(void);
extern void Prev(void);
extern void GetDefaultPixmap(struct icon_info *item);
extern void parsemouse(char *tline);
extern void parsekey(char *tline);
extern void parseicon(char *tline);
extern Bool SortItem(struct icon_info *item);
extern struct icon_info *MinItem(struct icon_info *head);
extern void ExecuteKey(XEvent event);
extern int matchWildcards(char *pattern, char *string);
extern Bool GetBackPixmap(void);
extern int LookInList(struct icon_info *item);
extern char *stripcpy2(char *source);
extern void mapicons(void);
extern int desk_cond(struct icon_info *item);
extern int itemcmp(struct icon_info *item1, struct icon_info *item2);
extern XErrorHandler myErrorHandler(Display *dpy, XErrorEvent *event);


extern Display *dpy;			/* which display are we talking to */
extern Window Root;
extern Window main_win;
extern Window holder_win;
extern Window icon_win;
extern int screen;
extern int d_depth;
extern Pixel fore_pix, back_pix, icon_fore_pix, icon_back_pix;
extern GC  NormalGC;
extern GC  ReliefGC;
extern int ButtonWidth,ButtonHeight;
extern XFontStruct *font;
extern int num_rows;
extern int num_columns;
extern int Lines;
extern int interval;
extern int max_icon_width;
extern int max_icon_height;
extern int primary, secondary;
extern int icon_win_width, icon_win_height;
extern Pixmap IconwinPixmap;
extern char *IconwinPixmapFile;

struct icon_info
{
  char *action;
  char *name;
  char *window_name;
  char *res_class;
  char *res_name;
  char *icon_file;
  int x;
  int y;
  int icon_w;
  int icon_h;
  Pixmap iconPixmap;		/* pixmap for the icon */
  Pixmap icon_maskPixmap;
  Window IconWin;
  Window icon_pixmap_w;
  XWMHints *wmhints;
  int icon_depth;
  long id;
  long desk;
  long flags;
  long extra_flags;
  struct icon_info *next;
  struct icon_info *prev;
};

struct iconfile
{
  char *name;
  char *iconfile;
  struct iconfile *next;
};

struct mousefunc
{
  int mouse;
  int type;
  char *action;
  struct mousefunc *next;
};

struct keyfunc
{
  char *name;
  KeyCode keycode;
  char *action;
  struct keyfunc *next;
};

extern struct icon_info *Head;
extern struct icon_info *Tail;
extern struct func *Actions;
extern struct iconfile *IconListHead;
extern struct iconfile *IconListTail;

extern char *iconPath;
extern char *pixmapPath;

extern int icon_relief;



#define NOPLACE -1
#define LEFT 0
#define RIGHT 1
#define TOP 2
#define BOTTOM 3

#include "../../fvwm/fvwm.h"

#define NO_CLICK 0
#define CLICK 1
#define ONE_AND_A_HALF_CLICKS 3
#define DOUBLE_CLICK 2

/* sorting */
#define UNSORT     0
#define WINDOWNAME 1
#define ICONNAME   2
#define RESNAME    3
#define RESCLASS   4

/* local flags */
#define HIDE_H        (1<<0)
#define HIDE_V        (1<<1)
#define SETWMICONSIZE (1<<2)
#define CURRENT_ONLY  (1<<3)

#define DEFAULTICON   (1<<0)
#define NONAME        (1<<1)
#define NOCLASS       (1<<2)

