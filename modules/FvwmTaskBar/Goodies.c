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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include "libs/ftime.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Colorset.h"
#include "libs/Module.h"
#include "libs/Rectangles.h"
#include "libs/FGettext.h"
#include "Goodies.h"
#include "FvwmTaskBar.h"
#include "Mallocs.h"
#include "Colors.h"
#include "minimail.xbm"

#define MAILCHECK_DEFAULT 10

extern Display *dpy;
extern Window Root, win;
extern int Fvwm_fd[2];
extern int screen;
extern char *Module;
extern int win_width, win_height, win_y, win_border, RowHeight, Midline;
extern rectangle screen_g;
extern Pixel back, fore;
extern int colorset;
extern int Clength;
extern GC blackgc, hilite, shadow, checkered;
extern FlocaleWinString *FwinString;

GC statusgc = 0;
GC tipsgc = 0;
FlocaleFont *FStatusFont;
int stwin_width = 100, goodies_width = 0;
int anymail, unreadmail, newmail, mailcleared = 0;
int goodies_fontheight, clock_width;
char *mailpath = NULL;
char *clockfmt = NULL;
char *datefmt = NULL;
Bool do_display_clock = True;
int BellVolume = DEFAULT_BELL_VOLUME;
Pixmap mailpix = None;
Pixmap wmailpix = None;
Pixmap pmask = None;
Pixmap pclip = None;
Bool do_check_mail = True;
Bool has_mailpath = True;
int mailcheck_interval = MAILCHECK_DEFAULT;
char *TipsFore = "black",
     *TipsBack = "LightYellow",
     *MailCmd  = "Exec xterm -e mail";
int tipscolorset = -1;
int IgnoreOldMail = False;
int ShowTips = False;
char *statusfont_string = NULL;
int last_date = -1;

void cool_get_inboxstatus(void);

#define gray_width  8
#define gray_height 8
extern unsigned char gray_bits[];

/*                x  y  w  h  tw th open type *text   win */
TipStruct Tip = { 0, 0, 0, 0,  0, 0,   0,   0, NULL, None };


static void CreateOrUpdateGoodyGC(void)
{
  XGCValues gcval;
  unsigned long gcmask;
  Pixel pfore;
  Pixel pback;

  if (colorset >= 0)
  {
    pfore = Colorset[colorset].fg;
    pback = Colorset[colorset].bg;
  }
  else
  {
    pfore = fore;
    pback = back;
  }
  gcmask = GCForeground | GCBackground | GCGraphicsExposures;
  gcval.foreground = pfore;
  gcval.background = pback;
  gcval.graphics_exposures = False;
  if (FStatusFont->font != NULL)
  {
    gcval.font = FStatusFont->font->fid;
    gcmask = GCFont;
  }
  if (statusgc)
    XChangeGC(dpy, statusgc, gcmask, &gcval);
  else
    statusgc = fvwmlib_XCreateGC(dpy, win, gcmask, &gcval);

  if (do_check_mail)
  {
    if (mailpix)
      XFreePixmap(dpy, mailpix);
    mailpix = XCreatePixmapFromBitmapData(
      dpy, win, (char *)minimail_bits, minimail_width, minimail_height,
      pfore, pback, Pdepth);
    if (wmailpix)
      XFreePixmap(dpy, wmailpix);
    wmailpix = XCreatePixmapFromBitmapData(
      dpy, win, (char *)minimail_bits, minimail_width, minimail_height,
      PictureBlackPixel(), PictureWhitePixel(), Pdepth);
    goodies_width += minimail_width + 7;
  }
  else if (do_display_clock)
  {
    goodies_width += 3;
  }
  else
  {
	  goodies_width += 0;
  }

}

Bool change_goody_colorset(int cset, Bool force)
{
  if (cset < 0 || (cset != tipscolorset && !force))
    return False;
  CreateOrUpdateGoodyGC();

  return True;
}

static char *goodyopts[] =
{
  "BellVolume",
  "Mailbox",
  "Mailcheck",
  "ClockFormat",
  "StatusFont",
  "TipsFore",
  "TipsBack",
  "TipsColorset",
  "MailCommand",
  "IgnoreOldMail",
  "ShowTips",
  "DateFormat",
  NULL
};

/* Parse 'goodies' specific resources */
Bool GoodiesParseConfig(char *tline)
{
  char *rest;
  char *option;
  int i;

  option = tline + Clength;
  i = GetTokenIndex(option, goodyopts, -1, &rest);
  while (*rest && *rest != '\n' && isspace(*rest))
    rest++;
  switch(i)
  {
  case 0: /* BellVolume */
    BellVolume = atoi(rest);
    break;
  case 1: /* Mailbox */
    if (strcasecmp(rest, "None") == 0)
    {
      has_mailpath = False;
    }
    else
    {
      int len;

      UpdateString(&mailpath, rest);
      len = strlen(mailpath);
      if (len > 0 && mailpath[len-1] == '\n')
	mailpath[len-1] = 0;
      has_mailpath = True;
    }
    do_check_mail = (has_mailpath && (mailcheck_interval > 0));
    break;
  case 2: /* Mailcheck */
    mailcheck_interval = MAILCHECK_DEFAULT;
    sscanf(rest, "%d", &mailcheck_interval);
    do_check_mail = (has_mailpath && (mailcheck_interval > 0));
    break;
  case 3: /* ClockFormat */
    UpdateString(&clockfmt, rest);
    do_display_clock = True;
    if (clockfmt && clockfmt[0] == '\0' && datefmt && datefmt[0] == '\0')
    {
	    do_display_clock = False;
    }
    break;
  case 4: /* StatusFont */
    CopyStringWithQuotes(&statusfont_string, rest);
    break;
  case 5: /* TipsFore */
    CopyString(&TipsFore, rest);
    tipscolorset = -1;
    break;
  case 6: /* TipsBack */
    CopyString(&TipsBack, rest);
    tipscolorset = -1;
    break;
  case 7: /* TipsColorset */
    tipscolorset = -1;
    tipscolorset = atoi(rest);
    AllocColorset(tipscolorset);
    break;
  case 8: /* MailCommand */
    CopyString(&MailCmd, rest);
    break;
  case 9: /* IgnoreOldMail */
    IgnoreOldMail = True;
    break;
  case 10: /* ShowTips */
    ShowTips = True;
    break;
  case 11: /* DateFormat */
    UpdateString(&datefmt, rest);
    do_display_clock = True;
    if (clockfmt && clockfmt[0] == '\0' && datefmt && datefmt[0] == '\0')
    {
	    do_display_clock = False;
    }
    break;
  default:
    /* unknow option */
    return False;
  } /* switch */

  return True;
}

void LoadGoodiesFont(void)
{
	if ((FStatusFont =
	     FlocaleLoadFont(dpy, statusfont_string, Module)) == NULL)
	{
		fprintf(stderr, "%s: Couldn't load font. Exiting!\n",Module);
		exit(1);
	}
	goodies_fontheight = FStatusFont->height;
}

void InitGoodies(void)
{
	struct passwd *pwent;
	char tmp[1024];

	if (mailpath == NULL)
	{
		strcpy(tmp, DEFAULT_MAIL_PATH);
		pwent = getpwuid(getuid());
		strcat(tmp, (char *) (pwent->pw_name));
		UpdateString(&mailpath, tmp);
	}

	CreateOrUpdateGoodyGC();
	if (clockfmt)
	{
		if (!do_display_clock)
		{
			clock_width = 0;
		}
		else
		{
			struct tm *tms;
			static time_t timer;
			static char str[24];
			time(&timer);
			tms = localtime(&timer);
			strftime(str, 24, clockfmt, tms);
			clock_width = FlocaleTextWidth(
				FStatusFont, str, strlen(str)) + 4;
		}
	}
	else
	{
		clock_width = FlocaleTextWidth(FStatusFont, "XX:XX", 5) + 4;
	}
	goodies_width += clock_width;
	stwin_width = goodies_width;
}

void Draw3dBox(Window wn, int x, int y, int w, int h, XRectangle *bounding)
{
	XRectangle r;

	if (bounding)
	{
		r.x = bounding->x;
		r.y = bounding->y;
		r.width = bounding->width;
		r.height = bounding->height;
	}
	else
	{
		r.x = x;
		r.y = y;
		r.width = w;
		r.height = h;
	}
	XClearArea(dpy, wn, r.x, r.y, r.width, r.height, False);

	XDrawLine(dpy, win, shadow, x, y, x+w-2, y);
	XDrawLine(dpy, win, shadow, x, y, x, y+h-2);

	XDrawLine(dpy, win, hilite, x, y+h-1, x+w-1, y+h-1);
	XDrawLine(dpy, win, hilite, x+w-1, y+h-1, x+w-1, y);
}

void DrawGoodies(XEvent *evp)
{
	XRectangle rect;
	struct tm *tms;
	static char str[40];
	static time_t timer;
	static int last_mail_check = -1;
	int x = win_width - stwin_width;
	int y = 0;
	int w = stwin_width;
	int h = RowHeight;
	Region t_region;

	if (goodies_width == 0)
	{
		return;
	}
	if (evp)
	{
		if (!frect_get_intersection(
			x, y, w, h,
			evp->xexpose.x, evp->xexpose.y,
			evp->xexpose.width, evp->xexpose.height,
			&rect))
		{
			return;
		}
	}
	else
	{
		rect.x = x;
		rect.y = y;
		rect.width = w;
		rect.height = h;
	}
	time(&timer);
	tms = localtime(&timer);
	if (clockfmt)
	{
		strftime(str, 24, clockfmt, tms);
		if (str[0] == '0') str[0]=' ';
	}
	else
	{
		strftime(str, 15, "%R", tms);
	}

	Draw3dBox(win, x, y, w, h, &rect);
	t_region = XCreateRegion();
	XUnionRectWithRegion (&rect, t_region, t_region);
	FwinString->win = win;
	FwinString->gc = statusgc;
	FwinString->str = str;
	FwinString->x = x + 4;
	FwinString->y = ((RowHeight - goodies_fontheight) >> 1) +
		FStatusFont->ascent;
	FwinString->flags.has_clip_region = True;
	FwinString->clip_region = t_region;
	if (colorset >= 0)
	{
		FwinString->colorset = &Colorset[colorset];
		FwinString->flags.has_colorset = True;
	}
	else
	{
		FwinString->flags.has_colorset = False;
	}
	FlocaleDrawString(dpy, FStatusFont, FwinString, 0);
	FwinString->flags.has_clip_region = False;
	XDestroyRegion(t_region);

	if (!do_check_mail)
	{
		return;
	}
	if (timer - last_mail_check >= mailcheck_interval)
	{
		cool_get_inboxstatus();
		last_mail_check = timer;
		if (newmail)
			XBell(dpy, BellVolume);
	}

	if (!mailcleared && (unreadmail || newmail))
	{
		XCopyArea(dpy, wmailpix, win, statusgc, 0, 0,
			  minimail_width, minimail_height,
			  win_width - stwin_width + clock_width + 3,
			  ((RowHeight - minimail_height) >> 1));
	}
	else if (!IgnoreOldMail /*&& !mailcleared*/ && anymail)
	{
		XCopyArea(dpy, mailpix, win, statusgc, 0, 0,
			  minimail_width, minimail_height,
			  win_width - stwin_width + clock_width + 3,
			  ((RowHeight - minimail_height) >> 1));
	}

	if (Tip.open)
	{
		if (Tip.type == DATE_TIP)
		{
			if (tms->tm_mday != last_date)
			{
				/* reflect date change */
				CreateDateWindow(); /* This automatically
						     * deletes any old window */
			}
		}
		last_date = tms->tm_mday;
		RedrawTipWindow();
	}
}

int MouseInClock(int x, int y)
{
  int clockl = win_width - stwin_width;
  int clockr = win_width - stwin_width + clock_width + (do_check_mail ? 2 : 3);
  return (x>clockl && x<clockr && y>1 && y<RowHeight-2);
}

int MouseInMail(int x, int y)
{
  int maill = win_width - stwin_width + clock_width + 2;
  int mailr = win_width - (do_check_mail ? 0 : 3);
  return (x>=maill && x<mailr && y>1 && y<RowHeight-2);
}

void CreateDateWindow(void)
{
  struct tm *tms;
  static time_t timer;
  static char str[40];

  time(&timer);
  tms = localtime(&timer);
  if (datefmt) {
    strftime(str, 40, datefmt, tms);
  } else {
    strftime(str, 40, "%A, %B %d, %Y", tms);
  }
  last_date = tms->tm_mday;

  PopupTipWindow(win_width, 0, str);
}

void CreateMailTipWindow()
{
	const char *str;

	if (!anymail)
	{
		str = _("No new mail");
	}
	else if (newmail || unreadmail)
	{
		str = _("You have new mail");
	}
	else
	{
		str = _("You have mail");
	}

	PopupTipWindow(win_width, 0, str);
}

void RedrawTipWindow(void)
{
  /* FIXME: I am sure that we redraw to often */
  if (Tip.text) {
    FwinString->win = Tip.win;
    FwinString->gc = tipsgc;
    FwinString->str = Tip.text;
    FwinString->x = 3;
    FwinString->y = 2 + FStatusFont->ascent;
    FwinString->flags.has_clip_region = False;
    if (tipscolorset >= 0)
    {
       FwinString->colorset = &Colorset[tipscolorset];
       FwinString->flags.has_colorset = True;
    }
    else
    {
       FwinString->flags.has_colorset = False;
    }
    if (FftSupport && FStatusFont->fftf.fftfont != NULL)
    {
	    XClearArea(dpy, Tip.win, 0, 0, Tip.w, Tip.h, False);
    }
    FlocaleDrawString(dpy, FStatusFont, FwinString, 0);
    XRaiseWindow(dpy, Tip.win);
  }
}

void PopupTipWindow(int px, int py, const char *text)
{
  int newx, newy;
  Window child;

  if (!ShowTips)
    return;
  if (Tip.win != None)
    DestroyTipWindow();

  Tip.tw = FlocaleTextWidth(FStatusFont, (char *)text, strlen(text)) + 6;
  Tip.th = FStatusFont->height + 4;
  XTranslateCoordinates(dpy, win, Root, px, py, &newx, &newy, &child);

  Tip.x = newx;
/*   if (win_y == win_border) */
  if (win_y < Midline)
    Tip.y = newy + RowHeight;
  else
    Tip.y = newy - Tip.th -2;

  Tip.w = Tip.tw;
  Tip.h = Tip.th;

  if (Tip.x + Tip.tw + 4 > screen_g.x + screen_g.width - 5)
    Tip.x = screen_g.x + screen_g.width - Tip.tw - 9;
  if (Tip.x < screen_g.x + 5)
    Tip.x = screen_g.x + 5;

  UpdateString(&Tip.text, text);
  CreateTipWindow(Tip.x, Tip.y, Tip.w, Tip.h);

  if (Tip.open)
    XMapRaised(dpy, Tip.win);
}

void ShowTipWindow(int open)
{
  if (!ShowTips) return;
  if (open)
  {
    if (Tip.win != None)
    {
      XMapRaised(dpy, Tip.win);
      RedrawTipWindow();
    }
  }
  else
  {
    XUnmapWindow(dpy, Tip.win);
  }
  Tip.open = open;
}

void CreateTipWindow(int x, int y, int w, int h)
{
  unsigned long gcmask;
  unsigned long winattrmask = CWBackPixel | CWBorderPixel | CWEventMask |
			      CWColormap |CWSaveUnder | CWOverrideRedirect;
  XSetWindowAttributes winattr;
  GC cgc = None;
  GC gc0 = None;
  GC gc1 = None;
  XGCValues gcval;
  Pixmap pchk = None;
  Pixel tip_fore;
  Pixel tip_back;
  colorset_t *cset = NULL;

  if (tipscolorset >= 0)
  {
    cset = &Colorset[tipscolorset];
    tip_fore = cset->fg;
    tip_back = cset->bg;
  }
  else
  {
    tip_fore = GetColor(TipsFore);
    tip_back = GetColor(TipsBack);
  }

  winattr.background_pixel = tip_back;
  winattr.border_pixel = PictureBlackPixel();
  winattr.colormap = Pcmap;
  winattr.override_redirect = True;
  winattr.save_under = True;
  winattr.event_mask = ExposureMask;
  Tip.win = XCreateWindow(dpy, Root, x, y, w+4, h+4, 0, Pdepth, InputOutput,
			  Pvisual, winattrmask, &winattr);

  gcmask = GCForeground | GCBackground | GCGraphicsExposures;
  gcval.graphics_exposures = False;
  gcval.foreground = tip_fore;
  gcval.background = tip_back;
  if (FStatusFont->font != NULL)
  {
    gcval.font = FStatusFont->font->fid;
    gcmask |= GCFont;
  }
  if (tipsgc)
    XChangeGC(dpy, tipsgc, gcmask, &gcval);
  else
    tipsgc = fvwmlib_XCreateGC(dpy, Tip.win, gcmask, &gcval);

  pmask = XCreatePixmap(dpy, Tip.win, w+4, h+4, 1);
  pclip = XCreatePixmap(dpy, Tip.win, w+4, h+4, 1);

  gcmask = GCForeground | GCBackground | GCFillStyle | GCStipple |
	   GCGraphicsExposures;
  gcval.foreground = 1;
  gcval.background = 0;
  gcval.fill_style = FillStippled;
  if (pchk == None)
    pchk = XCreatePixmapFromBitmapData(dpy, Tip.win, (char *)gray_bits,
				       gray_width, gray_height, 1, 0, 1);
  gcval.stipple = pchk;
  if (cgc)
    XChangeGC(dpy, cgc, gcmask, &gcval);
  else
    cgc = fvwmlib_XCreateGC(dpy, pmask, gcmask, &gcval);

  gcmask = GCForeground | GCBackground | GCGraphicsExposures | GCFillStyle;
  gcval.graphics_exposures = False;
  gcval.fill_style = FillSolid;
  gcval.foreground = 0;
  gcval.background = 0;
  if (gc0)
    XChangeGC(dpy, gc0, gcmask, &gcval);
  else
    gc0 = fvwmlib_XCreateGC(dpy, pmask, gcmask, &gcval);

  gcval.foreground = 1;
  gcval.background = 1;
  if (gc1)
    XChangeGC(dpy, gc1, gcmask, &gcval);
  else
    gc1 = fvwmlib_XCreateGC(dpy, pmask, gcmask, &gcval);

  XFillRectangle(dpy, pmask, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pmask, cgc, 3, 3, w+4, h+4);
  XFillRectangle(dpy, pmask, gc1, 0, 0, w+1, h+1);
  XFillRectangle(dpy, pclip, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pclip, gc1, 1, 1, w-1, h-1);
  if (FShapesSupported)
  {
    FShapeCombineMask(dpy, Tip.win, FShapeBounding, 0, 0, pmask, FShapeSet);
    FShapeCombineMask(dpy, Tip.win, FShapeClip,     0, 0, pclip, FShapeSet);
  }
  if (tipscolorset >= 0 && (cset->pixmap || cset->shape_mask))
  {
     SetWindowBackground(
       dpy, Tip.win, w + 4, h + 4, cset, Pdepth, tipsgc, False);
  }
  XFreeGC(dpy, gc0);
  XFreeGC(dpy, gc1);
  XFreeGC(dpy, cgc);
  XFreePixmap(dpy, pchk);
}

void DestroyTipWindow()
{
  XFreePixmap(dpy, pmask);
  XFreePixmap(dpy, pclip);
  XDestroyWindow(dpy, Tip.win);
  if (Tip.text)
  {
    free(Tip.text);
    Tip.text = NULL;
  }
  Tip.win = None;
}

/*-----------------------------------------------------*/
/* Get file modification time                          */
/* (based on the code of 'coolmail' By Byron C. Darrah */
/*-----------------------------------------------------*/

void cool_get_inboxstatus(void)
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

void HandleMailClick(XEvent event)
{
  static Time lastclick = 0;
  if (event.xbutton.time - lastclick < 250)
  {
    SendText(Fvwm_fd, MailCmd, 0);
  }
  lastclick = event.xbutton.time;
  mailcleared = 1;
}
