/**
    This module, FvwmProxy, is an original work by Jason Weber.

    Copyright 2002, Jason Weber.  No guarantees or warrantees or anything
    are provided or implied in any way whatsoever.  Use this program at
    your own risk.

    Interim License: Until this code reaches acceptable maturity,
    its use is limited to evaluation only.  This code may not
    be redistribuited until further notice.

    If and when this software is officially merged into Fvwm project,
    it will likely adopt the licensing terms therein.
*/

// vim:ts=8:shiftwidth=8:

#ifndef FvwmProxy_h
#define FvwmProxy_h

//* TODO check what headers we really need
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
#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

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
	Window			proxy;
	int			proxyx,proxyy;
	int			proxyw,proxyh;
	FvwmPicture		picture;
	char			*name;
	char			*iconname;
	struct sProxyWindow	*next;
} ProxyWindow;


#endif	// FvwmProxy_h
