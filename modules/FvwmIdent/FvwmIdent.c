/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation and Nobutaka Suzuki <nobuta-s@is.aist-nara.ac.jp>
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE

#define YES "Yes"
#define NO  "No"

#include "config.h"

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
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include "../../fvwm/module.h"
#include "FvwmIdent.h"

char *MyName;
int fd_width;
int fd[2];

Display *dpy;			/* which display are we talking to */
Window Root;
int screen;
int x_fd;
int d_depth;
int ScreenWidth, ScreenHeight;

char *BackColor = "white";
char *ForeColor = "black";
char *font_string = "fixed";

Pixel back_pix, fore_pix;
GC  NormalGC;
Window main_win;
Window app_win;
XFontStruct *font;

int Width, Height,win_x,win_y;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask | KeyReleaseMask)

static Atom wm_del_win;

struct target_struct target;
int found=0;

static int ListSize=0;

struct Item* itemlistRoot = NULL;
int max_col1, max_col2;
char id[15], desktop[10], swidth[10], sheight[10], borderw[10], geometry[30];
char mymin_aspect[11], max_aspect[11];

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
  char *display_name = NULL;
  int Clength;
  char *tline;

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);
  Clength = strlen(MyName);

  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  /* Dead pipe == dead fvwm */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* An application window may have already been selected - look for it */
  sscanf(argv[4],"%x",(unsigned int *)&app_win);

  /* Open the Display */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  d_depth = DefaultDepth(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  SetMessageMask(fd,M_CONFIGURE_WINDOW|M_WINDOW_NAME|M_ICON_NAME|
		 M_RES_CLASS| M_RES_NAME| M_END_WINDOWLIST|M_CONFIG_INFO|
		 M_END_CONFIG_INFO);
  /* scan config file for set-up parameters */
  /* Colors and fonts */

  GetConfigLine(fd,&tline);

  while(tline != (char *)0)
    {
      if(strlen(tline)>1)
	{
	  if(strncasecmp(tline, CatString3(MyName,"Font",""),Clength+4)==0)
	    {
	      CopyString(&font_string,&tline[Clength+4]);
	    }
	  else if(strncasecmp(tline,CatString3(MyName,"Fore",""),
				Clength+4)==0)
	    {
	      CopyString(&ForeColor,&tline[Clength+4]);
	    }
	  else if(strncasecmp(tline,CatString3(MyName, "Back",""),
				Clength+4)==0)
	    {
	      CopyString(&BackColor,&tline[Clength+4]);
	    }
	}
      GetConfigLine(fd,&tline);
    }

  if(app_win == 0)
    GetTargetWindow(&app_win);

  fd_width = GetFdWidth();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  Loop(fd);
  return 0;
}

/**************************************************************************
 *
 * Read the entire window list from fvwm
 *
 *************************************************************************/
void Loop(int *fd)
{
  unsigned long header[4], *body;

  while(1)
    {
      if(ReadFvwmPacket(fd[1],header,&body) > 0)
	{
	  process_message(header[1],body);
	  free(body);
	}
    }
}


/**************************************************************************
 *
 * Process window list messages
 *
 *************************************************************************/
void process_message(unsigned long type,unsigned long *body)
{
  switch(type)
    {
    case M_CONFIGURE_WINDOW:
      list_configure(body);
      break;
    case M_WINDOW_NAME:
      list_window_name(body);
      break;
    case M_ICON_NAME:
      list_icon_name(body);
      break;
    case M_RES_CLASS:
      list_class(body);
      break;
    case M_RES_NAME:
      list_res_name(body);
      break;
    case M_END_WINDOWLIST:
      list_end();
      break;
    default:
      break;

    }
}




/***********************************************************************
 *
 * Detected a broken pipe - time to exit
 *
 **********************************************************************/
void DeadPipe(int nonsense)
{
  freelist();
  exit(0);
}

/***********************************************************************
 *
 * Got window configuration info - if its our window, safe data
 *
 ***********************************************************************/
void list_configure(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0])
     ||((body[19] != 0)&&(app_win == (Window)body[19]))
     ||((body[19] != 0)&&(app_win == (Window)body[20])))
    {
      app_win = body[1];
      target.id = body[0];
      target.frame = body[1];
      target.frame_x = body[3];
      target.frame_y = body[4];
      target.frame_w = body[5];
      target.frame_h = body[6];
      target.desktop = body[7];
      target.flags = body[8];
      target.title_h = body[9];
      target.border_w = body[10];
      target.base_w = body[11];
      target.base_h = body[12];
      target.width_inc = body[13];
      target.height_inc = body[14];
      target.gravity = body[21];
      found = 1;
    }

}

/*************************************************************************
 *
 * Capture  Window name info
 *
 ************************************************************************/
void list_window_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncpy(target.name,(char *)&body[3],255);
    }
}

/*************************************************************************
 *
 * Capture  Window Icon name info
 *
 ************************************************************************/
void list_icon_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncat(target.icon_name,(char *)&body[3],255);
    }
}


/*************************************************************************
 *
 * Capture  Window class name info
 *
 ************************************************************************/
void list_class(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncat(target.class,(char *)&body[3],255);
    }
}


/*************************************************************************
 *
 * Capture  Window resource info
 *
 ************************************************************************/
void list_res_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncat(target.res,(char *)&body[3],255);
    }
}


/*************************************************************************
 *
 * End of window list, open an x window and display data in it
 *
 ************************************************************************/
XSizeHints mysizehints;
void list_end(void)
{
  XGCValues gcv;
  unsigned long gcm;
  int lmax,height;
  XEvent Event;
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned int JunkMask;
  int x,y;

  if(!found)
    {
/*    fprintf(stderr,"%s: Couldn't find app window\n",MyName); */
      exit(0);
    }

  close(fd[0]);
  close(fd[1]);

  /* load the font */
  if ((font = XLoadQueryFont(dpy, font_string)) == NULL)
    {
      if ((font = XLoadQueryFont(dpy, "fixed")) == NULL)
	exit(1);
    };

  /* make window infomation list */
  MakeList();

  /* size and create the window */
  lmax = max_col1 + max_col2 + 15;

  height = ListSize*(font->ascent+font->descent);

  mysizehints.flags=
    USSize|USPosition|PWinGravity|PResizeInc|PBaseSize|PMinSize|PMaxSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = lmax+10;
  mysizehints.height=height+10;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  XQueryPointer( dpy, Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);
  mysizehints.win_gravity = NorthWestGravity;

  if((y+height+100)>ScreenHeight)
    {
      y = ScreenHeight - height - 10;
      mysizehints.win_gravity = SouthWestGravity;
    }

  if((x+lmax+100)>ScreenWidth)
    {
      x = ScreenWidth - lmax - 10;
      if((y+height+100)>ScreenHeight)
	mysizehints.win_gravity = SouthEastGravity;
      else
	mysizehints.win_gravity = NorthEastGravity;
    }
  mysizehints.x = x;
  mysizehints.y = y;



  if(d_depth < 2)
    {
      back_pix = GetColor("white");
      fore_pix = GetColor("black");
    }
  else
    {
      back_pix = GetColor(BackColor);
      fore_pix = GetColor(ForeColor);

    }

  main_win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
				 mysizehints.width,mysizehints.height,
				 0,fore_pix,back_pix);
  XSetTransientForHint(dpy,main_win,app_win);
  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,main_win,&wm_del_win,1);

  XSetWMNormalHints(dpy,main_win,&mysizehints);
  XSelectInput(dpy,main_win,MW_EVENTS);
  change_window_name(&MyName[1]);

  gcm = GCForeground|GCBackground|GCFont;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.font =  font->fid;
  NormalGC = XCreateGC(dpy, Root, gcm, &gcv);
  XMapWindow(dpy,main_win);

  /* Window is created. Display it until the user clicks or deletes it. */
  while(1)
    {
      XNextEvent(dpy,&Event);
      switch(Event.type)
	{
	case Expose:
	  if(Event.xexpose.count == 0)
	    RedrawWindow();
	  break;
	case KeyRelease:
	case ButtonRelease:
	  freelist();
	  exit(0);
	case ClientMessage:
	  if (Event.xclient.format==32 && Event.xclient.data.l[0]==wm_del_win)
	    {
	      freelist();
	      exit(0);
	    }
	default:
	  break;
	}
    }

}



/**********************************************************************
 *
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *
 *********************************************************************/
void GetTargetWindow(Window *app_win)
{
  XEvent eventp;
  int val = -10,trials;

  trials = 0;
  while((trials <100)&&(val != GrabSuccess))
    {
      val=XGrabPointer(dpy, Root, True,
		       ButtonReleaseMask,
		       GrabModeAsync, GrabModeAsync, Root,
		       XCreateFontCursor(dpy,XC_crosshair),
		       CurrentTime);
      if(val != GrabSuccess)
	{
	  usleep(1000);
	}
      trials++;
    }
  if(val != GrabSuccess)
    {
      fprintf(stderr,"%s: Couldn't grab the cursor!\n",MyName);
      exit(1);
    }
  XMaskEvent(dpy, ButtonReleaseMask,&eventp);
  XUngrabPointer(dpy,CurrentTime);
  XSync(dpy,0);
  *app_win = eventp.xany.window;
  if(eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;
}

/************************************************************************
 *
 * Draw the window
 *
 ***********************************************************************/
void RedrawWindow(void)
{
  int fontheight,i=0;
  struct Item *cur = itemlistRoot;

  fontheight = font->ascent + font->descent;

  while(cur != NULL)
    {
      /* first column */
      XDrawString(dpy,main_win,NormalGC,5,5+font->ascent+i*fontheight,
		  cur->col1,strlen(cur->col1));
      /* second column */
      XDrawString(dpy,main_win,NormalGC,10+max_col1,5+font->ascent+i*fontheight,
		  cur->col2,strlen(cur->col2));
      ++i;
      cur = cur->next;
    }
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;

  if (XStringListToTextProperty(&str,1,&name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
  XSetWMName(dpy,main_win,&name);
  XSetWMIconName(dpy,main_win,&name);
  XFree(name.value);
}


/**************************************************************************
*
* Add s1(string at first column) and s2(string at second column) to itemlist
*
 *************************************************************************/
void AddToList(char *s1, char* s2)
{
  int tw1, tw2;
  struct Item* item, *cur = itemlistRoot;

  tw1 = XTextWidth(font, s1, strlen(s1));
  tw2 = XTextWidth(font, s2, strlen(s2));
  max_col1 = max_col1 > tw1 ? max_col1 : tw1;
  max_col2 = max_col2 > tw2 ? max_col2 : tw2;

  item = (struct Item*)safemalloc(sizeof(struct Item));

  item->col1 = s1;
  item->col2 = s2;
  item->next = NULL;

  if (cur == NULL)
    itemlistRoot = item;
  else {
    while(cur->next != NULL)
       cur = cur->next;
    cur->next = item;
  }
  ListSize++;
}

void MakeList(void)
{
  int bw,width,height,x1,y1,x2,y2;
  char loc[20];
  static char xstr[6],ystr[6];

  ListSize = 0;

  bw = 2*target.border_w;
  width = target.frame_w - bw;
  height = target.frame_h - target.title_h - bw;

  sprintf(desktop, "%ld",  target.desktop);
  sprintf(id,      "0x%x", (unsigned int)target.id);
  sprintf(swidth,  "%d",   width);
  sprintf(sheight, "%d",   height);
  sprintf(borderw, "%ld",  target.border_w);
  sprintf(xstr,    "%ld",  target.frame_x);
  sprintf(ystr,    "%ld",  target.frame_y);

  AddToList("Name:",          target.name);
  AddToList("Icon Name:",     target.icon_name);
  AddToList("Class:",         target.class);
  AddToList("Resource:",      target.res);
  AddToList("Window ID:",     id);
  AddToList("Desk:",          desktop);
  AddToList("Width:",         swidth);
  AddToList("Height:",        sheight);
  AddToList("X (current page):",   xstr);
  AddToList("Y (current page):",   ystr);
  AddToList("Boundary Width:", borderw);
  AddToList("Sticky:",        (target.flags & STICKY 	? YES : NO));
  AddToList("Ontop:",         (target.flags & ONTOP  	? YES : NO));
  AddToList("NoTitle:",       (target.flags & TITLE  	? NO : YES));
  AddToList("Iconified:",     (target.flags & ICONIFIED ? YES : NO));
  AddToList("Transient:",     (target.flags & TRANSIENT ? YES : NO));

  switch(target.gravity)
    {
    case ForgetGravity:
      AddToList("Gravity:", "Forget");
      break;
    case NorthWestGravity:
      AddToList("Gravity:", "NorthWest");
      break;
    case NorthGravity:
      AddToList("Gravity:", "North");
      break;
    case NorthEastGravity:
      AddToList("Gravity:", "NorthEast");
      break;
    case WestGravity:
      AddToList("Gravity:", "West");
      break;
    case CenterGravity:
      AddToList("Gravity:", "Center");
      break;
    case EastGravity:
      AddToList("Gravity:", "East");
      break;
    case SouthWestGravity:
      AddToList("Gravity:", "SouthWest");
      break;
    case SouthGravity:
      AddToList("Gravity:", "South");
      break;
    case SouthEastGravity:
      AddToList("Gravity:", "SouthEast");
      break;
    case StaticGravity:
      AddToList("Gravity:", "Static");
      break;
    default:
      AddToList("Gravity:", "Unknown");
      break;
    }
  x1 = target.frame_x;
  if(x1 < 0)
    x1 = 0;
  x2 = ScreenWidth - x1 - target.frame_w;
  if(x2 < 0)
    x2 = 0;
  y1 = target.frame_y;
  if(y1 < 0)
    y1 = 0;
  y2 = ScreenHeight - y1 -  target.frame_h;
    if(y2 < 0)
    y2 = 0;
  width = (width - target.base_w)/target.width_inc;
  height = (height - target.base_h)/target.height_inc;

  sprintf(loc,"%dx%d",width,height);
  strcpy(geometry, loc);

  if ((target.gravity == EastGravity) ||(target.gravity == NorthEastGravity)||
      (target.gravity == SouthEastGravity))
    sprintf(loc,"-%d",x2);
  else
    sprintf(loc,"+%d",x1);
  strcat(geometry, loc);

  if((target.gravity == SouthGravity)||(target.gravity == SouthEastGravity)||
     (target.gravity == SouthWestGravity))
    sprintf(loc,"-%d",y2);
  else
    sprintf(loc,"+%d",y1);
  strcat(geometry, loc);
  AddToList("Geometry:", geometry);

#if 0
  {
    char tmp[20], *foo;
    sprintf(tmp,"%d", target.base_w);
    foo = strdup(tmp);
    AddToList("  - base_w:", foo);
    sprintf(tmp,"%d", target.width_inc);
    foo = strdup(tmp);
    AddToList("  - width_inc:", foo);
    sprintf(tmp,"%d", target.base_h);
    foo = strdup(tmp);
    AddToList("  - base_h:", foo);
    sprintf(tmp,"%d", target.height_inc);
    foo = strdup(tmp);
    AddToList("  - height_inc:", foo);
  }
#endif

  {
    Atom *protocols = NULL, *ap;
    Atom _XA_WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    XWMHints *wmhintsp = XGetWMHints(dpy,target.id);
    int i,n;
    Boolean HasTakeFocus=False,InputField=True;
    char *focus_policy="",*ifstr="",*tfstr="";

    if (wmhintsp)
    {
      InputField=wmhintsp->input;
      ifstr=InputField?"True":"False";
      XFree(wmhintsp);
    }
    else
    {
      ifstr="XWMHints missing";
    }
    if (XGetWMProtocols(dpy,target.id,&protocols,&n))
    {
      for (i = 0, ap = protocols; i < n; i++, ap++)
      {
        if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
          HasTakeFocus = True;
      }
      tfstr=HasTakeFocus?"Present":"Absent";
      XFree(protocols);
    }
    else
    {
      tfstr="XGetWMProtocols failed";
    }
    if (HasTakeFocus)
    {
      if (InputField)
      {
        focus_policy = "Locally Active";
      }
      else
      {
        focus_policy = "Globally Active";
      }
    }
    else
    {
      if (InputField)
      {
        focus_policy = "Passive";
      }
      else
      {
        focus_policy = "No Input";
      }
    }
    AddToList("Focus Policy:",focus_policy);
    AddToList("  - Input Field:",ifstr);
    AddToList("  - WM_TAKE_FOCUS:",tfstr);
    {
      long supplied_return;             /* flags, hints that were supplied */
      int getrc;
      XSizeHints *size_hints = XAllocSizeHints(); /* the size hints */
      if ((getrc = XGetWMSizeHints(dpy,target.id, /* get size hints */
                          size_hints,    /* Hints */
                          &supplied_return,
                          XA_WM_ZOOM_HINTS))) {
        if (supplied_return & PAspect) { /* if window has a aspect ratio */
          sprintf(mymin_aspect, "%d/%d", size_hints->min_aspect.x,
                  size_hints->min_aspect.y);
          AddToList("Minimum aspect ratio:",mymin_aspect);
          sprintf(max_aspect, "%d/%d", size_hints->max_aspect.x,
                  size_hints->max_aspect.y);
          AddToList("Maximum aspect ratio:",max_aspect);
        } /* end aspect ratio */
        XFree(size_hints);
      } /* end getsizehints worked */
    }
  }
}

void freelist(void)
{
  struct Item* cur = itemlistRoot, *cur2;

  while(cur != NULL)
    {
      cur2 = cur;
      cur = cur->next;
      free(cur2);
    }
}


void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmInitBanner: can't %s %s\n", a,b);
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

  XGetWindowAttributes(dpy,Root,&attributes);
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





