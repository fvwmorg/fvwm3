/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE

#define UPDATE_ONLY 1
#define ALL 2
#define PROP_SIZE 1024

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../fvwm/module.h"

#include "FvwmTalk.h"

char *MyName;
int fd_width,screen, d_depth;
int fd[2];

int x_fd;
Window Root;
Display *dpy;
/* char *font_string = "helvetica"; */
char *font_string = "fixed";
char *ForeColor = "green";
char *BackColor = "black";
char *display_name = NULL;

XFontStruct *font;
Pixel fore_pix,back_pix;
static Atom wm_del_win;
unsigned long valuemask;
XSetWindowAttributes attributes;
XWMHints wmhints;
Window window;
char last_error[256],previous_line[256];
char Text[256];
int pos = 0;

XSizeHints sizehints =
{
  (PMinSize | PResizeInc | PBaseSize | PWinGravity | PMaxSize),
  0, 0, 100, 100,	                /* x, y, width and height */
  1, 1,		                        /* Min width and height */
  0, 0,		                        /* Max width and height */
  1, 1,	                         	/* Width and height increments */
  {0, 0}, {0, 0},                       /* Aspect ratio - not used */
  1, 1,                 		/* base size */
  (NorthWestGravity)                    /* gravity */
};

Pixel GetColor(char *name);
void nocolor(char *a, char *b);
XGCValues gcv;
GC myGC;
int My_XNextEvent(Display *dpy, XEvent *event);
void DrawWindow(int mode);
void paste_primary(int window,int property,int Delete);
void request_selection(int time);

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
  XWMHints        wm_hints;
  XClassHint      class_hints;
  XTextProperty   window_name;

  /* Save the program name - its used for error messages and option parsing */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* Initialize X connection */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);

  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  if(Root == None)
    {
      fprintf(stderr,"%s: Screen %d is not valid ", MyName, (int)screen);
      exit(1);
    }
  d_depth = DefaultDepth(dpy, screen);

  fd_width = GetFdWidth();

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

  /* load the font */
  if ((font = XLoadQueryFont(dpy, font_string)) == NULL)
    {
      if ((font = XLoadQueryFont(dpy, "fixed")) == NULL)
	{
	  fprintf(stderr,"%s: No fonts available\n",MyName);
	  exit(1);
	}
    };


  fore_pix = GetColor(ForeColor);
  back_pix = GetColor(BackColor);

  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
  attributes.background_pixel = back_pix;
  attributes.border_pixel = fore_pix;
  attributes.event_mask = KeyPressMask | ExposureMask | ButtonPressMask;

  sizehints.width = 8*XTextWidth(font,"MMMMMMMMMM",10);
  sizehints.height = 3*(font->ascent+font->descent+2)+2;
  sizehints.x = 0;
  sizehints.y = 0;
  sizehints.width_inc = 0.1*XTextWidth(font,"MMMMMMMMMM",10);
  sizehints.height_inc = 3*(font->ascent+font->descent+2)+2;
  sizehints.base_width = 1;
  sizehints.base_height = 3*(font->ascent+font->descent+2)+2;
  sizehints.min_width = 1;
  sizehints.min_height = 3*(font->ascent+font->descent+2)+2;
  sizehints.max_height = 3*(font->ascent+font->descent+2)+2;
  sizehints.max_width = 26*XTextWidth(font,"MMMMMMMMMM",10);

  window = XCreateWindow (dpy, Root, 0, 0, sizehints.width,sizehints.height,
			  (unsigned int) 1,
			  CopyFromParent, InputOutput,
			  (Visual *) CopyFromParent,
			  valuemask, &attributes);

  class_hints.res_name = "FvwmTalk";
  class_hints.res_class =  "FvwmTalk";

  wm_hints.flags = InputHint;
  wm_hints.input = True;;

  XSetWMProtocols(dpy,window,&wm_del_win,1);
  XSetWMNormalHints(dpy,window,&sizehints);
  /* XStringListToTextProperty(&(argv[0]), 1, &window_name); */
  XStringListToTextProperty(&temp, 1, &window_name);
  XSetWMProperties(dpy, window, &window_name, &window_name,
      argv, argc, &sizehints, &wm_hints, &class_hints);

  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.font = font->fid;
  myGC = XCreateGC(dpy,window,GCForeground|GCBackground|GCFont,&gcv);


  previous_line[0] = 0;
  last_error[0] = 0;
  XMapWindow(dpy,window);
  Loop(fd);
  return 0;
}



/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(int *fd)
{
  KeySym keysym;
  static XComposeStatus compose = {NULL,0};
  XEvent event;
  char kbuf[100];
  int count;

  pos = 0;
  Text[0] = 0;

  while(1)
    {
      if(My_XNextEvent(dpy, &event))
	{
	  switch(event.type)
	    {
	    case KeyPress:
	      count = XLookupString(&(event.xkey),kbuf,99,&keysym,&compose);
	      kbuf[count] = (unsigned char)0;
	      if((keysym == XK_BackSpace )||(keysym == XK_Delete))
		{
		  if(pos > 0)
		    Text[--pos] = 0;
		}
	      else if((keysym == XK_Return)||(keysym == XK_KP_Enter))
		{
		  SendText(fd,Text,0);
		  last_error[0] = 0;
		  strncpy(previous_line,Text,255);
		  previous_line[255] = 0;
		  pos = 0;
		  Text[pos] = 0;
		  DrawWindow(ALL);
		}
	      else
		{
		  if(pos + count < 255)
		    {
		      strcat(Text,kbuf);
		      pos += count;
		    }
		  else
		    XBell(dpy,0);
		}
	      DrawWindow(UPDATE_ONLY);
	      break;
	    case ButtonPress:
	      if(event.xbutton.button == 2)
		request_selection(event.xbutton.time);
	      break;
	    case Expose:
	      DrawWindow(ALL);
	      break;
	    case ClientMessage:
	      if ((event.xclient.format==32) &&
		  (event.xclient.data.l[0]==wm_del_win))
		{
		  exit(0);
		}
	      break;
	    case SelectionNotify:
	      paste_primary(event.xselection.requestor,
			    event.xselection.property,True);
	    default:
	      break;
	    }
	}
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  fprintf(stderr,"FvwmTalk: dead pipe\n");
  exit(0);
}



/****************************************************************************
 *
 * Loads a single color
 *
 ****************************************************************************/
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy, Root,&attributes);
  color.pixel = 0;
   if (!XParseColor (dpy, attributes.colormap, name, &color))
     {
       nocolor("parse",name);
     }
   else if(!XAllocColor (dpy, attributes.colormap, &color))
     {
       nocolor("alloc",name);
     }
  return color.pixel;
}


void nocolor(char *a, char *b)
{
  fprintf(stderr,"%s: can't %s %s\n", MyName, a,b);

}


/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ****************************************************************************/
int My_XNextEvent(Display *dpy, XEvent *event)
{
  fd_set in_fdset;
  unsigned long header[HEADER_SIZE];
  int count;
  static int miss_counter = 0;
  unsigned long *body;

  if(XPending(dpy))
    {
      XNextEvent(dpy,event);
      return 1;
    }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  select(fd_width,SELECT_TYPE_ARG234 &in_fdset, 0, 0, NULL);

  if(FD_ISSET(x_fd, &in_fdset))
    {
      if(XPending(dpy))
	{
	  XNextEvent(dpy,event);
	  miss_counter = 0;
	  return 1;
	}
      else
	miss_counter++;
      if(miss_counter > 100)
	DeadPipe(0);
    }

  if(FD_ISSET(fd[1], &in_fdset))
    {
      if((count = ReadFvwmPacket(fd[1],header,&body)) > 0)
	 {
	   if(header[1] == M_ERROR || header[1] == M_STRING)
	     {
	       strncpy(last_error,(char *)(&body[3]),255);
	       /*last_error[strlen(last_error)-1] = 0;*/
	       last_error[strlen(last_error)] = 0;
	       last_error[255] = 0;
	       XClearArea(dpy,window,0,0,10000,10000,1);
	     }

	   free(body);
	 }
    }
  return 0;
}



void request_selection(int time)
{
  Atom sel_property;

  if (XGetSelectionOwner(dpy,XA_PRIMARY) == None)
    {
      /*  No primary selection so use the cut buffer.
       */
      paste_primary(DefaultRootWindow(dpy),XA_CUT_BUFFER0,False);
      return;
    }
  sel_property = XInternAtom(dpy,"VT_SELECTION",False);
  XConvertSelection(dpy,XA_PRIMARY,XA_STRING,sel_property,window,time);
}

void paste_primary(int window,int property,int Delete)
{
  Atom actual_type;
  int actual_format,i;
  unsigned long nitems, bytes_after, nread;
  unsigned char *data, *data2;

  if (property == None)
    return;

  nread = 0;
  do
    {
      if (XGetWindowProperty(dpy,window,property,nread/4,PROP_SIZE,Delete,
                             AnyPropertyType,&actual_type,&actual_format,
                             &nitems,&bytes_after,(unsigned char **)&data)
          != Success)
        return;
      if (actual_type != XA_STRING)
        return;

      data2 = data;
      /* want to make a \n to \r mapping for cut and paste only */
      for(i=0;i<nitems;i++)
        {
          if(*data == '\n')
            *data = '\r';
          data++;
        }

      if(255-pos > 0)
	{
	  strncat(Text,(char *)data2,255-pos-nitems);
	  pos = strlen(Text);
	  DrawWindow(UPDATE_ONLY);
	}
      nread += nitems;
      XFree(data2);
    } while (bytes_after > 0);
}

void DrawWindow(int mode)
{
  int w;

  if(mode == ALL)
    {
      XClearWindow(dpy,window);
      XDrawImageString(dpy,window,myGC,2,(font->ascent+font->descent+2)+
		       font->ascent+2,last_error,strlen(last_error));
      XDrawImageString(dpy,window,myGC,2, font->ascent+2,
		       previous_line,strlen(previous_line));
    }
  else
    {
      XClearArea(dpy,window,0,
		 2*(font->ascent+font->descent+2),10000,10000,0);
    }
  XDrawImageString(dpy,window,myGC,2,2*(font->ascent+font->descent+2)+
		   font->ascent+2,Text,pos);
  w=XTextWidth(font,Text,pos);
  XDrawLine(dpy,window,myGC,4+w,2*(font->ascent+font->descent+2)+2,
	    4+w,2*(font->ascent+font->descent+2)+
	    font->ascent+font->descent);
}
