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

typedef struct sProxyWindow
{
	Window			window;
	int			x,y;
	int			w,h;
	int			desk;
	Window			proxy;
	int			proxyx,proxyy;
	int			proxyw,proxyh;
	FvwmPicture		picture;
	char			*name;
	char			*iconname;
	struct sProxyWindow	*next;
	struct sProxyWindow	*prev;
	struct
	{
		unsigned is_shown : 1;
		unsigned is_iconified : 1;
	} flags;
} ProxyWindow;


#endif	/* FvwmProxy_h */
