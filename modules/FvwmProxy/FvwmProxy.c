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
#include "libs/safemalloc.h"

#include "FvwmProxy.h"

/* ---------------------------- local definitions --------------------------- */

/* defaults for things we put in a configuration file */

#define PROXY_MOVE		False	/* move window when proxy is dragged */
#define PROXY_WIDTH		180
#define PROXY_HEIGHT		60
#define PROXY_SEPARATION	10
#define PROXY_MINWIDTH		15
#define PROXY_MINHEIGHT		10

#define STARTUP_DEBUG		False	/* store output before log is opened */

#if 0
#define CMD_SHOW		"Function FvwmProxyShowFunc"
#define CMD_HIDE		"Function FvwmProxyHideFunc"
#define CMD_ABORT		"Function FvwmProxyAbortFunc"
#define CMD_SELECT		"Function FvwmProxySelectFunc"
#define CMD_MARK		"Function FvwmProxyMarkFunc"
#else
#define CMD_SELECT		"WindowListFunc $w"
#define CMD_CLICK1		"Raise"
#define CMD_CLICK3		"Lower"
#endif

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
static Window focusWindow;
static FILE *errorFile;
static XTextProperty windowName;
static int deskNumber=0;
static int mousex,mousey;
static ProxyWindow *firstProxy=NULL;
static ProxyWindow *lastProxy=NULL;
static ProxyWindow *selectProxy=NULL;
static ProxyWindow *startProxy=NULL;
static ProxyWindow *enterProxy=NULL;
static XGCValues xgcv;
static int are_windows_shown = 0;
static FlocaleWinString *FwinString;

static int cset_normal = 0;
static int cset_select = 0;
static char *font_name = NULL;
static FlocaleFont *Ffont;
static int enterSelect=False;
static int showMiniIcons=True;
static int proxyMove=PROXY_MOVE;
static int proxyWidth=PROXY_WIDTH;
static int proxyHeight=PROXY_HEIGHT;
static int proxySeparation=PROXY_SEPARATION;

static char commandBuffer[256];
static char *logfilename=NULL;
#if 0
static char *show_command;
static char *hide_command;
static char *mark_command;
static char *select_command;
static char *abort_command;
#endif

#if STARTUP_DEBUG
static char startupText[10000];
#endif

typedef enum
{
	PROXY_ACTION_SELECT = 0,
	PROXY_ACTION_SHOW,
	PROXY_ACTION_HIDE,
	PROXY_ACTION_ABORT,
	PROXY_ACTION_MARK,
	PROXY_ACTION_UNMARK,
	/* this one *must* be last */
	PROXY_ACTION_CLICK,
	PROXY_ACTION_LAST = PROXY_ACTION_CLICK + NUMBER_OF_BUTTONS
} proxy_action_t;

char *ClickAction[PROXY_ACTION_LAST];

static int (*originalXErrorHandler)(Display *,XErrorEvent *);
static int (*originalXIOErrorHandler)(Display *);

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions (options) ------------------- */

static void LinkAction(char *string)
{
	char *token;

	token = PeekToken(string, &string);
	if (strncasecmp(token, "Click", 5) == 0)
	{
		int b;
		int i;
		i = sscanf(string, "%d", &b);
		if (i > 0 && b >=1 && b <= NUMBER_OF_MOUSE_BUTTONS)
		{
			if (ClickAction[PROXY_ACTION_CLICK + b - 1] != NULL)
			{
				free(ClickAction[PROXY_ACTION_CLICK + b - 1]);
			}
			ClickAction[PROXY_ACTION_CLICK + b - 1] =
				safestrdup(string);
		}
	}
	else if(StrEquals(token, "Select"))
	{
		if (ClickAction[PROXY_ACTION_SELECT] != NULL)
		{
			free(ClickAction[PROXY_ACTION_SELECT]);
		}
		ClickAction[PROXY_ACTION_SELECT] = safestrdup(string);
	}
	else if(StrEquals(token, "Show"))
	{
		if (ClickAction[PROXY_ACTION_SHOW] != NULL)
		{
			free(ClickAction[PROXY_ACTION_SHOW]);
		}
		ClickAction[PROXY_ACTION_SHOW] = safestrdup(string);
	}
	else if(StrEquals(token, "Hide"))
	{
		if (ClickAction[PROXY_ACTION_HIDE] != NULL)
		{
			free(ClickAction[PROXY_ACTION_HIDE]);
		}
		ClickAction[PROXY_ACTION_HIDE] = safestrdup(string);
	}
	else if(StrEquals(token, "Abort"))
	{
		if (ClickAction[PROXY_ACTION_ABORT] != NULL)
		{
			free(ClickAction[PROXY_ACTION_ABORT]);
		}
		ClickAction[PROXY_ACTION_ABORT] = safestrdup(string);
	}
	else if(StrEquals(token, "Mark"))
	{
		if (ClickAction[PROXY_ACTION_MARK] != NULL)
		{
			free(ClickAction[PROXY_ACTION_MARK]);
		}
		ClickAction[PROXY_ACTION_MARK] = safestrdup(string);
	}
	else if(StrEquals(token, "Unmark"))
	{
		if (ClickAction[PROXY_ACTION_UNMARK] != NULL)
		{
			free(ClickAction[PROXY_ACTION_UNMARK]);
		}
		ClickAction[PROXY_ACTION_UNMARK] = safestrdup(string);
	}

	return;
}

#if 0
static void parse_cmd(char **ret_cmd, char *cmd)
{
	if (*ret_cmd != NULL)
	{
		free(*ret_cmd);
		*ret_cmd = NULL;
	}
	if (cmd != NULL)
	{
		*ret_cmd = safestrdup(cmd);
	}

	return;
}
#endif

static Bool parse_options(void)
{
	char *tline;

#if 0
	show_command = safestrdup(CMD_SHOW);
	hide_command = safestrdup(CMD_HIDE);
	abort_command = safestrdup(CMD_ABORT);
	mark_command = safestrdup(CMD_MARK);
	select_command = safestrdup(CMD_SELECT);
#endif

	memset(ClickAction, 0, sizeof(ClickAction));
	ClickAction[PROXY_ACTION_SELECT] = strdup(CMD_SELECT);
	ClickAction[PROXY_ACTION_CLICK + 0] = strdup(CMD_CLICK1);
	if (NUMBER_OF_MOUSE_BUTTONS > 2)
	{
		ClickAction[PROXY_ACTION_CLICK + 2] = strdup(CMD_CLICK3);
	}
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

		/* dump leading whitespace */
		while(*tline==' ' || *tline=='\t')
			tline++;
#if STARTUP_DEBUG
		strcat(startupText,resource);
		strcat(startupText," |");
		strcat(startupText,tline);
		strcat(startupText,"|\n");
#endif
		if(!strncasecmp(resource,"Action",6))
		{
			LinkAction(tline);
		}
		else if (StrEquals(resource, "Colorset"))
		{
			if (sscanf(tline, "%d", &cset_normal) < 1)
			{
				cset_normal = 0;
			}
		}
		else if (StrEquals(resource, "SelectColorset"))
		{
			if (sscanf(tline, "%d", &cset_select) < 1)
			{
				cset_select = 0;
			}
		}
		else if (StrEquals(resource, "Font"))
		{
			if (font_name != NULL)
			{
				free(font_name);
			}
			font_name = safestrdup(tline);
		}
		else if (StrEquals(resource, "LogFile"))
		{
			if (logfilename != NULL)
			{
				free(logfilename);
			}
			logfilename = safestrdup(tline);
		}
		else if (StrEquals(resource, "ShowMiniIcons"))
		{
			showMiniIcons = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "EnterSelect"))
		{
			enterSelect = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "ProxyMove"))
		{
			proxyMove = ParseToggleArgument(tline, NULL, 0, 1);
		}
		else if (StrEquals(resource, "Width"))
		{
			if (sscanf(tline, "%d", &proxyWidth) < 1)
				proxyWidth=PROXY_MINWIDTH;
		}
		else if (StrEquals(resource, "Height"))
		{
			if (sscanf(tline, "%d", &proxyHeight) < 1)
				proxyHeight=PROXY_MINHEIGHT;
		}
		else if (StrEquals(resource, "Separation"))
		{
			if (sscanf(tline, "%d", &proxySeparation) < 1)
				proxySeparation=False;
		}
#if 0
		else if (StrEquals(resource, "ShowCommand"))
		{
			parse_cmd(&show_command, next);
		}
		else if (StrEquals(resource, "HideCommand"))
		{
			parse_cmd(&hide_command, next);
		}
		else if (StrEquals(resource, "AbortCommand"))
		{
			parse_cmd(&abort_command, next);
		}
		else if (StrEquals(resource, "MarkCommand"))
		{
			parse_cmd(&mark_command, next);
		}
		else if (StrEquals(resource, "SelectCommand"))
		{
			parse_cmd(&select_command, next);
		}

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
	if (p)
	{
		if (p->name)
		{
			free(p->name);
		}
		if (p->iconname)
		{
			free(p->iconname);
		}
		if (p->proxy != None)
		{
			XDestroyWindow(dpy, p->proxy);
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

static void send_command_to_fvwm(char *command, Window w)
{
	if (command == NULL || *command == 0)
	{
		return;
	}
	SendText(fd, command, w);

	return;
}


static ProxyWindow *FindProxy(Window window)
{
	ProxyWindow *proxy=firstProxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
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
	int texty,maxy;
	int cset;
	FvwmPicture *picture = &proxy->picture;
	int drawMiniIcon=(showMiniIcons && proxy->picture.picture != None);

	if (!proxy)
		return;

	x=0;
	y=0;
	w=proxy->proxyw;
	h=proxy->proxyh;

	texty=(h+ Ffont->ascent - Ffont->descent)/2;	/* center */
	if(drawMiniIcon)
		texty+=8;				/* half icon height */

	maxy=h-Ffont->descent-4;
	if(texty>maxy)
		texty=maxy;

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
	RelieveRectangle(dpy,proxy->proxy, 0,0, w - 1,h - 1, hi_gc,sh_gc, 2);
	if (proxy->iconname != NULL)
	{
		int text_width = FlocaleTextWidth(
			Ffont,proxy->iconname,strlen(proxy->iconname));
		int edge=(w-text_width)/2;
		if(edge<5)
			edge=5;

		FwinString->str = proxy->iconname;
		FwinString->win = proxy->proxy;
		FwinString->x = edge;
		FwinString->y = texty;
		FwinString->gc = fg_gc;
		FwinString->flags.has_colorset = False;
		if (cset >= 0)
		{
			FwinString->colorset = &Colorset[cset];
			FwinString->flags.has_colorset = True;
		}
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
	}
	if(drawMiniIcon)
		DrawPicture(proxy->proxy, (w-16)/2, 4, picture, cset);
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
	if (proxy->proxy == None)
	{
		long eventMask=ButtonPressMask|ExposureMask|ButtonMotionMask;
		if(enterSelect)
			eventMask|=EnterWindowMask;

		attributes.override_redirect = True;
		proxy->proxy = XCreateWindow(
			dpy, rootWindow, proxy->proxyx, proxy->proxyy,
			proxy->proxyw, proxy->proxyh,border,
			DefaultDepth(dpy,screen), InputOutput, Pvisual,
			valuemask, &attributes);
		XSelectInput(dpy,proxy->proxy,eventMask);
	}
	else
	{
		XMoveWindow(dpy, proxy->proxy, proxy->proxyx, proxy->proxyy);
	}
	XMapRaised(dpy, proxy->proxy);
	DrawProxyBackground(proxy);
	proxy->flags.is_shown = 1;

	return;
}

static void OpenWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		OpenOneWindow(proxy);
	}

	selectProxy = NULL;
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
		XUnmapWindow(dpy, proxy->proxy);
		proxy->flags.is_shown = 0;
	}

	return;
}

static void CloseWindows(void)
{
	ProxyWindow *proxy;

	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		CloseOneWindow(proxy);
	}

	return;
}

static Bool SortProxiesOnce(void)
{
	Bool change=False;

	ProxyWindow *proxy;
	ProxyWindow *next;

	int x1,x2;

	lastProxy=NULL;
	for (proxy=firstProxy; proxy != NULL && proxy->next != NULL;
		proxy=proxy->next)
	{
		x1=proxy->proxyx;
		x2=proxy->next->proxyx;

		if( x1>x2 || (x1==x2 && proxy->proxyy > proxy->next->proxyy) )
		{
			change=True;
			next=proxy->next;

			if(lastProxy)
				lastProxy->next=next;
			else
				firstProxy=next;
			proxy->next=next->next;
			next->next=proxy;
		}

		lastProxy=proxy;
	}

	lastProxy=NULL;
	for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
	{
		proxy->prev=lastProxy;
		lastProxy=proxy;
	}

	return change;
}

static void SortProxies(void)
{
	while (SortProxiesOnce() == True)
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
		if(dx<(proxyWidth+proxySeparation) &&
		   dy<proxyHeight+proxySeparation )
		{
			rc = True;
			if(proxyWidth-dx<proxyHeight-dy)
			{
				fprintf(errorFile,"Adjust X\n");
				if(proxy->proxyx<other->proxyx)
				{
					other->proxyx=
						proxy->proxyx+ proxy->proxyw+
						proxySeparation;
				}
				else
				{
					proxy->proxyx=
						other->proxyx+ other->proxyw+
						proxySeparation;
				}
			}
			else
			{
				fprintf(errorFile,"Adjust y\n");
				if(proxy->proxyy<other->proxyy)
				{
					other->proxyy=
						proxy->proxyy+ proxy->proxyh+
						proxySeparation;
				}
				else
				{
					proxy->proxyy=
						other->proxyy+ other->proxyh+
						proxySeparation;
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
		for (proxy=firstProxy; proxy != NULL; proxy=proxy->next)
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
		proxy->next = firstProxy;
		firstProxy = proxy;
		proxy->window=target;
	}
	proxy->x=wx;
	proxy->y=wy;
	proxy->desk=desk;
	proxy->w=wsx;
	proxy->h=wsy;
	proxy->proxyw=proxyWidth;
	proxy->proxyh=proxyHeight;
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

	for (proxy=firstProxy, prev = NULL; proxy != NULL;
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
		firstProxy = proxy->next;
	}
	else
	{
		prev->next = proxy->next;
	}
	if (selectProxy == proxy)
	{
		selectProxy = NULL;
	}
	if (enterProxy == proxy)
	{
		enterProxy = NULL;
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

	enterProxy=NULL;
	selectProxy=NULL;

	send_command_to_fvwm(ClickAction[PROXY_ACTION_SHOW], None);
	are_windows_shown = 1;
	CloseWindows();
	ReshuffleWindows();
	OpenWindows();

	return;
}

static void MarkProxy(ProxyWindow *new_proxy)
{
	ProxyWindow *old_proxy;

	old_proxy = selectProxy;
	selectProxy = new_proxy;
	if (selectProxy != old_proxy)
	{
		if (old_proxy != NULL)
		{
			DrawProxyBackground(old_proxy);
			DrawProxy(old_proxy);
		}
		if (selectProxy != NULL)
		{
			DrawProxyBackground(selectProxy);
			DrawProxy(selectProxy);
		}
	}
	if (old_proxy != NULL)
		send_command_to_fvwm(ClickAction[PROXY_ACTION_UNMARK],
				old_proxy->window);
	if (selectProxy != NULL)
		send_command_to_fvwm(ClickAction[PROXY_ACTION_MARK],
				selectProxy->window);

	return;
}

static void HideProxies(void)
{
	if (!are_windows_shown)
	{
		return;
	}
	are_windows_shown = 0;
	CloseWindows();

	return;
}

static void SelectProxy(void)
{
	ProxyWindow *proxy;

	HideProxies();
	if(selectProxy)
		send_command_to_fvwm(ClickAction[PROXY_ACTION_SELECT],
				selectProxy->window);

	send_command_to_fvwm(ClickAction[PROXY_ACTION_HIDE], None);

	for(proxy=firstProxy; proxy != NULL; proxy=proxy->next)
		if(proxy==selectProxy)
		{
			startProxy=proxy;
			break;
		}

	selectProxy=NULL;

	return;
}

static void AbortProxies(boid)
{
	HideProxies();
	send_command_to_fvwm(ClickAction[PROXY_ACTION_ABORT], None);
	selectProxy = NULL;

	return;
}

static void change_cset(int cset)
{
	if (cset == cset_normal)
	{
		ProxyWindow *proxy;

		for (proxy = firstProxy; proxy != NULL; proxy = proxy->next)
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
		if(deskNumber!=body[0])
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
	case M_FOCUS_CHANGE:
	{
		focusWindow=bh->w;
		fprintf(errorFile,"M_FOCUS_CHANGE 0x%x\n",(int)focusWindow);
	}
	case M_STRING:
	{
		char *message=(char*)&body[3];
		char *token;
		char *next;
		int prev;

		fprintf(errorFile, "M_STRING \"%s\"\n", message);
		token = PeekToken(message, &next);
		prev=(StrEquals(token, "Prev"));
		if(StrEquals(token, "Next") || prev)
		{
			ProxyWindow *lastSelect=selectProxy;
			ProxyWindow *newSelect=selectProxy;
			ProxyWindow *first=prev? lastProxy: firstProxy;

			/* auto-show if not already shown */
			if (!are_windows_shown)
				StartProxies();

			if(startProxy && startProxy->desk==deskNumber)
			{
				newSelect=startProxy;
				startProxy=NULL;
			}
			else
			{
				if(newSelect)
				{
					if(prev)
						newSelect=newSelect->prev;
					else
						newSelect=newSelect->next;
				}
				if(!newSelect)
					newSelect=first;
				while(newSelect!=lastSelect &&
					newSelect->desk!=deskNumber)
				{
					if(prev)
						newSelect=newSelect->prev;
					else
						newSelect=newSelect->next;
					if(!newSelect && lastSelect)
						newSelect=first;
				}
			}

			MarkProxy(newSelect);
		}
		else if(StrEquals(token, "Circulate"))
		{
			/* auto-show if not already shown */
			if (!are_windows_shown)
				StartProxies();

			Window w=(selectProxy)? selectProxy->window:focusWindow;

			strcpy(commandBuffer,next);
			strcat(commandBuffer," SendToModule FvwmProxy Mark");

			fprintf(errorFile, "0x%x:0x%x Circulate \"%s\"\n",
				(int)selectProxy,(int)w,commandBuffer);
			if(next)
				SendFvwmPipe(fd,commandBuffer,w);
		}
		else if(StrEquals(token, "Show"))
		{
			StartProxies();
		}
		else if(StrEquals(token, "Hide"))
		{
			SelectProxy();
		}
		else if(StrEquals(token, "Abort"))
		{
			AbortProxies(True);
		}
		else if(StrEquals(token, "Mark"))
		{
/*
			Window w;

			if (next == NULL)
			{
				proxy = NULL;
			}
			else if (sscanf(next, "0x%x", (int *)&w) < 1)
			{
				proxy = NULL;
			}
			else
*/
			{
				focusWindow=bh->w;
				proxy = FindProxy(bh->w);
				fprintf(errorFile,
					"Mark proxy 0x%x win 0x%x\n",
					(int)proxy,(int)bh->w);
			}
			MarkProxy(proxy);
		}
		break;
	}
	case M_CONFIG_INFO:
	{
		char *tline, *token;

		tline = (char*)&(body[3]);
		token = PeekToken(tline, &tline);
		if (StrEquals(token, "Colorset"))
		{
			int cset;
			cset = LoadColorset(tline);
			change_cset(cset);
		}
		break;
	}
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
	int dx,dy;

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
		proxy = FindProxy(window);
		if(proxy)
		{
#if FALSE
			/* TODO setup from Fvwm config */
			if(pEvent->xbutton.button==Button1)
				XRaiseWindow(dpy,proxy->window);
			else if(pEvent->xbutton.button==Button3)
				XLowerWindow(dpy,proxy->window);
#else
			int button=pEvent->xbutton.button;
			if(button >= 1 && button<=NUMBER_OF_MOUSE_BUTTONS)
			{
				fprintf(errorFile,"SendFvwmPipe %s 0x%x %d\n",
					ClickAction[PROXY_ACTION_CLICK+
								button-1],
					(int)proxy->window,button);
				SendFvwmPipe(fd,
					ClickAction[PROXY_ACTION_CLICK+
								button-1],
					proxy->window);
			}

#endif
		}
		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
		break;
	case MotionNotify:
		proxy = FindProxy(window);
		fprintf(errorFile,"MotionNotify %4d,%4d\n",
			pEvent->xmotion.x_root,pEvent->xmotion.y_root);
		dx=pEvent->xbutton.x_root-mousex;
		dy=pEvent->xbutton.y_root-mousey;
		if(proxy && proxyMove)
		{
			sprintf(commandBuffer,"Silent Move w%dp w%dp",dx,dy);
			SendText(fd,commandBuffer,proxy->window);
			fprintf(errorFile,">>> %s\n",commandBuffer);
		}

		mousex=pEvent->xbutton.x_root;
		mousey=pEvent->xbutton.y_root;
		break;
	case EnterNotify:
		proxy = FindProxy(pEvent->xcrossing.window);
		if (pEvent->xcrossing.mode == NotifyNormal)
		{
			MarkProxy(proxy);
			enterProxy = proxy;
		}
		else if (pEvent->xcrossing.mode == NotifyUngrab &&
			 proxy != NULL && proxy != selectProxy &&
			 proxy != enterProxy)
		{
			MarkProxy(proxy);
			enterProxy = proxy;
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
		{
			DispatchEvent(&event);
		}
	}
}

/* ---------------------------- interface functions ------------------------- */

int main(int argc, char **argv)
{
	char *titles[1];

#if STARTUP_DEBUG
	startupText[0]=0;
#endif

	if (argc < 6)
	{
		fprintf(stderr,"FvwmProxy should only be executed by fvwm!\n");
		fflush(errorFile);
		exit(1);
	}

	FlocaleInit(LC_CTYPE, "", "", "FvwmProxy");
	MyName = GetFileNameFromPath(argv[0]);

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);

	originalXIOErrorHandler=XSetIOErrorHandler(myXIOErrorHandler);
	originalXErrorHandler=XSetErrorHandler(myXErrorHandler);

	if (!(dpy=XOpenDisplay(NULL)))
	{
		fprintf(stderr,"can't open display\n");
		exit (1);
	}
	titles[0]="FvwmProxy";
	if(XStringListToTextProperty(titles,1,&windowName) == 0)
	{
		fprintf(stderr,"Proxy_CreateBar() could not allocate space"
			" for window title");
	}

	PictureInitCMap(dpy);
	FScreenInit(dpy);
	FRenderInit(dpy);
	AllocColorset(0);
	FlocaleAllocateWinString(&FwinString);
	screen = DefaultScreen(dpy);
	rootWindow = RootWindow(dpy,screen);
	xgcv.plane_mask=AllPlanes;
	miniIconGC=fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	fg_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	hi_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);
	sh_gc = fvwmlib_XCreateGC(dpy,rootWindow,GCPlaneMask,&xgcv);

	x_fd = XConnectionNumber(dpy);
	fd_width = GetFdWidth();

	SetMessageMask(
		fd, M_STRING| M_CONFIGURE_WINDOW| M_ADD_WINDOW| M_FOCUS_CHANGE|
		M_DESTROY_WINDOW| M_NEW_DESK| M_NEW_PAGE| M_ICON_NAME|
		M_WINDOW_NAME| M_MINI_ICON| M_ICONIFY| M_DEICONIFY|
		M_CONFIG_INFO| M_END_CONFIG_INFO);

	if (parse_options() == False)
		exit(1);

	if(!logfilename)
		logfilename="/dev/null";
	errorFile=fopen(logfilename,"a");

	fprintf(errorFile,"FvwmProxy >>>>>>>>> STARTUP\n");
	fflush(errorFile);
#if STARTUP_DEBUG
	fprintf(errorFile,"startup:\n%s-----\n",startupText);
#endif
	fprintf(errorFile,"started\n");
	fflush(errorFile);

	if ((Ffont = FlocaleLoadFont(dpy, font_name, MyName)) == NULL)
	{
		fprintf(stderr,"%s: Couldn't load font. Exiting!\n", MyName);
		exit(1);
	}
	if (Ffont->font != NULL)
	{
		XSetFont(dpy,fg_gc,Ffont->font->fid);
	}
	SendInfo(fd,"Send_WindowList",0);
	SendFinishedStartupNotification(fd);
	Loop(fd);

	return 0;
}
