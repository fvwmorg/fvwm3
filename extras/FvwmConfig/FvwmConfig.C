#include "config.h"

#include <stdio.h>
#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "WinBase.h"
#include "WinButton.h"
#include "WinButton.h"
#include "WinInput.h"
#include "WinColorSelector.h"

extern "C" {
#include "fvwmlib.h"
}
#include "../../fvwm/module.h"

#define ClickToFocus (1<<10)
#define SloppyFocus (1<<11)
int fd[2];
char *MyName;

extern "C" void SetupPipeHandler(void);

 
WinInput *inwin;
WinButton * cfocus, *cmouse,*fmouse, *fclick, *fsloppy, *nescroll, *escroll;
WinColorSelector *hibackselwin, *hiforeselwin, *backselwin, *foreselwin;
WinBase *base;
Window pwin;

void finish(WinBase *which)
{
  exit(0);
}

void abortaction(int newstate, WinButton *which)
{
  exit(0);
}

char tmp[2000];
int focus = -1;
int colormap = -1;
int edgescroll = -1;
float back_red = -1;
float back_green = -1;
float back_blue = -1;
float fore_red = -1;
float fore_green = -1;
float fore_blue = -1;

float hi_back_red = -1;
float hi_back_green = -1;
float hi_back_blue = -1;
float hi_fore_red = -1;
float hi_fore_green = -1;
float hi_fore_blue = -1;
int size_read = -1;


void testaction(int newstate, WinButton *which)
{
  int i;
  char *t;

  if(focus == 1)
    SendInfo(fd,"Style \"*\" ClickToFocus\n",0);
  else if(focus == 0)
    SendInfo(fd,"Style \"*\" MouseFocus\n",0);
  else if(focus == 2)
    SendInfo(fd,"Style \"*\" SloppyFocus\n",0);    

  if(colormap == 1)
    SendInfo(fd,"ColormapFocus FollowsFocus",0);
  else if (colormap == 0)
    SendInfo(fd,"ColormapFocus FollowsMouse",0);

  t = inwin->GetLine();
  if(strlen(inwin->GetLine()) > 2)
    {
      if(sscanf(t,"%dx%d", &i, &i) == 2)
	{
	  sprintf(tmp,"DeskTopSize %s\n",t);
	  SendInfo(fd,tmp,0);
	}
      else
	{
	  inwin->SetLabel("");
	  inwin->RedrawWindow(1);
	}
    }
  else
    {
      inwin->SetLabel("");
      inwin->RedrawWindow(1);
    }

  if(fore_red >= 0)
    {
      sprintf(tmp,"Style \"*\" ForeColor #%04x%04x%04x",(int)fore_red,
	      (int)fore_green, (int)fore_blue);
      SendInfo(fd,tmp,0);
    }
  if(back_red >= 0)
    {
      sprintf(tmp,"Style \"*\" BackColor #%04x%04x%04x",(int)back_red,
	      (int)back_green, (int)back_blue);
      SendInfo(fd,tmp,0);
    }

  if((hi_fore_red >= 0)||(hi_back_red >= 0))
    {
      sprintf(tmp,"HilightColor #%04x%04x%04x #%04x%04x%04x",(int)hi_fore_red,
	      (int)hi_fore_green, (int)hi_fore_blue,(int)hi_back_red,
	      (int)hi_back_green, (int)hi_back_blue);
      SendInfo(fd,tmp,0);
    }
  if(edgescroll == 1)
    SendInfo(fd,"EdgeScroll 100 100",0);
  else if (edgescroll == 0)
    SendInfo(fd,"EdgeScroll 0 0",0);	     
  
  if((focus == 1)||(focus == 0)||(fore_red >= 0)||(back_red >=0))
    SendInfo(fd,"Recapture",0);
}

void commitaction(int newstate, WinButton *which)
{
  char fname[2000];
  char *homedir;
  FILE *nfd;
  char *t;
  int i;

  testaction(newstate, which);
  homedir = getenv("HOME");
  if(homedir == NULL)
    fname[0] = 0;
  else
    strcpy(fname,homedir);
  strcat(fname,"/.fvwmrc-config");

  nfd = fopen(fname,"w");
  if(nfd)
    {
      if(focus == 1)
	fprintf(nfd,"Style \"*\" ClickToFocus\n");
      else if(focus == 0)
	fprintf(nfd,"Style \"*\" MouseFocus\n");
      else if(focus == 2)
	fprintf(nfd,"Style \"*\" SloppyFocus\n");          

      if(colormap == 1)
	fprintf(nfd,"ColormapFocus FollowsFocus\n");
      else if (colormap == 0)
	fprintf(nfd,"ColormapFocus FollowsMouse\n");
      
      t = inwin->GetLine();
      if(strlen(inwin->GetLine()) > 2)
	{
	  if(sscanf(t,"%dx%d", &i, &i) == 2)
	    fprintf(nfd,"DeskTopSize %s\n",inwin->GetLine());
	}
      
      if(edgescroll == 1)
	fprintf(nfd,"EdgeScroll 100 100\n");
      else if (edgescroll == 0)
	fprintf(nfd,"EdgeScroll 0 0\n");	     
      if(fore_red >= 0)
	{
	  fprintf(nfd,"Style \"*\" ForeColor #%04x%04x%04x\n",(int)fore_red,
		  (int)fore_green, (int)fore_blue);
	}
      if(back_red >= 0)
	{
	  fprintf(nfd,"Style \"*\" BackColor #%04x%04x%04x\n",(int)back_red,
		  (int)back_green, (int)back_blue);
	}      
      if((hi_fore_red >= 0)||(hi_back_red >= 0))
	{
	  fprintf(nfd,"HilightColor #%04x%04x%04x #%04x%04x%04x\n",
		  (int)hi_fore_red,(int)hi_fore_green, (int)hi_fore_blue,
		  (int)hi_back_red, (int)hi_back_green, (int)hi_back_blue);
	}
      fclose(nfd);
    }
  exit(0);
}

void backcolorhandler(float red, float green, float blue, 
		      WinColorSelector *which)
{
  back_red = red;
  back_green = green;
  back_blue = blue;
}

void forecolorhandler(float red, float green, float blue, 
		     WinColorSelector *which)
{
  fore_red = red;
  fore_green = green;
  fore_blue = blue;
}

void hibackcolorhandler(float red, float green, float blue, 
		      WinColorSelector *which)
{
  hi_back_red = red;
  hi_back_green = green;
  hi_back_blue = blue;
}

void hiforecolorhandler(float red, float green, float blue, 
		     WinColorSelector *which)
{
  hi_fore_red = red;
  hi_fore_green = green;
  hi_fore_blue = blue;
}

void  colormapmouseaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(colormap == 0)
	colormap = -1;
    }
  else
    {
      cfocus->PopOut(); 
      colormap = 0;
      cfocus->RedrawWindow(0);
    }
 
}

void  Scrollaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(edgescroll == 1)
	edgescroll = -1;
    }
  else
    {
      nescroll->PopOut();
      edgescroll = 1;
      nescroll->RedrawWindow(0);
    }
}
void  Noscrollaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(edgescroll == 0)
	 edgescroll= -1;
    }
  else
    {
      escroll->PopOut();
      edgescroll = 0;
      escroll->RedrawWindow(0);
    }
}
void  colormapfocusaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(colormap==1)
	colormap = -1;
    }
  else
    {
      cmouse->PopOut(); 
      colormap = 1;
      cmouse->RedrawWindow(0);
    }
}

void  focusmouseaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(focus == 0)
      focus = -1;
    }
  else
    {
      fclick->PopOut();
      focus = 0;
      fclick->RedrawWindow(0);
      fsloppy->PopOut();
      fsloppy->RedrawWindow(0);
    }
}
void  focusclickaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(focus == 1)
	focus = -1;
    }
  else
    {
      fmouse->PopOut();
      focus = 1;
      fmouse->RedrawWindow(0);
      fsloppy->PopOut();
      fsloppy->RedrawWindow(0);
    }
}
void  focussloppyaction(int newstate, WinButton *which)
{
  if(newstate == 1)
    {
      if(focus == 2)
	focus = -1;
    }
  else
    {
      focus = 2;
      fmouse->PopOut();
      fmouse->RedrawWindow(0);
      fclick->PopOut();
      fclick->RedrawWindow(0);
    }
}

void ReadPacket(int fd)
{
  unsigned long header[HEADER_SIZE];
  unsigned long *body;
  Pixel hipixel, hibackpixel;
  XColor colorcell;
  int n;

  ReadFvwmPacket(fd, header, &body);
  
  if(header[1] == M_FOCUS_CHANGE)
    {
      if(hi_fore_red < 0)
	{
	  hipixel = body[3];
	  hibackpixel = body[4];
	  colorcell.pixel = hipixel;
	  n = XQueryColor(hiforeselwin->dpy, 
			  DefaultColormap(hiforeselwin->dpy,
					  hiforeselwin->Screen),
			  &colorcell);
	  hi_fore_red = colorcell.red & 0xffff;
	  hi_fore_green = colorcell.green & 0xffff;
	  hi_fore_blue = colorcell.blue & 0xffff;
	  hiforeselwin->SetCurrentValue(hi_fore_red, hi_fore_green, hi_fore_blue);
	  colorcell.pixel = hibackpixel;
	  XQueryColor(hibackselwin->dpy, 
		      DefaultColormap(hiforeselwin->dpy,
				      hiforeselwin->Screen),
		      &colorcell);
	  hi_back_red = colorcell.red & 0xffff;
	  hi_back_green = colorcell.green & 0xffff;
	  hi_back_blue = colorcell.blue & 0xffff;
	  hibackselwin->SetCurrentValue(hi_back_red, hi_back_green, hi_back_blue);
	}
    }
  else if((header[1] == M_CONFIGURE_WINDOW)&&(body[0] == pwin))
    {
      if(fore_red < 0)
	{
	  hipixel = body[22];
	  hibackpixel = body[23];
	  colorcell.pixel = hipixel;
	  XQueryColor(foreselwin->dpy,
		      DefaultColormap(hiforeselwin->dpy,
				      hiforeselwin->Screen),
		      &colorcell);
	  fore_red = colorcell.red & 0xffff;
	  fore_green = colorcell.green & 0xffff;
	  fore_blue = colorcell.blue & 0xffff;
	  foreselwin->SetCurrentValue(fore_red, fore_green, fore_blue);
	  colorcell.pixel = hibackpixel;
	  XQueryColor(backselwin->dpy,
		      DefaultColormap(hiforeselwin->dpy,
				      hiforeselwin->Screen),
		      &colorcell);
	  back_red = colorcell.red & 0xffff;
	  back_green = colorcell.green & 0xffff;
	  back_blue = colorcell.blue & 0xffff;
	  backselwin->SetCurrentValue(back_red, back_green, back_blue);      
	  if(body[8] & ClickToFocus)
	    focus = 1;
	  if(body[8] & SloppyFocus)
	    focus = 2;
	  if(focus == 1)
	    {
	      fclick->PushIn();
	      fclick->RedrawWindow(0);
	      fmouse->PopOut();
	      fmouse->RedrawWindow(0);
	      fsloppy->PopOut();
	      fsloppy->RedrawWindow(0);
	    }
	  else if (focus == 2)
	    {
	      fclick->PopOut();
	      fclick->RedrawWindow(0);
	      fmouse->PopOut();
	      fmouse->RedrawWindow(0);
	      fsloppy->PushIn();
	      fsloppy->RedrawWindow(0);
	    }
	  else
	    {
	      fclick->PopOut();
	      fclick->RedrawWindow(0);
	      fmouse->PushIn();
	      fmouse->RedrawWindow(0);
	      fsloppy->PopOut();
	      fsloppy->RedrawWindow(0);
	    }
	  
	}
    }
  else if(header[1] == M_NEW_PAGE)
    {
      char size[100];
      int vxmax, vymax, sx,sy,nx,ny;

      if(size_read < 0)
	{
	  vxmax = body[3];
	  vymax = body[4];
	  sx = base->ScreenWidth();
	  sy = base->ScreenHeight();
	  nx = vxmax/sx + 1;
	  ny = vymax/sy + 1;
	  sprintf(size,"%dx%d",nx,ny);
	  inwin->SetLabel(size);    
	  size_read = 1;
	}
    }
  free((char *)body);
}

#define WINDOW_HEIGHT 400
int main(int argc, char **argv)
{
  char *temp, *s;

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
  
  /* Dead pipe == Fvwm died */
  SetupPipeHandler();
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  WinInitialize(argv,argc);
  WinAddInput(fd[1],ReadPacket);
  SendInfo(fd,"Send_WindowList",0);
  WinBase a(NULL,300,WINDOW_HEIGHT,0,0);
  pwin = a.win;
  base = &a;
  a.SetWindowName("Braindead Fvwm Configuration Editor");
  a.SetIconName("Braindead");
  a.SetBevelWidth(3);
  a.SetGeometry((a.ScreenWidth() - 200)/2,(a.ScreenHeight()-200)/2,
		300,WINDOW_HEIGHT,
		1,1,500,500,1,1,1,1,CenterGravity); 

  WinText keyboard(&a,280,20,10,6,"Keyboard Focus");
  keyboard.SetBevelWidth(0);
  WinButton focusclick(&a,90,20,15,32,"Click");
  WinButton focusmouse(&a,90,20,105,32,"Follows Mouse");
  WinButton focussloppy(&a,90,20,195,32,"Trails Mouse");
  focusmouse.SetToggleAction(focusmouseaction);
  focusclick.SetToggleAction(focusclickaction);
  focussloppy.SetToggleAction(focussloppyaction);

  WinBase divider1(&a,290,4,5,58);
  divider1.SetBevelWidth(2);
  WinText colormap(&a,280,20,10,62,"Colormap Focus");
  colormap.SetBevelWidth(0);
  WinButton colormapfocus(&a,100,20,50,88,"Follows Focus");
  WinButton colormapmouse(&a,100,20,150,88,"Follows Mouse");
  colormapmouse.SetToggleAction(colormapmouseaction);
  colormapfocus.SetToggleAction(colormapfocusaction);

  WinBase divider2(&a,290,4,5,114);
  divider2.SetBevelWidth(2);
  WinText desktop(&a,280,20,10,120,"Desktop");
  desktop.SetBevelWidth(0);
  WinText desktopsize(&a,30,20,20,144,"Size");
  desktopsize.SetBevelWidth(0);
  WinInput deskinput(&a,40,20,50,144,NULL);
  deskinput.SetBevelWidth(2);

  WinButton EdgeScroll(&a,80,20,110,144,"Edge Scroll");
  WinButton NoEdgeScroll(&a,100,20,190,144,"No Edge Scroll");
  EdgeScroll.SetToggleAction(Scrollaction);
  NoEdgeScroll.SetToggleAction(Noscrollaction);

  WinBase divider3(&a,290,4,5,170);
  divider3.SetBevelWidth(2);
  WinText backcolorlabel(&a,140,20,5,176,"Default Back Color");
  WinText forecolorlabel(&a,140,20,155,176,"Default Fore Color");
  WinColorSelector defaultbackcolor(&a,140,70,5,196);
  WinColorSelector defaultforecolor(&a,140,70,155,196);
  backselwin = &defaultbackcolor;
  foreselwin = &defaultforecolor;
  defaultbackcolor.SetBevelWidth(0);
  defaultforecolor.SetBevelWidth(0);
  backcolorlabel.SetBevelWidth(0);
  forecolorlabel.SetBevelWidth(0);
  defaultbackcolor.SetMotionAction(backcolorhandler);
  defaultforecolor.SetMotionAction(forecolorhandler);

  WinBase divider4(&a,290,4,5,272);
  divider4.SetBevelWidth(2);
  WinText hibackcolorlabel(&a,140,20,5,278,"Hilight Back Color");
  WinText hiforecolorlabel(&a,140,20,155,278,"Hilight Fore Color");

  WinColorSelector hibackcolor(&a,140,70,5,298);
  WinColorSelector hiforecolor(&a,140,70,155,298);
  hibackselwin = &hibackcolor;
  hiforeselwin = &hiforecolor;
  hibackcolor.SetBevelWidth(0);
  hiforecolor.SetBevelWidth(0);
  hibackcolorlabel.SetBevelWidth(0);
  hiforecolorlabel.SetBevelWidth(0);
  hibackcolor.SetMotionAction(hibackcolorhandler);
  hiforecolor.SetMotionAction(hiforecolorhandler);

  
  WinButton testbutton(&a,60,20,30,WINDOW_HEIGHT-30,"Test");
  WinButton commitbutton(&a,60,20,120,WINDOW_HEIGHT-30,"Commit");
  testbutton.MakeMomentary();
  WinButton abortbutton(&a,60,20,210,WINDOW_HEIGHT-30,"Abort");

  commitbutton.SetToggleAction(commitaction);
  testbutton.SetToggleAction(testaction);
  abortbutton.SetToggleAction(abortaction);
  fclick = &focusclick;
  fsloppy = &focussloppy;
  fmouse = &focusmouse;
  cmouse = &colormapmouse;
  cfocus = &colormapfocus;
  escroll = &EdgeScroll;
  nescroll = &NoEdgeScroll;
  inwin = &deskinput;
  a.Map();

  while(1)
    {
      WinLoop();
    }
  return 0;
}
