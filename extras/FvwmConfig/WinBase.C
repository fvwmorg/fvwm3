#include "config.h"

#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "WinBase.h"

char *def_geom_string = NULL;
char *def_fg_string = DEFAULT_FORECOLOR;
char *def_bg_string = DEFAULT_BACKCOLOR;
char *def_font = DEFAULT_FONT;
char *def_bw = DEFAULT_BEVEL;
char *def_resname = "XThing";
char *def_display = NULL;
char **orig_argv = NULL;
int orig_argc = 0;

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/
#include <sys/wait.h>
#ifdef HAVE_WAITPID
#define ReapChildren()  while ((waitpid(-1, NULL, WNOHANG)) > 0);
#else
#define ReapChildren()  while ((wait3(NULL, WNOHANG, NULL)) > 0);
#endif
   
int GetFdWidth(void)
{
#ifdef HAVE_SYSCONF
  return sysconf(_SC_OPEN_MAX);
#else
  return getdtablesize();
#endif
}

int My_XNextEvent(Display *dpy, XEvent *event);
static int MyXAllocColor(Display *dpy,Colormap cmap,int screen, XColor *color);
/**************************************************************************
 *
 * Standard options 
 *
 *************************************************************************/
struct res_list
{
  char **dataptr;
  char *keyword;
};

     
struct res_list  string_resource_list[] =
{
  {&def_geom_string,                "geometry"},
  {&def_geom_string,                "geom"},
  {&def_fg_string,                  "foreground"},
  {&def_fg_string,                  "fg"},
  {&def_bg_string,                  "background"},
  {&def_bg_string,                  "bg"},
  {&def_font,                       "font"},
  {&def_bw,                         "bevelwidth"},
  {&def_bw,                         "bw"},
  {0,                           NULL}
};


/***************************************************************************
 *
 * Declarations for static members of WinBase
 *
 ***************************************************************************/
Display     *WinBase::dpy = 0;
Window       WinBase::Root = None;
int          WinBase::Screen;
GC           WinBase::DefaultReliefGC;
GC           WinBase::DefaultShadowGC;
GC           WinBase::DefaultForeGC;
XFontStruct *WinBase::DefaultFont;
Pixel        WinBase::DefaultBackColor;
Pixel        WinBase::DefaultReliefColor;
Pixel        WinBase::DefaultShadowColor;
Pixel        WinBase::DefaultForeColor;
Colormap     WinBase::cmap;

static Atom wm_del_win;
static int fd_width;
static int x_fd;
struct iodesc
{
  int fd;
  void (*func)(int);
};
struct iodesc *wininputdesc;
struct iodesc *winoutputdesc;
static int wininputcount;
static int winoutputcount;

void WinInitialize(char **argv, int argc)
{
  int i = 0,j,done;
  char *tmp;

  orig_argv = argv;
  orig_argc = argc;

  for(i=1;i<argc;i++)
    {
      if(strcmp(argv[i],"-display")==0)
	def_display = argv[i+1];
    }


  WinBase::dpy = XOpenDisplay(def_display);
  if(WinBase::dpy == 0)
    {
      cerr <<"Can't open display\n";
      exit(-1);
    }

  fd_width = GetFdWidth();
  wininputdesc = new struct iodesc[fd_width];
  winoutputdesc = new struct iodesc[fd_width];
  wininputcount = 0;
  winoutputcount = 0;
  for(i=0;i<fd_width;i++)
    {
      wininputdesc[i].fd = -1;
      winoutputdesc[i].fd = -1;
    }
  x_fd = XConnectionNumber(WinBase::dpy);

  if(argv[0] !=0)
    def_resname = argv[0];

  /* Check for defaults in .Xdefaults. */
  i=0;
  while(string_resource_list[i].keyword != NULL)
    {
      if((tmp = XGetDefault(WinBase::dpy,def_resname,
			    string_resource_list[i].keyword)) != (char *)0)
	{
	  *string_resource_list[i].dataptr = tmp;
	}
      
      i++;
    }

  /* Check for command line options */
  for(i=1;i<argc;i++)
    {
      if(argv[i][0] == '-')
	{
	  j=0;
	  done = 0;
	  while((string_resource_list[j].keyword != NULL)&&(!done))
	    {
	      if(strcmp(&argv[i][1],string_resource_list[j].keyword)==0)
		{
		  *string_resource_list[j].dataptr = &argv[i+1][0];
		  i++;
		  done = 1;
		}
	      j++;
	    }
	}
    }
}

int WinAddInput(int fd, void (*readfunc)(int))
{
  if(wininputcount < fd_width)
    {
      wininputdesc[wininputcount].func = readfunc;
      wininputdesc[wininputcount].fd = fd;
      wininputcount++;
      return 0;
    }
  else
    return 1;
}

int WinAddOutput(int fd, void (*writefunc)(int))
{
  if(winoutputcount < fd_width)
    {
      winoutputdesc[winoutputcount].func = writefunc;
      winoutputdesc[winoutputcount].fd = fd;
      winoutputcount++;
      return 0;
    }
  else
    return 1;
}

WinBase::WinBase(WinBase* ParentW, int width,int height,int x_loc, int y_loc)
{
  Window pwindow;
  XGCValues gcv;
  XSetWindowAttributes attributes;      /* attributes for creating window */
  unsigned long mask;
  int create_defaults = 0;

  Parent = ParentW;
  if(Root == None)
    {
      /* This is the first window opened. */
      create_defaults = 1;

      /* Open X connection */
      if(dpy == 0)
	{
	  dpy = XOpenDisplay(def_display);
	  if(dpy == 0)
	    {
	      cerr <<"Can't open display\n";
	      if(def_display != NULL)
		cerr <<def_display;
	      cerr <<"\n";
		  
	      exit(-1);
	    }
	}
      Screen = DefaultScreen(dpy);
      Root = RootWindow(dpy,Screen);

    }
  if(create_defaults)
    {
      cmap = DefaultColormap(dpy,Screen);
      DefaultBackColor = GetColor(def_bg_string,dpy,cmap,Screen);
      GetColor("black",dpy,cmap,Screen);
      GetColor("white",dpy,cmap,Screen);
      if(DefaultBackColor == 0xffffffff)
	DefaultBackColor = WhitePixel(dpy,Screen);
      DefaultReliefColor = GetHilite(DefaultBackColor,dpy,Screen, cmap);
      DefaultShadowColor = GetShadow(DefaultBackColor,dpy,Screen, cmap);
      DefaultForeColor = GetColor(def_fg_string,dpy,cmap,Screen);
      if(DefaultForeColor == 0xffffffff)
	DefaultForeColor = BlackPixel(dpy,Screen);
      
    }

  if(Parent == NULL)
    pwindow = Root;
  else
    pwindow = Parent->win;
  x = x_loc;
  y = y_loc;
  w = width;
  h = height;

  if(Parent == NULL)
    {
      BackColor = DefaultBackColor;
      ForeColor = DefaultForeColor;
      ReliefColor = DefaultReliefColor;
      ShadowColor = DefaultShadowColor;
      bw = atoi(def_bw);
    }
  else
    {
      BackColor = Parent->BackColor;
      ForeColor = Parent->ForeColor;
      ReliefColor = Parent->ReliefColor;
      ShadowColor = Parent->ShadowColor;
      bw = Parent->bw;
    }
  popped_out = 1;

  name_set = 0;
  icon_name_set = 0;

  CloseWindowAction = NULL;

  mask = CWBackPixel | CWEventMask;
  attributes.background_pixel = BackColor;
  attributes.event_mask = 
    ButtonPressMask|ButtonReleaseMask|ExposureMask|KeyPressMask|
      StructureNotifyMask;
  win = XCreateWindow(dpy,pwindow,x_loc,y_loc,width,height,0,
		      CopyFromParent,InputOutput,CopyFromParent,
		      mask,&attributes);
  if(create_defaults)
    {
      DefaultFont = XLoadQueryFont(dpy, def_font);
      if(DefaultFont == NULL)
	{
	  cerr <<"Can't load font "<<def_font<<"\n";
	  DefaultFont = XLoadQueryFont(dpy, "fixed");	
	}
      if(DefaultFont == NULL)
	{
	  cerr <<"Giving up\n";
	  exit(1);
	}

      mask = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
	GCForeground|GCBackground| GCFont;
      gcv.background = DefaultBackColor;
      gcv.line_width = 0;
      gcv.function = GXcopy;
      gcv.font = DefaultFont->fid;
      gcv.plane_mask = AllPlanes;
      gcv.graphics_exposures = False;
      
      gcv.foreground = DefaultReliefColor;
      DefaultReliefGC = XCreateGC(dpy,win,mask,&gcv);
      gcv.foreground = DefaultShadowColor;
      DefaultShadowGC = XCreateGC(dpy,win,mask,&gcv);
      gcv.foreground = DefaultForeColor;
      DefaultForeGC = XCreateGC(dpy,win,mask,&gcv);
    }
  if(Parent == NULL)
    {
      XClassHint classhints;
      XWMHints wmhints;
      XSizeHints normal_hints;
      XTextProperty name;

      main_window = this;
      ReliefGC = DefaultReliefGC;
      ShadowGC = DefaultShadowGC;
      ForeGC = DefaultForeGC;
      Font = DefaultFont;
      normal_hints.flags = PWinGravity|PMinSize|PResizeInc;
      normal_hints.width_inc = 1;
      normal_hints.height_inc = 1;
      normal_hints.min_width = 1;
      normal_hints.min_height = 1;
      normal_hints.win_gravity = NorthWestGravity;
      wmhints.input = True;
      wmhints.initial_state = NormalState;
      wmhints.flags = InputHint|StateHint;
      classhints.res_name = def_resname;
      classhints.res_class = def_resname;
  
      if (XStringListToTextProperty(&def_resname,1,&name) == 0)
	{
	  cerr <<"cannot allocate window name";
	  return;
	}

      XSetWMProperties(dpy,win, &name, &name,orig_argv, 
		       orig_argc, &normal_hints,&wmhints,&classhints);
    }
  else
    {
      main_window = Parent->main_window;
      mask = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
	GCForeground|GCBackground| GCFont;
      if(Parent->ReliefGC == DefaultReliefGC)
	ReliefGC = DefaultReliefGC;
      else
	{
	  XGetGCValues(dpy,Parent->ReliefGC,mask,&gcv);
	  ReliefGC = XCreateGC(dpy,win,mask,&gcv);
	}
      if(Parent->ShadowGC == DefaultShadowGC)
	ShadowGC = DefaultShadowGC;
      else
	{
	  XGetGCValues(dpy,Parent->ShadowGC,mask,&gcv);
	  ShadowGC = XCreateGC(dpy,win,mask,&gcv);
	}

      if(Parent->ForeGC == DefaultForeGC)
	ForeGC = DefaultForeGC;
      else
	{
	  XGetGCValues(dpy,Parent->ForeGC,mask,&gcv);
	  ForeGC = XCreateGC(dpy,win,mask,&gcv);
	}
      Font = Parent->DefaultFont;
    }
  RegisterWindow(win,this);

  if(wm_del_win == 0)
    wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);
}


WinBase::~WinBase()
{
  XDestroyWindow(dpy,win);
  UnregisterWindow(this);
}

void WinBase::Map()
{
  XMapSubwindows(dpy,win);
  XMapRaised(dpy,win);
  XSync(dpy,0);
}

void WinBase::RedrawWindow(int clear)
{
  if(clear)
    XClearWindow(dpy,win);
  DrawCallback(NULL);
}

void WinBase::DrawCallback(XEvent *event)
{
  int i;
  GC gc1;
  GC gc2;

  if((!event)||(event->xexpose.count == 0))
    {
      if(popped_out)
	{
	  gc1 = ShadowGC;
	  gc2 = ReliefGC;
	}
      else
	{
	  gc2 = ShadowGC;
	  gc1 = ReliefGC;
	}
      for(i=0;i<bw;i++)
	{
	  XDrawLine(dpy,win,gc1,i,h-1-i,w-1-i,h-1-i);
	  XDrawLine(dpy,win,gc1,w-1-i,i,w-1-i,h-1-i);
	  XDrawLine(dpy,win,gc2,i,i,w-1-i,i);
	  XDrawLine(dpy,win,gc2,i,i,i,h-1-i);
	}
    }
  

}

void WinBase::BPressCallback(XEvent *event)
{
}

void WinBase::KPressCallback(XEvent *event)
{
}

void WinBase::BReleaseCallback(XEvent *event)
{
}

void WinBase::ResizeCallback(int new_w, int new_h, XEvent *event)
{
  w = new_w;
  h = new_h;
  RedrawWindow(1);
}

void WinBase::MotionCallback(XEvent *event)
{
}
 
void WinBase::SetSize(int new_w, int new_h)
{
  XResizeWindow(dpy,win,new_w,new_h);
  w = new_w;
  h = new_h;
}



void WinBase::SetPosition(int new_x, int new_y)
{
  XMoveWindow(dpy,win,new_x,new_y);
  x = new_x;
  y = new_y;
}

void WinBase::SetGeometry(int new_x, int new_y, int new_w, int new_h, 
			  int min_width, int min_height, 
			  int max_width, int max_height, 
			  int resize_inc_w, int resize_inc_h,
			  int base_width, int base_height,
			  int gravity)
{
  XSizeHints normal_hints;
  
  normal_hints.flags = USPosition|USSize|
    PMinSize|PMaxSize|PResizeInc|PBaseSize|PWinGravity;
  normal_hints.x = new_x;
  normal_hints.y = new_y;
  normal_hints.width = new_w;
  normal_hints.height = new_h;
  normal_hints.min_width = min_width;
  normal_hints.min_height = min_height;
  normal_hints.max_width = max_width;
  normal_hints.max_height = max_height;
  normal_hints.width_inc = resize_inc_w;
  normal_hints.height_inc = resize_inc_h;
  normal_hints.base_width = base_width;
  normal_hints.base_height = base_height;
  normal_hints.win_gravity = gravity;
  XSetWMNormalHints(dpy,win, &normal_hints);
  SetPosition(new_x,new_y);
  SetSize(new_w, new_h);
}

void WinBase::SetCloseWindowAction(void (*NewAction)(WinBase *which))
{
  CloseWindowAction = NewAction;
}
void WinBase::SetBackColor(char *newcolor = DEFAULT_BACKCOLOR)
{
  XGCValues gcv;
  XSetWindowAttributes attributes;      /* attributes for creating window */
  unsigned long mask;

  BackColor = GetColor(newcolor,dpy,cmap,Screen);
  if(BackColor == 0xffffffff)
    BackColor = DefaultBackColor;
  ReliefColor = GetHilite(BackColor,dpy,Screen, cmap);
  ShadowColor = GetShadow(BackColor,dpy,Screen, cmap);
  if(ForeGC != DefaultForeGC)
    {
      XFreeGC(dpy,ForeGC);
    }
  if(ShadowGC != DefaultShadowGC)
    {
      XFreeGC(dpy,ShadowGC);
    }
  if(ReliefGC != DefaultReliefGC)
    {
      XFreeGC(dpy,ReliefGC);
    }

  mask = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
    GCForeground|GCBackground|GCFont;
  gcv.background = BackColor;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.font = Font->fid;
  gcv.graphics_exposures = False;
  
  gcv.foreground = ReliefColor;
  ReliefGC = XCreateGC(dpy,win,mask,&gcv);
  gcv.foreground = ShadowColor;
  ShadowGC = XCreateGC(dpy,win,mask,&gcv);
  gcv.foreground = ForeColor;
  ForeGC = XCreateGC(dpy,win,mask,&gcv);
  mask = CWBackPixel;
  attributes.background_pixel = BackColor;
  XChangeWindowAttributes(dpy,win, mask,&attributes);
  XClearWindow(dpy,win);
}
void WinBase::SetBevelWidth(int new_bw)
{
  bw = new_bw;
}
void WinBase::PushIn()
{
  popped_out = 0;

}

void WinBase::PopOut()
{
  popped_out = 1;
}

void WinBase::MakeTransient(WinBase *TransientFor)
{
  XSetTransientForHint(dpy,win,TransientFor->win);
}

void WinBase::SetForeColor(char *newcolor = DEFAULT_FORECOLOR)
{
  XGCValues gcv;
  unsigned long mask;
  
  ForeColor = GetColor(newcolor,dpy,cmap,Screen);
  if(ForeColor == 0xffffffff)
    ForeColor = DefaultForeColor;
  if(ForeGC!= DefaultForeGC)
    XFreeGC(dpy,ShadowGC);

  mask = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
    GCForeground|GCBackground|GCFont;
  gcv.background = BackColor;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  gcv.foreground = DefaultForeColor;
  gcv.font = Font->fid;
  ForeGC = XCreateGC(dpy,win,mask,&gcv);
}
void WinBase::SetFont(char *newfont=DEFAULT_FONT)
{
  XGCValues gcv;
  unsigned long mask;

  Font = XLoadQueryFont(dpy, newfont);
  if(Font == NULL)
    Font = DefaultFont;

  if(ForeGC != DefaultForeGC)
    {
      XFreeGC(dpy,ForeGC);
    }
  if(ShadowGC != DefaultShadowGC)
    {
      XFreeGC(dpy,ShadowGC);
    }
  if(ReliefGC != DefaultReliefGC)
    {
      XFreeGC(dpy,ReliefGC);
    }

  mask = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|
    GCForeground|GCBackground|GCFont;
  gcv.background = BackColor;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.font = Font->fid;
  gcv.graphics_exposures = False;
  
  gcv.foreground = ReliefColor;
  ReliefGC = XCreateGC(dpy,win,mask,&gcv);
  gcv.foreground = ShadowColor;
  ShadowGC = XCreateGC(dpy,win,mask,&gcv);
  gcv.foreground = ForeColor;
  ForeGC = XCreateGC(dpy,win,mask,&gcv);
}

void WinBase::SetWindowName(char *new_name)
{
  XTextProperty name;
  
  if (XStringListToTextProperty(&new_name,1,&name) == 0)
    {
      cerr <<"cannot allocate window name";
      return;
    }
  XSetWMName(dpy,win,&name);
  XFree(name.value);                     
  if(icon_name_set == 0)
    {
      SetIconName(new_name);
      icon_name_set = 0;
    }
  name_set = 1;
}
void WinBase::SetIconName(char *new_name)
{
  XTextProperty name;
  
  if (XStringListToTextProperty(&new_name,1,&name) == 0)
    {
      cerr<<"cannot allocate icon name";
      return;
    }
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);                
  icon_name_set = 1;
}

void WinBase::SetWindowClass(char *resclass)
{
  XClassHint class_hint;

  class_hint.res_name = def_resname;
  class_hint.res_class = resclass;
  XSetClassHint(dpy,win,&class_hint);
}

/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow(Pixel background, Display *dpy, int Screen, Colormap cmap) 
{
  XColor bg_color, white_p;

  bg_color.pixel = background;
  XQueryColor(dpy,cmap,&bg_color);

  white_p.pixel = WhitePixel(dpy,Screen);
  XQueryColor(dpy,cmap,&white_p);
 
  if((bg_color.red < white_p.red/5)&&
     (bg_color.green < white_p.green/5)&&
     (bg_color.blue < white_p.blue/5))
    {
      bg_color.red = white_p.red/4;
      bg_color.green = white_p.green/4;
      bg_color.blue =  white_p.blue/4;
    }
  else
    {
      bg_color.red = (bg_color.red*50)/100;
      bg_color.green = (bg_color.green*50)/100;
      bg_color.blue = (bg_color.blue*50)/100;
    }
  
  if(!MyXAllocColor(dpy,cmap,Screen,&bg_color))
    {
      cout <<"Can't allocate shadow color\n";
      bg_color.pixel = BlackPixel(dpy,Screen);
    }
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite(Pixel background, Display *dpy, int Screen, Colormap cmap) 
{
  XColor bg_color, white_p;
  int r,g,b;
  bg_color.pixel = background;
  XQueryColor(dpy,cmap,&bg_color);

  white_p.pixel = WhitePixel(dpy,Screen);
  XQueryColor(dpy,cmap,&white_p);


  if((bg_color.red > 2*white_p.red/3)&&
     (bg_color.green > 2*white_p.green/3)&&
     (bg_color.blue > 2*white_p.blue/3))
    {
      r = white_p.red*3/4;
      g = white_p.green*3/4;
      b =  white_p.blue*3/4;
    }
  else
    {
      r = bg_color.red + (white_p.red - bg_color.red)/2;
      g = bg_color.green + (white_p.green - bg_color.green)/2;
      b = bg_color.blue + (white_p.blue - bg_color.blue)/2;
    }
  if(r > white_p.red)
    bg_color.red = white_p.red;
  else
    bg_color.red = r;
  if(g > white_p.green)
    bg_color.green = white_p.green;
  else
    bg_color.green = g;
  if(b > white_p.blue)
    bg_color.blue = white_p.blue;
  else
    bg_color.blue = b;
  
  if(!MyXAllocColor(dpy,cmap,Screen,&bg_color))
    {
      cout <<"Can't allocate highlight color\n";
      bg_color.pixel = WhitePixel(dpy,Screen);;
    }
  return bg_color.pixel;
}

/****************************************************************************
 *
 * Loads a single color
 *
 ****************************************************************************/
Pixel GetColor(char *name, Display *dpy, Colormap cmap, int Screen)
{
  XColor color;

  color.pixel = 0xffffffff;
  if (!XParseColor (dpy, cmap, name, &color))
    {
      cerr<< "Can't parse color "<<name<<"\n";
    }
  else if(!MyXAllocColor (dpy, cmap, Screen,&color))
    {
      cerr<< "Can't allocate color "<<name<<"\n";
    }
  return color.pixel;
}

static int MyXAllocColor(Display *dpy,Colormap cmap,int screen, XColor *color)
{
  XColor *allcolors;
  int ncolors,i;
  int bestpixel = 0;
  float rdist, bdist, gdist, distance,bestdistance = 2e10;

  if(XAllocColor (dpy, cmap, color))
    return 1;

  ncolors = DisplayCells(dpy,screen);
  allcolors = new XColor[ncolors];
  for(i=0;i<ncolors;i++)
    allcolors[i].pixel = i;
  XQueryColors(dpy,cmap,allcolors,ncolors);
  for(i=0;i<ncolors;i++)
    {
      rdist = color->red- allcolors[i].red;
      bdist = color->blue- allcolors[i].blue;
      gdist = color->green- allcolors[i].green;
      distance = rdist*rdist+bdist*bdist+gdist*gdist;
      if(distance < bestdistance)
	{
	  bestdistance = distance;
	  bestpixel = i;
	}
    }
  color->red = allcolors[bestpixel].red;
  color->green = allcolors[bestpixel].green;
  color->blue = allcolors[bestpixel].blue;
  if(XAllocColor (dpy, cmap, color))
    return 1;
  else
    return 0;
}

struct registered
{
  Window win;
  WinBase *thing;
  int occupied;
};
static struct registered thinglist[100];



void RegisterWindow(Window win, WinBase *a)
{
  static int firsttime = 1;
  int i;

  if(firsttime)
    {
      firsttime = 0;
      for(i=0;i<100;i++)
	{
	  thinglist[i].occupied = 0;
	}
    }
  
  for(i=0;i<100;i++)
    {
      if(thinglist[i].occupied == 0)
	{
	  thinglist[i].occupied = 1;
	  thinglist[i].win = win;
	  thinglist[i].thing = a;
	  return;
	}
    }
  cout <<"Too many windows\n";
  exit(-1);
}


void UnregisterWindow(WinBase *thing)
{
  int i;
  for(i=0;i<100;i++)
    {
      if((thinglist[i].occupied ==1)&&
	 (thinglist[i].thing == thing))
	 {
	   thinglist[i].occupied = 0;
	   return;
	 }
    }
}
  
void WinLoop()
{
  XEvent event;
  int i;
  Display *dpy;
  
  i=0;
  while((thinglist[i].occupied == 0)&&(i<100))
    {
      i++;
    }
  if(i<100)
    dpy = thinglist[i].thing->dpy; 
  else
    return;

  while(!My_XNextEvent(dpy,&event));

  for(i=0;i<100;i++)
    {
      if((thinglist[i].occupied)&&(event.xany.window == thinglist[i].win))
	{
	  switch(event.type)
	    {
	    case Expose:
	    case GraphicsExpose:
	      thinglist[i].thing->DrawCallback(&event);
	      break;
	    case ButtonPress:
	      thinglist[i].thing->BPressCallback(&event);
	      break;
	    case ButtonRelease:
	      thinglist[i].thing->BReleaseCallback(&event);
	      break;
	    case KeyPress:
	      thinglist[i].thing->KPressCallback(&event);
	      break;
	    case ConfigureNotify:
	      if((thinglist[i].thing->w!= event.xconfigure.width)||
		(thinglist[i].thing->h!= event.xconfigure.height))
		thinglist[i].thing->ResizeCallback(event.xconfigure.width, 
						   event.xconfigure.height,&event);
	      break;
	    case ClientMessage:
	      if (event.xclient.format == 32 && event.xclient.data.l[0] == wm_del_win)
		{
		  if(thinglist[i].thing->CloseWindowAction == NULL)
		    exit(0);
		  else
		    thinglist[i].thing->CloseWindowAction(thinglist[i].thing);
		}
	      break;
	    case MotionNotify:
	      thinglist[i].thing->MotionCallback(&event);	
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
}


/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
int My_XNextEvent(Display *dpy, XEvent *event)
{
  fd_set in_fdset,out_fdset;
  Window child;
  Window targetWindow;
  int i,count;
  int retval;

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_ZERO(&out_fdset);
  for(i=0; i<wininputcount; i++)
    {
      FD_SET(wininputdesc[i].fd, &in_fdset);
    }

  for(i=0; i<winoutputcount; i++)
    {
      FD_SET(winoutputdesc[i].fd, &out_fdset);
    }

  /* Do this IMMEDIATELY prior to select, to prevent any nasty
   * queued up X events from just hanging around waiting to be
   * flushed */
  XFlush(dpy);
  if(XPending(dpy))
    {
      XNextEvent(dpy,event);
      return 1;
    }

  /* Zap all those zombies! */
  /* If we get to here, then there are no X events waiting to be processed.
   * Just take a moment to check for dead children. */
  ReapChildren();

  XFlush(dpy);
#ifdef __hpux
  retval=select(fd_width,(int *)&in_fdset, (int *)&out_fdset,0, NULL);
#else
  retval=select(fd_width,&in_fdset, &out_fdset, 0, NULL);
#endif
  
  /* Check for module input. */
  for(i=0;i<fd_width;i++)
    {
      if((wininputdesc[i].fd >= 0)&&(FD_ISSET(wininputdesc[i].fd, &in_fdset)))
	{
	  wininputdesc[i].func(wininputdesc[i].fd);
	}
      if((winoutputdesc[i].fd>=0)&&(FD_ISSET(winoutputdesc[i].fd, &out_fdset)))
	{
	  winoutputdesc[i].func(winoutputdesc[i].fd);
	}
    }
  return 0;
}



