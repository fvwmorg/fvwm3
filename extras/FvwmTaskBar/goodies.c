#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Display *dpy;
Window Root, win;
int screen,d_depth,ScreenWidth,ScreenHeight;
unsigned long  back, fore;
GC  graph,shadow,hilite, blackgc, whitegc;
XSizeHints hints;
XGCValues gcval;
unsigned long gcmask;
XEvent Event;
static struct itimerval timeout;
static struct sigaction action;
XFontStruct *ButtonFont;
time_t timer;


void Draw();


/******************************************************************************
  ChangeWindowName - Self explanitory
    Original work from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ChangeWindowName(char *str)
{
  XTextProperty name;
  if (XStringListToTextProperty(&str,1,&name) == 0) {
    fprintf(stderr,"goodies: cannot allocate window name.\n");
    return;
  }
  XSetWMName(dpy,win,&name);
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);
}

void alarm_handler() {
  time(&timer);
  Draw();
}



void Draw() {
  struct tm *tms;
  char str[10]; 

  time(&timer);
  tms = localtime(&timer);
  strftime(str, 10, "%R", tms);
  XClearWindow(dpy, win);
  XDrawString(dpy,win,whitegc,0,10,str, strlen(str));
}


void main(int argc, char *argv[]) {

   if (!(dpy = XOpenDisplay(""))) {
      fprintf(stderr,"%s: can't open display %s", argv[0],
	      XDisplayName(""));
      exit (1);
   }
   screen= DefaultScreen(dpy);
   Root = RootWindow(dpy, screen);
   d_depth = DefaultDepth(dpy, screen);
   
   ScreenHeight = DisplayHeight(dpy,screen);
   ScreenWidth = DisplayWidth(dpy,screen);

   back = WhitePixel(dpy, screen);
   fore = BlackPixel(dpy, screen);

   if ((ButtonFont=XLoadQueryFont(dpy,"fixed"))==NULL) exit(1);

   gcval.foreground=fore;
   gcval.background=back;
   gcval.font=ButtonFont->fid;
   gcmask=GCForeground|GCBackground;
   whitegc=XCreateGC(dpy,Root,gcmask,&gcval);
   
   gcval.foreground=back;
   gcval.background=back;
   gcval.font=ButtonFont->fid;
   gcmask=GCForeground|GCBackground;
   blackgc=XCreateGC(dpy,Root,gcmask,&gcval);
   
   hints.x = 0;
   hints.y = 0;
   hints.width = XTextWidth(ButtonFont, "XX:XX", 5);
   hints.height = ButtonFont->ascent + ButtonFont->descent;
   hints.width_inc=0;
   hints.height_inc=0;
   hints.flags|=USPosition;
   hints.win_gravity=SouthEastGravity;
   win=XCreateSimpleWindow(dpy,Root,hints.x,hints.y,hints.width,hints.height,1,
			   fore,back);
   XSetWMNormalHints(dpy,win,&hints);
   
   XSelectInput(dpy,win,(ExposureMask | KeyPressMask));
   ChangeWindowName(argv[0]);
   XMapRaised(dpy,win);

   timeout.it_interval.tv_sec = 1;
   timeout.it_interval.tv_usec = 0;
   timeout.it_value.tv_sec = 1;
   timeout.it_value.tv_usec = 0;
   action.sa_handler = (void (*)())alarm_handler;
   sigemptyset(&action.sa_mask);
   action.sa_flags=SA_RESTART;
   sigaction(SIGALRM,&action,NULL); 
   setitimer(ITIMER_REAL,&timeout,NULL); 
   
   for (;;)
   while(XPending(dpy)) {
     XNextEvent(dpy,&Event);
     if (Event.type==Expose) {
       Draw();
     }
   } 
}
