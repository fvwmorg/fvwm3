#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/shape.h>

#include "Goodies.h"
#include "minimail.xbm"

extern Display *dpy;
extern Window Root, win;
extern int win_width, win_height, win_y, win_border, d_depth,
       ScreenWidth, ScreenHeight, RowHeight; 
extern Pixel back, fore;
extern int Clength;
extern GC blackgc, hilite, shadow, checkered;

GC statusgc, dategc;
XFontStruct *StatusFont;
int stwin_width = 100, goodies_width = 0;
int anymail, unreadmail, newmail, mailcleared = 0;
int fontheight, clock_width;
char *mailpath = NULL;
char *clockfmt = NULL;
int BellVolume = DEFAULT_BELL_VOLUME;
Pixmap mailpix, wmailpix, pmask, pclip;
int NoMailCheck = False;
char *DateFore = "black",
     *DateBack = "LightYellow",
     *MailCmd  = "Exec xterm -e mail";
int IgnoreOldMail = False;
int ShowTips = False;
char *statusfont_string = "fixed";
int last_date = -1;

void cool_get_inboxstatus();

#define gray_width  8
#define gray_height 8
extern unsigned char gray_bits[];

/*                x  y  w  h  tw th open type *text   win */
TipStruct Tip = { 0, 0, 0, 0,  0, 0,   0,   0, NULL, None };


/* Parse 'goodies' specific resources */
void GoodiesParseConfig(char *tline, char *Module) {
  if(mystrncasecmp(tline,CatString3(Module, "BellVolume",""),
				Clength+10)==0) {
    BellVolume = atoi(&tline[Clength+11]);
  } else if(mystrncasecmp(tline,CatString3(Module, "Mailbox",""),
				Clength+11)==0) {
    if (mystrncasecmp(&tline[Clength+11], "None") == 0) {
      NoMailCheck = True;
    } else {
      UpdateString(&mailpath, &tline[Clength+11]); 
    }
  } else if(mystrncasecmp(tline,CatString3(Module, "ClockFormat",""),
			  Clength+11)==0) {
    UpdateString(&clockfmt, &tline[Clength+12]);
    clockfmt[strlen(clockfmt)-1] = 0;
  } else if(mystrncasecmp(tline, CatString3(Module, "StatusFont",""),
                          Clength+10)==0) {
    CopyString(&statusfont_string,&tline[Clength+11]);
  } else if(mystrncasecmp(tline,CatString3(Module, "TipsFore",""),
                               Clength+8)==0) {
    CopyString(&DateFore, &tline[Clength+9]);
  } else if(mystrncasecmp(tline,CatString3(Module, "TipsBack",""),
                               Clength+8)==0) {
    CopyString(&DateBack, &tline[Clength+9]);
  } else if(mystrncasecmp(tline,CatString3(Module, "MailCommand",""),
                               Clength+11)==0) {
    CopyString(&MailCmd, &tline[Clength+12]);
  } else if(mystrncasecmp(tline,CatString3(Module, "IgnoreOldMail",""),
                               Clength+13)==0) {
    IgnoreOldMail = True;
  } else if(mystrncasecmp(tline,CatString3(Module, "ShowTips",""),
                               Clength+8)==0) {
    ShowTips = True;
  }
}

void InitGoodies() {
  struct passwd *pwent;
  char tmp[1024];
  XGCValues gcval;
  unsigned long gcmask;
  
  if (mailpath == NULL) {
    strcpy(tmp, DEFAULT_MAIL_PATH);
    pwent = getpwuid(getuid());
    strcat(tmp, (char *) (pwent->pw_name));
    UpdateString(&mailpath, tmp);
  }

  if ((StatusFont = XLoadQueryFont(dpy, statusfont_string)) == NULL) {
    if ((StatusFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
      ConsoleMessage("Couldn't load fixed font. Exiting!\n");
      exit(1);
    }
  }

  fontheight = StatusFont->ascent + StatusFont->descent;
  
  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
  gcval.foreground = fore;
  gcval.background = back;
  gcval.font = StatusFont->fid;
  gcval.graphics_exposures = False;
  statusgc = XCreateGC(dpy, Root, gcmask, &gcval);
  
  if (!NoMailCheck) {
    mailpix = XCreatePixmapFromBitmapData(dpy, win, (char *)minimail_bits,
					  minimail_width, minimail_height,
					  fore,back, d_depth);
    wmailpix = XCreatePixmapFromBitmapData(dpy, win, (char *)minimail_bits,
					   minimail_width, minimail_height,
					   fore,GetColor("white"), d_depth);
    
    goodies_width += minimail_width + 7;
  }
  if (clockfmt) {
    struct tm *tms;
    static time_t timer;
    static char str[24];
    time(&timer);
    tms = localtime(&timer);
    strftime(str, 24, clockfmt, tms);
    clock_width = XTextWidth(StatusFont, str, strlen(str)) + 4;
  }
  else clock_width = XTextWidth(StatusFont, "XX:XX", 5) + 4;
  goodies_width += clock_width;
  stwin_width = goodies_width;
}

void Draw3dBox(Window wn, int x, int y, int w, int h)
{
  XClearArea(dpy, wn, x, y, w, h, False);
  
  XDrawLine(dpy, win, shadow, x, y, x+w-2, y);
  XDrawLine(dpy, win, shadow, x, y, x, y+h-2);
  
  XDrawLine(dpy, win, hilite, x, y+h-1, x+w-1, y+h-1);
  XDrawLine(dpy, win, hilite, x+w-1, y+h-1, x+w-1, y);
}

void DrawGoodies() {
  struct tm *tms;
  static char str[40];
  static time_t timer;
  static int last_mail_check = -1;

  time(&timer);
  tms = localtime(&timer);
  if (clockfmt) {
    strftime(str, 24, clockfmt, tms);
    if (str[0] == '0') str[0]=' ';
  } else {
    strftime(str, 15, "%R", tms);
  }

  Draw3dBox(win, win_width - stwin_width, 0, stwin_width, RowHeight);
  XDrawString(dpy,win,statusgc,
	      win_width - stwin_width + 4,
	      ((RowHeight - fontheight) >> 1) +StatusFont->ascent,
	      str, strlen(str));

  if (NoMailCheck) return;
  if (timer - last_mail_check >= 10) {
    cool_get_inboxstatus();
    last_mail_check = timer;
    if (newmail)
      XBell(dpy, BellVolume);
  }

  if (!mailcleared && (unreadmail || newmail))
    XCopyArea(dpy, wmailpix, win, statusgc, 0, 0,
	      minimail_width, minimail_height,
	      win_width - stwin_width + clock_width + 3,
	      ((RowHeight - minimail_height) >> 1));
  else if (!IgnoreOldMail /*&& !mailcleared*/ && anymail)
    XCopyArea(dpy, mailpix, win, statusgc, 0, 0,
              minimail_width, minimail_height,
              win_width - stwin_width + clock_width + 3,
              ((RowHeight - minimail_height) >> 1));

  if (Tip.open) {
    if (Tip.type == DATE_TIP)
      if (tms->tm_mday != last_date) /* reflect date change */
        CreateDateWindow(); /* This automatically deletes any old window */
    last_date = tms->tm_mday;
    RedrawTipWindow();
  }

}

int MouseInClock(int x, int y) {
  int clockl = win_width - stwin_width;
  int clockr = win_width - stwin_width + clock_width;
  return (x>=clockl && x<clockr && y>1 && y<RowHeight-2);
}

int MouseInMail(int x, int y) {
  int maill = win_width - stwin_width + clock_width;
  int mailr = win_width;
  return (x>=maill && x<mailr && y>1 && y<RowHeight-2);
}

void CreateDateWindow() {
  struct tm *tms;
  static time_t timer;
  static char str[40];

  time(&timer);
  tms = localtime(&timer);
  strftime(str, 40, "%A, %B %d, %Y", tms);
  last_date = tms->tm_mday;

  PopupTipWindow(win_width, 0, str);
}

void CreateMailTipWindow() {
  char str[20];

  if (!anymail) return;
  sprintf(str, "You have %smail", (newmail || unreadmail) ? "new " : "");
  PopupTipWindow(win_width, 0, str);
}

void RedrawTipWindow() {
  if (Tip.text) {
    XDrawString(dpy, Tip.win, dategc, 3, Tip.th-4,
                     Tip.text, strlen(Tip.text));
    XRaiseWindow(dpy, Tip.win);  /*****************/
  }
}

void PopupTipWindow(int px, int py, char *text) {
  int newx, newy;
  Window child;

  if (!ShowTips) return;

  if (Tip.win != None) DestroyTipWindow();

  Tip.tw = XTextWidth(StatusFont, text, strlen(text)) + 6;
  Tip.th = StatusFont->ascent + StatusFont->descent + 4;
  XTranslateCoordinates(dpy, win, Root, px, py, &newx, &newy, &child);

  Tip.x = newx;
  if (win_y == win_border)
    Tip.y = newy + RowHeight;
  else
    Tip.y = newy - Tip.th -2;

  Tip.w = Tip.tw;
  Tip.h = Tip.th;

  if (Tip.x+Tip.tw+4 > ScreenWidth-5) Tip.x = ScreenWidth-Tip.tw-9;
  if (Tip.x < 5) Tip.x = 5;

  UpdateString(&Tip.text, text);
  CreateTipWindow(Tip.x, Tip.y, Tip.w, Tip.h);
  if (Tip.open) XMapRaised(dpy, Tip.win);
}

void ShowTipWindow(int open) {
  if (open) {
    if (Tip.win != None) {
      XMapRaised(dpy, Tip.win);
    }
  } else {
    XUnmapWindow(dpy, Tip.win);
  }
  Tip.open = open;
}

void CreateTipWindow(int x, int y, int w, int h) {
  unsigned long gcmask;
  unsigned long winattrmask = CWBackPixel | CWBorderPixel | CWEventMask |
                              CWSaveUnder | CWOverrideRedirect;
  XSetWindowAttributes winattr;
  GC cgc, gc0, gc1;
  XGCValues gcval;
  Pixmap pchk;

  winattr.background_pixel = GetColor(DateBack);
  winattr.border_pixel = GetColor("black");
  winattr.override_redirect = True;
  winattr.save_under = True;
  winattr.event_mask = ExposureMask;

  Tip.win = XCreateWindow(dpy, Root, x, y, w+4, h+4, 0,
                          CopyFromParent, CopyFromParent, CopyFromParent,
                          winattrmask, &winattr);

  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
  gcval.graphics_exposures = False;
  gcval.foreground = GetColor(DateFore);
  gcval.background = GetColor(DateBack);
  gcval.font = StatusFont->fid;
  dategc = XCreateGC(dpy, Root, gcmask, &gcval);

  pmask = XCreatePixmap(dpy, Root, w+4, h+4, 1);
  pclip = XCreatePixmap(dpy, Root, w+4, h+4, 1);

  gcmask = GCForeground | GCBackground | GCFillStyle | GCStipple |
           GCGraphicsExposures;
  gcval.foreground = 1;
  gcval.background = 0;
  gcval.fill_style = FillStippled;
  pchk = XCreatePixmapFromBitmapData(dpy, Root, (char *)gray_bits,
                                     gray_width, gray_height, 1, 0, 1);
  gcval.stipple = pchk;
  cgc = XCreateGC(dpy, pmask, gcmask, &gcval);

  gcmask = GCForeground | GCBackground | GCGraphicsExposures | GCFillStyle;
  gcval.graphics_exposures = False;
  gcval.fill_style = FillSolid;
  gcval.foreground = 0;
  gcval.background = 0;
  gc0 = XCreateGC(dpy, pmask, gcmask, &gcval);

  gcval.foreground = 1;
  gcval.background = 1;
  gc1 = XCreateGC(dpy, pmask, gcmask, &gcval);

  XFillRectangle(dpy, pmask, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pmask, cgc, 3, 3, w+4, h+4);
  XFillRectangle(dpy, pmask, gc1, 0, 0, w+1, h+1);

  XFillRectangle(dpy, pclip, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pclip, gc1, 1, 1, w-1, h-1);

  XShapeCombineMask(dpy, Tip.win, ShapeBounding, 0, 0, pmask, ShapeSet);
  XShapeCombineMask(dpy, Tip.win, ShapeClip,     0, 0, pclip, ShapeSet);

  XFreeGC(dpy, gc0);
  XFreeGC(dpy, gc1);
  XFreeGC(dpy, cgc);
  XFreePixmap(dpy, pchk);
}

void DestroyTipWindow() {
  XFreePixmap(dpy, pclip);
  XFreePixmap(dpy, pmask);
  XFreeGC(dpy, dategc);
  XDestroyWindow(dpy, Tip.win);
  if (Tip.text) { free(Tip.text); Tip.text = NULL; }
  Tip.win = None;
}

/*-----------------------------------------------------*/
/* Get file modification time                          */
/* (based on the code of 'coolmail' By Byron C. Darrah */
/*-----------------------------------------------------*/

void cool_get_inboxstatus()
{
   static off_t oldsize = 0;
   off_t  newsize;
   struct stat st;
   int fd;

   fd = open (mailpath, O_RDONLY, 0);
   if (fd < 0)
   {
      anymail    = 0;
      newmail    = 0;
      unreadmail = 0;
      newsize = 0;
   }
   else
   {
      fstat(fd, &st);
      close(fd);
      newsize = st.st_size;

      if (newsize > 0)
         anymail = 1;
      else
         anymail = 0;

      if (st.st_mtime >= st.st_atime && newsize > 0)
         unreadmail = 1;
      else
         unreadmail = 0;

      if (newsize > oldsize && unreadmail) {
         newmail = 1;
         mailcleared = 0;
      }
      else
         newmail = 0;
   }

   oldsize = newsize;
}

/*---------------------------------------------------------------------------*/

void HandleMailClick(XEvent event) {
  static Time lastclick = 0;
  if (event.xbutton.time - lastclick < 250) {
    SendFvwmPipe(MailCmd, 0);
  }
  lastclick = event.xbutton.time;
  mailcleared = 1;
}
