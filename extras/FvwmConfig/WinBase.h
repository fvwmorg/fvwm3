#ifndef win_h
#define win_h

#include <X11/X.h>
#include <X11/Xlib.h>

/* let's define Pixel if it is not done yet */
#if ! defined(_XtIntrinsic_h) && ! defined(PIXEL_ALREADY_TYPEDEFED)
typedef unsigned long Pixel;    /* Index into colormap */
#define PIXEL_ALREADY_TYPEDEFED
#endif

#define DEFAULT_BEVEL "2"
#define DEFAULT_WIDTH 100
#define DEFAULT_HEIGHT 100
#define DEFAULT_X 0
#define DEFAULT_Y 0
#define DEFAULT_BACKCOLOR "#908090"
#define DEFAULT_FORECOLOR "black"
#define DEFAULT_FONT "fixed"



class WinBase
{
 public:
  static Display *dpy;
  static Window Root;
  static int Screen;
  static Pixel DefaultBackColor;
  static Pixel DefaultReliefColor;
  static Pixel DefaultShadowColor;
  static Pixel DefaultForeColor;
  static GC DefaultReliefGC;
  static GC DefaultShadowGC;
  static GC DefaultForeGC;
  static XFontStruct *DefaultFont;
  static Colormap cmap;
  
  Window win;
  WinBase *Parent;
  WinBase *main_window;
  int name_set;
  int icon_name_set;
  int x;
  int y;
  int w;
  int h;
  int bw;
  char popped_out;
  Pixel BackColor;
  Pixel ReliefColor;
  Pixel ShadowColor;
  Pixel ForeColor;
  GC ReliefGC;
  GC ShadowGC;
  GC ForeGC;
  XFontStruct *Font;
  void (*CloseWindowAction)(WinBase *which);
  
  WinBase(WinBase *Parent = NULL,
	   int width = DEFAULT_WIDTH,int height=DEFAULT_HEIGHT, 
	   int x_loc=DEFAULT_X, int y_loc=DEFAULT_Y);
  ~WinBase();
  void Map();

  /* These routines are called in response to X Events. */
  virtual void DrawCallback(XEvent *event = NULL);
  virtual void BPressCallback(XEvent *event = NULL);
  virtual void BReleaseCallback(XEvent *event = NULL);
  virtual void KPressCallback(XEvent *event = NULL);
  virtual void ResizeCallback(int new_w, int new_h, XEvent *event = NULL);
  virtual void MotionCallback(XEvent *event = NULL);
 
  /* These are user-callable routines */
  void SetSize(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT);
  void SetPosition(int x = DEFAULT_X, int y = DEFAULT_Y);
  void SetGeometry(int new_x, int new_y, int new_w, int new_h, 
			  int min_width=1, int min_height=1, 
			  int max_width=32767, int max_height=32767, 
			  int resize_inc_w=1, int resize_inc_h=1,
			  int base_width=1, int base_height=1,
			  int gravity=NorthWestGravity);
  void SetCloseWindowAction(void (*CloseWindowAction)(WinBase *which));
  void SetBackColor(char *bcolor);
  void SetForeColor(char *fcolor);
  void SetBevelWidth(int bw);
  void SetFont(char *font);
  void PushIn(void);
  void PopOut(void);
  void MakeTransient(WinBase *TransientFor);
  void SetWindowName(char *name);
  void SetIconName(char *name);
  void SetWindowClass(char *resclass);
  void RedrawWindow(int clear);
  inline int ScreenWidth(){return XDisplayWidth(dpy,Screen);};
  inline int ScreenHeight(){return XDisplayHeight(dpy,Screen);};
};


void WinInitialize(char **argv, int argc);
int WinAddInput(int fd, void (*readfunc)(int));
int WinAddOutput(int fd, void (*writefunc)(int));
void WinLoop(void);
Pixel GetColor(char *name, Display *dpy, Colormap cmap,int Screen);
void RegisterWindow(Window win, WinBase *a);
void UnregisterWindow(WinBase *thing);
Pixel GetShadow(Pixel background, Display *dpy, int Screen, Colormap cmap) ;
Pixel GetHilite(Pixel background, Display *dpy, int Screen, Colormap cmap) ;


#endif



