#ifndef FVWMLIB_X11_H
#define FVWMLIB_X11_H

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/Xrandr.h>

#ifdef XPM
#define XpmSupport 1
#include <X11/xpm.h>
#else
#define XpmSupport 0
#endif

#ifdef HAVE_PNG
#define PngSupport 1
#include <png.h>
#else
#define PngSupport 0
#include <setjmp.h>
#endif

#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

#ifdef HAVE_XRENDER
#define XRenderSupport 1
#include <X11/extensions/Xrender.h>
typedef Picture XRenderPicture;
#else
#define XRenderSupport 0
#endif

#ifdef HAVE_XFT
/* no compat to avoid problems in the future */
#define _XFT_NO_COMPAT_ 1
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#endif

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#ifdef HAVE_XSHM
#define XShmSupport 1
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#else
#define XShmSupport 0
#endif

#ifdef SESSION
#define SessionSupport 1
#include <X11/SM/SMlib.h>
#include <X11/ICE/ICEutil.h>
#else
#define SessionSupport 0
#endif

#ifdef HAVE_X11_XKBLIB_H
#include <X11/XKBlib.h>
#define fvwm_KeycodeToKeysym(d, k, l, g) \
	(XkbKeycodeToKeysym((d), (k), (g), (l)))
#else
#define fvwm_KeycodeToKeysym(d, k, x, i) (XKeycodeToKeysym((d), (k), (i)))
#endif

#endif /* FVWMLIB_X11_H */
