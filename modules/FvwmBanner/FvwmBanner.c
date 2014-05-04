/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 * FvwmBanner
 *
 *  Show Fvwm Banner
 *
 */

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Module.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/FRenderInit.h"
#include "libs/Graphics.h"
#include "libs/Parse.h"
#include "libs/Strings.h"

/* migo (16-Sep-1999): How about to do this configurable? */
/*#include "fvwm-logo-old.xpm"*/
/*#include "fvwm-logo-colorful.xpm"*/
#include "fvwm-logo-current.xpm"
#include "fvwm-logo-current.xbm"

typedef struct _PImageIcon {
	Pixmap pixmap;
	Pixmap mask;
	int width;
	int height;
	int depth;
}        PImageIcon;

/*
 * A few function prototypes
 */
void RedrawWindow(void);
void GetXPMData(char **);
void GetXBMData(void);
void GetImageFile(char *,char *);
void change_window_name(char *str);
static void parseOptions (int fd[2]);

PImageIcon view;
Window win;

char *imagePath = NULL;
char *imageName = NULL;
static char *MyName = NULL;
static int MyNameLen;

int timeout = 3000000; /* default time of 3 seconds */

Display *dpy;                   /* which display are we talking to */
Window Root;
int screen;
int x_fd;
XSizeHints mysizehints;
Pixel back_pix, fore_pix;
GC NormalGC,FGC;
static Atom wm_del_win;
Bool no_wm = False;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask)

/*
 *
 * Creates an icon window as needed
 *
 */
int main(int argc, char **argv)
{
  char *display_name = NULL, *string = NULL;
  int retval = 0;
  XEvent Event;
  fd_set in_fdset;
  struct timeval value;
  int fd[2];
  XSetWindowAttributes attr;
  int wx;
  int wy;

  /* Save our program  name - for error messages */
  string = strrchr (argv[0], '/');
  if (string != (char *) 0) string++;

  MyNameLen=strlen(string)+1;           /* account for '*' */
  MyName = xmalloc(MyNameLen + 1);     /* account for \0 */
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
    imageName = xmalloc(strlen(argv[6]) + 1);
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
  x_fd = XConnectionNumber(dpy);

  flib_init_graphics(dpy);
  /* SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO  | M_SENDCONFIG); */
  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO);
  parseOptions(fd);

  /* chick in the neck situation:
   * can't load pixmaps into non-default visual without a window
   * don't know what size the window should be until pixmap is loaded
   */
  attr.background_pixmap = None;
  attr.border_pixel = 0;
  attr.colormap = Pcmap;
  attr.override_redirect = no_wm;
  win = XCreateWindow(dpy, Root, -20, -20, 10, 10, 0, Pdepth, InputOutput,
		      Pvisual,
		      CWColormap|CWBackPixmap|CWBorderPixel|CWOverrideRedirect,
		      &attr);

  /* Get the xpm banner */
  if (imageName)
    GetImageFile(imageName,imagePath);
  else
    GetXPMData(fvwm_logo_xpm);

  if (view.depth != Pdepth)
  {
	  XGCValues gcv;
	  GC gc;
	  Pixmap temp;

	  gcv.background= PictureWhitePixel();
	  gcv.foreground= PictureBlackPixel();
	  gc = fvwmlib_XCreateGC(dpy, win,
				 GCForeground | GCBackground, &gcv);
	  temp = XCreatePixmap(dpy, win, view.width, view.height, Pdepth);
	  XCopyPlane(dpy, view.pixmap, temp, gc,
		     0,0,view.width,view.height, 0,0,1);
	  XFreePixmap(dpy, view.pixmap);
	  XFreeGC(dpy,gc);
	  view.pixmap = temp;
  }
  XSetWindowBackgroundPixmap(dpy, win, view.pixmap);

  /* Create a window to hold the banner */
  mysizehints.flags = USSize | USPosition | PWinGravity | PResizeInc
		     | PBaseSize | PMinSize | PMaxSize;
  mysizehints.width = view.width;
  mysizehints.height = view.height;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  mysizehints.win_gravity = NorthWestGravity;

  FScreenCenterOnScreen(
    NULL, FSCREEN_PRIMARY, &wx, &wy, view.width, view.height);
  FScreenMangleScreenIntoUSPosHints(FSCREEN_PRIMARY, &mysizehints);

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);

  XSetWMNormalHints(dpy,win,&mysizehints);
  change_window_name("FvwmBanner");

  XMoveResizeWindow(dpy, win, wx, wy, mysizehints.width, mysizehints.height);

  if (FHaveShapeExtension && view.mask != None)
    FShapeCombineMask(dpy, win, FShapeBounding, 0, 0, view.mask, FShapeSet);

  XMapWindow(dpy,win);
  XSync(dpy,0);
  XSelectInput(dpy,win,ButtonReleaseMask);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  /* Display the window */
  value.tv_usec = timeout % 1000000;
  value.tv_sec = timeout / 1000000;
  while(1)
  {
    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);

    if(!FPending(dpy))

      retval=select(x_fd + 1,SELECT_FD_SET_CAST &in_fdset, 0, 0, &value);

    if (retval==0)
    {
      XDestroyWindow(dpy,win);
      XSync(dpy,0);
      exit(0);
    }

    if (FD_ISSET(x_fd, &in_fdset))
    {
      /* read a packet */
      FNextEvent(dpy,&Event);
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

/*
 *
 * Looks for a color XPM icon file
 *
 */
void GetXBMData(void)
{
  if ((view.pixmap =
       XCreatePixmapFromBitmapData(dpy, win, (char *)fvwm_logo_xbm_bits,
				   fvwm_logo_xbm_width, fvwm_logo_xbm_height,
				   BlackPixel(dpy,screen),
				   WhitePixel(dpy,screen),
				   Pdepth)) == None)
  {
    fprintf(stderr,"FvwmBanner: ERROR couldn't convert data to pixmap\n");
    exit(1);
  }
  view.width = fvwm_logo_xbm_width;
  view.height = fvwm_logo_xbm_height;
  view.depth = Pdepth;
  view.mask = None;
}

void GetXPMData(char **data)
{
  if(!PImageLoadPixmapFromXpmData(
	     dpy, win, 0, data, &view.pixmap, &view.mask,
	     &view.width, &view.height, &view.depth))
  {
    GetXBMData();
  }
}

void GetImageFile(char *file, char *path)
{
	char *full_file = NULL;
	FvwmPictureAttributes fpa;

	fpa.mask = FPAM_NO_ALLOC_PIXELS | FPAM_DITHER | FPAM_NO_ALPHA;
	if (file)
		full_file = PictureFindImageFile(file,path,R_OK);

	if (full_file)
	{
		if(PImageLoadPixmapFromFile(
			dpy, Root, full_file, &view.pixmap, &view.mask, None,
			&view.width, &view.height, &view.depth, 0, NULL, 0,
			fpa))
		{
			return;
		}
		fprintf(stderr,"FvwmBanner: ERROR loading image file\n");
	}
	else
	{
		fprintf(stderr,
			"FvwmBanner: ERROR finding image file in ImagePath\n");
		GetXPMData(fvwm_logo_xpm);
	}
}

static void parseOptions(int fd[2])
{
  char *tline= NULL;
  char *tok;

  InitGetConfigLine(fd,MyName);
  while (GetConfigLine (fd, &tline),tline != NULL)
  {
    if (strlen(tline) > 1)
    {
      tok = PeekToken(tline, &tline);
      if (StrEquals(tok, XINERAMA_CONFIG_STRING))
      {
	FScreenConfigureModule(tline);
	continue;
      }
      if (StrEquals(tok, "ImagePath"))
      {
	CopyString(&imagePath, tline);
	if (imagePath[0] == 0)
	{
	  free (imagePath);
	  imagePath = (char *) 0;
	}
	continue;
      }
      if (strncasecmp(tok,MyName,MyNameLen))
      {
	/* if not for me: ignore it*/
	continue;
      }
      /* start of interesting part */
      tok += MyNameLen;
      if (StrEquals(tok, "Pixmap"))
      {
	if (imageName == (char *) 0)
	{
	  CopyString(&imageName, tline);
	  if (imageName[0] == 0)
	  {
	    free (imageName);
	    imageName = (char *) 0;
	  }
	}
	continue;
      }
      if (StrEquals(tok, "NoDecor"))
      {
	no_wm = True;
	continue;
      }
      if (StrEquals(tok, "Timeout"))
      {
	timeout = atoi(tline) * 1000000;
	continue;
      }
    }
  }
  return;
}

/*
 *  Change the window name displayed in the title bar.
 */
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


/*
 *
 *  Procedure:
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 */

/*ARGSUSED*/
RETSIGTYPE DeadPipe (int nonsense)
{
  exit (0);
  SIGNAL_RETURN;
}
