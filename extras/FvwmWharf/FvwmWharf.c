/* Wharf.c. by Bo Yang. 
 * Modifications: Copyright 1995 by Bo Yang.
 *
 * modifications made by Frank Fejes for AfterStep 
 * Copyright 1996
 *
 * folder code Copyright 1996 by Beat Christen.
 *
 * swallowed button actions Copyright 1996 by Kaj Groner
 * 
 * based on GoodStuff.c by Robert Nation 
 * The GoodStuff module, and the entire GoodStuff program, and the concept for
 * interfacing that module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. 
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.  */

/* 
 * Various enhancements Copyright 1996 Alfredo K. Kojima
 * 
 * button pushing styles
 * configurable border drawing
 * Change of icon creation code. Does not use shape extension anymore.
 * 	each icon window now contains the whole background 
 * OffiX drop support added
 * animation added
 * withdraw on button2 click
 * icon overlaying
 * sound bindings
 */

#define TRUE 1
#define FALSE 0
#define DOUBLECLICKTIME 1

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
#include "../../fvwm/module.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <syslog.h>

#include "Wharf.h"
#include "../../version.h"
#define AFTER_ICONS 1
/*#include "../../afterstep/asbuttons.h" pdg */

#ifdef ENABLE_DND
#include "OffiX/DragAndDrop.h"
#include "OffiX/DragAndDropTypes.h"
#endif

#include "stepgfx.h"

/*
 * You may want to raise the following values if your machine is fast
 */
#define ANIM_STEP	2   /* must be >= 1. Greater is smoother and slower */
#define ANIM_STEP_MAIN	1   /* same for the main folder */
#define ANIM_DELAY	10

#ifdef ENABLE_SOUND
#define WHEV_PUSH		0
#define WHEV_CLOSE_FOLDER	1
#define WHEV_OPEN_FOLDER	2
#define WHEV_CLOSE_MAIN		3
#define WHEV_OPEN_MAIN		4
#define WHEV_DROP		5
#define MAX_EVENTS		6

int SoundActive = 0;
char *Sounds[6]={".",".",".",".",".","."};
char *SoundPlayer=NULL;
char *SoundPath=".";

pid_t SoundThread;
int PlayerChannel[2];

/*char *ModulePath=AFTERDIR; pdg */
#endif

char *MyName;

Display *dpy;
int x_fd,fd_width;
int ROWS = FALSE;

Window Root;
int screen;
int flags;
long d_depth;
Bool NoBorder=0;
Bool Pushed = 0;
Bool Pushable = 1;
Bool ForceSize=0;
Pixel back_pix, fore_pix, light_grey;
GC  NormalGC, HiReliefGC, HiInnerGC;

GC MaskGC, DefGC;
int AnimationStyle=0,AnimateMain=0;
int PushStyle=0;
int AnimationDir=1;

Window main_win;
int Width, Height,win_x,win_y;
unsigned int display_width, display_height;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask |\
		     ButtonPressMask | LeaveWindowMask | EnterWindowMask)
XSizeHints mysizehints;
int num_buttons = 0;
int num_folderbuttons = MAX_BUTTONS;
int num_folders = 0;
int num_rows = 0;
int num_columns = 0;
int max_icon_width = 30,max_icon_height = 0;
int BUTTONWIDTH, BUTTONHEIGHT;
int x= -100000,y= -100000,w= -1,h= -1,gravity = NorthWestGravity;
int new_desk = 0;
int pageing_enabled = 1;
int ready = 0;

int CurrentButton = -1;
int fd[2];

struct button_info Buttons[BUTTON_ARRAY_LN];
struct folder_info Folders[FOLDER_ARRAY_LN];

char *iconPath = NULL;
char *pixmapPath = NULL;

static Atom wm_del_win;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_NAME;
#ifdef ENABLE_DND
Atom DndProtocol;
Atom DndSelection;
#endif
int TextureType=TEXTURE_BUILTIN;
char *BgPixmapFile=NULL;
int FromColor[3]={0x4000,0x4000,0x4000}, ToColor[3]={0x8000,0x8000,0x8000};
Pixel BgColor=0;
int MaxColors=16;
int Withdrawn; 

#define DIR_TOLEFT	1
#define DIR_TORIGHT	2
#define DIR_TOUP	3
#define DIR_TODOWN	4

#ifdef ENABLE_SOUND
void waitchild(int bullshit)
{
    int stat;
    
    wait(&stat);
    SoundActive=0;
}
#endif

unsigned int lock_mods[256];
void FindLockMods(void);


/***********************************************************************
 *
 *  Procedure:
 *	main - start of afterstep
 *
 ***********************************************************************
 */
void main(int argc, char **argv)
{
  char *display_name = NULL;
  int i,j;
  Window root;
  int x,y,border_width,button;
  int depth;
  char *temp, *s;
  char set_mask_mesg[50];
  temp = argv[0];
  
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;
  
  MyName = safemalloc(strlen(temp)+1);
  strcpy(MyName, temp);
  
  for(i=0;i<BUTTON_ARRAY_LN;i++)
    {
#ifdef ENABLE_DND
      Buttons[i].drop_action = NULL;
#endif	
      Buttons[i].title = NULL;
      Buttons[i].action = NULL;
      Buttons[i].iconno = 0;
      for(j=0;j<MAX_OVERLAY;j++) {	
	  Buttons[i].icons[j].file = NULL;
	  Buttons[i].icons[j].w = 0;
	  Buttons[i].icons[j].h = 0;
	  Buttons[i].icons[j].mask = None;	/* pixmap for the icon mask */
	  Buttons[i].icons[j].icon = None;
	  Buttons[i].icons[j].depth = 0;
      }	
      Buttons[i].IconWin = None;
      Buttons[i].completeIcon = None;	
      Buttons[i].up = 1;                        /* Buttons start up */
      Buttons[i].hangon = NULL;                 /* don't wait on anything yet*/
      Buttons[i].folder = -1;
    }
  signal (SIGPIPE, DeadPipe);  
  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by AfterStep!\n",
		MyName, VERSION);
      exit(1);
    }
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);
  
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);

  fd_width = GetFdWidth();

  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  if(Root == None) 
    {
      fprintf(stderr,"%s: Screen %d is not valid ", MyName, screen);
      exit(1);
    }
  display_width = DisplayWidth(dpy, screen);
  display_height = DisplayHeight(dpy, screen);
  
  d_depth = DefaultDepth(dpy, screen);

  SetMessageMask(fd, M_NEW_DESK | M_END_WINDOWLIST | M_MAP | M_WINDOW_NAME |
		 M_RES_CLASS | M_CONFIG_INFO | M_END_CONFIG_INFO | M_RES_NAME);
/*
  sprintf(set_mask_mesg,"SET_MASK %lu\n",
	  (unsigned long)(M_NEW_DESK |
			  M_END_WINDOWLIST| 
			  M_MAP|
			  M_RES_NAME|
			  M_RES_CLASS|
			  M_WINDOW_NAME));

  SendText(fd,set_mask_mesg,0);
*/
  ParseOptions(argv[3]);
  if(num_buttons == 0)
    {
      fprintf(stderr,"%s: No Buttons defined. Quitting\n", MyName);
      exit(0);
    }
    
#ifdef ENABLE_SOUND    
    /* startup sound subsystem */
     if (SoundActive) {
	if (pipe(PlayerChannel)<0) {
	    fprintf(stderr,"%s: could not create pipe. Disabling sound\n");
	    SoundActive=0;
	} else {
	    signal(SIGCHLD,waitchild);
	    SoundThread=fork();
	    if (SoundThread<0) {
		fprintf(stderr,"%s: could not fork(). Disabling sound",
			MyName);
		perror(".");
		SoundActive=0;
	    } else if (SoundThread==0) { /* in the sound player process */
		char *margv[9], *name;
		int i;

		margv[0]="ASSound";
		name = findIconFile("ASSound",ModulePath,X_OK);
		if(name == NULL) {
		    fprintf(stderr,"Wharf: couldn't find ASSound\n");
		    SoundActive = 0;
		} else {
		    margv[1]=safemalloc(16);
		    close(PlayerChannel[1]);
		    sprintf(margv[1],"%x",PlayerChannel[0]);
		    if (SoundPlayer!=NULL) 
		      margv[2]=SoundPlayer;
		    else 
		      margv[2]="-";
		    for(i=0;i<MAX_EVENTS;i++) {
			if (Sounds[i][0]=='.') {
			    margv[i+3]=Sounds[i];
			} else {
			    margv[i+3]=safemalloc(strlen(Sounds[i])
						  +strlen(SoundPath)+4);
			    sprintf(margv[i+3],"%s/%s",SoundPath,Sounds[i]);
			}
		    }
		    margv[i+3]=NULL;
		    execvp(name,margv);
		    fprintf(stderr,"Wharf: couldn't spawn ASSound\n");
		    exit(1);
		}
	    } else { /* in parent */
		close(PlayerChannel[0]);
	    }
	}
     }    
#endif
    
    CreateShadowGC();

    switch (TextureType) {
     case TEXTURE_PIXMAP:
	if (BgPixmapFile==NULL) {
	    fprintf(stderr,"%s: No Button background pixmap defined.Using default\n", MyName);
	    goto Builtin;
	}
	Buttons[BACK_BUTTON].icons[0].file=BgPixmapFile;	
	if (GetXPMFile(BACK_BUTTON,0))
	  break;
	else goto Solid;
     case TEXTURE_GRADIENT:
     case TEXTURE_HGRADIENT:
     case TEXTURE_HCGRADIENT:
     case TEXTURE_VGRADIENT:
     case TEXTURE_VCGRADIENT:
	if (GetXPMGradient(BACK_BUTTON, FromColor, ToColor, MaxColors,TextureType))
	  break;
	else goto Solid;

     case TEXTURE_BUILTIN:
	Builtin:
	TextureType=TEXTURE_BUILTIN;
/*	if (GetXPMData( BACK_BUTTON, button_xpm))
	  break;
pdg */
     default:
Solid:
	TextureType=TEXTURE_SOLID;
	if (GetSolidXPM(BACK_BUTTON, BgColor))
	  break;
	else {
	    fprintf( stderr, "back Wharf button creation\n");
	    exit(-1);
	}
    }
  for(i=0;i<num_buttons;i++)
    {
	for(j=0;j<Buttons[i].iconno;j++) {
	    LoadIconFile(i,j);
	}	
    }
  for(i=num_folderbuttons;i<MAX_BUTTONS;i++) {
     	for(j=0;j<Buttons[i].iconno;j++) {
	    LoadIconFile(i,j);
	}
  }
#ifdef ENABLE_DND
  DndProtocol=XInternAtom(dpy,"DndProtocol",False);
  DndSelection=XInternAtom(dpy,"DndSelection",False);
#endif

    
  CreateWindow();
  for(i=0;i<num_buttons;i++) {
      CreateIconWindow(i, &main_win);
  }
  for(i=num_folderbuttons;i<MAX_BUTTONS;i++)
    CreateIconWindow(i, Buttons[i].parent);
  XGetGeometry(dpy,main_win,&root,&x,&y,
	       (unsigned int *)&Width,(unsigned int *)&Height,
 	       (unsigned int *)&border_width,(unsigned int *)&depth);

  for(i=0;i<num_rows;i++)
    for(j=0;j<num_columns; j++)
      {
	button = i*num_columns + j;
	ConfigureIconWindow(button,i,j);
      };
  for(i=0;i<num_folders;i++)
    for(j=0;j<Folders[i].count;j++)
      if(num_columns < num_rows) {	  
	ConfigureIconWindow(Folders[i].firstbutton+j,0, j);
      } else {	  
	ConfigureIconWindow(Folders[i].firstbutton+j,j, 0);
      }
  /* dirty hack to make swallowed app background be textured */
  XSetWindowBackgroundPixmap(dpy, main_win, Buttons[BACK_BUTTON].icons[0].icon);
  XMapSubwindows(dpy,main_win);
  XMapWindow(dpy,main_win);
  for(i=0;i<num_folders;i++)
      XMapSubwindows(dpy, Folders[i].win);
    
  FindLockMods();

  /* request a window list, since this triggers a response which
   * will tell us the current desktop and paging status, needed to
   * indent buttons correctly */
  SendText(fd,"Send_WindowList",0);
  Loop();

}
    
/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
    
void Loop(void)
{
  Window *CurrentWin=None;
  int x,y,w,h,xoff,yoff,border_width,CurrentRow,CurrentColumn,CurrentBase=0;
  long depth;
  XEvent Event;
  int NewButton,i,j,button,tw,th,i2, bl=-1;
  int LastMapped=-1;
  char *temp;
  time_t t, tl = (time_t) 0;
  int CancelPush=0;
    
  while(1)
    {
      if(My_XNextEvent(dpy,&Event))
	{
	  switch(Event.type)
	    {

	    case Expose:
	      for(x=0;x<num_folders;x++)
		if(Event.xany.window == Folders[x].win )
		  {
		    RedrawWindow(&Folders[x].win,num_folderbuttons, -1, Folders[x].cols,Folders[x].rows);
		    for(y=1;y<=Folders[x].count;y++)
		    if(num_columns<num_rows)
		      RedrawUnpushedOutline(&Folders[x].win, 1, y);
		    else
		      RedrawUnpushedOutline(&Folders[x].win, y, 1);
		  }
		    if (Pushed)
                break;
	      if((Event.xexpose.count == 0)&&
		 (Event.xany.window == main_win))
		{
		    if(ready < 1)
		      ready ++;
		    RedrawWindow(&main_win,0, -1, num_rows, num_columns);
		}
	      break;
	      
	    case ButtonPress:
	      if (Event.xbutton.button != Button1) {
		  if (Event.xbutton.button == Button2) {
		      static int LastX, LastY;
		      
		      if (LastMapped != -1) {
			  CloseFolder(LastMapped);
			  Folders[LastMapped].mapped = NOTMAPPED;
			  LastMapped=-1;
		      }		      
		      if (Withdrawn) {
#ifdef ENABLE_SOUND			  
			  PlaySound(WHEV_OPEN_MAIN);
#endif			  	      
			  if (AnimationStyle>0 && AnimateMain)
			    OpenFolder(-1,LastX,LastY,Width,Height,
				       AnimationDir);
			  else
			    XMoveResizeWindow(dpy,main_win,LastX,LastY,
					      Width,Height);
			  Withdrawn=0;
		      } else {
			  Window junk;
			  int junk2,junk3,junk4,junk5;
			  int CornerX, CornerY;

#ifdef ENABLE_SOUND			  
			  PlaySound(WHEV_CLOSE_MAIN);
#endif			  
			  XGetGeometry(dpy,main_win,&junk,&LastX,&LastY,
				       &junk2,&junk3,&junk4,&junk5);
			  XTranslateCoordinates(dpy,main_win,Root,
						LastX,LastY,
						&LastX,&LastY,&junk);
			  if (num_rows<num_columns) { /* horizontal */
			      if (LastY > display_height/2) {
				  CornerY = display_height-BUTTONHEIGHT;
			      } else {
				  CornerY = 0;
			      }
			      if (Event.xbutton.x>num_columns*BUTTONWIDTH/2) {
				  CornerX = display_width - BUTTONWIDTH;
				  AnimationDir = DIR_TOLEFT;
			      } else {
				  CornerX = 0;
				  AnimationDir = DIR_TORIGHT;
			      }
			      if (AnimationStyle>0 && AnimateMain) {
				  CloseFolder(-1);
				  XMoveWindow(dpy,main_win, CornerX, CornerY);
			      } else {
				  XMoveResizeWindow(dpy,main_win, 
						    CornerX, CornerY,
						    BUTTONWIDTH,BUTTONHEIGHT);
			      }
			  } else {	/* vertical */
			      if (LastX > display_width/2) {
				  CornerX = display_width - BUTTONWIDTH;
			      } else {
				  CornerX = 0;
			      }
			      if (Event.xbutton.y>num_rows*BUTTONHEIGHT/2) {
				  CornerY = display_height-BUTTONHEIGHT;
				  AnimationDir = DIR_TOUP;
			      } else {
				  CornerY = 0;
				  AnimationDir = DIR_TODOWN;
			      }
			      if (AnimationStyle>0 && AnimateMain) {
				  CloseFolder(-1);
				  XMoveWindow(dpy,main_win, CornerX, CornerY);
			      } else {
				  XMoveResizeWindow(dpy,main_win, 
						    CornerX, CornerY,
						    BUTTONWIDTH,BUTTONHEIGHT);
			      }
			  }
			  Withdrawn=1;
		      }
		  }
		  break;
	      }
#ifdef ENABLE_SOUND		    
		PlaySound(WHEV_PUSH);
#endif		 
	      CancelPush = 0;
	      CurrentWin = &Event.xbutton.window;
	      CurrentBase = 0;
	      CurrentRow = (Event.xbutton.y/BUTTONHEIGHT);
	      CurrentColumn = (Event.xbutton.x/BUTTONWIDTH);
	      if (*CurrentWin!=main_win) {
		  CurrentButton = CurrentBase + CurrentColumn*num_rows
		    + CurrentRow*num_columns;
	      }	else {
		  CurrentButton = CurrentBase + CurrentColumn
		    + CurrentRow*num_columns;
		  if (CurrentButton>=num_buttons) {
		      CurrentButton = -1;
		      break;
		  }
	      }		

              for(x=0;x<num_buttons;x++)
                {
                  if (*CurrentWin == Buttons[x].IconWin)
                    {
                      CurrentButton = x;
                      CurrentRow = x / num_columns;
                      CurrentColumn = x % num_columns;
                    }
                }

	      for(x=0;x<num_folders;x++)
		if(*CurrentWin == Folders[x].win)
		  {
		    CurrentBase = Folders[x].firstbutton;
		    if (num_rows<num_columns) 
			CurrentButton = CurrentBase + CurrentRow;
		     else
			CurrentButton = CurrentBase + CurrentColumn;
		  }
	      i = CurrentRow+1;
	      j = CurrentColumn +1;
		
              if (Buttons[CurrentButton].swallow == 1 ||
                  Buttons[CurrentButton].swallow == 2 ||
                  Buttons[CurrentButton].action == NULL)
		break;
		
	      if (Pushable)
		{
                  if (Buttons[CurrentButton].swallow != 3 &&
                      Buttons[CurrentButton].swallow != 4)
                    {
                      Pushed = 1;
                      RedrawPushed(CurrentWin, i, j);
                    }
		}
	      if (mystrncasecmp(Buttons[CurrentButton].action,"Folder",6)==0) {
                Window junk;
                int junk2,junk3,junk4,junk5;
                XGetGeometry(dpy,main_win,&junk,&x,&y,
                          &junk2,&junk3,&junk4,&junk5);
                XTranslateCoordinates(dpy,main_win,Root,
                          x,y,
                          &x,&y,&junk);
/* kludge until Beat takes a look */
if ((num_columns == 1) && (num_rows == 1))
		MapFolder(Buttons[CurrentButton].folder, 
			  &LastMapped,
                          x, y,
			  1,1);
else
		MapFolder(Buttons[CurrentButton].folder, 
			  &LastMapped,
                          x, y,
			  CurrentRow, CurrentColumn);	      
	      }		
              break;
	     case EnterNotify:
		CancelPush = 0;
		break;
	     case LeaveNotify:
		CancelPush = 1;
		break;
#ifdef ENABLE_DND
	     case ClientMessage:
		if (Event.xclient.message_type==DndProtocol) {
		    unsigned long  dummy_r,size;
		    Atom dummy_a;
		    int dummy_f;
		    unsigned char *data, *Command;

		    Window dummy_rt, dummy_c;
		    int dummy_x, dummy_y, base, pos_x, pos_y;
		    unsigned int dummy;
		    
/*		    if (Event.xclient.data.l[0]!=DndFile ||
			Event.xclient.data.l[0]!=DndFiles ||
			Event.xclient.data.l[0]!=DndExe
			)
		      break; */
 
		    XQueryPointer(dpy,main_win,
				  &dummy_rt,&dummy_c,
				  &dummy_x,&dummy_y,
				  &pos_x,&pos_y,
				  &dummy);
		    base = 0;
		    dummy_y = (pos_y/BUTTONHEIGHT);
		    dummy_x= (pos_x/BUTTONWIDTH);
		    dummy = base + dummy_x + dummy_y*num_columns;

		    /*
		    for(x=0;x<num_folders;x++) {
			if(Event.xbutton.window == Folders[x].win) {
			    base = Folders[x].firstbutton;
			    dummy = base + dummy_y + dummy_x -1;
			}
		    } */
		    if (Buttons[dummy].drop_action == NULL)
		      break;
		    dummy_x++;
		    dummy_y++;
		    CurrentWin=Buttons[dummy].parent;
		    if (Pushable) {
			RedrawPushedOutline(CurrentWin, dummy_y, dummy_x);
			XSync(dpy, 0);
		    }
		    XGetWindowProperty(dpy, Root, DndSelection, 0L,
				       100000L, False, AnyPropertyType,
				       &dummy_a, &dummy_f,
				       &size,&dummy_r,
				       &data);
		    if (Event.xclient.data.l[0]==DndFiles) {
			for (dummy_r = 0; dummy_r<size-1; dummy_r++) {
			    if (data[dummy_r]==0)
			      data[dummy_r]=' ';
			}
		    }
#ifdef ENABLE_SOUND
		    PlaySound(WHEV_DROP);
#endif		      
		    Command=(unsigned char *)safemalloc(strlen((char *)data)
				    + strlen((char *)(Buttons[dummy].drop_action)));
		    sprintf((char *)Command,Buttons[dummy].drop_action,
			    data,Event.xclient.data.l[0]);
		    SendInfo(fd,(char *)Command,0); 
		    free(Command);
		    if (Pushable) {
			sleep_a_little(50000);
			XClearWindow(dpy,Buttons[dummy].IconWin);
			RedrawUnpushedOutline(CurrentWin, dummy_y, dummy_x);
		    }
		} 
		break;
#endif /* ENABLE_DND */		      		
            case ButtonRelease:
	      if ((Event.xbutton.button != Button1) ||
		  (Buttons[CurrentButton].swallow == 1) ||
		  (Buttons[CurrentButton].swallow == 2) ||
		  (Buttons[CurrentButton].action == NULL)) {
		  break;		  
	      }		

              CurrentRow = (Event.xbutton.y/BUTTONHEIGHT);
              CurrentColumn = (Event.xbutton.x/BUTTONWIDTH);

              if (Pushable) 
              {
		if (Buttons[CurrentButton].swallow != 3 &&
		    Buttons[CurrentButton].swallow != 4)
		  {
		    Pushed=0;
		    RedrawUnpushed(CurrentWin, i, j);
		  }
              }
	      if (CancelPush)
		  break;
	      if (*CurrentWin!=main_win) {
		  NewButton = CurrentBase + CurrentColumn*num_rows
		    + CurrentRow*num_columns;
	      }	else {
		  NewButton = CurrentBase + CurrentColumn
		    + CurrentRow*num_columns;
	      }		
	      
 	      for(x=0;x<num_folders;x++)
		if(*CurrentWin == Folders[x].win)
		  {
		    if (num_rows<num_columns) 
			NewButton = Folders[x].firstbutton + CurrentRow;
		     else
			NewButton = Folders[x].firstbutton + CurrentColumn;
		  }
	      for (x=0;x<num_buttons;x++)
	        {
		  if (*CurrentWin == Buttons[x].IconWin)
		    {
		      NewButton = x;
		      CurrentRow = x / num_columns;
		      CurrentColumn = x % num_columns;
		    }
		}

              if(NewButton == CurrentButton)
                {
		  t = time( 0);
		  bl = -1;
		  tl = -1;
		  if(mystrncasecmp(Buttons[CurrentButton].action,"Folder",6)!=0)
		    {
		      if (LastMapped != -1 && CurrentWin != &main_win)
			{
			  CloseFolder(LastMapped);
			  Folders[LastMapped].mapped = NOTMAPPED;
			  LastMapped = -1;
			}
		      SendInfo(fd,Buttons[CurrentButton].action,0);
		    }		  
		  if((Buttons[CurrentButton].action)&&
		     (mystrncasecmp(Buttons[CurrentButton].action,"exec",4)== 0))
		    {  
		      i=4;
		      while((Buttons[CurrentButton].action[i] != 0)&&
			    (Buttons[CurrentButton].action[i] != '"'))
			i++;
		      i2=i+1;

		      while((Buttons[CurrentButton].action[i2] != 0)&&
			    (Buttons[CurrentButton].action[i2] != '"'))
			i2++;
		      if(i2 - i >1)
			{
                          Buttons[CurrentButton].hangon = safemalloc(i2-i);
                          strncpy(Buttons[CurrentButton].hangon,
				  &Buttons[CurrentButton].action[i+1],i2-i-1);
                          Buttons[CurrentButton].hangon[i2-i-1] = 0;
                          Buttons[CurrentButton].up = 0;
			  if (Buttons[CurrentButton].swallow == 3 ||
			      Buttons[CurrentButton].swallow == 4)
                            Buttons[CurrentButton].swallow = 4;
			  else
                            Buttons[CurrentButton].swallow = 0;
			}
		    }
                }
              break;
	      
	      /*
		case ClientMessage:
		if ((Event.xclient.format==32) && 
		(Event.xclient.data.l[0]==wm_del_win))
		{
		DeadPipe(1);
		}
		break;
		case PropertyNotify:
		if (Pushed)
		break;
		for(i=0;i<num_rows;i++)
		for(j=0;j<num_columns; j++)
		{
		button = i*num_columns + j;
		if(((Buttons[button].swallow == 3)||
		(Buttons[button.swallow == 4))&&
		(Event.xany.window == Buttons[button].IconWin)&&
		(Event.xproperty.atom == XA_WM_NAME))
		{
		XFetchName(dpy, Buttons[button].IconWin, &temp);
		if(strcmp(Buttons[button].title,"-")!=0)
		CopyString(&Buttons[button].title, temp);
		XFree(temp);
		XClearArea(dpy,main_win,j*BUTTONWIDTH,
		i*BUTTONHEIGHT, BUTTONWIDTH,BUTTONHEIGHT,0);
		RedrawWindow(&main_win,0, button, num_rows, num_columns);
		}
		}
		break;
		*/
	    default:
	      break;
	    }
	}
    }
}

void OpenFolder(int folder,int x, int y, int w, int h,  int direction)
{
    int winc, hinc;
    int cx, cy, cw, ch;
    Window win;
    int isize;

    if (folder<0) {
	    winc = BUTTONWIDTH/ANIM_STEP_MAIN;
	    hinc = BUTTONHEIGHT/ANIM_STEP_MAIN;
    } else {
	    winc = BUTTONWIDTH/ANIM_STEP;
	    hinc = BUTTONHEIGHT/ANIM_STEP;
    }
    
    if (folder>=0) {
	win = Folders[folder].win;
	Folders[folder].direction = direction;
	if (direction == DIR_TOLEFT || direction == DIR_TORIGHT)
	  isize = winc;
	else
	  isize = hinc;
    } else {
	win = main_win;
	if (direction == DIR_TOLEFT || direction == DIR_TORIGHT)	
	  isize = BUTTONWIDTH;
	else
	  isize = BUTTONHEIGHT;
    }
    cx = x;    cy = y;
    ch = h;    cw = w;
    if (AnimationStyle==0) {
 	XMapWindow(dpy, win);	
    } else    
    switch (direction) {
     case DIR_TOLEFT:
	cx = x+w;
	XMoveResizeWindow(dpy,win,cx,y, 1, h);
	XMapWindow(dpy, win);
	for(cw=isize;cw<=w;cw+=winc) {
	    cx -= winc;
	    sleep_a_little(ANIM_DELAY/2);
	    XMoveResizeWindow(dpy,win,cx,y, cw,h);
	    XSync(dpy,0);
	}
	break;
     case DIR_TORIGHT:
	XMoveResizeWindow(dpy,win,x,y, 1, h);
	XMapWindow(dpy, win);
	for(cw=isize;cw<=w;cw+=winc) {
	    sleep_a_little(ANIM_DELAY/2);
	    XMoveResizeWindow(dpy,win,x,y, cw,h);
	    XSync(dpy,0);
	}
	break;
     case DIR_TOUP:
	cy = y+h;
	XMoveResizeWindow(dpy,win,x,cy, w, 1);
	XMapWindow(dpy, win);
	for(ch=isize;ch<=h;ch+=hinc) {
	    cy -= hinc;
	    sleep_a_little(ANIM_DELAY/2);
	    XMoveResizeWindow(dpy,win,x,cy, w, ch);
	    XSync(dpy,0);
	}
	break;
     case DIR_TODOWN:
	XMoveResizeWindow(dpy,win,x,y, w, 1);
	XMapWindow(dpy, win);
	for(ch=isize;ch<=h;ch+=hinc) {
	    sleep_a_little(ANIM_DELAY/2);
	    XMoveResizeWindow(dpy,win,x,y, w, ch);
	    XSync(dpy,0);
	}
	break;
     default:
	XBell(dpy,100);
	fprintf(stderr,"WHARF INTERNAL BUG in OpenFolder()\n");
	exit(-1);
    }

    if (cw!=w || ch!=h || x != cx || cy != y || AnimationStyle==0)      
      XMoveResizeWindow(dpy,win,x,y,w,h);
}



void CloseFolder(int folder)
{
    int winc, hinc;
    int cx, cy, cw, ch;
    int x,y,w,h, junk_depth, junk_bd;
    int fsize, direction;
    Window win, junk_win;

#ifdef ENABLE_SOUND	
	PlaySound(WHEV_CLOSE_FOLDER);
#endif	
    if (folder<0) {
	winc = BUTTONWIDTH/ANIM_STEP_MAIN;
	hinc = BUTTONHEIGHT/ANIM_STEP_MAIN;
    } else {
	winc = BUTTONWIDTH/ANIM_STEP;
	hinc = BUTTONHEIGHT/ANIM_STEP;
    }
    if (folder < 0)  {
	win = main_win;
	direction = AnimationDir;
	if (direction==DIR_TOUP || direction==DIR_TODOWN)
	  fsize=BUTTONHEIGHT;
	else
	  fsize=BUTTONWIDTH;
    } else {
	direction = Folders[folder].direction;
	win = Folders[folder].win;
	if (direction==DIR_TOUP || direction==DIR_TODOWN)
	  fsize=hinc;
	else
	  fsize=winc;
    }
    if (AnimationStyle==0) {
	goto end;
    }
    XGetGeometry(dpy,win,&junk_win,&x,&y,&w,&h,&junk_bd,&junk_depth);
    XTranslateCoordinates(dpy,win,Root,x,y,&x,&y,&junk_win);
    switch (direction) {
     case DIR_TOLEFT:
	cx = x;
	for(cw=w;cw >= fsize; cw-=winc) {
	    XMoveResizeWindow(dpy,win,cx,y, cw,h);
	    XSync(dpy,0);
	    sleep_a_little(ANIM_DELAY);
	    cx += winc;
	}
	break;
     case DIR_TORIGHT:
	for(cw=w;cw >= fsize; cw-=winc) {
	    XMoveResizeWindow(dpy,win,x,y, cw,h);
	    XSync(dpy,0);
	    sleep_a_little(ANIM_DELAY);
	}	
	break;
     case DIR_TOUP:
	cy = y;
	for(ch=h;ch >= fsize; ch-=hinc) {
	    XMoveResizeWindow(dpy,win,x,cy, w,ch);
	    XSync(dpy,0);
	    sleep_a_little(ANIM_DELAY);
	    cy += hinc;
	}	
	break;
     case DIR_TODOWN: 
	for(ch=h;ch >= fsize; ch-=hinc) {
	    XMoveResizeWindow(dpy,win,x,y, w, ch);
	    XSync(dpy,0);
	    sleep_a_little(ANIM_DELAY);
	}	
	break;	
     default:
	XBell(dpy,100);
	fprintf(stderr,"WHARF INTERNAL BUG in CloseFolder()\n");
	exit(-1);
    }
    Folders[folder].direction = 0;
 end:    
    if (folder<0) {
	XResizeWindow(dpy,win,BUTTONWIDTH,BUTTONHEIGHT);
    } else {
	XUnmapWindow(dpy,win);
    }    
}


void MapFolder(int folder, int *LastMapped, int base_x, int base_y, int row, int col)
{
    int dir;
    
  if (Folders[folder].mapped ==ISMAPPED)
    {
      CloseFolder(folder);
      Folders[folder].mapped = NOTMAPPED;
      *LastMapped = -1;
    }
  else 
    {
      int folderx, foldery, folderw, folderh;
      if (*LastMapped != -1)
	{
	  CloseFolder(*LastMapped);
	  Folders[*LastMapped].mapped = NOTMAPPED;
	  *LastMapped = -1;
	}
      Folders[folder].mapped = ISMAPPED;
      if(num_columns < num_rows)
	{	    
	  if((base_x % display_width) > display_width / 2 ) {
	      folderx = base_x+(col-Folders[folder].count)*BUTTONWIDTH-2;
	      dir = DIR_TOLEFT;
	  }
	  else {
	      folderx = base_x+(col+1)*BUTTONHEIGHT+1;
	      dir = DIR_TORIGHT;
	  }	    
	  foldery = base_y+row*BUTTONHEIGHT;
	  folderw = Folders[folder].count*BUTTONWIDTH;
	  folderh = BUTTONHEIGHT;
	}
/* more kludgery */
      else if (num_columns == num_rows)
        {
/*
	  if((base_x % display_width) > display_width / 2 )
	    folderx = (col-Folders[folder].count)*BUTTONHEIGHT-2;
	  else
	    folderx = (col+1)*BUTTONHEIGHT+1;
*/
          if (ROWS) 
          {
            if ((base_y % display_height) > display_height / 2) {
		foldery = base_y-(Folders[folder].count)*BUTTONHEIGHT-2;
		dir = DIR_TOUP;
	    }	      
            else {
		foldery = base_y+BUTTONHEIGHT+2;
		dir = DIR_TODOWN;
	    }	      
            folderx = base_x;
	    folderw = BUTTONWIDTH;
	    folderh = (Folders[folder].count)*BUTTONHEIGHT;	      
          }
          else
          {
	    if((base_x % display_width) > display_width / 2 ) {
		folderx = base_x-(Folders[folder].count)*BUTTONWIDTH-2;
		dir = DIR_TOLEFT;
	    }	      
            else {
		folderx = base_x+BUTTONWIDTH+1;
		dir = DIR_TORIGHT;
	    }
            foldery = base_y-1;
	    folderh = BUTTONHEIGHT;
	    folderw = (Folders[folder].count)*BUTTONWIDTH;	      
          }
        }
      else
	{
	  if ((base_y % display_height) < display_height / 2) {
	    foldery  =base_y+(row+1)*BUTTONHEIGHT;
	    dir = DIR_TODOWN;
	  }
	  else {
	    foldery = base_y+(row-Folders[folder].count)*BUTTONHEIGHT;
	    dir = DIR_TOUP;
	  }
	  folderx = base_x+col*BUTTONWIDTH;
	  folderw = BUTTONWIDTH;
	  folderh = (Folders[folder].count)*BUTTONHEIGHT;
       	}

#ifdef ENABLE_SOUND	
	PlaySound(WHEV_OPEN_FOLDER);
#endif	
	XMoveWindow(dpy, Folders[folder].win, folderx, foldery);
	OpenFolder(folder,folderx, foldery, folderw, folderh, dir);	
	*LastMapped = folder;
    }
}

void 
DrawOutline(Drawable d, int w, int h)
{
    if (NoBorder)
      return;
/* top */
    XDrawLine( dpy, d, HiInnerGC, 0, 0, w-1, 0);
    /*
    XDrawLine( dpy, d, HiInnerGC, 0, 1, w-1, 1);
*/	 
/* bottom */
    XFillRectangle(dpy, d, NormalGC, 0,h-2,w-1,h-1);

/* left */
    XDrawLine( dpy, d, HiInnerGC, 0, 1, 0, h-1);
    /*
    XDrawLine( dpy, d, HiInnerGC, 1, 2, 1, h-2);
     */
/* right */
    XDrawLine( dpy, d, NormalGC, w-1, 1, w-1, h-1);
    XDrawLine( dpy, d, NormalGC, w-2, 2, w-2, h-2);
}

void RedrawUnpushed(Window *win, int i, int j)
{
    if (PushStyle!=0) {
	XMoveResizeWindow(dpy, Buttons[CurrentButton].IconWin,
			  (j-1)*BUTTONWIDTH ,(i-1)*BUTTONHEIGHT,
			  BUTTONWIDTH, BUTTONHEIGHT);
    } else {
	XCopyArea( dpy, Buttons[CurrentButton].completeIcon,
		  Buttons[CurrentButton].IconWin, NormalGC, 0, 0,
		  Buttons[BACK_BUTTON].icons[0].w, 
		  Buttons[BACK_BUTTON].icons[0].h,
		  0,0);
    }    
    RedrawWindow(win,0, CurrentButton, num_rows, num_columns);
    
    RedrawUnpushedOutline(win, i, j);
}

void RedrawUnpushedOutline(Window *win, int i, int j)
{
/* top */
    if (NoBorder) {
      return;
    }
    
    XDrawLine( dpy, *win, HiInnerGC, 
	      j*BUTTONWIDTH-BUTTONWIDTH, i*BUTTONHEIGHT-BUTTONHEIGHT,
	      j*BUTTONWIDTH,i*BUTTONHEIGHT-BUTTONHEIGHT);
/*
    XDrawLine( dpy, *win, HiInnerGC, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT+1, j*BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT+1);
 */
    /* left */
    XDrawLine( dpy, *win, HiInnerGC, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT+1, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-1);
   /* 
    XDrawLine( dpy, *win, HiInnerGC, j*BUTTONWIDTH
	      -BUTTONWIDTH+1, i*BUTTONHEIGHT-BUTTONHEIGHT+2, 
	      j*BUTTONWIDTH-BUTTONWIDTH+1 ,i*BUTTONHEIGHT-1);
    */
    /* right */
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH
	      +BUTTONWIDTH-2, i*BUTTONHEIGHT-BUTTONHEIGHT+2, j*BUTTONWIDTH
	      -BUTTONWIDTH+BUTTONWIDTH-2 ,i*BUTTONHEIGHT-1);
    
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH
	      +BUTTONWIDTH-1, i*BUTTONHEIGHT-BUTTONHEIGHT+1, 
	      j*BUTTONWIDTH-BUTTONWIDTH+BUTTONWIDTH-1 ,i*BUTTONHEIGHT-1);
    
    /* bottom */
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH
	      -BUTTONWIDTH+1, i*BUTTONHEIGHT-1, j*BUTTONWIDTH-BUTTONWIDTH
	      +BUTTONWIDTH-2,i*BUTTONHEIGHT-1);
    
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH
	      +1, i*BUTTONHEIGHT-2, j*BUTTONWIDTH-BUTTONWIDTH+BUTTONWIDTH-2,
	      i*BUTTONHEIGHT-2);
}

void RedrawPushed(Window *win, int i,int j)
{
    if (PushStyle!=0) {
	XMoveResizeWindow(dpy, Buttons[CurrentButton].IconWin,
			  2+(j-1)*BUTTONWIDTH,(i-1)*BUTTONHEIGHT+2,
			  BUTTONWIDTH-2, BUTTONHEIGHT-2);
    } else {
	XCopyArea( dpy, Buttons[CurrentButton].completeIcon,
		  Buttons[CurrentButton].IconWin, NormalGC, 2, 2,
		  Buttons[BACK_BUTTON].icons[0].w-2,
		  Buttons[BACK_BUTTON].icons[0].h-2, 4, 4);
	XCopyArea( dpy, Buttons[BACK_BUTTON].icons[0].icon,
		  Buttons[CurrentButton].IconWin, NormalGC, 2, 2,
		  2, BUTTONHEIGHT, 2, 2);
	XCopyArea( dpy, Buttons[BACK_BUTTON].icons[0].icon,
		  Buttons[CurrentButton].IconWin, NormalGC, 2, 2,
		  BUTTONWIDTH, 2, 2, 2);
    }
    RedrawWindow(win,0, CurrentButton, num_rows, num_columns);
    RedrawPushedOutline(win, i,j);
}

void RedrawPushedOutline(Window *win, int i, int j)
{
    GC gc1;
    /* Top Hilite */
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT, j*BUTTONWIDTH,i*BUTTONHEIGHT
	      -BUTTONHEIGHT);
/*
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT+1, j*BUTTONWIDTH,i*BUTTONHEIGHT
	      -BUTTONHEIGHT+1);
 */
    /* Left Hilite */
    
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-BUTTONHEIGHT+1, j*BUTTONWIDTH-BUTTONWIDTH,
	      i*BUTTONHEIGHT-1);
   /* 
    XDrawLine( dpy, *win, NormalGC, j*BUTTONWIDTH-BUTTONWIDTH
	      +1, i*BUTTONHEIGHT-BUTTONHEIGHT+2, j*BUTTONWIDTH-BUTTONWIDTH+1,
	      i*BUTTONHEIGHT-1);
    */
    if (PushStyle!=0) {
	gc1 = HiReliefGC;
    } else {
	gc1 = HiInnerGC;
    }      

    /* Right Hilite */
    
    XDrawLine( dpy, *win, HiReliefGC, j*BUTTONWIDTH
	      -BUTTONWIDTH+BUTTONWIDTH-2, i*BUTTONHEIGHT-BUTTONHEIGHT+2,
	      j*BUTTONWIDTH-BUTTONWIDTH+BUTTONWIDTH-2 ,i*BUTTONHEIGHT-1);
    
    XDrawLine( dpy, *win, gc1, j*BUTTONWIDTH
	      -BUTTONWIDTH+BUTTONWIDTH-1, i*BUTTONHEIGHT-BUTTONHEIGHT+1,
	      j*BUTTONWIDTH-BUTTONWIDTH+BUTTONWIDTH-1 ,i*BUTTONHEIGHT-1);
    
    /* Bottom Hilite */    
    XDrawLine( dpy, *win, gc1, j*BUTTONWIDTH
	      -BUTTONWIDTH+1, i*BUTTONHEIGHT-1, j*BUTTONWIDTH-BUTTONWIDTH
	      +BUTTONWIDTH-2,i*BUTTONHEIGHT-1);
    
    XDrawLine( dpy, *win, HiReliefGC, j*BUTTONWIDTH
	      -BUTTONWIDTH+1, i*BUTTONHEIGHT-2, j*BUTTONWIDTH-BUTTONWIDTH
	      +BUTTONWIDTH-2,i*BUTTONHEIGHT-2);
}
/************************************************************************
 *
 * Draw the window 
 *
 ***********************************************************************/
void RedrawWindow(Window *win, int firstbutton, int newbutton, 
		  int num_rows, int num_columns)
{
  int i,j,button;
  XEvent dummy;
  
  if(ready < 1)
    return;
  
  while (XCheckTypedWindowEvent (dpy, *win, Expose, &dummy));

  for(i=0;i<num_rows;i++)
    for(j=0;j<num_columns; j++)
      {
	button = firstbutton+i*num_columns + j;
	if((newbutton == -1)||(newbutton == button))
	  {
	      if(((Buttons[button].swallow == 3)||
	          (Buttons[button].swallow == 4))&&
		  (Buttons[button].IconWin != None))
		XSetWindowBorderWidth(dpy,Buttons[button].IconWin,0);
	  }
	  RedrawUnpushedOutline(win,i,j);
      }
}


/*******************************************************************
 * 
 * Create GC's
 * 
 ******************************************************************/
void CreateShadowGC(void)
{
  XGCValues gcv;
  unsigned long gcm;
    
    if(d_depth < 2)
    {
      back_pix = GetColor("white");
      fore_pix = GetColor("black");
    }
  else 
    {
      if (TextureType>0 && TextureType < 128) {
	  MakeShadowColors(dpy, FromColor, ToColor, &fore_pix, &light_grey);
      } else {
	  back_pix = GetColor("grey40");
	  fore_pix = GetColor("grey17");
	  light_grey = GetColor("white");
      }	
    }
  gcm = GCForeground|GCBackground|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;
      
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  NormalGC = XCreateGC(dpy, Root, gcm, &gcv);  

  gcv.foreground = back_pix;
  gcv.background = fore_pix;
  HiReliefGC = XCreateGC(dpy, Root, gcm, &gcv);

  gcv.foreground = light_grey;
  gcv.background = fore_pix;
  HiInnerGC = XCreateGC(dpy, Root, gcm, &gcv);
    
  gcm = GCForeground;
  gcv.foreground = fore_pix;
  MaskGC = XCreateGC(dpy, Root, gcm, &gcv);
    
  DefGC = DefaultGC(dpy, screen);    
}

/************************************************************************
 *
 * Sizes and creates the window 
 *
 ***********************************************************************/
void CreateWindow(void)
{
  int first_avail_button,i;

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);

  /* Allow for multi-width/height buttons */
  first_avail_button = num_buttons;
  
  if(num_buttons > MAX_BUTTONS)
    {
      fprintf(stderr,"%s: Out of Buttons!\n",MyName);
      exit(0);
    }
      
  /* size and create the window */
  if((num_rows == 0)&&(num_columns == 0))
    num_columns = 1;
  if(num_columns == 0)
    {
      num_columns = num_buttons/num_rows;
      while(num_rows * num_columns < num_buttons)
	num_columns++;
    }
  if(num_rows == 0)
    {
      num_rows = num_buttons/num_columns;
      while(num_rows * num_columns < num_buttons)
	num_rows++;
    }

  while(num_rows * num_columns < num_buttons)
    num_columns++;

  mysizehints.flags = PWinGravity| PResizeInc | PBaseSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = BUTTONWIDTH*num_columns;
  mysizehints.height= BUTTONHEIGHT*num_rows;
  mysizehints.width_inc = num_columns;
  mysizehints.height_inc = num_rows;
  mysizehints.base_height = num_rows - 1;
  mysizehints.base_width = num_columns - 1;

  if(x > -100000)
    {
      if (x <= -1)
	{
          mysizehints.x = DisplayWidth(dpy,screen) + x - mysizehints.width-1;
	  gravity = NorthEastGravity;
	}
      else if ((x == 0) && (flags & 16))
        mysizehints.x = DisplayWidth(dpy,screen) - mysizehints.width-2;
      else
	mysizehints.x = x;
      if ( y<0)
	{
	  mysizehints.y = DisplayHeight(dpy,screen) + y - mysizehints.height-2;
	  gravity = SouthWestGravity;
	}
      else 
	mysizehints.y = y;

      if((x < 0) && (y < 0))
	gravity = SouthEastGravity;	
      mysizehints.flags |= USPosition;
    }

  mysizehints.win_gravity = gravity;

  main_win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
				 mysizehints.width,mysizehints.height,
				 0,0,back_pix);

  for(i=0;i<num_folders;i++)
    {
      if(num_columns <num_rows)
	{
	  Folders[i].cols = 1;
	  Folders[i].rows = Folders[i].count;
	}
      else if ((num_columns == num_rows) && (!ROWS))
      {
	  Folders[i].cols = 1;
	  Folders[i].rows = Folders[i].count;
      }
      else
	{
	  Folders[i].cols = Folders[i].count;
	  Folders[i].rows = 1;
	}
      Folders[i].win = XCreateSimpleWindow(dpy, Root, 0,0,
					   BUTTONWIDTH*Folders[i].rows,BUTTONHEIGHT*Folders[i].cols,
					   0,0,back_pix);
      XSetWMNormalHints(dpy,Folders[i].win,&mysizehints);
      XSelectInput(dpy, Folders[i].win, MW_EVENTS);
     }
  
  XSetWMProtocols(dpy,main_win,&wm_del_win,1);

  XSetWMNormalHints(dpy,main_win,&mysizehints);

  XSelectInput(dpy, main_win, MW_EVENTS);
  change_window_name(MyName);
}


void nocolor(char *a, char *b)
{
 fprintf(stderr,"%s: can't %s %s\n", MyName, a,b);
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

/************************************************************************
 *
 * Dead pipe handler
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  int i,j,button;

#ifdef ENABLE_SOUND
    int val=-1;
    write(PlayerChannel[1],&val,sizeof(val));
    if (SoundThread != 0)
      kill(SoundThread,SIGUSR1);
#endif    
  for(i=0;i<num_rows;i++)
    for(j=0;j<num_columns; j++)
      {
	button = i*num_columns + j;
        /* delete swallowed windows, but not modules (afterstep handles those) */
	if(((Buttons[button].swallow == 3)||(Buttons[button].swallow == 4))&&
	    (Buttons[button].module == 0))
	  {
	    my_send_clientmessage(Buttons[button].IconWin,wm_del_win,CurrentTime);
	    XSync(dpy,0);
	  }
      }
  XSync(dpy,0);
  exit(0);
}

int TOTHEFOLDER = -1;
/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void ParseOptions(char *filename)
{
  char line[256];
  char *tline,*orig_tline,*tmp;
  int Clength, len;

  GetConfigLine(fd, &tline);
  orig_tline = tline;
  Clength = strlen(MyName);
  while(tline != NULL && tline[0] != '\0')
    {
      int g_x, g_y;
      unsigned width,height;
      while(isspace(*tline))tline++;

      if((strlen(&tline[0])>1)&&
	 (mystrncasecmp(tline,CatString3("*", MyName, "Geometry"),Clength+9)==0))
	{
	  tmp = &tline[Clength+9];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    {
	      tmp++;
	    }
	  tmp[strlen(tmp)-1] = 0;
	  
	  flags = XParseGeometry(tmp,&g_x,&g_y,&width,&height);
	  if (flags & WidthValue) 
	    w = width;
	  if (flags & HeightValue) 
	    h = height;
	  if (flags & XValue) 
	    x = g_x;
	  if (flags & YValue) 
	    y = g_y;
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Rows"),Clength+5)==0))
	{
	  len=sscanf(&tline[Clength+5],"%d",&num_rows);
	  if(len < 1)
	    num_rows = 0;
            ROWS = TRUE;
	}
      else if((strlen(&tline[0])>1)&&
	      (mystrncasecmp(tline,CatString3("*",MyName,"Columns"),Clength+8)==0))
	{
	  len=sscanf(&tline[Clength+8],"%d",&num_columns);
	  if(len < 1)
	    num_columns = 0;
            ROWS = FALSE;
	}
      else if((strlen(&tline[0])>1)&&
              (mystrncasecmp(tline,CatString3("*",MyName,"NoPush"),Clength+5)==0))
        {
	  Pushable = 0;
        } else if((strlen(&tline[0])>1)&&
              (mystrncasecmp(tline,CatString3("*",MyName,"FullPush"),Clength+9)==0))
        {
	  PushStyle = 1;
        } else if((strlen(&tline[0])>1)&&
              (mystrncasecmp(tline,CatString3("*",MyName,"NoBorder"),Clength+9)==0))
        {
	  NoBorder = 1;
        } else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"ForceSize"),Clength+10)==0)) {
	    ForceSize = 1;
        } else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"TextureType"),Clength+12)==0)) {
	    if (sscanf(&tline[Clength+12],"%d",&TextureType)<1)
	      TextureType = TEXTURE_BUILTIN;
	} else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"MaxColors"),Clength+10)==0)) {

	    if (sscanf(&tline[Clength+10],"%d",&MaxColors)<1)
	      MaxColors = 16;
	} else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"BgColor"),Clength+8)==0)) {
	    char *tmp;
	    tmp=safemalloc(strlen(tline));
	    sscanf(&tline[Clength+8],"%s",tmp);
	    BgColor=GetColor(tmp);
	    free(tmp);
	} else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"TextureColor"),Clength+13)==0)) {
	    char *c1, *c2;
	    XColor color;
	    XWindowAttributes attributes;

	    XGetWindowAttributes(dpy,Root,&attributes);
	    len = strlen(&tline[Clength+13]);
	    c1 = safemalloc(len);
	    c2 = safemalloc(len);
	    if (sscanf(&tline[Clength+13],"%s %s",c1,c2)!=2) {
		fprintf(stderr,"%s:You must specify two colors for the texture\n",MyName);
		FromColor[0]=0;
		FromColor[1]=0;
		FromColor[2]=0;
		ToColor[0]=0;
		ToColor[1]=0;
		ToColor[2]=0;
	    }
	    if (!XParseColor (dpy, attributes.colormap, c1, &color))
	    {
		nocolor("parse",c1);
		TextureType=TEXTURE_BUILTIN;
	    } else {
		FromColor[0]=color.red;
		FromColor[1]=color.green;
		FromColor[2]=color.blue;
	    }
	    if (!XParseColor (dpy, attributes.colormap, c2, &color))
	    {
		nocolor("parse",c2);
		TextureType=TEXTURE_BUILTIN;
	    } else {
		ToColor[0]=color.red;
		ToColor[1]=color.green;
		ToColor[2]=color.blue;
	    }
	    free(c1);
	    free(c2);
	} else if ((strlen(&tline[0])>1)
	  &&(mystrncasecmp(tline,CatString3("*",MyName,"Pixmap"),Clength+7)==0)) {
	    CopyString(&BgPixmapFile,&tline[Clength+7]);
	} else if((strlen(&tline[0])>1)&&
		  (mystrncasecmp(tline,CatString3("*",MyName,"AnimateMain"),Clength+12)==0))
        {
	    AnimateMain = 1;
        }
	else if((strlen(&tline[0])>1)&&
		(mystrncasecmp(tline,CatString3("*",MyName,"Animate"),Clength+8)==0))
        {
	    if ((tline[Clength+9]!='M') && (tline[Clength+9]!='m'))
	      AnimationStyle = 1;
        }
#ifdef ENABLE_SOUND	
	else if((strlen(&tline[0])>1)&&
		(mystrncasecmp(tline,CatString3("*",MyName,"Player"),Clength+7)==0))
        {
	    CopyString(&SoundPlayer, &tline[Clength+7]);
        } else if((strlen(&tline[0])>1)&&
		(mystrncasecmp(tline,CatString3("*",MyName,"Sound"),Clength+6)==0))
        {
	    bind_sound(&tline[Clength+6]);
	    SoundActive = 1;
        }
#endif	
	 else if((strlen(&tline[0])>1)
		  &&(mystrncasecmp(tline,CatString3("*", MyName, ""),Clength+1)==0)
		  && (num_buttons < MAX_BUTTONS))
	{
	    /* check if this is a invalid option */
	    if (!isspace(tline[Clength+1])) 
	      fprintf(stderr,"%s:invalid option %s\n",MyName,tline);
	    else
	      match_string(&tline[Clength+1]);
	}
      else if((strlen(&tline[0])>1)&&(mystrncasecmp(tline,"IconPath",8)==0))
	{
	  CopyString(&iconPath,&tline[8]);
	}
      else if((strlen(&tline[0])>1)&&(mystrncasecmp(tline,"PixmapPath",10)==0))
	{
	  CopyString(&pixmapPath,&tline[10]);
	}
#ifdef ENABLE_SOUND	
      else if((strlen(&tline[0])>1)&&(mystrncasecmp(tline,"*AudioDir",9)==0))
	{
	  CopyString(&SoundPath,&tline[9]);
	} 
      else if((strlen(&tline[0])>1)&&(mystrncasecmp(tline,"ModulePath",11)==0))
	{
	  CopyString(&ModulePath,&tline[11]);
	}
#endif	
      GetConfigLine(fd, &tline);
      orig_tline = tline;
    }
#ifdef ENABLE_DND
    /* ignore last button if there's nothing bound to it */
    if ((Buttons[num_buttons-1].drop_action!=NULL) &&
	(Buttons[num_buttons-1].iconno==0)) {
	num_buttons--;
    }
#endif
  return;
}

/*
 * Gets a word of a given index in the line, stripping any blanks
 * The returned word is newly allocated
 */
#ifdef ENABLE_SOUND
char *get_token(char *tline, int index)
{
    char *start, *end;
    int i,c,size;
    char *word;
    
    index++; /* index is 0 based */
    size = strlen(tline);
    i=c=0;
    start=end=tline;
    while (i<index && c<size) {
	start=end;
	while(isspace(*start) && c<size) {
	    start++;
	    c++;
	}
	end=start;
	while(!isspace(*end) && c<size) {
	    end++;
	    c++;
	}
	if (end==start) return NULL;
	i++;
    }
    if (i<index) return NULL;
    word=safemalloc(end-start+1);
    strncpy(word, start, end-start);
    word[end-start]=0;
    return word;
}

/**************************************************************************
 * 
 * Parses a sound binding
 * 
 **************************************************************************/
void bind_sound(char *tline)
{
    char *event, *sound;
    
    event = get_token(tline,0);
    if (event==NULL) {
	fprintf(stderr,"%s:bad sound binding %s\n",MyName,tline);
	return;
    }
    sound = get_token(tline,1);
    if (sound==NULL) {
	free(event);
	fprintf(stderr,"%s:bad sound binding %s\n",MyName,tline);
	return;
    }
    if (strcmp(event,"open_folder")==0) {
	Sounds[WHEV_OPEN_FOLDER]=sound;
    } else if (strcmp(event,"close_folder")==0) {
	Sounds[WHEV_CLOSE_FOLDER]=sound;
    } else if (strcmp(event,"open_main")==0) { 
	Sounds[WHEV_OPEN_MAIN]=sound;
    } else if (strcmp(event,"close_main")==0) {
	Sounds[WHEV_CLOSE_MAIN]=sound;
    } else if (strcmp(event,"push")==0) {
	Sounds[WHEV_PUSH]=sound;
    } else if (strcmp(event,"drop")==0) {
	Sounds[WHEV_DROP]=sound;
    } else {
	fprintf(stderr,"%s:bad event %s in sound binding\n",MyName,event);
	free(sound);
    }
    free(event);
    return;
}
#endif /* ENABLE_SOUND */

/**************************************************************************
 *
 * Parses a button command line from the config file 
 *
 *************************************************************************/
void match_string(char *tline)
{
  int len,i,i2,n,j,k;
  char *ptr,*start,*end,*tmp;
  struct button_info *actual;

  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;

  /* read next word. Its the button label. Users can specify "" 
   * NoIcon, or whatever to skip the label */
  /* read to next space */
  start = tline;
  end = tline;
  while((!isspace(*end))&&(*end!='\n')&&(*end!=0))
    end++;
  len = end - start;
  ptr = safemalloc(len+1);
  strncpy(ptr,start,len);
  ptr[len] = 0;

  if (strncmp(ptr,"~Folder",7)==0)
    {
      TOTHEFOLDER = -1;
      Folders[num_folders].firstbutton = num_folderbuttons;
      num_folders++;
      free(ptr);
      return;
    }

  if(TOTHEFOLDER==-1)
    {
      actual = &Buttons[num_buttons++];
      actual->parent = &main_win;
    }
  else
    {
      actual = &Buttons[--num_folderbuttons];
      actual->folder = num_folders;
      actual->parent = &Folders[num_folders].win;
    };
  
  actual->title = ptr;

  /* read next word. Its the icon bitmap/pixmap label. Users can specify "" 
   * NoIcon, or whatever to skip the label */
  /* read to next space */
  start = end;
  /* skip spaces */
  while(isspace(*start)&&(*start != '\n')&&(*start != 0))
    start++;
  end = start;
  while((!isspace(*end))&&(*end!='\n')&&(*end!=0))
    end++;
  len = end - start;
  ptr = safemalloc(len+1);
  strncpy(ptr,start,len);
  ptr[len] = 0;
  /* separate icon files to be overlaid */
  i2 = len;
  j=k=0;
  for(i=0;i<MAX_OVERLAY;i++) {
      while (ptr[j]!=',' && j<i2) j++;
      actual->icons[i].file=safemalloc(j-k+1);
      strncpy(actual->icons[i].file,&(ptr[k]),j-k);
      actual->icons[i].file[j-k]=0;
      actual->iconno++;
      j++;
      k=j;
      if (j>=i2) break;
  }
  tline = end;
  for (i=num_buttons - 2;i>=0;i--)
    {
      if (strcmp(Buttons[i].title, actual->title) == 0)
        {
          actual = &Buttons[i];
          num_buttons--;
          for(i=0;i<actual->iconno;i++) {
          free(actual->icons[i].file);
        }
      break;
    }
  }
  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
#ifdef ENABLE_DND
  if (mystrncasecmp(tline,"dropexec",8)==0) {
      /* get command to execute for dropped stuff */

      if(TOTHEFOLDER==-1) {
	  num_buttons--; /* make the next parsed thing the button for this */
	  free(ptr);
	  for(i=0;i<actual->iconno;i++) {
	      free(actual->icons[i].file);
	  }
	  actual->iconno=0;	  
      } else {
	  num_folderbuttons++;
	  free(ptr);
	  for(i=0;i<actual->iconno;i++) {
	      free(actual->icons[i].file);
	  }
	  actual->iconno=0;	  
	  fprintf(stderr,"Drop in Folders not supported. Ignoring option\n");
	  return;
      }

      tline=strstr(tline,"Exec");
      len = strlen(tline);
      tmp = tline + len -1;
      while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=tline)) {
	  tmp--;
	  len--;
      }
      ptr = safemalloc(len+1);
      actual->drop_action=ptr;
      strncpy(ptr,tline,len);
      ptr[len]=0;
  } else
#endif        
  if(mystrncasecmp(tline,"swallow",7)==0 || mystrncasecmp(tline,"maxswallow",10)==0)
    {
      /* Look for swallow "identifier", in which
	 case Wharf spawns and gobbles up window */
      i=7;
      while((tline[i] != 0)&&
	    (tline[i] != '"'))
	i++;
      i2=i+1;
      while((tline[i2] != 0)&&
	    (tline[i2] != '"'))
	i2++;
      actual->maxsize =
                 mystrncasecmp(tline,"maxswallow",10) == 0 ? 1 : 0;
      if(i2 - i >1)
	{
	  actual->hangon = safemalloc(i2-i);
	  strncpy(actual->hangon,&tline[i+1],i2-i-1);
	  actual->hangon[i2-i-1] = 0;
	  actual->swallow = 1;
	}
      n = 7;
      n = i2+1;
      while((isspace(tline[n]))&&(tline[n]!=0))
	n++;
      len = strlen(&tline[n]);
      tmp = tline + n + len -1;
      while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=(tline + n)))
	{
	  tmp--;
	  len--;
	}
      ptr = safemalloc(len+6);
      if(mystrncasecmp(&tline[n],"Module",6)==0)
	{
	  ptr[0] = 0;
          actual->module = 1;
	}	  
      else
	strcpy(ptr,"Exec ");
      i2 = strlen(ptr);
      strncat(ptr,&tline[n],len);
      ptr[i2+len]=0;
      SendText(fd,ptr,0);     
      free(ptr);
      actual->action = NULL;
    }
  else
    {
      if(!TOTHEFOLDER)
	{
	  Folders[num_folders].count++;
	}

      len = strlen(tline);
      tmp = tline + len -1;
      while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=tline))
	{
	  tmp--;
	  len--;
	}
      ptr = safemalloc(len+1);
      strncpy(ptr,tline,len);
      ptr[len]=0;
      
      if (strncmp(ptr,"Folder",6)==0)
	{
	  TOTHEFOLDER = 0;
	  Folders[num_folders].count = 0;
	  actual->folder = num_folders;
	  Folders[num_folders].mapped = NOTMAPPED;
	}
      actual->action = ptr;
    }
  return;
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;
  int i;

  if (XStringListToTextProperty(&str,1,&name) == 0) 
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
  XSetWMName(dpy,main_win,&name);
  XSetWMIconName(dpy,main_win,&name);
  for(i=0;i<num_folders;i++)
    {
      XSetWMName(dpy, Folders[i].win,&name);
      XSetWMIconName(dpy, Folders[i].win,&name);
    }
  XFree(name.value);
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
      if((count = ReadFvwmPacket(fd[1], header, &body)) > 0)
	{
	  process_message(header[1],body);
	  free(body);
	}
    }
  return 0;
}

void CheckForHangon(unsigned long *body)
{
  int button,i,j;
  char *cbody;

  cbody = (char *)&body[3];
  for(i=0;i<num_rows;i++)
    for(j=0;j<num_columns; j++)
      {
	button = i*num_columns + j;      
	if(Buttons[button].hangon != NULL)
	  {
	    if(strcmp(cbody,Buttons[button].hangon)==0)
	      {
		if(Buttons[button].swallow == 1)
		  {
		    Buttons[button].swallow = 2;
		    if(Buttons[button].IconWin != None)
		      {
			XDestroyWindow(dpy,Buttons[button].IconWin);
		      }
		    Buttons[button].IconWin = (Window)body[0];
		    free(Buttons[button].hangon);
		    Buttons[button].hangon = NULL;
		  }
		else
		  {
		    if (Buttons[button].swallow == 4)
		      Buttons[button].swallow = 3;
		    Buttons[button].up = 1;
		    free(Buttons[button].hangon);
		    Buttons[button].hangon = NULL;
		    RedrawWindow(&main_win,0, button, num_rows, num_columns);
		  }
	      }
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
/*    case M_TOGGLE_PAGING:
      pageing_enabled = body[0];
      RedrawWindow(&main_win,0, -1, num_rows, num_columns);
      break;
 pdg */
    case M_NEW_DESK:
      new_desk = body[0];
      RedrawWindow(&main_win,0, -1, num_rows, num_columns);
      break;
    case M_END_WINDOWLIST:
      RedrawWindow(&main_win,0, -1, num_rows, num_columns);
    case M_MAP:
      swallow(body);
    case M_RES_NAME:
    case M_RES_CLASS:
    case M_WINDOW_NAME:
      CheckForHangon(body);
      break;
    default:
      break;
    }
}





/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void my_send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;
  
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
}


void swallow(unsigned long *body)
{
  char *temp;
  int button,i,j;
  long supplied;

  for(i=0;i<num_rows;i++)
    for(j=0;j<num_columns; j++)
      {
	button = i*num_columns + j; 
	if((Buttons[button].IconWin == (Window)body[0])&&
	   (Buttons[button].swallow == 2))
	  {
	    Buttons[button].swallow = 3;
	    /* "Swallow" the window! */

	    XReparentWindow(dpy,Buttons[button].IconWin, main_win,
			    j*BUTTONWIDTH+4, i*BUTTONHEIGHT+4);
	    XMapWindow(dpy,Buttons[button].IconWin);
	    XSelectInput(dpy,(Window)body[0],
			 PropertyChangeMask);
	    if (Buttons[button].action)
	      {
		/*
	        XGrabButton(dpy, Button1Mask | Button2Mask, None,
		            (Window)body[0],
	                    False, ButtonPressMask | ButtonReleaseMask,
			    GrabModeAsync, GrabModeAsync, None, None);
	        XGrabButton(dpy, Button1Mask | Button2Mask, LockMask,
		*/
     unsigned *mods = lock_mods;
     do XGrabButton(dpy, Button1Mask | Button2Mask, *mods,

			    (Window)body[0],
	                    False, ButtonPressMask | ButtonReleaseMask,
			    GrabModeAsync, GrabModeAsync, None, None);
     while (*mods++);

	      }
	    if (Buttons[button].maxsize) {
	      Buttons[button].icons[0].w = 55;
	      Buttons[button].icons[0].h = 57;
	    }
	    else {
	      Buttons[button].icons[0].w = ICON_WIN_WIDTH;
	      Buttons[button].icons[0].h = ICON_WIN_HEIGHT;
	    }
	    if (!XGetWMNormalHints (dpy, Buttons[button].IconWin,
				    &Buttons[button].hints, 
				    &supplied))
	      Buttons[button].hints.flags = 0;
	    
	    XResizeWindow(dpy,(Window)body[0], Buttons[button].icons[0].w,
			  Buttons[button].icons[0].h);
	    XMoveWindow(dpy,Buttons[button].IconWin,
			j*BUTTONWIDTH +
			(BUTTONWIDTH - Buttons[button].icons[0].w)/2,
			i*BUTTONHEIGHT + 
			(BUTTONHEIGHT - Buttons[button].icons[0].h)/2);
	    
	    XFetchName(dpy, Buttons[button].IconWin, &temp);
	    XClearArea(dpy, main_win,j*BUTTONWIDTH, i*BUTTONHEIGHT,
		       BUTTONWIDTH,BUTTONHEIGHT,0);
	    if(strcmp(Buttons[button].title,"-")!=0)
	      CopyString(&Buttons[button].title, temp);
	    RedrawWindow(&main_win,0, -1, num_rows, num_columns);
	    XFree(temp);
	  }
      }
}



void FindLockMods(void)
{
  int m, i, knl;
  char* kn;
  KeySym ks;
  KeyCode kc, *kp;
  unsigned lockmask, *mp;
  XModifierKeymap* mm = XGetModifierMapping(dpy);
  lockmask = LockMask;
  if (mm)
    {
      kp = mm->modifiermap;
      for (m = 0; m < 8; m++)
        {
          for (i = 0; i < mm->max_keypermod; i++)
            {
      	if ((kc = *kp++) &&
      	    ((ks = XKeycodeToKeysym(dpy, kc, 0)) != NoSymbol))
      	  {
      	    kn = XKeysymToString(ks);
      	    knl = strlen(kn);
      	    if ((knl > 6) && (strcasecmp(kn + knl - 4, "lock") == 0))
      		lockmask |= (1 << m);
      	  }
            }
        }
      XFreeModifiermap(mm);
    }
  lockmask &= ~(ShiftMask | ControlMask);

  mp = lock_mods;
  for (m = 0, i = 1; i < 256; i++)
    {
      if ((i & lockmask) > m)
          m = *mp++ = (i & lockmask);
    }
  *mp = 0;
}


/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 * 
 ***********************************************************************/
void ConstrainSize (XSizeHints *hints, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))

  
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;

  if(hints->flags & PMinSize)
    {
      minWidth = hints->min_width;
      minHeight = hints->min_height;
      if(hints->flags & PBaseSize)
	{
	  baseWidth = hints->base_width;
	  baseHeight = hints->base_height;
	}
      else
	{
	  baseWidth = hints->min_width;
	  baseHeight = hints->min_height;
	}
    }
  else if(hints->flags & PBaseSize)
    {
      minWidth = hints->base_width;
      minHeight = hints->base_height;
      baseWidth = hints->base_width;
      baseHeight = hints->base_height;
    }
  else
    {
      minWidth = 1;
      minHeight = 1;
      baseWidth = 1;
      baseHeight = 1;
    }
  
  if(hints->flags & PMaxSize)
    {
      maxWidth = hints->max_width;
      maxHeight = hints->max_height;
    }
  else
    {
      maxWidth = 10000;
      maxHeight = 10000;
    }
  if(hints->flags & PResizeInc)
    {
      xinc = hints->width_inc;
      yinc = hints->height_inc;
    }
  else
    {
      xinc = 1;
      yinc = 1;
    }
  
  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth) dwidth = minWidth;
  if (dheight < minHeight) dheight = minHeight;
  
  if (dwidth > maxWidth) dwidth = maxWidth;
  if (dheight > maxHeight) dheight = maxHeight;
  
  
  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;
  
  
  /*
   * Third, adjust for aspect ratio
   */
#define maxAspectX hints->max_aspect.x
#define maxAspectY hints->max_aspect.y
#define minAspectX hints->min_aspect.x
#define minAspectY hints->min_aspect.y
  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   * 
   */
  
  if (hints->flags & PAspect)
    {
      if (minAspectX * dheight > minAspectY * dwidth)
	{
	  delta = makemult(minAspectX * dheight / minAspectY - dwidth,
			   xinc);
	  if (dwidth + delta <= maxWidth) 
	    dwidth += delta;
	  else
	    {
	      delta = makemult(dheight - dwidth*minAspectY/minAspectX,
			       yinc);
	      if (dheight - delta >= minHeight) dheight -= delta;
	    }
	}
      
      if (maxAspectX * dheight < maxAspectY * dwidth)
	{
	  delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
			   yinc);
	  if (dheight + delta <= maxHeight)
	    dheight += delta;
	  else
	    { 
	      delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
			       xinc);
	      if (dwidth - delta >= minWidth) dwidth -= delta;
	    }
	}
    }
  
  *widthp = dwidth;
  *heightp = dheight;
  return;
}


#ifdef ENABLE_SOUND
void   PlaySound(int event)
{
    int timestamp;
    
    if (!SoundActive) 
      return;
    if (Sounds[event]==NULL) return;
    write(PlayerChannel[1],&event,sizeof(event));
    timestamp = clock();
    write(PlayerChannel[1],&timestamp,sizeof(timestamp));    
    /*
    kill(SoundThread,SIGUSR1);
     */
}
#endif
