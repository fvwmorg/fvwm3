/*
   FvwmButtons, copyright 1996, Jarl Totland

 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.

*/

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

/* ------------------------------- includes -------------------------------- */
#include "config.h"
#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#ifdef XPM
#include <X11/xpm.h>
#endif
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "FvwmButtons.h"
#include "misc.h" /* ConstrainSize() */
#include "parse.h" /* ParseOptions() */
#include "icons.h" /* CreateIconWindow(), ConfigureIconWindow() */
#include "draw.h"


#define MW_EVENTS   (ExposureMask |\
		     StructureNotifyMask |\
		     ButtonReleaseMask | ButtonPressMask |\
		     KeyReleaseMask | KeyPressMask)
/* SW_EVENTS are for swallowed windows... */
#define SW_EVENTS   (PropertyChangeMask | StructureNotifyMask |\
		     ResizeRedirectMask | SubstructureNotifyMask)

#ifdef DEBUG_FVWM
#define MySendText(a,b,c) {\
  fprintf(stderr,"%s: Sending text to fvwm: \"%s\"\n",MyName,(b));\
  SendText((a),(b),(c));}
#else
#define MySendText(a,b,c) SendText((a),(b),(c));
#endif

/* --------------------------- external functions -------------------------- */
extern void DumpButtons(button_info*);
extern void SaveButtons(button_info*);

/* ------------------------------ prototypes ------------------------------- */

void DeadPipe(int nonsense) __attribute__((__noreturn__));
static void DeadPipeCleanup(void);
static RETSIGTYPE TerminateHandler(int sig);
void SetButtonSize(button_info*,int,int);
/* main */
void Loop(void);
void RedrawWindow(button_info*);
void RecursiveLoadData(button_info*,int*,int*);
void CreateWindow(button_info*,int,int);
void nocolor(const char *a, const char *b) __attribute__((__noreturn__));
int My_XNextEvent(Display *dpy, XEvent *event);
void process_message(unsigned long type,unsigned long *body);
extern void send_clientmessage (Display *disp, Window w, Atom a,
				Time timestamp);
void CheckForHangon(unsigned long*);
Window GetRealGeometry(Display*,Window,int*,int*,ushort*,ushort*,
		       ushort*,ushort*);
void swallow(unsigned long*);
void AddButtonAction(button_info*,int,char*);
char *GetButtonAction(button_info*,int);

void DebugEvents(XEvent*);

panel_info *seekpanel(button_info *);
void Slide(panel_info *, button_info *);

/* -------------------------------- globals ---------------------------------*/

Display *Dpy;
Window Root;
GC trans_gc = NULL;
Window MyWindow;
char *MyName;
XFontStruct *font;
int screen;

int x_fd;
fd_set_size_t fd_width;

char *config_file = NULL;

static Atom _XA_WM_DEL_WIN;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_NORMAL_HINTS;
Atom _XA_WM_NAME;

char *imagePath = NULL;

Pixel hilite_pix, back_pix, shadow_pix, fore_pix;
GC  NormalGC;
/* needed for relief drawing only */
GC  ShadowGC;
int Width,Height;

int x= -30000,y= -30000,w= -1,h= -1,gravity = NorthWestGravity;
int new_desk = 0;
int ready = 0;
int xneg = 0, yneg = 0;

button_info *CurrentButton = NULL;
int fd[2];

button_info *UberButton=NULL;

panel_info *MainPanel = NULL, *CurrentPanel = NULL, *PanelIndex;
int dpw, dph;

int save_color_limit;                   /* Color limit, if any */

/* ------------------------------ Misc functions ----------------------------*/

#ifdef DEBUG
char *mymalloc(int length)
{
  int i=length;
  char *p=safemalloc(length);
  while(i)
    p[--i]=255;
  return p;
}
#endif

#ifdef I18N_MB
/*
 * fake XFetchName() function
 */
static char **XFetchName_islist = NULL; /* XXX */
static Status MyXFetchName(dpy, win, winname)
Display *dpy;
Window win;
char **winname;
{
  XTextProperty text;
  char **list;
  int nitems;

  if (XGetWMName(dpy, win, &text)) {
    if (text.value) {
      text.nitems = strlen(text.value);
      if (text.encoding == XA_STRING) {
        /* STRING encoding, use this as it is */
        *winname = (char *)text.value;
        return 0;
      } else {
        /* not STRING encoding, try to convert */
        if (XmbTextPropertyToTextList(dpy, &text, &list, &nitems) >= Success
            && nitems > 0 && *list) {
          XFree(text.value);
          *winname = *list;
          XFetchName_islist = list; /* XXX */
        } else {
          if (list) XFreeStringList(list);
          XFree(text.value); /* return of XGetWMName() */
          XGetWMName(dpy, win, &text); /* XXX: read again ? */
          *winname = (char *)text.value;
        }
      }
      return 0;
    }
  }
  return 1;
}
static int MyXFree(data)
void *data;
{
  if (XFetchName_islist != NULL) {
    XFreeStringList(XFetchName_islist);
    XFetchName_islist = NULL;
    return 0;
  }
  return XFree(data);
}
#define XFetchName(x,y,z) MyXFetchName(x,y,z)
#define XFree(x) MyXFree(x)
#endif

/**
*** Some fancy routines straight out of the manual :-) Used in DeadPipe.
**/
Bool DestroyedWindow(Display *d,XEvent *e,char *a)
{
  if(e->xany.window == (Window)a)
    if((e->type == DestroyNotify && e->xdestroywindow.window == (Window)a)||
       (e->type == UnmapNotify && e->xunmap.window == (Window)a))
      return True;
  return False;
}

int IsThereADestroyEvent(button_info *b)
{
  XEvent event;
  return XCheckIfEvent(Dpy,&event,DestroyedWindow,(char*)b->IconWin);
}

/**
*** DeadPipe()
*** Externally callable function to quit! Note that DeadPipeCleanup
*** is an exit-procedure and so will be called automatically
**/
void DeadPipe(int whatever)
{
  exit(0);
}

/**
*** TerminateHandler()
*** Signal handler that will make the event-loop terminate
**/
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
}

/**
*** DeadPipeCleanup()
*** Remove all the windows from the Button-Bar, and close them as necessary
**/
static void DeadPipeCleanup(void)
{
  button_info *b,*ub=UberButton;
  int button=-1;

  signal(SIGPIPE, SIG_IGN);/* Xsync may cause SIGPIPE */

  XSync(Dpy,0); /* Wait for thing to settle down a bit */
  XGrabServer(Dpy); /* We don't want interference right now */
  while(NextButton(&ub,&b,&button,0))
    {
      /* delete swallowed windows */
      if((buttonSwallowCount(b)==3) && b->IconWin)
	{
#         ifdef DEBUG_HANGON
	  fprintf(stderr,"%s: Button 0x%06x window 0x%x (\"%s\") is ",
		  MyName,(ushort)b,(ushort)b->IconWin,b->hangon);
#         endif
	  if(!IsThereADestroyEvent(b)) { /* Has someone destroyed it? */
	    if(!(buttonSwallow(b)&b_NoClose))
	      {
		if(buttonSwallow(b)&b_Kill)
		  {
		    XKillClient(Dpy,b->IconWin);
#                   ifdef DEBUG_HANGON
		    fprintf(stderr,"now killed\n");
#                   endif
		  }
		else
		  {
		    send_clientmessage(Dpy,b->IconWin,_XA_WM_DEL_WIN,
				       CurrentTime);
#                   ifdef DEBUG_HANGON
		    fprintf(stderr,"now deleted\n");
#                   endif
		  }
	      }
	    else
	      {
#               ifdef DEBUG_HANGON
		fprintf(stderr,"now unswallowed\n");
#               endif
		XReparentWindow(Dpy,b->IconWin,Root,b->x,b->y);
		XMoveWindow(Dpy,b->IconWin,b->x,b->y);
		XResizeWindow(Dpy,b->IconWin,b->w,b->h);
		XSetWindowBorderWidth(Dpy,b->IconWin,b->bw);
	      }
	  }
#         ifdef DEBUG_HANGON
	  else
	    fprintf(stderr,"already handled\n");
#         endif
	}
    }
  XUngrabServer(Dpy); /* We're through */
  XSync(Dpy,0); /* Let it all die down again so we can catch our X errors... */

  /* Hey, we have to free the pictures too! */
  button=-1;ub=UberButton;
  while(NextButton(&ub,&b,&button,1))
    {
      if(b->flags&b_Icon)
	DestroyPicture(Dpy,b->icon);
      if(b->flags&b_IconBack)
	DestroyPicture(Dpy,b->backicon);
      if(b->flags&b_Container && b->c->flags&b_IconBack
	 && !(b->c->flags&b_TransBack))
	DestroyPicture(Dpy,b->c->backicon);
    }
}

/**
*** SetButtonSize()
*** Propagates global geometry down through the buttonhierarchy.
**/
void SetButtonSize(button_info *ub,int w,int h)
{
  int i=0,dx,dy;
  if(!ub || !(ub->flags&b_Container))
    {
      fprintf(stderr,"%s: BUG: Tried to set size of noncontainer\n",MyName);
      exit(2);
    }
  if(ub->c->num_rows==0 || ub->c->num_columns==0)
    {
      fprintf(stderr,"%s: BUG: Set size when rows/cols was unset\n",MyName);
      exit(2);
    }
  w*=ub->BWidth;
  h*=ub->BHeight;

  if(ub->parent)
    {
      i=buttonNum(ub);
      ub->c->xpos=buttonXPos(ub,i);
      ub->c->ypos=buttonYPos(ub,i);
    }
  dx=buttonXPad(ub)+buttonFrame(ub);
  dy=buttonYPad(ub)+buttonFrame(ub);
  ub->c->xpos+=dx;
  ub->c->ypos+=dy;
  w-=2*dx;
  h-=2*dy;
  ub->c->ButtonWidth=w/ub->c->num_columns;
  ub->c->ButtonHeight=h/ub->c->num_rows;

  i=0;
  while(i<ub->c->num_buttons)
    {
      if(ub->c->buttons[i] && ub->c->buttons[i]->flags&b_Container)
	SetButtonSize(ub->c->buttons[i],
		      ub->c->ButtonWidth,ub->c->ButtonHeight);
      i++;
    }
}


/**
*** AddButtonAction()
**/
void AddButtonAction(button_info *b,int n,char *action)
{
  int l;
  char *s;
  char *t;

  if(!b || n<0 || n>3 || !action)
    {
      fprintf(stderr,"%s: BUG: AddButtonAction failed\n",MyName);
      exit(2);
    }
  if(b->flags&b_Action)
    {
      if(b->action[n])
	free(b->action[n]);
    }
  else
    {
      int i;
      b->action=(char**)mymalloc(4*sizeof(char*));
      for(i=0;i<4;b->action[i++]=NULL);
      b->flags|=b_Action;
    }

  while (*action && isspace((unsigned char)*action))
    action++;
  l = strlen(action);
  if (l > 1)
    {
      switch (action[0])
      {
      case '\"':
      case '\'':
      case '`':
	s = SkipQuote(action, NULL, "", "");
	/* Strip outer quotes */
	if (*s == 0)
	  {
	    action++;
	    l -= 2;
	  }
	break;
      default:
	break;
      }
    }
  t = (char *)mymalloc(l + 1);
  memmove(t, action, l);
  t[l] = 0;
  b->action[n] = t;

}

/**
*** GetButtonAction()
**/
char *GetButtonAction(button_info *b,int n)
{
  if(!b || !(b->flags&b_Action) || !(b->action) || n<0 || n>3)
    return NULL;
  return b->action[n];
}

#ifdef SHAPE
/**
*** SetTransparentBackground()
*** use the Shape extension to create a transparent background.
***   Patrice Fortier
**/
void SetTransparentBackground(button_info *ub,int w,int h)
{
  Pixmap pmap_mask;
  button_info *b;
  Window root_return;
  int x_return, y_return;
  unsigned int width_return, height_return;
  unsigned int border_width_return;
  unsigned int depth_return;
  int number, i;
  XFontStruct *font;

  pmap_mask = XCreatePixmap(Dpy,MyWindow,w,h,1);

  if (trans_gc == NULL) {
    XGCValues gcv;

    /* create a GC for doing transparency */
    trans_gc = XCreateGC(Dpy,pmap_mask,(unsigned long)0,&gcv);
  }

  XSetForeground(Dpy,trans_gc,0);
  XFillRectangle(Dpy,pmap_mask,trans_gc,0,0,w,h);
  XSetForeground(Dpy,trans_gc,1);

  /*
   * if button has an icon, draw a rect with the same size as the icon
   * (or the mask of the icon),
   * else draw a rect with the same size as the button.
   */

  i=-1;
  while(NextButton(&ub,&b,&i,0))
    {
      if(b->flags&b_Icon)
	{
	  XGetGeometry(Dpy,b->IconWin,&root_return,&x_return,&y_return,
		       &width_return,&height_return,
		       &border_width_return,&depth_return);

	  number=buttonNum(b);
	  if (b->icon->mask == None) {
	    XFillRectangle(Dpy,pmap_mask,trans_gc,x_return, y_return,
			   b->icon->width,b->icon->height);
	  } else {
	    XCopyArea(Dpy,b->icon->mask,pmap_mask,trans_gc,0,0,
		      b->icon->width,b->icon->height,x_return,y_return);
	  }
	}
      else
	{
	  number=buttonNum(b);
	  XFillRectangle(Dpy,pmap_mask,trans_gc,buttonXPos(b,number),
			 buttonYPos(b,number),
			 buttonWidth(b),buttonHeight(b));
	}

      /* handle button's title */
      font=buttonFont(b);
      if(b->flags&b_Title && font)
	{
	  XSetFont(Dpy,trans_gc,font->fid);
	  DrawTitle(b,pmap_mask,trans_gc);
	}
    }
  XShapeCombineMask(Dpy,MyWindow,ShapeBounding,0,0,pmap_mask,ShapeSet);
}
#endif

/**
*** myErrorHandler()
*** Shows X errors made by FvwmButtons.
**/
XErrorHandler oldErrorHandler=NULL;
int myErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* some errors are acceptable, mostly they're caused by
   * trying to update a lost  window */
  if((event->error_code == BadWindow)||(event->request_code == X_GetGeometry)||
     (event->error_code==BadDrawable)||(event->request_code==X_GrabButton))
    return 0 ;

  fprintf(stderr,"%s: Cause of next X Error.\n",MyName);
  /* return (*oldErrorHandler)(dpy,event); */
  return 0;
}

/* ---------------------------------- main ----------------------------------*/

/**
*** main()
**/
int main(int argc, char **argv)
{
  int i;
  Window root;
  int x,y,maxx,maxy,border_width,depth;
  button_info *b,*ub;
  panel_info *LastPanel;

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif
  MyName = GetFileNameFromPath(argv[0]);

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGTERM);
# ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
# else
    sigact.sa_flags = 0;
# endif
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
  }
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
                     sigmask(SIGINT)  |
                     sigmask(SIGHUP)  |
                     sigmask(SIGQUIT) |
                     sigmask(SIGTERM) );
#endif

  signal(SIGPIPE, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif

  if(argc<6 || argc>8)
    {
      fprintf(stderr,"%s v%s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  if(argc>6) /* There is a naming argument here! */
    {
      free(MyName);
      MyName=strdup(argv[6]);
    }

  if(argc>7) /* There is a config file here! */
    {
      config_file=strdup(argv[7]);
    }

  fd[0]=atoi(argv[1]);
  fd[1]=atoi(argv[2]);
  if (!(Dpy = XOpenDisplay(NULL)))
    {
      fprintf(stderr,"%s: Can't open display %s", MyName,
	      XDisplayName(NULL));
      exit (1);
    }
  InitPictureCMap(Dpy);

  x_fd=XConnectionNumber(Dpy);
  fd_width=GetFdWidth();

  screen=DefaultScreen(Dpy);
  Root=RootWindow(Dpy, screen);
  if(Root==None)
    {
      fprintf(stderr,"%s: Screen %d is not valid\n",MyName,screen);
      exit(1);
    }

  oldErrorHandler=XSetErrorHandler(myErrorHandler);

  UberButton=(button_info*)mymalloc(sizeof(button_info));
  memset(UberButton, 0, sizeof(button_info));
  UberButton->BWidth=1;
  UberButton->BHeight=1;
  MakeContainer(UberButton);

  dpw = DisplayWidth(Dpy,screen);
  dph = DisplayHeight(Dpy,screen);

# ifdef DEBUG_INIT
  fprintf(stderr,"%s: Parsing...",MyName);
# endif

  CurrentPanel = MainPanel
               = (panel_info *) mymalloc(sizeof(panel_info));
  memset(CurrentPanel, 0, sizeof(panel_info));
  MainPanel->next = NULL;
  MainPanel->uber = UberButton;
  UberButton->title   = MyName;
  UberButton->swallow = 1; /* the panel is shown */

  ParseOptions(UberButton);

  for (CurrentPanel = MainPanel, LastPanel = NULL;
       CurrentPanel != NULL;
       LastPanel = CurrentPanel, CurrentPanel = CurrentPanel->next)
  {
    UberButton = CurrentPanel->uber;

    /* Don't quit if only a subpanel is empty */
    if(UberButton->c->num_buttons==0)
      {
	if (LastPanel == NULL)
	  {
	    fprintf(stderr,"%s: No buttons defined. Quitting\n", MyName);
	    exit(0);
	  }
	else
	  {
	    /* Remove empty panel and leak any memory in the uber button */
	    LastPanel->next = CurrentPanel->next;
	    continue;
	  }
      }

# ifdef DEBUG_INIT
    fprintf(stderr,"OK\n%s: Shuffling...",MyName);
# endif

    ShuffleButtons(UberButton);
    NumberButtons(UberButton);

# ifdef DEBUG_INIT
    fprintf(stderr,"OK\n%s: Loading data...\n",MyName);
# endif

    /* Load fonts and icons, calculate max buttonsize */
    maxx=0;maxy=0;
    RecursiveLoadData(UberButton,&maxx,&maxy);

# ifdef DEBUG_INIT
    fprintf(stderr,"%s: Creating main window...",MyName);
# endif

    CreateWindow(UberButton,maxx,maxy);

    CurrentPanel->uber->IconWinParent = MyWindow;
    CurrentPanel->uber->icon_w = maxx;
    CurrentPanel->uber->icon_h = maxy;

# ifdef DEBUG_INIT
    fprintf(stderr,"OK\n%s: Creating icon windows...",MyName);
# endif

    i=-1;ub=UberButton;
    while(NextButton(&ub,&b,&i,0))
      if(b->flags&b_Icon)
	{
#ifdef DEBUG_INIT
	  fprintf(stderr,"0x%06x...",(ushort)b);
#endif
	  CreateIconWindow(b);
	}

# ifdef DEBUG_INIT
    fprintf(stderr,"OK\n%s: Configuring windows...",MyName);
# endif

    XGetGeometry(Dpy,MyWindow,&root,&x,&y,(ushort*)&Width,(ushort*)&Height,
		 (ushort*)&border_width,(ushort*)&depth);
    SetButtonSize(UberButton,Width,Height);
    i=-1;ub=UberButton;
    while(NextButton(&ub,&b,&i,0))
      ConfigureIconWindow(b);

# ifdef SHAPE
    if(UberButton->c->flags&b_TransBack)
      SetTransparentBackground(UberButton,Width,Height);
# endif

    i=-1;ub=UberButton;
    while(NextButton(&ub,&b,&i,0))
      MakeButton(b);
  }
  CurrentPanel = MainPanel;
  UberButton   = CurrentPanel->uber;
  MyWindow     = UberButton->IconWinParent;

# ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Mapping windows...",MyName);
# endif

  XMapSubwindows(Dpy,MyWindow);
  XMapWindow(Dpy,MyWindow);

  SetMessageMask(fd, M_NEW_DESK | M_END_WINDOWLIST | M_MAP | M_WINDOW_NAME
		 | M_RES_CLASS | M_CONFIG_INFO | M_END_CONFIG_INFO | M_RES_NAME
		 | M_SENDCONFIG);

  /* request a window list, since this triggers a response which
   * will tell us the current desktop and paging status, needed to
   * indent buttons correctly */
  MySendText(fd,"Send_WindowList",0);

# ifdef DEBUG_INIT
  fprintf(stderr,"OK\n%s: Startup complete\n",MyName);
# endif

  /*
  ** Now that we have finished initialising everything,
  ** it is safe(r) to install the clean-up handlers ...
  */
  atexit(DeadPipeCleanup);
  Loop();

  return 0;
}

/* -------------------------------- Main Loop -------------------------------*/

/**
*** Loop
**/
void Loop(void)
{
  XEvent Event;
  KeySym keysym;
  char buffer[10],*tmp,*act;
  int i,i2,button;
  button_info *ub,*b;
  panel_info *ppi;
#ifndef OLD_EXPOSE
  int ex=10000,ey=10000,ex2=0,ey2=0;
#endif

  while( !isTerminated )
    {
      if(My_XNextEvent(Dpy,&Event))
	{
	switch(Event.type)
	  {
	  case Expose:
            PanelIndex = MainPanel;
            while (PanelIndex &&
		   (PanelIndex->uber->IconWinParent != Event.xany.window))
              PanelIndex = PanelIndex->next;
            if (PanelIndex)
            { UberButton = PanelIndex->uber;
              MyWindow   = UberButton->IconWinParent;
            }
            else
              break;

#ifdef OLD_EXPOSE
		if(Event.xexpose.count == 0)
		  {
		    button=-1;ub=UberButton;
		    while(NextButton(&ub,&b,&button,1))
		      {
			if(!ready && !(b->flags&b_Container))
			  MakeButton(b);
			RedrawButton(b,1);
		      }
		    if(!ready)
		      ready++;
		  }
#else
		ex=min(ex,Event.xexpose.x);
		ey=min(ey,Event.xexpose.y);
		ex2=max(ex2,Event.xexpose.x+Event.xexpose.width);
		ey2=max(ey2,Event.xexpose.y+Event.xexpose.height);

		if(Event.xexpose.count==0)
		  {
		    button=-1;ub=UberButton;
		    while(NextButton(&ub,&b,&button,1))
		      {
			if(b->flags&b_Container)
			  {
			    x=buttonXPos(b,buttonNum(b));
			    y=buttonYPos(b,buttonNum(b));
			  }
			else
			  {
			    x=buttonXPos(b,button);
			    y=buttonYPos(b,button);
			  }
			if(!(ex > x + buttonWidth(b) || ex2 < x ||
			     ey > y + buttonHeight(b) || ey2 < y))
			  {
			    if(ready<1 && !(b->flags&b_Container))
			      MakeButton(b);
			    RedrawButton(b,1);
			  }
		      }
		    if(ready<1)
		      ready++;

		    ex=ey=10000;ex2=ey2=0;
		  }
#endif
	    break;

	  case ConfigureNotify:
	  {
	    unsigned int depth,tw,th,border_width;
	    Window root;

	    XGetGeometry(Dpy, MyWindow, &root, &x, &y,
			 (ushort*)&tw,(ushort*)&th,
			 (ushort*)&border_width,(ushort*)&depth);
	    if(tw!=Width || th!=Height)
	      {
		Width=tw;
		Height=th;
		SetButtonSize(UberButton,Width,Height);
		button=-1;ub=UberButton;
		while(NextButton(&ub,&b,&button,0))
		  MakeButton(b);
		RedrawWindow(NULL);
	      }
	  }
	  break;

	  case KeyPress:
	    XLookupString(&Event.xkey,buffer,10,&keysym,0);
	    if(keysym!=XK_Return && keysym!=XK_KP_Enter && keysym!=XK_Linefeed)
	      break;	                /* fall through to ButtonPress */

	  case ButtonPress:
            PanelIndex = MainPanel;
            b = NULL;
            do
              if (PanelIndex->uber->swallow) /* is the panel shown? */
              {
                UberButton = PanelIndex->uber;
                MyWindow   = UberButton->IconWinParent;
                if (Event.xany.window == MyWindow)
		{
		  CurrentButton = b =
		    select_button(UberButton,Event.xbutton.x,Event.xbutton.y);
		}
              }
            while (!b && PanelIndex->next && (PanelIndex = PanelIndex->next));

	    if(!b || !(b->flags&b_Action) ||
	       ((act=GetButtonAction(b,Event.xbutton.button)) == NULL &&
		(act=GetButtonAction(b,0)) == NULL))
	      {
		CurrentButton=NULL;
		break;
	      }

	    /* record the panel, the button pressed */
            CurrentPanel = PanelIndex;
            UberButton = CurrentPanel->uber;
            MyWindow   = UberButton->IconWinParent;

	    RedrawButton(b,0);
	    if(strncasecmp(act,"popup",5)!=0)
	    {
	      if (strncasecmp(act, "panel-", 6) == 0)
                Slide(seekpanel(b), b);
	      break;
            }
	    else /* i.e. action is Popup */
	      XUngrabPointer(Dpy,CurrentTime); /* And fall through */

	  case KeyRelease:
	  case ButtonRelease:
            PanelIndex = MainPanel;
            b = NULL;
            do
            { if (PanelIndex->uber->swallow)
              {
		UberButton = PanelIndex->uber;
                MyWindow   = UberButton->IconWinParent;
                if (Event.xany.window == MyWindow)
		  b=select_button(UberButton,Event.xbutton.x,Event.xbutton.y);
              }
            } while (!b && (PanelIndex = PanelIndex->next));

	    if(!(act=GetButtonAction(b,Event.xbutton.button)))
	      act=GetButtonAction(b,0);
	    if(b && b==CurrentButton && act)
	      {
		if(strncasecmp(act,"Exec",4)==0)
		  {
		    /* close current subpanel */
		    if (PanelIndex != MainPanel &&
			!PanelIndex->flags.stay_up_on_select)
		      Slide(PanelIndex, NULL);

		    /* Look for Exec "identifier", in which case the button
		       stays down until window "identifier" materializes */
		    i=4;
		    while(isspace((unsigned char)act[i]))
		      i++;
		    if(act[i] == '"')
		      {
			i2=i+1;
			while(act[i2]!=0 && act[i2]!='"')
			  i2++;

			if(i2-i>1)
			  {
			    b->flags|=b_Hangon;
			    b->hangon = mymalloc(i2-i);
			    strncpy(b->hangon,&act[i+1],i2-i-1);
			    b->hangon[i2-i-1] = 0;
			  }
			i2++;
		      }
		    else
		      i2=i;

		    tmp=mymalloc(strlen(act)+1);
		    strcpy(tmp,"Exec ");
		    while(act[i2]!=0 && isspace((unsigned char)act[i2]))
		      i2++;
		    strcat(tmp,&act[i2]);
		    MySendText(fd,tmp,0);
		    free(tmp);
		  } /* exec */
		else if(strncasecmp(act,"DumpButtons",11)==0)
		  DumpButtons(UberButton);
		else if(strncasecmp(act,"SaveButtons",11)==0)
		  SaveButtons(UberButton);
		else if(strncasecmp(act,"panel",5) != 0)
		  MySendText(fd,act,0);
		if(strncasecmp(act,"panel",5) != 0 &&
		   PanelIndex != MainPanel &&
		   PanelIndex->flags.close_on_select)
		  Slide(PanelIndex, NULL);
	      } /* act */

            /* recover the old record */
	    /* the panel, the button pressed */
            UberButton = CurrentPanel->uber;
            MyWindow   = UberButton->IconWinParent;

	    b=CurrentButton;
	    CurrentButton=NULL;
	    if(b)
	      RedrawButton(b,0);
	    break;

	  case ClientMessage:
	    if(Event.xclient.format==32 &&
	       Event.xclient.data.l[0]==_XA_WM_DEL_WIN)
	      {
		for (ppi = MainPanel->next; ppi != NULL;
		     ppi = ppi->next)
		  {
		    if (ppi->uber->IconWinParent == Event.xany.window)
		      {
			/* Only close the panel */
			Slide(ppi, NULL);
			break;
		      }
		  }
		if (ppi == NULL)
		  DeadPipe(1);
	      }
	    break;

	  case PropertyNotify:
	    if(Event.xany.window==None)
	      break;
	    ub=UberButton;button=-1;
	    while(NextButton(&ub,&b,&button,0))
	      if((buttonSwallowCount(b)==3) && Event.xany.window==b->IconWin)
		{
		  if(Event.xproperty.atom==XA_WM_NAME &&
		     buttonSwallow(b)&b_UseTitle)
		    {
		      if(b->flags&b_Title)
			free(b->title);
		      b->flags|=b_Title;
		      XFetchName(Dpy,b->IconWin,&tmp);
		      CopyString(&b->title,tmp);
		      XFree(tmp);
		      MakeButton(b);
		    }
		  else if((Event.xproperty.atom==XA_WM_NORMAL_HINTS) &&
		     (!(buttonSwallow(b)&b_NoHints)))
		    {
		      long supp;
		      if(!XGetWMNormalHints(Dpy,b->IconWin,b->hints,&supp))
			b->hints->flags = 0;
		      MakeButton(b);
		    }
		  RedrawButton(b,1);
		}
	    break;

	  /* Not really sure if this is abandon all hope.. */
	  /* case UnmapNotify: */
	  case DestroyNotify:
	    ub=UberButton;button=-1;
	    while(NextButton(&ub,&b,&button,0))
	      if((buttonSwallowCount(b)==3) && Event.xany.window==b->IconWin)
		{
#                 ifdef DEBUG_HANGON
		  fprintf(stderr,
			  "%s: Button 0x%06x lost its window 0x%x (\"%s\")",
			  MyName,(ushort)b,(ushort)b->IconWin,b->hangon);
#                 endif
		  b->swallow&=~b_Count;
		  b->IconWin=None;
		  if(buttonSwallow(b)&b_Respawn && b->hangon && b->spawn)
		    {
#                     ifdef DEBUG_HANGON
		      fprintf(stderr,", respawning\n");
#                     endif
		      b->swallow|=1;
		      b->flags|=b_Swallow|b_Hangon;
		      MySendText(fd,b->spawn,0);
		    }
		  else
		    {
		      b->flags&=~b_Swallow;
#                     ifdef DEBUG_HANGON
		      fprintf(stderr,"\n");
#                     endif
		    }
		  break;
		}
	    break;

	  default:
#           ifdef DEBUG_EVENTS
	    fprintf(stderr,"%s: Event fell through unhandled\n",MyName);
#           endif
	    break;
	  }
      }
    }
}

/**
*** RedrawWindow()
*** Draws the window by traversing the button tree, draws all if NULL is given,
*** otherwise only the given button.
**/
void RedrawWindow(button_info *b)
{
  int button;
  XEvent dummy;
  button_info *ub;
  static Bool initial_redraw = True;
  Bool clear_buttons;

  if(ready<1)
    return;

  /* Flush expose events */
  while (XCheckTypedWindowEvent (Dpy, MyWindow, Expose, &dummy))
    ;

  if(b)
    {
      RedrawButton(b,0);
      return;
    }

  /* Clean out the entire window first */
  if (initial_redraw == False)
  {
    XClearWindow(Dpy,MyWindow);
    clear_buttons = False;
  }
  else
  {
    initial_redraw = False;
    clear_buttons = True;
  }

  button=-1;
  ub=UberButton;
  while(NextButton(&ub,&b,&button,1))
    RedrawButton(b,1);
}

/**
*** LoadIconFile()
**/
int LoadIconFile(char *s,Picture **p)
{
  *p=CachePicture(Dpy,Root,imagePath,s, save_color_limit);
  if(*p)
    return 1;
  return 0;
}

/**
*** RecursiveLoadData()
*** Loads colors, fonts and icons, and calculates buttonsizes
**/
void RecursiveLoadData(button_info *b,int *maxx,int *maxy)
{
  int i,j,x=0,y=0;
  XFontStruct *font;
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
  XFontSet fontset;
#endif

  if(!b) return;

#ifdef DEBUG_LOADDATA
  fprintf(stderr,"%s: Loading: Button 0x%06x: colors",MyName,(ushort)b);
#endif

  /* Load colors */
  if(b->flags&b_Fore)
    b->fc=GetColor(b->fore);
  if(b->flags&b_Back)
    {
      if(b->flags&b_IconBack)
	{
	  if(!LoadIconFile(b->back,&b->backicon))
	    b->flags&=~b_Back;
	}
      else
	{
	  b->bc=GetColor(b->back);
	  b->hc=GetHilite(b->bc);
	  b->sc=GetShadow(b->bc);
	}
    }
  if(b->flags&b_Container)
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", colors2");
#     endif
      if(b->c->flags&b_Fore)
	b->c->fc=GetColor(b->c->fore);
      if(b->c->flags&b_Back)
	{
	  if(b->c->flags&b_IconBack && !(b->c->flags&b_TransBack))
	    {
	      if(!LoadIconFile(b->c->back_file,&b->c->backicon))
		b->c->flags&=~b_IconBack;
	    }

	    {
	      b->c->bc=GetColor(b->c->back);
	      b->c->hc=GetHilite(b->c->bc);
	      b->c->sc=GetShadow(b->c->bc);
	    }
	}
    }

  /* Load the font */
  if(b->flags&b_Font)
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", font \"%s\"",b->font_string);
#     endif

      if(strncasecmp(b->font_string,"none",4)==0)
	b->font=NULL;
#ifdef I18N_MB
      else if(!(b->fontset=XCreateFontSet(Dpy,b->font_string,&ml,&mc,&ds)))
	{
	  b->font = NULL;
	  b->flags&=~b_Font;
	  fprintf(stderr,"%s: Couldn't load fontset %s\n",MyName,
		  b->font_string);
	}
      else
	{
	  /* fontset found */
	  XFontsOfFontSet(fontset, &fs_list, &ml);
	  b->font = fs_list[0];
	}
#else
      else if(!(b->font=XLoadQueryFont(Dpy,b->font_string)))
	{
	  b->flags&=~b_Font;
	  fprintf(stderr,"%s: Couldn't load font %s\n",MyName,
		  b->font_string);
	}
#endif
    }

  if(b->flags&b_Container && b->c->flags&b_Font)
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", font2 \"%s\"",b->c->font_string);
#     endif
      if(strncasecmp(b->c->font_string,"none",4)==0)
	b->c->font=NULL;
#ifdef I18N_MB
      else if(!(b->c->fontset=XCreateFontSet(Dpy,b->c->font_string,&ml,&mc,&ds)))
	{
	  fprintf(stderr,"%s: Couldn't load fontset %s\n",MyName,
		  b->c->font_string);
	  if(b==UberButton)
	    {
#ifdef STRICTLY_FIXED
	      if(!(b->c->fontset=XCreateFontSet(Dpy,"fixed",&ml,&mc,&ds))) {
#else
	      if(!(b->c->fontset=XCreateFontSet(Dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds))) {
#endif
		fprintf(stderr,"%s: Couldn't load fontset fixed\n",MyName);
		b->c->font = NULL;
	      }
	    }
	  else {
	    b->c->font = NULL;
	    b->c->flags&=~b_Font;
	  }
	}
      else
	{
	  /* fontset found */
	  XFontsOfFontSet(b->c->fontset, &fs_list, &ml);
	  b->c->font = fs_list[0];
	}
#else
      else if(!(b->c->font=XLoadQueryFont(Dpy,b->c->font_string)))
	{
	  fprintf(stderr,"%s: Couldn't load font %s\n",MyName,
		  b->c->font_string);
	  if(b==UberButton)
	    {
	      if(!(b->c->font=XLoadQueryFont(Dpy,"fixed")))
		fprintf(stderr,"%s: Couldn't load font fixed\n",MyName);
	    }
	  else
	    b->c->flags&=~b_Font;
	}
#endif
    }



  /* Calculate subbutton sizes */
  if(b->flags&b_Container && b->c->num_buttons)
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", entering container\n");
#     endif
      for(i=0;i<b->c->num_buttons;i++)
	if(b->c->buttons[i])
	  RecursiveLoadData(b->c->buttons[i],&x,&y);

      if(b->c->flags&b_Size)
	{
	  x=b->c->minx;
	  y=b->c->miny;
	}
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,"%s: Loading: Back to container 0x%06x",MyName,(ushort)b);
#     endif

      b->c->ButtonWidth=x;
      b->c->ButtonHeight=y;
      x*=b->c->num_columns;
      y*=b->c->num_rows;
    }



  i=0;j=0;

  /* Load the icon */
  if(b->flags&b_Icon && LoadIconFile(b->icon_file,&b->icon))
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", icon \"%s\"",b->icon_file);
#     endif
      i=b->icon->width;
      j=b->icon->height;
    }
  else
    b->flags&=~b_Icon;

#ifdef I18N_MB
  if(b->flags&b_Title && (fontset = buttonFontSet(b)) && (font=buttonFont(b)))
#else
  if(b->flags&b_Title && (font=buttonFont(b)))
#endif
    {
#     ifdef DEBUG_LOADDATA
      fprintf(stderr,", title \"%s\"",b->title);
#     endif
      if(buttonJustify(b)&b_Horizontal)
	{
	  i+=buttonXPad(b)+XTextWidth(font,b->title,strlen(b->title));
	  j=max(j,font->ascent+font->descent);
	}
      else
	{
	  i=max(i,XTextWidth(font,b->title,strlen(b->title)));
	  j+=font->ascent+font->descent;
	}
    }

  x+=i;
  y+=j;

  if(b->flags&b_Size)
    {
      x=b->minx;
      y=b->miny;
    }

  x+=2*(buttonFrame(b)+buttonXPad(b));
  y+=2*(buttonFrame(b)+buttonYPad(b));

  x/=b->BWidth;
  y/=b->BHeight;

  *maxx=max(x,*maxx);
  *maxy=max(y,*maxy);
# ifdef DEBUG_LOADDATA
  fprintf(stderr,", size %ux%u, done\n",x,y);
# endif
}

/**
*** CreateWindow()
*** Sizes and creates the window
**/
void CreateWindow(button_info *ub,int maxx,int maxy)
{
  XSizeHints mysizehints;
  XGCValues gcv;
  unsigned long gcm;
  XClassHint myclasshints;
  XSetWindowAttributes xswa;

  x = CurrentPanel->uber->x; /* Geometry x where to put the panel */
  y = CurrentPanel->uber->y; /* Geometry y where to put the panel */
  xneg = CurrentPanel->uber->w;
  yneg = CurrentPanel->uber->h;

  if(maxx<16)
    maxx=16;
  if(maxy<16)
    maxy=16;

# ifdef DEBUG_INIT
  fprintf(stderr,"making atoms...");
# endif

  _XA_WM_DEL_WIN = XInternAtom(Dpy,"WM_DELETE_WINDOW",0);
  _XA_WM_PROTOCOLS = XInternAtom (Dpy, "WM_PROTOCOLS",0);

# ifdef DEBUG_INIT
  fprintf(stderr,"sizing...");
# endif

  mysizehints.flags = PWinGravity | PResizeInc | PBaseSize;

  mysizehints.base_width=mysizehints.base_height=0;

/* This should never be executed anyway, let's remove it.
  if(ub->flags&b_Frame)
    {
      mysizehints.base_width+=2*abs(ub->framew);
      mysizehints.base_height+=2*abs(ub->framew);
    }
  if(ub->flags&b_Padding)
    {
      mysizehints.base_width+=2*ub->xpad;
      mysizehints.base_height+=2*ub->ypad;
    }
*/
  mysizehints.width=mysizehints.base_width+maxx;
  mysizehints.height=mysizehints.base_height+maxy;
  mysizehints.width_inc=ub->c->num_columns;
  mysizehints.height_inc=ub->c->num_rows;
  mysizehints.base_height+=ub->c->num_rows*2;
  mysizehints.base_width+=ub->c->num_columns*2;

  if(w>-1) /* from geometry */
    {
# ifdef DEBUG_INIT
  fprintf(stderr,"constraining (w=%i)...",w);
# endif
      ConstrainSize(&mysizehints,&w,&h);
      mysizehints.width = w;
      mysizehints.height = h;
      mysizehints.flags |= USSize;
    }

# ifdef DEBUG_INIT
  fprintf(stderr,"gravity...");
# endif
  mysizehints.x=0;
  mysizehints.y=0;
  if(x > -30000)
    {
      if (xneg)
	{
	  mysizehints.x = DisplayWidth(Dpy,screen) + x - mysizehints.width;
	  gravity = NorthEastGravity;
	}
      else
	mysizehints.x = x;
      if (yneg)
	{
	  mysizehints.y = DisplayHeight(Dpy,screen) + y - mysizehints.height;
	  gravity = SouthWestGravity;
	}
      else
	mysizehints.y = y;
      if(xneg && yneg)
	gravity = SouthEastGravity;
      mysizehints.flags |= USPosition;
    }
  mysizehints.win_gravity = gravity;

# ifdef DEBUG_INIT
  if(mysizehints.flags&USPosition)
    fprintf(stderr,"create(%i,%i,%u,%u,1,%u,%u)...",
	    mysizehints.x,mysizehints.y,
	    mysizehints.width,mysizehints.height,
	    (ushort)fore_pix,(ushort)back_pix);
  else
    fprintf(stderr,"create(-,-,%u,%u,1,%u,%u)...",
	    mysizehints.width,mysizehints.height,
	    (ushort)fore_pix,(ushort)back_pix);
# endif

  xswa.colormap = Pcmap;
  xswa.border_pixel = 0;
  xswa.background_pixmap = None;
  MyWindow = XCreateWindow(Dpy,Root,mysizehints.x,mysizehints.y,
			   mysizehints.width,mysizehints.height,0,Pdepth,
			   InputOutput,Pvisual,
			   CWColormap|CWBackPixmap|CWBorderPixel,&xswa);

# ifdef DEBUG_INIT
  fprintf(stderr,"colors...");
# endif
  if(Pdepth < 2) {
    back_pix = GetColor("white");
    fore_pix = GetColor("black");
    hilite_pix = back_pix;
    shadow_pix = fore_pix;
  } else {
    back_pix = GetColor(ub->c->back);
    fore_pix = GetColor(ub->c->fore);
    hilite_pix = GetHilite(back_pix);
    shadow_pix = GetShadow(back_pix);
  }
  if(ub->c->flags&b_IconBack && !(ub->c->flags&b_TransBack)) {
    XSetWindowBackgroundPixmap(Dpy,MyWindow,ub->c->backicon->picture);
  } else {
    XSetWindowBackground(Dpy,MyWindow,back_pix);
  }



# ifdef DEBUG_INIT
  fprintf(stderr,"properties...");
# endif
  XSetWMProtocols(Dpy,MyWindow,&_XA_WM_DEL_WIN,1);

#if 0
  myclasshints.res_name=strdup((CurrentPanel == MainPanel)
                                 ? MyName : CurrentPanel->uber->title);
#else
  if (CurrentPanel == MainPanel)
  {
    myclasshints.res_name=strdup(MyName);
  }
  else
  {
    myclasshints.res_name=(char *)malloc(strlen(MyName)+6);
    strcpy(myclasshints.res_name,MyName);
    strcat(myclasshints.res_name,"Panel");
  }
#endif
  myclasshints.res_class=strdup((CurrentPanel == MainPanel)
                                 ? "FvwmButtons" : "FvwmButtonsPanel");

  {
    XTextProperty mynametext;
    char *list[]={NULL,NULL};
    list[0]=(CurrentPanel == MainPanel) ? MyName : CurrentPanel->uber->title;
    if(!XStringListToTextProperty(list,1,&mynametext))
      {
	fprintf(stderr,"%s: Failed to convert name to XText\n",MyName);
	exit(1);
      }
    XSetWMProperties(Dpy,MyWindow,&mynametext,&mynametext,
		     NULL,0,&mysizehints,NULL,&myclasshints);
    XFree(mynametext.value);
  }

  XSelectInput(Dpy,MyWindow,MW_EVENTS);

# ifdef DEBUG_INIT
  fprintf(stderr,"GC...");
# endif
  gcm = GCForeground|GCBackground;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  if(ub && ub->c && ub->c->font && ub->font)
    {
      gcv.font = ub->c->font->fid;
      gcm |= GCFont;
    }
  NormalGC = XCreateGC(Dpy, MyWindow, gcm, &gcv);
  gcv.foreground = shadow_pix;
  gcv.background = fore_pix;
  ShadowGC = XCreateGC(Dpy, MyWindow, gcm, &gcv);

  free(myclasshints.res_class);
  free(myclasshints.res_name);
}

/* ----------------------------- color functions --------------------------- */

#ifdef XPM
#  define MyAllocColor(a,b,c) PleaseAllocColor(c)
#else
#  define MyAllocColor(a,b,c) XAllocColor(a,b,c)
#endif

/**
*** PleaseAllocColor()
*** Allocs a color through XPM, which does it's closeness thing if out of
*** space.
**/
#ifdef XPM
int PleaseAllocColor(XColor *color)
{
  char *xpm[] = {"1 1 1 1",NULL,"x"};
  XpmAttributes attr;
  XImage *dummy1=None,*dummy2=None;
  static char buf[20];

  sprintf(buf,"x c #%04x%04x%04x",color->red,color->green,color->blue);
  xpm[1]=buf;
  attr.valuemask=XpmCloseness|XpmVisual|XpmColormap|XpmDepth;
  attr.closeness=40000; /* value used by fvwm and fvwmlib */
  attr.visual = Pvisual;
  attr.colormap = Pcmap;
  attr.depth = Pdepth;

  if(XpmCreateImageFromData(Dpy,xpm,&dummy1,&dummy2,&attr)!=XpmSuccess)
    {
      fprintf(stderr,"%s: Unable to get similar color\n",MyName);
      exit(1);
    }
  color->pixel=XGetPixel(dummy1,0,0);
  if(dummy1!=None)XDestroyImage(dummy1);
  if(dummy2!=None)XDestroyImage(dummy2);
  return 1;
}
#endif

/**
*** nocolor()
*** Complain
**/
void nocolor(const char *a, const char *b)
{
  fprintf(stderr,"%s: Can't %s %s, quitting, sorry...\n", MyName, a,b);
  exit(1);
}


/* --------------------------------------------------------------------------*/

#ifdef DEBUG_EVENTS
void DebugEvents(XEvent *event)
{
  char *event_names[]={NULL,NULL,
			 "KeyPress","KeyRelease","ButtonPress",
			 "ButtonRelease","MotionNotify","EnterNotify",
			 "LeaveNotify","FocusIn","FocusOut",
			 "KeymapNotify","Expose","GraphicsExpose",
			 "NoExpose","VisibilityNotify","CreateNotify",
			 "DestroyNotify","UnmapNotify","MapNotify",
			 "MapRequest","ReparentNotify","ConfigureNotify",
			 "ConfigureRequest","GravityNotify","ResizeRequest",
			 "CirculateNotify","CirculateRequest","PropertyNotify",
			 "SelectionClear","SelectionRequest","SelectionNotify",
			 "ColormapNotify","ClientMessage","MappingNotify"};
  fprintf(stderr,"%s: Received %s event from window 0x%x\n",
	  MyName,event_names[event->type],(ushort)event->xany.window);
}
#endif

#ifdef DEBUG_FVWM
void DebugFvwmEvents(unsigned long type)
{
  char *events[]={
"M_NEW_PAGE",
"M_NEW_DESK",
"M_ADD_WINDOW",
"M_RAISE_WINDOW",
"M_LOWER_WINDOW",
"M_CONFIGURE_WINDOW",
"M_FOCUS_CHANGE",
"M_DESTROY_WINDOW",
"M_ICONIFY",
"M_DEICONIFY",
"M_WINDOW_NAME",
"M_ICON_NAME",
"M_RES_CLASS",
"M_RES_NAME",
"M_END_WINDOWLIST",
"M_ICON_LOCATION",
"M_MAP",
"M_ERROR",
"M_CONFIG_INFO",
"M_END_CONFIG_INFO",
"M_ICON_FILE",
"M_DEFAULTICON",NULL};
  int i=0;
  while(events[i])
    {
      if(type&1<<i)
	fprintf(stderr,"%s: Received %s message from fvwm\n",MyName,events[i]);
      i++;
    }
}
#endif

/**
*** My_XNextEvent()
*** Waits for next X event, or for an auto-raise timeout.
**/
int My_XNextEvent(Display *Dpy, XEvent *event)
{
  fd_set in_fdset;
  static int miss_counter = 0;

  if(XPending(Dpy))
    {
      XNextEvent(Dpy,event);
#     ifdef DEBUG_EVENTS
      DebugEvents(event);
#     endif
      return 1;
    }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  if (fvwmSelect(fd_width,SELECT_FD_SET_CAST &in_fdset, 0, 0, NULL) > 0)
  {

  if(FD_ISSET(x_fd, &in_fdset))
    {
      if(XPending(Dpy))
	{
	  XNextEvent(Dpy,event);
	  miss_counter = 0;
#         ifdef DEBUG_EVENTS
	  DebugEvents(event);
#         endif
	  return 1;
	}
      else
	miss_counter++;
      if(miss_counter > 100)
	DeadPipe(0);
    }

  if(FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket* packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
	  DeadPipe(0);
      else
	  process_message( packet->type, packet->body );
    }

  }
  return 0;
}

/**
*** SpawnSome()
*** Is run after the windowlist is checked. If any button hangs on UseOld,
*** it has failed, so we try to spawn a window for them.
**/
void SpawnSome(void)
{
  static char first=1;
  button_info *b,*ub=UberButton;
  int button=-1;
  if(!first)
    return;
  first=0;
  while(NextButton(&ub,&b,&button,0))
    if((buttonSwallowCount(b)==1) && b->flags&b_Hangon &&
       buttonSwallow(b)&b_UseOld)
      if(b->spawn)
	{
#         ifdef DEBUG_HANGON
	  fprintf(stderr,"%s: Button 0x%06x did not find a \"%s\" window, %s",
		  MyName,(ushort)b,b->hangon,"spawning own\n");
#         endif
	  SendText(fd,b->spawn,0);
	}
}

/**
*** process_message()
*** Process window list messages
**/
void process_message(unsigned long type,unsigned long *body)
{
# ifdef DEBUG_FVWM
  DebugFvwmEvents(type);
# endif
  panel_info *PanelIndex = MainPanel;
  do
  {
    UberButton = PanelIndex->uber;
    MyWindow   = UberButton->IconWinParent;

  switch(type)
    {
    case M_NEW_DESK:
      new_desk = body[0];
      RedrawWindow(NULL);
      break;
    case M_END_WINDOWLIST:
      SpawnSome();
      RedrawWindow(NULL);
      break;
    case M_MAP:
      swallow(body);
      break;
    case M_RES_NAME:
    case M_RES_CLASS:
    case M_WINDOW_NAME:
      CheckForHangon(body);
      break;
    case M_CONFIG_INFO:
      {
/* this is where you put the stuff for FvwmButtons to follow colorsets
   in real time
   	  if (UberButton->c->flags & b_FvwmLook))
	  SetWindowBackground(Dpy, MyWindow, UberButton->c->ButtonWidth
			      * UberButton->c->num_columns,
			      UberButton->c->ButtonHeight
			      * UberButton->c->num_rows, G->bg, G->depth,
			      G->foreGC);
*/
      }
      break;
    default:
      break;
    }

  } while ((PanelIndex = PanelIndex->next));
}


/* --------------------------------- swallow code -------------------------- */

/**
*** CheckForHangon()
*** Is the window here now?
**/
void CheckForHangon(unsigned long *body)
{
  button_info *b,*ub=UberButton;
  int button=-1;
  ushort d;
  char *cbody;
  cbody = (char *)&body[3];

  while(NextButton(&ub,&b,&button,0))
    if(b->flags&b_Hangon && strcmp(cbody,b->hangon)==0)
      {
	/* Is this a swallowing button in state 1? */
	if(buttonSwallowCount(b)==1)
	  {
	    b->swallow&=~b_Count;
	    b->swallow|=2;
	    b->IconWin=(Window)body[0];
	    b->flags&=~b_Hangon;

            /* We get the parent of the window to compare with later... */
            b->IconWinParent=
	      GetRealGeometry(Dpy,b->IconWin,
			      &b->x,&b->y,&b->w,&b->h,&b->bw,&d);

#           ifdef DEBUG_HANGON
	    fprintf(stderr,"%s: Button 0x%06x %s 0x%lx \"%s\", parent 0x%lx\n",
		    MyName,(ushort)b,"will swallow window",body[0],cbody,
		    b->IconWinParent);
#           endif

	    if(buttonSwallow(b)&b_UseOld)
	      swallow(body);
	  }
	else
	  {
	    /* Else it is an executing button with a confirmed kill */
#           ifdef DEBUG_HANGON
	    fprintf(stderr,"%s: Button 0x%06x %s 0x%lx \"%s\", released\n",
		    MyName,(int)b,"hung on window",body[0],cbody);
#           endif
	    b->flags&=~b_Hangon;
	    free(b->hangon);
	    b->hangon=NULL;
	    RedrawButton(b,0);

	  }
	break;
      }
    else if(buttonSwallowCount(b)>=2 && (Window)body[0]==b->IconWin)
      break;      /* This window has already been swallowed by someone else! */
}

/**
*** GetRealGeometry()
*** Traverses window tree to find the real x,y of a window. Any simpler?
*** Returns parent window, or None if failed.
**/
Window GetRealGeometry(Display *dpy,Window win,int *x,int *y,ushort *w,
		     ushort *h,ushort *bw,ushort *d)
{
  Window root;
  Window rp=None;
  Window *children;
  unsigned int n;

  if(!XGetGeometry(dpy,win,&root,x,y,w,h,bw,d))
    return None;

  /* Now, x and y are not correct. They are parent relative, not root... */
  /* Hmm, ever heard of XTranslateCoordinates!? */

  XTranslateCoordinates(dpy,win,root,*x,*y,x,y,&rp);

  XQueryTree(dpy,win,&root,&rp,&children,&n);
  if (children)
    XFree(children);

  return rp;
}

/**
*** swallow()
*** Executed when swallowed windows get mapped
**/
void swallow(unsigned long *body)
{
  char *temp;
  button_info *ub=UberButton,*b;
  int button=-1;
  ushort d;
  Window p;

  while(NextButton(&ub,&b,&button,0))
    if((b->IconWin==(Window)body[0]) && (buttonSwallowCount(b)==2))
      {
	/* Store the geometry in case we need to unswallow. Get parent */
	p=GetRealGeometry(Dpy,b->IconWin,&b->x,&b->y,&b->w,&b->h,&b->bw,&d);
#       ifdef DEBUG_HANGON
	fprintf(stderr,"%s: Button 0x%06x %s 0x%lx, with parent 0x%lx\n",
		MyName,(ushort)b,"trying to swallow window",body[0],p);
#       endif

	if(p==None) /* This means the window is no more */ /* NO! wrong */
	  {
	    fprintf(stderr,"%s: Window 0x%lx (\"%s\") disappeared %s\n",
		    MyName,b->IconWin,b->hangon,"before swallow complete");
	    /* Now what? Nothing? For now: give up that button */
	    b->flags&=~(b_Hangon|b_Swallow);
	    return;
	  }

	if(p!=b->IconWinParent) /* The window has been reparented */
	  {
	    fprintf(stderr,"%s: Window 0x%lx (\"%s\") was %s (window 0x%lx)\n",
		    MyName,b->IconWin,b->hangon,"grabbed by someone else",p);

	    /* Request a new windowlist, we might have ignored another
	       matching window.. */
	    SendText(fd,"Send_WindowList",0);

	    /* Back one square and lose one turn */
	    b->swallow&=~b_Count;
	    b->swallow|=1;
	    b->flags|=b_Hangon;
	    return;
	  }

#       ifdef DEBUG_HANGON
	fprintf(stderr,"%s: Button 0x%06x swallowed window 0x%lx\n",
		MyName,(ushort)b,body[0]);
#       endif

	b->swallow&=~b_Count;
	b->swallow|=3;

	/* "Swallow" the window! Place it in the void so we don't see it
	   until it's MoveResize'd */
	XReparentWindow(Dpy,b->IconWin,MyWindow,-1500,-1500);
	XSelectInput(Dpy,b->IconWin,SW_EVENTS);
	if(buttonSwallow(b)&b_UseTitle)
	  {
	    if(b->flags&b_Title)
	      free(b->title);
	    b->flags|=b_Title;
	    XFetchName(Dpy,b->IconWin,&temp);
	    CopyString(&b->title,temp);
	    XFree(temp);
	  }
	XMapWindow(Dpy,b->IconWin);
	MakeButton(b);
	RedrawButton(b,1);
	break;
      }
}

/*
 * Button
 *   b->flags     |= b_Action (though b->hangon is ilegally used)
 *   b->action[0]  = "panel-u", "panel-l", "panel-d", or "panel-r"
 *   b->hangon     = panel title (case sensitive)
 * Panel
 *   uber->flags  |= b_Container (though the following fields are ilegally
 *                    used)
 *   uber->title   = the title of the panel
 *   uber->IconWinParent = the panel window, will be assigned to MyWindow
 *   uber->swallow = 0:hidden 1:shown
 *   uber->icon_w  = panel width
 *   uber->icon_h  = panel height
 *   uber->x       = x position to start the panel wrt the button b
 *   uber->y       = y position to start the panel wrt the button b
 *   uber->w       = xneg
 *   uber->h       = yneg
 *   uber->n       = cast to 'char' to record of the direction
 *
 *   CurrentPanel  = the panel where the button b was pressed to popup a new
 *                   panel
 */

#define PanelPopUpStep 32

panel_info *seekpanel(button_info *b)
{
  panel_info *PanelIndex = MainPanel->next; /* skip the main panel */

  while (PanelIndex && strcmp(b->hangon, PanelIndex->uber->title))
    PanelIndex = PanelIndex->next;

  return PanelIndex;
}

void Slide(panel_info *p, button_info *b)
{
  Window PanelWin;
  Window root;
  int x, y, iw, ih, BW, depth;
  char direction;
  ushort i, c, xstep = 0, ystep = 0, wstep = 0, hstep = 0;

  if (!p)
    /* no such panel */
    return;
  /* PanelWin is found */
  PanelWin = p->uber->IconWinParent;

  direction = b ? b->action[0][6] : (char) p->uber->n;

  if (p->uber->swallow)
  {
    /* shown ---> hidden */
    root = GetRealGeometry(Dpy, PanelWin, &x, &y,
                           (ushort*)&iw, (ushort*)&ih,
                           (ushort*)&BW, (ushort*)&depth);

    switch (direction)
    {
    case 'l':
      c = iw / PanelPopUpStep;
      xstep = wstep = PanelPopUpStep;
      ystep = hstep = 0;
      break;
    case 'r':
      c = iw / PanelPopUpStep;
      wstep = PanelPopUpStep;
      xstep = ystep = hstep = 0;
      break;
    case 'd':
      c = ih / PanelPopUpStep;
      hstep = PanelPopUpStep;
      xstep = ystep = wstep = 0;
      break;
    case 'g':
      /* just pop down without animation */
      c = 0;
      break;
    case 'u':
    default:
      c = ih / PanelPopUpStep;
      ystep = hstep = PanelPopUpStep;
      xstep = wstep = 0;
      break;
    }

    for (i = 1; i < c; i++)
    {
      iw -= wstep;
      ih -= hstep;
      x += xstep;
      y += ystep;
      XMoveResizeWindow(Dpy, PanelWin, x, y, iw, ih);
    }

    XUnmapWindow(Dpy, PanelWin);
    p->uber->swallow = 0;
  }
  else if (b != NULL)
  {
    /* hidden ---> shown */
    int ix = buttonXPos(b, b->n);  /* button in the CurrentPanel */
    int iy = buttonYPos(b, b->n);  /* button in the CurrentPanel */

    int mw = p->uber->icon_w;      /* panel menu width */
    int mh = p->uber->icon_h;      /* panel menu height */
    int w = mw;                    /* current width */
    int h = mh % PanelPopUpStep;   /* current height */

    root = GetRealGeometry(Dpy, CurrentPanel->uber->IconWinParent, &x, &y,
                           (ushort*)&iw, (ushort*)&ih,
                           (ushort*)&BW, (ushort*)&depth);

    x += p->uber->x + ix;
    y += p->uber->y + iy;
    c = 0;

    /* initial position and size */
    switch (direction)
    {
    case 'g':
      /* just pop up without animation */
      c = 0;
      break;
    case 'l':
    case 'r':
      h = mh;
      w = mw % PanelPopUpStep;
      if (w == 0)
      {
	w = PanelPopUpStep;
	c--;
      }
      if (direction == 'l')
      {
	x -= w;
	c += mw / PanelPopUpStep;
	xstep = wstep = PanelPopUpStep;
	ystep = hstep = 0;
      }
      else
      {
	x += b->BWidth * b->parent->c->ButtonWidth;
	c += mw / PanelPopUpStep;
	wstep = PanelPopUpStep;
	xstep = ystep = hstep = 0;
      }
      break;
    case 'd':
    case 'u':
    default:
      w = mw;
      h = mh % PanelPopUpStep;
      if (h == 0)
      {
	h = PanelPopUpStep;
	c--;
      }
      if (direction == 'd')
      {
	y += b->BHeight * b->parent->c->ButtonHeight;
	c += mh / PanelPopUpStep;
	hstep = PanelPopUpStep;
	xstep = ystep = wstep = 0;
      }
      else
      {
	y -= h;
	c += mh / PanelPopUpStep;
	ystep = hstep = PanelPopUpStep;
	xstep = wstep = 0;
      }
      break;
    }

    if (c > 0)
      XMoveResizeWindow(Dpy, PanelWin, x, y, w, h);
    XMapSubwindows(Dpy, PanelWin);
    XMapWindow(Dpy, PanelWin);

    for (i = 0; i < c; i++)
    {
      x -= xstep;
      w += wstep;
      y -= ystep;
      h += hstep;
      XMoveResizeWindow(Dpy, PanelWin, x, y, w, h);
    }

    p->uber->n = (int) direction;
    p->uber->swallow = 1;
  }
}
