/* -*-c-*- */
/* vim: set ts=8 shiftwidth=8: */

#ifndef FvwmProxy_h
#define FvwmProxy_h

/* TODO check what headers we really need */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
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
		unsigned soft_raise : 2;
		unsigned pad0 : 2;
		unsigned hard_raise : 2;
		unsigned pad1 : 2;

		unsigned soft_desk : 2;
		unsigned pad2 : 2;
		unsigned hard_desk : 2;
		unsigned pad3 : 2;

		unsigned soft_drag : 2;
		unsigned pad4 : 2;
		unsigned hard_drag : 2;
		unsigned pad5 : 2;

		unsigned soft_icon : 2;
		unsigned pad6 : 2;
		unsigned hard_icon : 2;
		unsigned pad7 : 2;

		unsigned pad8 : 4;

		unsigned is_soft : 1;
		unsigned pad9 : 4;

		unsigned is_weak : 1;
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

		unsigned soft_raise : 2;
		unsigned pad0 : 2;
		unsigned hard_raise : 2;
		unsigned pad1 : 2;

		unsigned soft_desk : 2;
		unsigned pad2 : 2;
		unsigned hard_desk : 2;
		unsigned pad3 : 2;

		unsigned soft_drag : 2;
		unsigned pad4 : 2;
		unsigned hard_drag : 2;
		unsigned pad5 : 2;

		unsigned soft_icon : 2;
		unsigned pad6 : 2;
		unsigned hard_icon : 2;
		unsigned pad7 : 2;

		unsigned pad8 : 4;

		unsigned auto_include : 1;
		unsigned pad9 : 3;

		unsigned auto_soft : 1;
		unsigned pad10 : 3;

		unsigned isolated : 1;
		unsigned pad11 : 3;

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
	int			stack;
	int			stack_desired;
	int			stack_tmp;
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
