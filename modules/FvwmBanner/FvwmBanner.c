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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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
Graphics *G;
Window Root;
int screen;
int x_fd;
int ScreenWidth, ScreenHeight;
XSizeHints mysizehints;
Pixel back_pix, fore_pix;
GC NormalGC,FGC;
static Atom wm_del_win;
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
  x_fd = XConnectionNumber(dpy);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  G = CreateGraphics(dpy);
  parseOptions(fd);

  /* chick in the neck situation:
   * can't load pixmaps into non-default visual without a window
   * don't know what size the window should be until pixmap is loaded
   */
  attr.background_pixmap = None;
  attr.border_pixel = 0;
  attr.colormap = G->cmap;
  attr.override_redirect = no_wm;
  win = XCreateWindow(dpy, Root, -20, -20, 10, 10, 0, G->depth, InputOutput,
		      G->viz,
		      CWColormap|CWBackPixmap|CWBorderPixel|CWOverrideRedirect,
		      &attr);

  /* Get the xpm banner */
  if (imageName)
    GetXPMFile(imageName,imagePath);
  else
    GetXPMData(fvwm2_big_xpm);
  XSetWindowBackgroundPixmap(dpy, win, view.pixmap);

  /* Create a window to hold the banner */
  mysizehints.flags = USSize | USPosition | PWinGravity | PResizeInc
		     | PBaseSize | PMinSize | PMaxSize;
  mysizehints.width = view.attributes.width;
  mysizehints.height = view.attributes.height;
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

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);

  XSetWMNormalHints(dpy,win,&mysizehints);
  change_window_name("FvwmBanner");

  XMoveResizeWindow(dpy, win, mysizehints.x, mysizehints.y, mysizehints.width,
		    mysizehints.height);

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
  view.attributes.valuemask = XpmReturnPixels | XpmCloseness | XpmExtensions
			      | XpmVisual | XpmColormap | XpmDepth;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */;
  view.attributes.visual = G->viz;
  view.attributes.colormap = G->cmap;
  view.attributes.depth = G->depth;
  if(XpmCreatePixmapFromData(dpy, win, data, &view.pixmap, &view.mask,
			     &view.attributes)!=XpmSuccess)
  {
    fprintf(stderr,"FvwmBanner: ERROR couldn't convert data to pixmap\n");
    exit(1);
  }
}
void GetXPMFile(char *file, char *path)
{
  char *full_file = NULL;

  view.attributes.valuemask = XpmReturnPixels | XpmCloseness | XpmExtensions
			      | XpmVisual | XpmColormap | XpmDepth;
  view.attributes.closeness = 40000 /* Allow for "similar" colors */;
  view.attributes.visual = G->viz;
  view.attributes.colormap = G->cmap;
  view.attributes.depth = G->depth;

  if (file)
    full_file = findImageFile(file,path,R_OK);

  if (full_file)
  {
    if(XpmReadFileToPixmap(dpy, Root, full_file, &view.pixmap, &view.mask,
			   &view.attributes) == XpmSuccess)
      return;
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
      if(strncasecmp(tline, DEFGRAPHSTR, DEFGRAPHLEN)==0)  {
        ParseGraphics(dpy, tline, G);
        SavePictureCMap(dpy, G->viz, G->cmap, G->depth);
        continue;
      }
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

