#include "config.h"
#include "fvwmlib.h"

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
extern void   CreateWindow(void);
extern Pixel  GetColor(char *name);
extern void   nocolor(char *a, char *b);
extern void   RedrawWindow(Window *win, int firstbutton, int newbutton, int num_rows, int num_columns);
extern void   match_string(char *tline);
#ifdef ENABLE_SOUND
extern void   bind_sound(char *tline);
extern void   PlaySound(int event);
#endif
extern void   Loop(void);
extern void   ParseOptions(char *);
extern char   *safemalloc(int length);
extern void   change_window_name(char *str);
extern int    My_XNextEvent(Display *dpy, XEvent *event);
extern void   DeadPipe(int nonsense);
extern void   LoadIconFile(int button,int ico);
extern void   CreateIconWindow(int button, Window *win);
extern void   ConfigureIconWindow(int button,int row, int column);
extern void   GetBitmapFile(int button,int ico);
extern int    GetXPMFile(int button,int ico);
extern int    GetXPMData(int button, char **data);
extern int    GetXPMGradient(int button, int from[3], int to[3], int maxcols,
		   int type);
extern int    GetSolidXPM(int button, Pixel pixel);
extern Bool Pushed;
extern Bool ForceSize;
extern void DrawOutline(Drawable d, int w, int h);
void process_message(unsigned long type,unsigned long *body);
void my_send_clientmessage (Window w, Atom a, Time timestamp);
void swallow(unsigned long *body);
void ConstrainSize (XSizeHints *hints, int *widthp, int *height);
void MapFolder(int folder, int *LastMapped, int base_x, int base_y, int row, int col);
void CloseFolder(int folder);
void OpenFolder(int folder,int x, int y, int w, int h,  int direction);
void RedrawPushed(Window *win, int i, int j);
void RedrawUnpushed(Window *win, int i, int j);
void RedrawUnpushedOutline(Window *win, int i,int j);
void RedrawPushedOutline(Window *win, int i, int j);
void CreateShadowGC(void);
extern Display *dpy;			/* which display are we talking to */
extern Window Root;
extern Window main_win;
extern int screen;
extern long d_depth;
extern Pixel back_pix, fore_pix;
extern GC  NormalGC, HiReliefGC, HiInnerGC;
extern GC  MaskGC;
extern int BUTTONWIDTH, BUTTONHEIGHT;
extern XFontStruct *font;
#define MAX_BUTTONS 100
#define BACK_BUTTON 100

#define MAX_OVERLAY 3

#define BUTTON_ARRAY_LN 102
#define FOLDER_ARRAY_LN 10

#define ICON_WIN_WIDTH	48
#define ICON_WIN_HEIGHT 48

#define ISMAPPED 0
#define NOTMAPPED 1

typedef struct icon_info {
    char *file;
    short w, h;
    Pixmap icon, mask;
    signed char depth;    
} icon_info;

struct button_info
{
  char *action;
  char *title;
  int iconno;
  icon_info icons[MAX_OVERLAY];

  Pixmap completeIcon;		/* icon with background */
  Window IconWin;
  Window *parent;
  XSizeHints hints;
  char *hangon;
  char up;
  char swallow;
  char maxsize;
  char module;
  int folder;
#ifdef ENABLE_DND
  char *drop_action;
#endif    
};

struct folder_info
{
  Window win;         /* Window of the Folder */
  int firstbutton;    /* index to Buttons, starting at end */
  int count;          /* count folded buttons */
  int mapped;         /* is the window visible or not ?? */
  int cols;           /* either 1 or me.count */
  int rows;           /* either me.count or 1 */
  int direction;      /* direction of the folder */
};

extern struct button_info Buttons[MAX_BUTTONS+2];
extern struct folder_info Folders[FOLDER_ARRAY_LN];

extern char *iconPath;
extern char *pixmapPath;





