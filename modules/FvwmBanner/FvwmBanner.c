/***************************************************************************
 * FvwmBanner
 *
 *  Show Fvwm Banner
 *
 ***************************************************************************/

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>

#include <X11/xpm.h>

#include <libs/fvwmlib.h>
#include <libs/Picture.h>
#include <libs/Module.h>


#include "icons/fvwm2_big.xpm"

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

char *imagePath = NULL;
char *imageName = NULL;
static char *MyName = NULL;
static int MyNameLen;

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
Bool no_wm = False;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask)

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
int main(int argc, char **argv)
{
  char *display_name = NULL, *string = NULL;
  int retval = 0;
  XEvent Event;
  fd_set in_fdset;
  fd_set_size_t fd_width = GetFdWidth();
  struct timeval value;
  int fd[2];
  XSetWindowAttributes attr;

  /* Save our program  name - for error messages */
  string = strrchr (argv[0], '/');
  if (string != (char *) 0) string++;

  MyNameLen=strlen(string)+1;		/* account for '*' */
  MyName = safemalloc(MyNameLen+1);	/* account for \0 */
  *MyName='*';
  strcpy (MyName+1, string);

  if(argc>=3)
  {
      /* sever our connection with fvwm, if we have one. */
      fd[0] = atoi(argv[1]);
      fd[1] = atoi(argv[2]);
  }
  else
  {
    fprintf (stderr,
	     "%s: Version "VERSION" should only be executed by fvwm!\n",
	     MyName+1);
    exit(1);
  }

  if (argc > 6) {
    imageName = safemalloc (strlen (argv[6]) + 1);
    strcpy (imageName, argv[6]);
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
  if (imageName)
    GetXPMFile(imageName,imagePath);
  else
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
				 0,fore_pix ,None);


  /* Set assorted info for the window */
  if (no_wm) {
    attr.override_redirect = True;
    XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &attr);
  }
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
  XSelectInput(dpy,win,ButtonReleaseMask);
  /* Display the window */
  value.tv_usec = timeout % 1000000;
  value.tv_sec = timeout / 1000000;
  while(1)
  {
    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);

    if(!XPending(dpy))

      retval=select(fd_width,SELECT_FD_SET_CAST &in_fdset, 0, 0, &value);

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
  return 0;
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void GetXPMData(char **data)
{
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
  char *full_file = NULL;

  view.attributes.valuemask = XpmReturnPixels| XpmCloseness | XpmExtensions;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */;

  if (file)
    full_file = findImageFile(file,path,R_OK);

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
    fprintf(stderr,"FvwmBanner: ERROR finding pixmap file in ImagePath\n");
  GetXPMData(fvwm2_big_xpm);
}

void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmBanner: can't %s %s\n", a,b);
}

static void parseOptions (int fd[2])
{
  char *tline= NULL;
  char *p;

  InitGetConfigLine(fd,MyName);
  while (GetConfigLine (fd, &tline),tline != NULL) {
    if (strlen (tline) > 1) {
      if (strncasecmp(tline, "ImagePath",9)==0) {
        CopyString (&imagePath, &tline[9]);
        if (imagePath[0] == 0) {
          free (imagePath);
          imagePath = (char *) 0;
        }
        continue;
      }
      if (strncasecmp(tline,MyName,MyNameLen)) { /* if not for me */
        continue;                       /* ignore it */
      }
      p = tline+MyNameLen;              /* start of interesting part */
      if (strncasecmp (p, "Pixmap", 6) == 0) {
	if (imageName == (char *) 0) {
	  CopyString (&imageName, p+7);
	  if (imageName[0] == 0) {
	    free (imageName);
	    imageName = (char *) 0;
	  }
	}
        continue;
      }
      if (strncasecmp (p, "NoDecor", 7) == 0) {
        no_wm = True;
        continue;
      }
      if (strncasecmp (p, "Timeout", 7) == 0) {
        timeout = atoi(p+8) * 1000000;
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

