/* -*-c-*- */
/* This module, FvwmProxy, is an original work by Jason Weber.
 *
 * This program is free software; you can redistribute it and/or modify
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

/* vim:ts=8:shiftwidth=8: */

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include <stdio.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/FRenderInit.h"
#include "libs/FRender.h"
#include "libs/Colorset.h"
#include "libs/Flocale.h"
#include "libs/gravity.h"
#include "libs/FScreen.h"
#include "libs/Picture.h"
#include "libs/PictureGraphics.h"

#include "FvwmProxy.h"

/***
    To use the Alt-Tab switching, try something like:
    Key Tab     A   M   SendToModule    FvwmProxy   Next

    To activate and deativate proxies other than with the built in Alt polling,
    use 'SendToModule FvwmProxy Show' and 'SendToModule FvwmProxy  Hide'.
*/

/* ---------------------------- local definitions --------------------------- */

/* things we might put in a configuration file */

#define	PROXY_ALT_POLLING	True	/* poll Alt key to Show/Hide proxies */
#define PROXY_MOVE		False	/* move window when proxy is dragged */
#define PROXY_WIDTH		180
#define PROXY_HEIGHT		60
#define PROXY_SEPARATE		10

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static char *MyName;
static int x_fd;
static fd_set_size_t fd_width;
static int fd[2];
static Display *dpy;
static unsigned long screen;
static GC fg_gc;
static GC hi_gc;
static GC sh_gc;
static GC miniIconGC;
static Window rootWindow;
static FILE *errorFile;
static char command[256];
static XTextProperty windowName;
static int altState=0;
static int deskNumber=0;
static int mousex,mousey;
static ProxyWindow *firstWindow=NULL;
static ProxyWindow *selectProxy=NULL;
static XGCValues xgcv;
static int are_windows_shown = 0;
static FlocaleWinString *FwinString;

static int cset_normal = 0;
static int cset_select = 0;
char *font_name = NULL;
FlocaleFont *Ffont;

static int (*originalXErrorHandler)(Display *,XErrorEvent *);
static int (*originalXIOErrorHandler)(Display *);

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions (options) ------------------- */

static Bool parse_options(void)
{
	char *tline;

	InitGetConfigLine(fd, CatString3("*", MyName, 0));
	for (GetConfigLine(fd, &tline); tline != NULL;
	     GetConfigLine(fd, &tline))
	{
		char *resource;
		char *token;
		char *next;

		token = PeekToken(tline, &next);
		if (StrEquals(token, "Colorset"))
		{
			LoadColorset(next);
			continue;
		}

		tline = GetModuleResource(tline, &resource, MyName);
		if (resource == NULL)
		{
			continue;
		}
		if (StrEquals(resource, "Colorset"))
		{
			if (sscanf(tline, "%d", &cset_normal) < 1)
			{
				cset_normal = 0;
			}
		}
		if (StrEquals(resource, "SelectColorset"))
		{
			if (sscanf(tline, "%d", &cset_select) < 1)
			{
				cset_select = 0;
			}
		}
		else if (StrEquals(resource, "Font"))
		{
			CopyStringWithQuotes(&font_name, tline);
		}
#if 0
		else if (StrEquals(resource, "ProxyGeometry"))
		{
			flags = FScreenParseGeometry(
				tline, &g_x, &g_y, &width, &height);
			if (flags & WidthValue)
			{
				/*!!!*/
			}
			if (flags & HeightValue)
			{
				/*!!!*/
			}
			if (flags & (XValue | YValue))
			{
				/*!!!error*/
			}
		}
#endif

		free(resource);
	}

	return True;
}

/* ---------------------------- local functions (classes) ------------------- */

/* classes */

static void ProxyWindow_ProxyWindow(ProxyWindow *p)
{
	memset(p, 0, sizeof *p);
}

static ProxyWindow *new_ProxyWindow(void)
{
	ProxyWindow *p=(ProxyWindow *)safemalloc(sizeof(ProxyWindow));
	ProxyWindow_ProxyWindow(p);
	return p;
}

static void delete_ProxyWindow(ProxyWindow *p)
{
	if(p)
	{
		if(p->name)
		{
			free(p->name);
		}
		if(p->iconname)
		{
			free(p->iconname);
		}
		free(p);
	}
}

/* ---------------------------- error handlers ------------------------------ */

static int myXErrorHandler(Display *display,XErrorEvent *error_event)
{
	const long messagelen=256;
	char buffer[messagelen],function[messagelen];
	char request_number[16];

	sprintf(request_number,"%d",error_event->request_code);
	sprintf(buffer,"UNKNOWN");
	XGetErrorDatabaseText(display,"XRequest",
			      request_number,buffer,function,messagelen);

	fprintf(errorFile,"non-fatal X error as follows, display 0x%x"
		" op %d:%d \"%s\" serial %u error %d\n",
		(unsigned int)display,
		error_event->request_code,error_event->minor_code,
		function,(unsigned int)error_event->serial,
		error_event->error_code);
#if 0
	/* !!!calling X routines from error handlers is not allowed! */
	XGetErrorText(display,error_event->error_code,buffer,messagelen);
	fprintf(errorFile,"  %s\n",buffer);
#endif

	return 0;
}

static int myXIOErrorHandler(Display *display)
{
	fprintf(errorFile,"fatal IO Error on display 0x%x\n",
		(unsigned int)display);

	originalXIOErrorHandler(display);

	fprintf(errorFile,"original X IO handler should not have returned\n");

	/* should never get this far */
	return 0;
}

/* ---------------------------- local functions ----------------------------- */

static ProxyWindow *FindProxy(Window window)
{
	ProxyWindow *proxy=firstWindow;

	for (proxy=firstWindow; proxy != NULL; proxy=proxy->next)
	{
		if(proxy->proxy==window || proxy->window==window)
			return proxy;
	}

	return NULL;
}

static void DrawPicture(
	Window window, int x, int y, FvwmPicture *picture, int cset)
{
	FvwmRenderAttributes fra;

	if(!picture || picture->picture == None)
		return;

	fra.mask = FRAM_DEST_IS_A_WINDOW;
	if (cset >= 0)
	{
		fra.mask |= FRAM_HAVE_ICON_CSET;
		fra.colorset = &Colorset[cset];
	}
	PGraphicsRenderPicture(
		dpy, window, picture, &fra, window, miniIconGC, None, None,
		0, 0, picture->width, picture->height,
		x, y, picture->width, picture->height, False);
}

static void DrawWindow(
	ProxyWindow *proxy, int x,int y,int w,int h)
{
	int direction;
	int ascent;
	int descent;
	XCharStruct overall;
	int edge, top;
	int cset;
	int text_width;
	FvwmPicture *picture = &proxy->picture;

	if (!proxy)
	{
		return;
	}
	x=0;
	y=0;
	w=proxy->proxyw;
	h=proxy->proxyh;
	if (proxy->iconname != NULL)
	{
		text_width = FlocaleTextWidth(
			Ffont,proxy->iconname,strlen(proxy->iconname));
		edge=(w-text_width)/2;
	}
	else
	{
		edge = w / 2;
	}
#if 0
	top=h - Ffont->descent - 4;
#endif
	top=(h+ Ffont->ascent - Ffont->descent)/2;	/* center */
	top+=8;				                /* HACK tweak */

	if(edge<5)
		edge=5;

	cset = (proxy==selectProxy) ? cset_select : cset_normal;
	XSetForeground(dpy,fg_gc,Colorset[cset].fg);
	XSetBackground(dpy,fg_gc,Colorset[cset].bg);
	XSetForeground(dpy,sh_gc,Colorset[cset].shadow);
	XSetForeground(dpy,hi_gc,Colorset[cset].hilite);
	/* FIXME: use clip redrawing (not really essential here) */
	if (FLF_FONT_HAS_ALPHA(Ffont,cset) || PICTURE_HAS_ALPHA(picture,cset))
	{
		XClearWindow(dpy,proxy->proxy);
	}
	RelieveRectangle(
		dpy, proxy->proxy, 0, 0, w - 1, h - 1, hi_gc, sh_gc, 2);
	if (proxy->iconname != NULL)
	{
		FwinString->str = proxy->iconname;
		FwinString->win = proxy->proxy;
		FwinString->x = edge;
		FwinString->y = top;
		FwinString->gc = fg_gc;
		FwinString->flags.has_colorset = False;
		if (cset >= 0)
		{
			FwinString->colorset = &Colorset[cset];
			FwinString->flags.has_colorset = True;
		}
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
	}
	DrawPicture(proxy->proxy, (w-16)/2, 8, picture, cset);
}

static void DrawProxy(ProxyWindow *proxy)
{
	if (proxy)
	{
		DrawWindow(
			proxy, proxy->proxyx, proxy->proxyy, proxy->proxyw,
			proxy->proxyh);
	}
}

static void DrawProxyBackground(ProxyWindow *proxy)
{
	int cset;

	if (proxy == NULL)
	{
		return;
	}
	cset = (proxy==selectProxy) ? cset_select : cset_normal;
	XSetForeground(dpy,fg_gc,Colorset[cset].fg);
	XSetBackground(dpy,fg_gc,Colorset[cset].bg);
	SetWindowBackground(
		dpy, proxy->proxy, proxy->proxyw, proxy->proxyh,
		&Colorset[cset], Pdepth, fg_gc, True);
}

static void OpenOneWindow(ProxyWindow *proxy)
{
	int border=0;
	unsigned long valuemask=CWOverrideRedirect;
	XSetWindowAttributes attributes;

	if (proxy == NULL)
	{
		return;
	}
	if (proxy->desk != deskNumber || proxy->flags.is_iconified)
	{
		return;
	}
	if (proxy->flags.is_shown)
	{
		return;
	}
	attributes.override_redirect = True;
	proxy->proxy=XCreateWindow(
		dpy, rootWindow, proxy->proxyx, proxy->proxyy,
		proxy->proxyw, proxy->proxyh,border,
		DefaultDepth(dpy,screen), InputOutput, Pvisual,
		valuemask, &attributes);
	XSelectInput(dpy,proxy->proxy,ButtonPressMask|ExposureMask|
		     ButtonMotionMask);
	XMapRaised(dpy,proxy->proxy);
	DrawProxyBackground(proxy);
	proxy->flags.is_shown = 1;

	return;
}

static void OpenWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstWindow; proxy != NULL; proxy=proxy->next)
	{
		OpenOneWindow(proxy);
	}

	return;
}

static void CloseOneWindow(ProxyWindow *proxy)
{
	if (proxy == NULL)
	{
		return;
	}
	if (proxy->flags.is_shown)
	{
		XDestroyWindow(dpy, proxy->proxy);
		proxy->flags.is_shown = 0;
	}

	return;
}

static void CloseWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstWindow; proxy != NULL; proxy=proxy->next)
	{
		CloseOneWindow(proxy);
	}

	return;
}

static Bool __SortProxies(void)
{
	Bool change=False;

	ProxyWindow *last=NULL;
	ProxyWindow *proxy;
	ProxyWindow *next;

	int x1,x2;

	for (proxy=firstWindow; proxy != NULL && proxy->next != NULL;
	     proxy=proxy->next)
	{
		x1=proxy->proxyx;
		x2=proxy->next->proxyx;

		if( x1>x2 || (x1==x2 && proxy->proxyy > proxy->next->proxyy) )
		{
			change=True;
			next=proxy->next;

			if(last)
				last->next=next;
			else
				firstWindow=next;
			proxy->next=next->next;
			next->next=proxy;
		}

		last=proxy;
	}

	return change;
}

static void SortProxies(void)
{
	while (__SortProxies() == True)
	{
		/* nothing */
	}

	return;
}

static Bool AdjustOneWindow(ProxyWindow *proxy)
{
	Bool rc = False;
	ProxyWindow *other=proxy->next;

	for (other=proxy->next; other; other=other->next)
	{
		int dx=abs(proxy->proxyx-other->proxyx);
		int dy=abs(proxy->proxyy-other->proxyy);
#if 0
		fprintf(errorFile,"dx=%d dy=%d\n",dx,dy);
#endif
		if(dx<(PROXY_WIDTH+PROXY_SEPARATE) &&
		   dy<PROXY_HEIGHT+PROXY_SEPARATE )
		{
			rc = True;
			if(PROXY_WIDTH-dx<PROXY_HEIGHT-dy)
			{
				fprintf(errorFile,"Adjust X\n");
				if(proxy->proxyx<other->proxyx)
				{
					other->proxyx=
						proxy->proxyx+ proxy->proxyw+
						PROXY_SEPARATE;
				}
				else
				{
					proxy->proxyx=
						other->proxyx+ other->proxyw+
						PROXY_SEPARATE;
				}
			}
			else
			{
				fprintf(errorFile,"Adjust y\n");
				if(proxy->proxyy<other->proxyy)
				{
					other->proxyy=
						proxy->proxyy+ proxy->proxyh+
						PROXY_SEPARATE;
				}
				else
				{
					proxy->proxyy=
						other->proxyy+ other->proxyh+
						PROXY_SEPARATE;
				}
			}
		}
	}

	return rc;
}

static void AdjustWindows(void)
{
	Bool collision=True;

	while(collision == True)
	{
		ProxyWindow *proxy;

		collision=False;
		for (proxy=firstWindow; proxy != NULL; proxy=proxy->next)
		{
			if (AdjustOneWindow(proxy) == True)
			{
				collision = True;
			}
		}
	}
}

static void ReshuffleWindows(void)
{
	if (are_windows_shown)
	{
		CloseWindows();
	}
	AdjustWindows();
	SortProxies();
	if (are_windows_shown)
	{
		OpenWindows();
	}

	return;
}

static void UpdateOneWindow(ProxyWindow *proxy)
{
	if (proxy == NULL)
	{
		return;
	}
	if (proxy->flags.is_shown)
	{
		ReshuffleWindows();
	}

	return;
}

static void ConfigureWindow(FvwmPacket *packet)
{
	unsigned long* body = packet->body;

	struct ConfigWinPacket *cfgpacket=(void *)body;
	int wx=cfgpacket->frame_x;
	int wy=cfgpacket->frame_y;
	int desk=cfgpacket->desk;
	int wsx=cfgpacket->frame_width;
	int wsy=cfgpacket->frame_height;
	Window target=cfgpacket->w;
	ProxyWindow *proxy;
	int is_new_window = 0;

	if (DO_SKIP_WINDOW_LIST(cfgpacket))
	{
		return;
	}
	proxy = FindProxy(target);
	if (proxy == NULL)
	{
		is_new_window = 1;
		proxy=new_ProxyWindow();
		proxy->next = firstWindow;
		firstWindow = proxy;
		proxy->window=target;
	}
	proxy->x=wx;
	proxy->y=wy;
	proxy->desk=desk;
	proxy->w=wsx;
	proxy->h=wsy;
	proxy->proxyw=PROXY_WIDTH;
	proxy->proxyh=PROXY_HEIGHT;
	proxy->proxyx=proxy->x + (proxy->w-proxy->proxyw)/2;
	proxy->proxyy=proxy->y + (proxy->h-proxy->proxyh)/2;
	proxy->flags.is_iconified = !!IS_ICONIFIED(cfgpacket);
	if (are_windows_shown)
	{
		if (is_new_window)
		{
			ReshuffleWindows();
		}
		else
		{
			CloseOneWindow(proxy);
			OpenOneWindow(proxy);
		}
	}

	return;
}

static void DestroyWindow(Window w)
{
	ProxyWindow *proxy;
	ProxyWindow *prev;

	for (proxy=firstWindow, prev = NULL; proxy != NULL;
	     prev = proxy, proxy=proxy->next)
	{
		if(proxy->proxy==w || proxy->window==w)
			break;
	}
	if (proxy == NULL)
	{
		return;
	}
	if (prev == NULL)
	{
		firstWindow = proxy->next;
	}
	else
	{
		prev->next = proxy->next;
	}
	if (selectProxy == proxy)
	{
		selectProxy = NULL;
	}
	CloseOneWindow(proxy);
	delete_ProxyWindow(proxy);

	return;
}

static void IconifyWindow(Window w, int is_iconified)
{
	ProxyWindow *proxy;

	proxy = FindProxy(w);
	if (proxy == NULL)
	{
		return;
	}
	proxy->flags.is_iconified = !!is_iconified;
	if (is_iconified)
	{
		if (proxy->flags.is_shown)
		{
			CloseOneWindow(proxy);
		}
	}
	else
	{
		if (are_windows_shown)
		{
			ReshuffleWindows();
			OpenOneWindow(proxy);
		}
	}

	return;
}

static void StartProxies(void)
{
	if (are_windows_shown)
	{
		return;
	}
	selectProxy=NULL;
	are_windows_shown = 1;
	CloseWindows();
	ReshuffleWindows();
	OpenWindows();
}

static void EndProxies(Bool release)
{
	if (!are_windows_shown)
	{
		return;
	}
	are_windows_shown = 0;
	if(selectProxy)
	{
		XRaiseWindow(dpy,selectProxy->window);

		sprintf(command,"WarpToWindow 50 50");
		SendText(fd,command,
			 selectProxy->window);
	}
	CloseWindows();
}

static void change_cset(int cset)
{
	if (cset == cset_normal)
	{
		ProxyWindow *proxy;

		for (proxy = firstWindow; proxy != NULL; proxy = proxy->next)
		{
			DrawProxyBackground(proxy);
		}
	}
	else if (cset == cset_select && selectProxy != NULL)
	{
		DrawProxyBackground(selectProxy);
	}

	return;
}

static void ProcessMessage(FvwmPacket* packet)
{
	unsigned long type = packet->type;
	unsigned long* body = packet->body;
	FvwmWinPacketBodyHeader *bh = (void *)body;
	ProxyWindow *proxy;

	switch (type)
	{
	case M_CONFIGURE_WINDOW:
	case M_ADD_WINDOW:
		ConfigureWindow(packet);
		break;
	case M_DESTROY_WINDOW:
		DestroyWindow(bh->w);
		break;
	case M_ICONIFY:
		IconifyWindow(bh->w, 1);
		break;
	case M_DEICONIFY:
		IconifyWindow(bh->w, 0);
		break;
	case M_WINDOW_NAME:
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			if (proxy->name != NULL)
			{
				free(proxy->name);
			}
			proxy->name = safestrdup((char*)&body[3]);
			UpdateOneWindow(proxy);
		}
		break;
	case M_ICON_NAME:
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			if (proxy->iconname != NULL)
			{
				free(proxy->iconname);
			}
			proxy->iconname = safestrdup((char*)&body[3]);
			UpdateOneWindow(proxy);
		}
		break;
	case M_NEW_DESK:
		if(deskNumber!=body[0] && altState)
		{
			deskNumber=body[0];
			if(are_windows_shown)
			{
				CloseWindows();
				OpenWindows();
			}
		}
		break;
	case M_NEW_PAGE:
		deskNumber=body[2];
		ReshuffleWindows();
		break;
	case M_MINI_ICON:
		proxy = FindProxy(bh->w);
		if (proxy != NULL)
		{
			proxy->picture.width=body[3];
			proxy->picture.height=body[4];
			proxy->picture.depth=body[5];
			proxy->picture.picture=body[6];
			proxy->picture.mask=body[7];
			proxy->picture.alpha=body[8];
			UpdateOneWindow(proxy);
		}
		break;
	case M_STRING:
	{
		char *message=(char*)&body[3];
		char *token;
		char *next;

		fprintf(errorFile, "M_STRING \"%s\"\n", message);
		token = PeekToken(message, &next);
		if (StrEquals(token, "Next"))
		{
			ProxyWindow *lastSelect=selectProxy;
			if(selectProxy)
			{
				selectProxy=selectProxy->next;
			}
			else
			{
				selectProxy=firstWindow;
			}
			DrawProxy(lastSelect);
			DrawProxy(selectProxy);
		}
		else if(StrEquals(token, "Show"))
		{
			StartProxies();
		}
		else if(StrEquals(token, "Hide"))
		{
			EndProxies(True);
		}
		else if(StrEquals(token, "Colorset"))
		{
			int cset;

			cset = LoadColorset(next);
			change_cset(cset);
		}
	}
	break;
	}

	fflush(errorFile);
}

static int My_XNextEvent(Display *dpy,XEvent *event)
{
	fd_set in_fdset;

	struct timeval timevalue,*timeout=&timevalue;
	timevalue.tv_sec = 0;
	timevalue.tv_usec = 100000;

	if(FPending(dpy))
	{
		FNextEvent(dpy,event);
		return 1;
	}

	FD_ZERO(&in_fdset);
	FD_SET(x_fd,&in_fdset);
	FD_SET(fd[1],&in_fdset);

	if( fvwmSelect(fd_width, &in_fdset, 0, 0, timeout) > 0)
	{
		if(FD_ISSET(x_fd, &in_fdset))
		{
			if(FPending(dpy))
			{
				FNextEvent(dpy,event);
				return 1;
			}
		}

		if(FD_ISSET(fd[1], &in_fdset))
		{
			FvwmPacket* packet = ReadFvwmPacket(fd[1]);
			if(!packet)
			{
				fprintf(errorFile,"\nNULL packet: exiting\n");
				fflush(errorFile);
				exit(0);
			}

			ProcessMessage(packet);
		}
	}

	return 0;
}

static void DispatchEvent(XEvent *pEvent)
{
	Window window=pEvent->xany.window;
	ProxyWindow *proxy;

	switch(pEvent->xany.type)
	{
	case Expose:
		proxy = FindProxy(window);
		if (proxy != NULL)
		{
			DrawWindow(
				proxy, pEvent->xexpose.x, pEvent->xexpose.y,
				pEvent->xexpose.width, pEvent->xexpose.height);
		}
		break;
	case ButtonPress:
	{
		ProxyWindow *proxy;
#if 0
		fprintf(errorFile,"ButtonPress %d\n",
			pEvent->xbutton.button);
#endif
		proxy = FindProxy(window);
		if(proxy)
		{
			/* TODO setup from Fvwm config */
			if(pEvent->xbutton.button==Button1)
				XRaiseWindow(dpy,proxy->window);
			else if(pEvent->xbutton.button==Button3)
				XLowerWindow(dpy,proxy->window);
		}
		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
	}
	break;
	case MotionNotify:
	{
		int dx,dy;
		ProxyWindow *proxy;

		proxy = FindProxy(window);
		fprintf(errorFile,"MotionNotify %4d,%4d\n",
			pEvent->xmotion.x_root,pEvent->xmotion.y_root);
		dx=pEvent->xbutton.x_root-mousex;
		dy=pEvent->xbutton.y_root-mousey;
		if(proxy)
		{
#if PROXY_MOVE
			sprintf(command,"Silent Move w%dp w%dp",dx,dy);
			SendText(fd,command,proxy->window);
			fprintf(errorFile,">>> %s\n",command);
#endif
		}

		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
	}
	break;
	default:
		fprintf(errorFile,"Unrecognized XEvent %d\n",
			pEvent->xany.type);
		break;
	}

	fflush(errorFile);
}

static void Loop(int *fd)
{
	XEvent event;
	long result;


	while(1)
	{
		if((result=My_XNextEvent(dpy,&event))==1)
			DispatchEvent(&event);

#if PROXY_ALT_POLLING
		{
			Window root_return, child_return;
			int root_x_return, root_y_return;
			int win_x_return, win_y_return;
			unsigned int mask_return;
			int oldAltState=altState;

			if (FQueryPointer(
				    dpy,rootWindow,&root_return, &child_return,
				    &root_x_return,&root_y_return,
				    &win_x_return,&win_y_return,
				    &mask_return) == False)
			{
				/* pointer is on another screen - ignore */
			}
			altState=((mask_return&0x8)!=0);

			if(!oldAltState && altState)
			{
				fprintf(errorFile,"ALT ON\n");
				StartProxies();
			}
			if(oldAltState && !altState)
			{
				fprintf(errorFile,"ALT OFF\n");
				EndProxies(True);
			}
		}
#endif
	}
}

#if 0
/* unused */
static void RaiseProxies(void)
{
	ProxyWindow *proxy;

	for (proxy=firstWindow; proxy != NULL; proxy=proxy->next)
	{
		XRaiseWindow(dpy,proxy->proxy);
	}
}
#endif

/* ---------------------------- interface functions ------------------------- */

int main(int argc, char **argv)
{
	char *titles[1];

	if (argc < 6)
	{
		fprintf(stderr,"FvwmProxy should only be executed by fvwm!\n");
		fflush(errorFile);
		exit(1);
	}

	FlocaleInit(LC_CTYPE, "", "", "FvwmProxy");
	MyName = GetFileNameFromPath(argv[0]);

	errorFile=fopen("/tmp/FvwmProxy.log","a");

	fprintf(errorFile,"FvwmProxy >>>>>>>>> STARTUP\n");
	fflush(errorFile);

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);

	originalXIOErrorHandler=XSetIOErrorHandler(myXIOErrorHandler);
	originalXErrorHandler=XSetErrorHandler(myXErrorHandler);

	if (!(dpy=XOpenDisplay(NULL)))
	{
		fprintf(errorFile,"can't open display\n");
		fflush(errorFile);
		exit (1);
	}
#if 0
	fprintf(errorFile,"Display opened\n");
	fflush(errorFile);
#endif
	titles[0]="FvwmProxy";
	if(XStringListToTextProperty(titles,1,&windowName) == 0)
	{
		fprintf(errorFile,"Proxy_CreateBar() could not allocate space"
			" for window title");
		fflush(errorFile);
	}

	PictureInitCMap(dpy);
	FScreenInit(dpy);
	FRenderInit(dpy);
	AllocColorset(0);
	FlocaleAllocateWinString(&FwinString);
	screen = DefaultScreen(dpy);
	rootWindow = RootWindow(dpy,screen);
#if 0
	XSync(dpy,False);
	fprintf(errorFile,"SYNC\n");
	fflush(errorFile);
	XSync(dpy,False);
	fprintf(errorFile,"SYNC\n");
	fflush(errorFile);
#endif
	xgcv.plane_mask=AllPlanes;
	miniIconGC=fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	fg_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	hi_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	sh_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);

	x_fd = XConnectionNumber(dpy);
	fd_width = GetFdWidth();

	SetMessageMask(
		fd, M_STRING| M_CONFIGURE_WINDOW| M_ADD_WINDOW|
		M_DESTROY_WINDOW| M_NEW_DESK| M_NEW_PAGE| M_ICON_NAME|
		M_WINDOW_NAME| M_MINI_ICON| M_ICONIFY| M_DEICONIFY|
		M_CONFIG_INFO| M_END_CONFIG_INFO);

	if (parse_options() == False)
	{
		exit(1);
	}
	if ((Ffont = FlocaleLoadFont(dpy, font_name, MyName)) == NULL)
	{
		fprintf(stderr, "%s: Couldn't load font. Exiting!\n", MyName);
		exit(1);
	}
	if (Ffont->font != NULL)
	{
		XSetFont(dpy,fg_gc,Ffont->font->fid);
	}
	SendInfo(fd,"Send_WindowList",0);
	SendFinishedStartupNotification(fd);
#if 0
	fprintf(errorFile,"Startup Finished\n");
	fflush(errorFile);
	fprintf(errorFile,"Setup Finished\n");
	fflush(errorFile);
#endif
	Loop(fd);
#if 0
	fprintf(errorFile,"Main Complete\n");
	fflush(errorFile);
#endif

	return 0;
}
