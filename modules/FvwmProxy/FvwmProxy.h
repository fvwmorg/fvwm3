/* -*-c-*- */
/* vim:ts=8:shiftwidth=8: */

#ifndef FvwmProxy_h
#define FvwmProxy_h

/* TODO check what headers we really need */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/Colorset.h"
#include "libs/fvwmsignal.h"
#include "fvwm/fvwm.h"
#include "libs/vpacket.h"
#include "libs/PictureBase.h"

typedef struct sWindowName
{
	char			*name;
	struct sWindowName	*next;
	struct
	{
		unsigned is_soft : 1;
	} flags;
} WindowName;

typedef struct sProxyGroup
{
	char			*name;
	WindowName		*includes;
	WindowName		*excludes;
	struct sProxyGroup	*next;
	struct
	{
		unsigned auto_include : 1;
		unsigned auto_soft : 1;
		unsigned isolated : 1;
		unsigned ignore_ids : 1;
	} flags;
} ProxyGroup;

typedef struct sGeometryStamp
{
	int			x,y;
	int			w,h;
	Window			window;
} GeometryStamp;

typedef struct sProxyWindow
{
	Window			window;
	int			leader;
	int			pid;
	int			ppid;
	int			x,y;
	int			w,h;
	int			border_width;
	int			title_height;
	int			goal_width;
	int			goal_height;
	int			incx,incy;
	int			desk;
	int			group;
	ProxyGroup		*proxy_group;
	Window			proxy;
	int			proxyx,proxyy;
	int			proxyw,proxyh;
	int			tweakx,tweaky;
	int			pending_config;
	int			raised;
	FvwmPicture		picture;
	char			*name;
	char			*iconname;
	struct sProxyWindow	*next;
	struct sProxyWindow	*prev;
	struct
	{
		unsigned is_shown : 1;
		unsigned is_iconified : 1;
		unsigned is_soft : 1;
		unsigned is_isolated : 1;
	} flags;
} ProxyWindow;

#endif	/* FvwmProxy_h */
