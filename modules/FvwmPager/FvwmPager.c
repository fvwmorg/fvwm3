/* This module, and the entire ModuleDebugger program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE 

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#ifdef ISC /* Saul */
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */
 
#include <stdlib.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../fvwm/module.h"

#include "../../libs/fvwmlib.h"       
#include "FvwmPager.h"
#include "../../version.h"
#include "../../fvwm/fvwm.h"

char *MyName;
int fd_width;
int fd[2];

/*************************************************************************
 * 
 * Screen, font, etc info
 *
 **************************************************************************/
ScreenInfo Scr;
PagerWindow *Start = NULL;
PagerWindow *FocusWin = NULL;

Display *dpy;			/* which display are we talking to */
int x_fd,fd_width;

char *PagerFore = "black";
char *PagerBack="white";
char *font_string = "fixed";
char *smallFont = NULL;
char *HilightC = "black";
char *WindowBack = NULL;
char *WindowFore = NULL;
char *WindowHiBack = NULL;
char *WindowHiFore = NULL;

int ShowBalloons = 0, ShowPagerBalloons = 0, ShowIconBalloons = 0;
char *BalloonTypeString = NULL;
char *BalloonBack = NULL;
char *BalloonFore = NULL;
char *BalloonFont = "fixed";
char *BalloonBorderColor = "Black";
int BalloonBorderWidth = 1;
int BalloonYOffset = 2;

int window_w=0, window_h=0, window_x=0, window_y=0;
int icon_x=-10000, icon_y=-10000, icon_w=0, icon_h=0;
int usposition = 0,uselabel = 1;
int xneg = 0, yneg = 0;
extern DeskInfo *Desks;
int StartIconic = 0;
int	MiniIcons = 0;
int Rows = -1, Columns = -1;
int desk1=0, desk2 =0;
int ndesks = 0;
Pixel win_back_pix = -1;
Pixel win_fore_pix = -1;
Pixel win_hi_back_pix = -1;
Pixel win_hi_fore_pix = -1;
/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
  char *temp, *s, *cptr;
  char *display_name = NULL;
  int itemp,i;
  char line[100];
  char mask_mesg[50];

  /* Save our program  name - for error messages */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;
  
  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName, temp);
  
  if(argc  < 6)
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }
  if(argc < 8)
    { 
      fprintf(stderr,"%s Version %s requires two arguments: %s n m\n",
	      MyName,VERSION,MyName);
      fprintf(stderr,"   where desktops n through m are displayed\n");
      exit(1);
    }
  
  /* Dead pipe == Fvwm died */
  signal (SIGPIPE, DeadPipe);  
  
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);
  
  fd_width = GetFdWidth();
  
  desk1 = atoi(argv[6]);
  desk2 = atoi(argv[7]);

  if(desk2 < desk1)
    {
      itemp = desk1;
      desk1 = desk2;
      desk2 = itemp;
    }
  ndesks = desk2 - desk1 + 1;
  
  Desks = (DeskInfo *)malloc(ndesks*sizeof(DeskInfo));
  for(i=0;i<ndesks;i++)
    {
      sprintf(line,"Desk %d",i+desk1);
      CopyString(&Desks[i].label,line);
      CopyString(&Desks[i].Dcolor,PagerBack);
#ifdef DEBUG
      fprintf(stderr,"[main]: Desks[%d].Dcolor == %s\n",i,Desks[i].Dcolor);
#endif /* DEBUG */
    }
  
  /* Initialize X connection */
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);
  
  Scr.screen= DefaultScreen(dpy);
  Scr.Root = RootWindow(dpy, Scr.screen);
  
  if(Scr.Root == None) 
    {
      fprintf(stderr,"%s: Screen %d is not valid ", MyName, (int)Scr.screen);
      exit(1);
    }
  Scr.d_depth = DefaultDepth(dpy, Scr.screen);

  SetMessageMask(fd,
                 M_ADD_WINDOW|
                 M_CONFIGURE_WINDOW|
                 M_DESTROY_WINDOW|
                 M_FOCUS_CHANGE|
                 M_NEW_PAGE|
                 M_NEW_DESK|
                 M_RAISE_WINDOW|
                 M_LOWER_WINDOW|
                 M_ICONIFY|
		 M_ICON_LOCATION|
		 M_DEICONIFY|
		 M_ICON_NAME|
		 M_CONFIG_INFO|
		 M_END_CONFIG_INFO|
		 M_MINI_ICON|
		 M_END_WINDOWLIST);
#ifdef DEBUG
  fprintf(stderr,"[main]: calling ParseOptions\n");
#endif
  ParseOptions();
#ifdef DEBUG
  fprintf(stderr,"[main]: back from calling ParseOptions, calling init pager\n");
#endif
  
  /* open a pager window */
  initialize_pager();
#ifdef DEBUG
  fprintf(stderr,"[main]: back from init pager, getting window list\n");
#endif
  
  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);
#ifdef DEBUG
  fprintf(stderr,"[main]: back from getting window list, looping\n");
#endif
  
  Loop(fd);
}

/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(int *fd)
{
  XEvent Event;
  
  while(1)
    {
      if(My_XNextEvent(dpy,&Event))
	DispatchEvent(&Event);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void process_message(unsigned long type,unsigned long *body)
{
  switch(type)
    {
    case M_ADD_WINDOW:
      list_configure(body);
      break;
    case M_CONFIGURE_WINDOW:
      list_configure(body);
      break;
    case M_DESTROY_WINDOW:
      list_destroy(body);
      break;
    case M_FOCUS_CHANGE:
      list_focus(body);
      break;
    case M_NEW_PAGE:
      list_new_page(body);
      break;
    case M_NEW_DESK:
      list_new_desk(body);
      break;
    case M_RAISE_WINDOW:
      list_raise(body);
      break;
    case M_LOWER_WINDOW:
      list_lower(body);
      break;
    case M_ICONIFY:
    case M_ICON_LOCATION:
      list_iconify(body);
      break;
    case M_DEICONIFY:
      list_deiconify(body);
      break;
    case M_ICON_NAME:
      list_icon_name(body);
      break;
	case M_MINI_ICON:
		list_mini_icon(body);
		break;
    case M_END_WINDOWLIST:
      list_end();
      break;
    default:
      list_unknown(body);
      break;
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
  exit(0);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_add - displays packet contents to stderr
 *
 ***********************************************************************/
void list_add(unsigned long *body)
{
  PagerWindow *t,**prev;
  int i=0;
  
  t = Start;
  prev = &Start;
  while(t!= NULL)
    {
      prev = &(t->next);
      t = t->next;
      i++;
    }
  *prev = (PagerWindow *)safemalloc(sizeof(PagerWindow));
  (*prev)->w = body[0];
  (*prev)->t = (char *)body[2];
  (*prev)->frame = body[1];
  (*prev)->x = body[3];
  (*prev)->y = body[4];
  (*prev)->width = body[5];
  (*prev)->height = body[6];
  (*prev)->desk = body[7];
  (*prev)->next = NULL;
  (*prev)->flags = body[8];
  (*prev)->pager_view_width = 0;
  (*prev)->pager_view_height = 0;
  (*prev)->icon_view_width = 0;
  (*prev)->icon_view_height = 0;
  (*prev)->icon_name = NULL;
  (*prev)->mini_icon.picture = 0;
  (*prev)->title_height = body[9];
  (*prev)->border_width = body[10];
  (*prev)->icon_w = body[19];
  (*prev)->icon_pixmap_w = body[20];
  if ((win_fore_pix != -1) && (win_back_pix != -1))
  {
        (*prev)->text = win_fore_pix;
        (*prev)->back = win_back_pix;
  }
  else
  {
        (*prev)->text = body[22];
        (*prev)->back = body[23];
  }
  AddNewWindow(*prev);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_configure - displays packet contents to stderr
 *
 ***********************************************************************/
void list_configure(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t== NULL)
    {
      list_add(body);

    }
  else
    {
      t->t = (char *)body[2];
      t->frame = body[1];
      t->frame_x = body[3];
      t->frame_y = body[4];
      t->frame_width = body[5];
      t->frame_height = body[6];
      t->title_height = body[9];
      t->border_width = body[10];
      t->flags = body[8];
      t->icon_w = body[19];
      t->icon_pixmap_w = body[20];
        if ((win_fore_pix != -1) && (win_back_pix != -1))
        {
            t->text = win_fore_pix;
            t->back = win_back_pix;
        }
        else
        {
	      t->text = body[22];
	      t->back = body[23];
        }
      if(t->flags & ICONIFIED)
	{
	  t->x = t->icon_x;
	  t->y = t->icon_y;
	  t->width = t->icon_width;
	  t->height = t->icon_height;
	  if(t->flags & SUPPRESSICON)
	    {
	      t->x = -10000;
	      t->y = -10000;
	    }
	}
      else
	{
	  t->x = t->frame_x;
	  t->y = t->frame_y;
	  t->width = t->frame_width;
	  t->height = t->frame_height;
	}
      if(t->desk != body[7])
	{
	  ChangeDeskForWindow(t,body[7]);
	}
      
      else
	MoveResizePagerView(t);
      if(FocusWin == t)
	Hilight(t,ON);
      else
	Hilight(t,OFF);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	list_destroy - displays packet contents to stderr
 *
 ***********************************************************************/
void list_destroy(unsigned long *body)
{
  PagerWindow *t,**prev;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  prev = &Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      prev = &(t->next);
      t = t->next;
    }
  if(t!= NULL)
    {
      if(prev != NULL)
	*prev = t->next;
      /* remove window from the chain */
      if(t->PagerView != None)
	XDestroyWindow(dpy,t->PagerView);
      XDestroyWindow(dpy,t->IconView);
      if(FocusWin == t)
	FocusWin = NULL;
      
      free(t);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	list_focus - displays packet contents to stderr
 *
 ***********************************************************************/
void list_focus(unsigned long *body)
{
  PagerWindow *t,*temp;
  Window target_w;
  extern Pixel focus_pix, focus_fore_pix;
  target_w = body[0];
  
  if ((win_hi_fore_pix != -1) && (win_hi_back_pix != -1))
  {
    focus_pix = win_hi_back_pix;
    focus_fore_pix = win_hi_fore_pix;
  }
  else
  {
    focus_pix = body[4];
    focus_fore_pix = body[3];
  }
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t != FocusWin)
    {
      temp = FocusWin;
      FocusWin = t;
      
      if(temp != NULL)
	Hilight(temp,OFF);
      if(FocusWin != NULL)
	Hilight(FocusWin,ON);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_page - displays packet contents to stderr
 *
 ***********************************************************************/
void list_new_page(unsigned long *body)
{
  Scr.Vx = (long)body[0];
  Scr.Vy = (long)body[1];
  Scr.CurrentDesk = (long)body[2];
  if((Scr.VxMax != body[3])||(Scr.VyMax != body[4]))
    {
      Scr.VxMax = body[3];
      Scr.VyMax = body[4];
      ReConfigure();
    }
  MovePage();
  MoveStickyWindows();
  Hilight(FocusWin,OFF);
  Hilight(FocusWin,ON);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_desk - displays packet contents to stderr
 *
 ***********************************************************************/
void list_new_desk(unsigned long *body)
{
  int oldDesk;
  
  oldDesk = Scr.CurrentDesk;
  Scr.CurrentDesk = (long)body[0];
  
  MovePage();
  
  DrawGrid(oldDesk - desk1,1);
  DrawGrid(Scr.CurrentDesk - desk1,1);
  MoveStickyWindows();
  Hilight(FocusWin,OFF);
  Hilight(FocusWin,ON);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_raise - displays packet contents to stderr
 *
 ***********************************************************************/
void list_raise(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->PagerView != None)
	XRaiseWindow(dpy,t->PagerView);
      XRaiseWindow(dpy,t->IconView);      
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	list_lower - displays packet contents to stderr
 *
 ***********************************************************************/
void list_lower(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->PagerView != None)
	XLowerWindow(dpy,t->PagerView);
      if((t->desk - desk1>=0)&&(t->desk - desk1<ndesks))
	XLowerWindow(dpy,Desks[t->desk - desk1].CPagerWin);
      XLowerWindow(dpy,t->IconView);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	list_unknow - handles an unrecognized packet.
 *
 ***********************************************************************/
void list_unknown(unsigned long *body)
{
  /*  fprintf(stderr,"Unknown packet type\n");*/
}

/***********************************************************************
 *
 *  Procedure:
 *	list_iconify - displays packet contents to stderr
 *
 ***********************************************************************/
void list_iconify(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t== NULL)
    {
      return;
    }
  else
    {
      t->t = (char *)body[2];
      t->frame = body[1];
      t->icon_x = body[3];
      t->icon_y = body[4];
      t->icon_width = body[5];
      t->icon_height = body[6];
      t->flags |= ICONIFIED;
      t->x = t->icon_x;
      t->y = t->icon_y;
      if(t->flags & SUPPRESSICON)
	{
	  t->x = -10000;
	  t->y = -10000;
	}
      t->width = t->icon_width;
      t->height = t->icon_height;

      /* if iconifying main pager window turn balloons on or off */
      if ( t->w == Scr.Pager_w )
	ShowBalloons = ShowIconBalloons;

      MoveResizePagerView(t);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	list_deiconify - displays packet contents to stderr
 *
 ***********************************************************************/

void list_deiconify(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t== NULL)
    {
      return;
    }
  else
    {
      t->flags &= ~ICONIFIED;
      t->x = t->frame_x;
      t->y = t->frame_y;
      t->width = t->frame_width;
      t->height = t->frame_height;

      /* if deiconifying main pager window turn balloons on or off */
      if ( t->w == Scr.Pager_w )
	ShowBalloons = ShowPagerBalloons;
      
      MoveResizePagerView(t);
      if(FocusWin == t)
	Hilight(t,ON);
      else
	Hilight(t,OFF);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	list_icon_name - displays packet contents to stderr
 *
 ***********************************************************************/
void list_icon_name(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  
  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->icon_name != NULL)
	free(t->icon_name);
      CopyString(&t->icon_name,(char *)(&body[3]));
      LabelWindow(t);
      LabelIconWindow(t);
    }
}


void list_mini_icon(unsigned long *body)
{
  PagerWindow	*t;
  Window target_w;
  target_w = body[0];
  t = Start;
  while (t && (t->w != target_w))
    t = t->next;
  if (t)
  {
    t->mini_icon.width   = body[3];
    t->mini_icon.height  = body[4];
    t->mini_icon.depth   = body[5];
    t->mini_icon.picture = body[6];
    t->mini_icon.mask    = body[7];
    PictureWindow (t);
    PictureIconWindow (t);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	list_end - displays packet contents to stderr
 *
 ***********************************************************************/
void list_end(void)
{
  unsigned int nchildren,i;
  Window root, parent, *children;
  PagerWindow *ptr;
  
  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
    return;
  
  for(i=0; i<nchildren;i++)
    {
      ptr = Start;
      while(ptr != NULL)
	{
	  if((ptr->frame == children[i])||(ptr->icon_w == children[i])||
	     (ptr->icon_pixmap_w == children[i]))
	    {
	      if(ptr->PagerView != None)
		XRaiseWindow(dpy,ptr->PagerView);
	      XRaiseWindow(dpy,ptr->IconView);
	    }
	  ptr = ptr->next;
	}
    }
  
  
  if(nchildren > 0)
    XFree((char *)children);
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
  int body_length;
  int count,count2 = 0;
  static int miss_counter = 0;
  unsigned long *body;
  int total;
  char *cbody;
  
  if(XPending(dpy))
    {
      XNextEvent(dpy,event);
      return 1;
    }
  
  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

#ifdef __hpux
  select(fd_width,(int *)&in_fdset, 0, 0, NULL);
#else
  select(fd_width,&in_fdset, 0, 0, NULL);
#endif  
  
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
      if(count = ReadFvwmPacket(fd[1],header,&body) > 0)
	 {
	   process_message(header[1],body);
	   free(body);
	 }
    }
  return 0;
}



/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void ParseOptions()
{
  char *tend,*tstart;
  char *tline= NULL,*tmp;
  char *colors;
  int Clength,n,desk;

  Scr.FvwmRoot = NULL;
  Scr.Hilite = NULL;
  Scr.VScale = 32;
  
  Scr.MyDisplayWidth = DisplayWidth(dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight(dpy, Scr.screen);
  
  Scr.VxMax = 3*Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = 3*Scr.MyDisplayHeight - Scr.MyDisplayHeight;
  if(Scr.VxMax <0)
    Scr.VxMax = 0;
  if(Scr.VyMax <0)
    Scr.VyMax = 0;
  Scr.Vx = 0;
  Scr.Vy = 0;
  
  Clength = strlen(MyName);
  GetConfigLine(fd,&tline);

  while(tline != NULL)
    {
      int g_x, g_y, flags;
      unsigned width,height;
      
      if((strlen(&tline[0])>1)&&
	 (mystrncasecmp(tline, CatString3("*", MyName, "Geometry"),Clength+9)==0))
	{
	  tmp = &tline[Clength+9];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    {
	      tmp++;
	    }
	  tmp[strlen(tmp)-1] = 0;
	  
	  flags = XParseGeometry(tmp,&g_x,&g_y,&width,&height);
	  if (flags & WidthValue) 
	    {
	      window_w = width;
	    }
	  if (flags & HeightValue) 
	    {
	      window_h = height;
	    }
	  if (flags & XValue) 
	    {
	      window_x = g_x;
	      usposition = 1;
	    }
	  if (flags & YValue) 
	    {
	      window_y = g_y;
	      usposition = 1;
	    }
	  if (flags & XNegative) 
	    {
	      xneg = 1;
	    }
	  if (flags & YNegative) 
	    {
	      window_y = g_y;
	      yneg = 1;
	    }
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline, CatString3("*", MyName, "IconGeometry"),
			     Clength+13)==0))
	{
	  tmp = &tline[Clength+13];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    {
	      tmp++;
	    }
	  tmp[strlen(tmp)-1] = 0;
	  
	  flags = XParseGeometry(tmp,&g_x,&g_y,&width,&height);
	  if (flags & WidthValue) 
	    icon_w = width;
	  if (flags & HeightValue) 
	    icon_h = height;
	  if (flags & XValue) 
	    {
	      icon_x = g_x;
	    }
	  if (flags & YValue) 
	    {
	      icon_y = g_y;
	    }
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Label"),Clength+6)==0))
	{
	  desk = desk1;
	  sscanf(&tline[Clength+6],"%d",&desk);
	  if((desk >= desk1)&&(desk <=desk2))
	    {
	      n = 0;
	      while(isspace(tline[Clength+6+n]))n++;
	      while(!isspace(tline[Clength+6+n]))n++;
	      free(Desks[desk - desk1].label);
	      CopyString(&Desks[desk - desk1].label,&tline[Clength+6+n]);
	    }
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline, CatString3("*", MyName, "Font"),Clength+5)==0))
	{
	  CopyString(&font_string,&tline[Clength+5]);
	  if(mystrncasecmp(font_string,"none",4) == 0)
	    uselabel = 0;
	  
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline, CatString3("*", MyName, "Fore"),Clength+5)==0))
	{
	  if(Scr.d_depth > 1)
	    CopyString(&PagerFore,&tline[Clength+5]);
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*", MyName, "Back"),Clength+5)==0))
	{
	  if(Scr.d_depth > 1)
	    {
	    CopyString(&PagerBack,&tline[Clength+5]);
	    for (n=0;n<ndesks;n++)
	      {
                free(Desks[n].Dcolor);
	        CopyString(&Desks[n].Dcolor,PagerBack);
#ifdef DEBUG
                fprintf(stderr,"[ParseOptions]: Back Desks[%d].Dcolor == %s\n",
                        n,Desks[n].Dcolor);
#endif
	      }
	    }
	}	
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"DeskColor"),Clength+10)==0))
	{
	  desk = desk1;
	  sscanf(&tline[Clength+10],"%d",&desk);
	  if((desk >= desk1)&&(desk <=desk2))
	    {
	      n = 0;
	      while(isspace(tline[Clength+10+n]))n++;
	      while(!isspace(tline[Clength+10+n]))n++;
	      free(Desks[desk - desk1].Dcolor);
	      CopyString(&Desks[desk - desk1].Dcolor,&tline[Clength+10+n]);
#ifdef DEBUG
              fprintf(stderr,
                      "[ParseOptions]: DeskColor Desks[%d].Dcolor == %s\n",
                      desk-desk1,Desks[desk-desk1].Dcolor);
#endif
	    }
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Hilight"),Clength+8)==0))
	{
	  if(Scr.d_depth > 1)
	    CopyString(&HilightC,&tline[Clength+8]);
	}	
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"SmallFont"),
			     Clength+10)==0))
	{
	    CopyString(&smallFont,&tline[Clength+10]);
            if(mystrncasecmp(smallFont,"none",4) == 0)
            {
              free(smallFont);
              smallFont = NULL;
            }
	}	
      else if ((strlen (&tline[0]) > 1) && (strncasecmp (tline, CatString3 ("*", MyName, "MiniIcons"), Clength + 9) == 0))
      {
              MiniIcons = 1;
      }
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"StartIconic"),
			     Clength+12)==0))
	{
	  StartIconic = 1;
	}	
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Rows"),
			     Clength+5)==0))
	{
	  sscanf(&tline[Clength+5],"%d",&Rows);
	}	
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Columns"),
			     Clength+8)==0))
	{
	  sscanf(&tline[Clength+8],"%d",&Columns);
	}
        else if((strlen(&tline[0])>1)&&
              (mystrncasecmp(tline,CatString3("*",MyName,"DeskTopScale"),
                             Clength+13)==0))
        {
          sscanf(&tline[Clength+13],"%d",&Scr.VScale);
        }
	else if ((strlen (&tline[0]) > 1) && (strncasecmp (tline, CatString3 ("*", MyName, "WindowColors"), Clength + 13) == 0))
	{
		if (Scr.d_depth > 1)
		{
			colors = &tline[Clength + 13];
			colors = GetNextToken (colors, &WindowFore);
			colors = GetNextToken (colors, &WindowBack);
			colors = GetNextToken (colors, &WindowHiFore);
			colors = GetNextToken (colors, &WindowHiBack);
		}
	}

      
      /* ... and get Balloon config options ...
         -- ric@giccs.georgetown.edu */
      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline, CatString3 ("*", MyName, "Balloons"),
                              Clength + 9) == 0) ) {
	CopyString(&BalloonTypeString, &tline[Clength+9]);

	if ( strncasecmp(BalloonTypeString, "Pager", 5) == 0 ) {
	   ShowPagerBalloons = 1;
	}
	else if ( strncasecmp(BalloonTypeString, "Icon", 4) == 0 ) {
	   ShowIconBalloons = 1;
	}
	else {
	  ShowPagerBalloons = 1;
	  ShowIconBalloons = 1;
	}

	/* turn this on initially so balloon window is created; later this
	   variable is changed to match ShowPagerBalloons or ShowIconBalloons
	   whenever we receive iconify or deiconify packets */
        ShowBalloons = 1;
      }

      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline, CatString3 ("*", MyName, "BalloonBack"),
                              Clength + 12) == 0) ) {
        if (Scr.d_depth > 1)
          CopyString(&BalloonBack, &tline[Clength+12]);
      }

      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline, CatString3 ("*", MyName, "BalloonFore"), 
                              Clength + 12) == 0) ) {
        if (Scr.d_depth > 1)
          CopyString(&BalloonFore, &tline[Clength+12]);
      }

      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline, CatString3 ("*", MyName, "BalloonFont"), 
                              Clength + 12) == 0) ) {
        CopyString(&BalloonFont, &tline[Clength+12]);
      }

      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline,
                              CatString3 ("*", MyName, "BalloonBorderColor"), 
                              Clength + 19) == 0) ) {
        CopyString(&BalloonBorderColor, &tline[Clength+19]);
      }
      
      else if ( (strlen(&tline[0]) > 1) &&
                (strncasecmp (tline,
                              CatString3 ("*", MyName, "BalloonBorderWidth"), 
                              Clength + 19) == 0) ) {
        sscanf(&tline[Clength+19], "%d", &BalloonBorderWidth);
      }
      
      else if ( (strlen(&tline[0]) > 1) &&
               (strncasecmp (tline,
                             CatString3 ("*", MyName, "BalloonYOffset"), 
                             Clength + 15) == 0) ) {
        sscanf(&tline[Clength+15], "%d", &BalloonYOffset);
      }

      GetConfigLine(fd,&tline);
    }
  return;
}
