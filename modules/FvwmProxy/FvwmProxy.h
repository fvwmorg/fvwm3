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
	int			desk;
	Window			proxy;
	int			proxyx,proxyy;
	int			proxyw,proxyh;
	FvwmPicture		picture;
	char			*name;
	char			*iconname;
	struct sProxyWindow	*next;
	struct
	{
		unsigned is_shown : 1;
		unsigned is_iconified : 1;
	} flags;
} ProxyWindow;


#endif	/* FvwmProxy_h */
