/***************************************************************************
 * FvwmBanner                                                           
 *                                                                           
 *  Show Fvwm Banner
 *                                                                           
 ***************************************************************************/

#include "../../configure.h"
#ifdef ISC
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>

#include <X11/xpm.h>


#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#include "../../libs/fvwmlib.h"     

#ifdef NEEDS_ALPHA_HEADER
#include "../../fvwm/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

#include "../../icons/fvwm2_big.xpm"
#if 0
#include "../../icons/k2.xpm"
#endif /* 0 */

#include "../../version.h"

typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
}        XpmIcon;

/**************************************************************************
 * A few function prototypes 
 **************************************************************************/
void RedrawWindow(void);
void GetXPMData(char **);
void GetXPMFile(char *,char *);
void change_window_name(char *str);
int flush_expose (Window w);
static void parseOptions (int fd[2]);

XpmIcon view;
Window win;

char *pixmapPath = NULL;
char *pixmapName = NULL;
char *myName = NULL;

int timeout = 3000000; /* default time of 3 seconds */

Display *dpy;			/* which display are we talking to */
Window Root;
int screen;
int x_fd;
int d_depth;
int ScreenWidth, ScreenHeight;
XSizeHints mysizehints;
Pixel back_pix, fore_pix;
GC NormalGC,FGC;
static Atom wm_del_win;
Colormap colormap;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask)

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
int main(int argc, char **argv)
{
  char *display_name = NULL, *string = NULL;
  int retval;
  XGCValues gcv;
  unsigned long gcm;
  XEvent Event;
  fd_set in_fdset;
  int fd_width ;
  time_t t;
  struct timeval value;
  int fd[2],i;
  int x1,x2,y1,y2;
  XRectangle rect;

  fd_width = GetFdWidth();
  
  /* Save our program  name - for error messages */
  string = strrchr (argv[0], '/');
  if (string != (char *) 0) string++;

  myName = safemalloc (strlen (string) + 1);
  strcpy (myName, string);

  if(argc>=3)
  {
      /* sever our connection with fvwm, if we have one. */
      fd[0] = atoi(argv[1]);
      fd[1] = atoi(argv[2]);

#if 0
      if(fd[0]>0)close(fd[0]);
      if(fd[1]>0)close(fd[1]);
#endif /* 0 */
  }
  else
  {
    fprintf (stderr,
	     "%s version %s should only be executed by fvwm!\n",
	     myName,
	     VERSION);
    exit(1);
  }

  if (argc > 6) {
    pixmapName = safemalloc (strlen (argv[6]) + 1);
    strcpy (pixmapName, argv[6]);
  }

  /* Open the display */
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"FvwmBanner: can't open display %s",
	      XDisplayName(display_name));
      exit (1);
    }
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  colormap = XDefaultColormap(dpy,screen);
  d_depth = DefaultDepth(dpy, screen);
  x_fd = XConnectionNumber(dpy);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  parseOptions(fd);
  
  /* Get the xpm banner */
  if (pixmapName)
    GetXPMFile(pixmapName,pixmapPath);
  else
#if 0
    if(d_depth > 4)
      GetXPMData(k2_xpm);
    else
#endif /* 0 */
      GetXPMData(fvwm2_big_xpm);

  /* Create a window to hold the banner */
  mysizehints.flags=
    USSize|USPosition|PWinGravity|PResizeInc|PBaseSize|PMinSize|PMaxSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = view.attributes.width;
  mysizehints.height=view.attributes.height;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  mysizehints.win_gravity = NorthWestGravity;

  mysizehints.x = (ScreenWidth - view.attributes.width)/2;
  mysizehints.y = (ScreenHeight - view.attributes.height)/2;

  win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
				 mysizehints.width,mysizehints.height,
				 1,fore_pix ,None);


  /* Set assorted info for the window */
  XSetTransientForHint(dpy,win,Root);
  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);

  XSetWMNormalHints(dpy,win,&mysizehints);
  change_window_name("FvwmBanner");

  XSetWindowBackgroundPixmap(dpy,win,view.pixmap);
#ifdef SHAPE
  if(view.mask != None)
    XShapeCombineMask(dpy, win, ShapeBounding,0,0,view.mask, ShapeSet);
#endif
  XMapWindow(dpy,win);
  XSync(dpy,0);
#if 0
  sleep_a_little(timeout);
#else
  XSelectInput(dpy,win,ButtonReleaseMask);
  /* Display the window */
  value.tv_usec = timeout % 1000000;
  value.tv_sec = timeout / 1000000;
  while(1)
  {
    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);

    if(!XPending(dpy))
#ifdef __hpux
      retval=select(fd_width,(int *)&in_fdset, 0, 0, &value);
#else
      retval=select(fd_width,&in_fdset, 0, 0, &value);
#endif
    if (retval==0)
    {
      XDestroyWindow(dpy,win);
      XSync(dpy,0);
      exit(0);
    }

    if(FD_ISSET(x_fd, &in_fdset))
    {
      /* read a packet */
      XNextEvent(dpy,&Event);
      switch(Event.type)
      {
        case ButtonRelease:
          XDestroyWindow(dpy,win);
          XSync(dpy,0);
          exit(0);
        case ClientMessage:
          if (Event.xclient.format==32 && Event.xclient.data.l[0]==wm_del_win)
          {
            XDestroyWindow(dpy,win);
            XSync(dpy,0);
            exit(0);
          }
        default:
          break;      
      }
    }
  }
#endif /* 0 */
  return 0;
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void GetXPMData(char **data)
{
  int code;

  view.attributes.valuemask = XpmReturnPixels| XpmCloseness | XpmExtensions;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */;
  if(XpmCreatePixmapFromData(dpy, Root, data,
                             &view.pixmap, &view.mask,
                             &view.attributes)!=XpmSuccess)
  {
    fprintf(stderr,"FvwmBanner: ERROR couldn't convert data to pixmap\n");
    exit(1);
  }
}
void GetXPMFile(char *file, char *path)
{
  int code;
  char *full_file;

  view.attributes.valuemask = XpmReturnPixels| XpmCloseness | XpmExtensions;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */;

  if (file)
    full_file = findIconFile(file,path,R_OK);

  if (full_file)
  {
    if(XpmReadFileToPixmap(dpy,
                           Root,
                           full_file,
                           &view.pixmap,
                           &view.mask,
                           &view.attributes) == XpmSuccess)
    {
      return;
    }
    fprintf(stderr,"FvwmBanner: ERROR reading pixmap file\n");
  }
  else
    fprintf(stderr,"FvwmBanner: ERROR finding pixmap file in PixmapPath\n");
  GetXPMData(fvwm2_big_xpm);
}

void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmBanner: can't %s %s\n", a,b);
}

static void parseOptions (int fd[2])
{
  char *tline= NULL;
  int   clength;

  clength = strlen (myName);

  while (GetConfigLine (fd, &tline),tline != NULL)
  {
    if (strlen (tline) > 1)
    {
      if (mystrncasecmp (tline,
			 CatString3 ("*", myName, "Pixmap"),
			 clength + 7) ==0)
      {
	if (pixmapName == (char *) 0)
        {
	  CopyString (&pixmapName, &tline[clength+7]);
	  if (pixmapName[0] == 0)
          {
	    free (pixmapName);
	    pixmapName = (char *) 0;
	  }
	}
        continue;
      }
      if (mystrncasecmp (tline,
			 CatString3 ("*", myName, "Timeout"),
			 clength + 8) ==0)
      {
        timeout = atoi(&tline[clength+8]) * 1000000;
        continue;
      }
      if (mystrncasecmp(tline, "PixmapPath",10)==0)
      {
        CopyString (&pixmapPath, &tline[10]);
        if (pixmapPath[0] == 0)
        {
          free (pixmapPath);
          pixmapPath = (char *) 0;
        }
        continue;
      }
    }
  }
  return;
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;
  
  if (XStringListToTextProperty(&str,1,&name) == 0) 
    {
      fprintf(stderr,"FvwmBanner: cannot allocate window name");
      return;
    }
  XSetWMName(dpy,win,&name);
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);
}


/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/

/*ARGSUSED*/
void DeadPipe (int nonsense)
{
  exit (0);
}

