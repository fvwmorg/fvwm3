/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 * fvwm include file
 ***********************************************************************/

#ifndef _FVWM_
#define _FVWM_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

/* the maximum number of mouse buttons fvwm knows about */
/* don't think that upping this to 5 will make everything
 * hunky-dory with 5 button mouses */
#define MAX_BUTTONS 3

#include <X11/Intrinsic.h>

#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
#endif

#define BW 1			/* border width */
#define BOUNDARY_WIDTH 7    	/* border width */
#define CORNER_WIDTH 16    	/* border width */

# define HEIGHT_EXTRA 4		/* Extra height for texts in popus */
# define HEIGHT_EXTRA_TITLE 4	/* Extra height for underlining title */
# define HEIGHT_SEPARATOR 4	/* Height of separator lines */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define NULLSTR ((char *) NULL)

/* contexts for button presses */
#define C_NO_CONTEXT	0
#define C_WINDOW	1
#define C_TITLE		2
#define C_ICON		4
#define C_ROOT		8
#define C_FRAME		16
#define C_SIDEBAR       32
#define C_L1            64
#define C_L2           128
#define C_L3           256
#define C_L4           512
#define C_L5          1024
#define C_R1          2048
#define C_R2          4096
#define C_R3          8192
#define C_R4         16384
#define C_R5         32768
#define C_RALL       (C_R1|C_R2|C_R3|C_R4|C_R5)
#define C_LALL       (C_L1|C_L2|C_L3|C_L4|C_L5)
#define C_ALL   (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|\
                 C_L1|C_L2|C_L3|C_L4|C_L5|C_R1|C_R2|C_R3|C_R4|C_R5)

typedef struct MyFont
{
  XFontStruct *font;		/* font structure */
  int height;			/* height of the font */
  int y;			/* Y coordinate to draw characters */
} MyFont;

typedef struct ColorPair
{
  Pixel fore;
  Pixel back;
} ColorPair;

#ifdef MINI_ICONS
#include "../libs/fvwmlib.h"
#endif

#ifdef USEDECOR
struct FvwmDecor;		/* definition in screen.h */
#endif

/*
  For 1 style statement, there can be any number of IconBoxes.
  The name list points at the first one in the chain.
 */
typedef struct icon_boxes_struct {
  struct icon_boxes_struct *next;       /* next icon_boxes or zero */
  int IconBox[4];                       /* x/y x/y for iconbox */
  short IconGrid[2];                    /* x incr, y incr */
  unsigned char IconFlags;               /* some bits */
  /* IconFill only takes 3 bits.  Defaults are top, left, vert co-ord first */
  /* eg: t l = 0,0,0; l t = 0,0,1; b r = 1,1,0 */
#define ICONFILLBOT (1<<0)
#define ICONFILLRGT (1<<1)
#define ICONFILLHRZ (1<<2)
} icon_boxes;

/* for each window that is on the display, one of these structures
 * is allocated and linked into a list
 */
typedef struct FvwmWindow
{
    struct FvwmWindow *next;	/* next fvwm window */
    struct FvwmWindow *prev;	/* prev fvwm window */
    struct FvwmWindow *stack_next; /* next (lower) fvwm window in stacking
				    * order*/
    struct FvwmWindow *stack_prev; /* prev (higher) fvwm window in stacking
				    * order */
    Window w;			/* the child window */
    int old_bw;			/* border width before reparenting */
    Window frame;		/* the frame window */
    Window Parent;              /* Ugly Ugly Ugly - it looks like you
				 * HAVE to reparent the app window into
				 * a window whose size = app window,
				 * or else you can't keep xv and matlab
				 * happy at the same time! */
    Window title_w;		/* the title bar window */
    Window sides[4];
    Window corners[4];          /* Corner pieces */
    int nr_left_buttons;
    int nr_right_buttons;
    Window left_w[5];
    Window right_w[5];
#ifdef USEDECOR
    struct FvwmDecor *fl;
#endif
    Window icon_w;		/* the icon window */
    Window icon_pixmap_w;	/* the icon window */
#ifdef SHAPE
    int wShaped;               /* is this a shaped window */
#endif
    int frame_x;		/* x position of frame */
    int frame_y;		/* y position of frame */
    int frame_width;		/* width of frame */
    int frame_height;		/* height of frame */
    int boundary_width;
    int corner_width;
    int bw;
    int title_x;
    int title_y;
    int title_height;		/* height of the title bar */
    int title_width;		/* width of the title bar */
    int icon_x_loc;		/* icon window x coordinate */
    int icon_xl_loc;		/* icon label window x coordinate */
    int icon_y_loc;		/* icon window y coordiante */
    int icon_w_width;		/* width of the icon window */
    int icon_w_height;		/* height of the icon window */
    int icon_t_width;		/* width of the icon title window */
    int icon_p_width;		/* width of the icon pixmap window */
    int icon_p_height;		/* height of the icon pixmap window */
    Pixmap iconPixmap;		/* pixmap for the icon */
    int iconDepth;		/* Drawable depth for the icon */
    Pixmap icon_maskPixmap;	/* pixmap for the icon mask */
    char *name;			/* name of the window */
    char *icon_name;		/* name of the icon */
    XWindowAttributes attr;	/* the child window attributes */
    XSizeHints hints;		/* normal hints */
    XWMHints *wmhints;		/* WM hints */
    XClassHint class;
    int Desk;                   /* Tells which desktop this window is on */
    int FocusDesk;		/* Where (if at all) was it focussed */
    int DeIconifyDesk;          /* Desk to deiconify to, for StubbornIcons */
    Window transientfor;

    unsigned long flags;
#ifdef MINI_ICONS
    char *mini_pixmap_file;
    Picture *mini_icon;
#endif
    char *icon_bitmap_file;

    int orig_x;                 /* unmaximized x coordinate */
    int orig_y;                 /* unmaximized y coordinate */
    int orig_wd;                /* unmaximized window width */
    int orig_ht;                /* unmaximized window height */

    int xdiff,ydiff;            /* used to restore window position on exit*/
    int *mwm_hints;
    int ol_hints;
    int functions;
    Window *cmap_windows;       /* Colormap windows property */
    int number_cmap_windows;    /* Should generally be 0 */
    Pixel ReliefPixel;
    Pixel ShadowPixel;
    Pixel TextPixel;
    Pixel BackPixel;
    unsigned long buttons;
    icon_boxes *IconBoxes;              /* zero or more iconboxes */
/*
    RBW - 11/13/1998 - new flags to supplement the flags word, implemented
    as named bit fields.
*/
    unsigned ViewportMoved : 1; /* To prevent double move in MoveViewport. */
} FvwmWindow;

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask {
  Bool needsCurrentDesk;
  Bool needsCurrentPage;
  Bool needsName;
  Bool needsNotName;
  Bool useCirculateHit;
  Bool useCirculateHitIcon;
  Bool useCirculateSkip;
  Bool useCirculateSkipIcon;
  unsigned long onFlags;
  unsigned long offFlags;
  char *name;
} WindowConditionMask;

/***************************************************************************
 * window flags definitions
 ***************************************************************************/
/* The first 13 items are mapped directly from the style structure's
 * flag value, so they MUST correspond to the first 13 entries in misc.h */
#define STARTICONIC             (1<<0)
#define ONTOP                   (1<<1) /* does window stay on top */
#define STICKY                  (1<<2) /* Does window stick to glass? */
#define WINDOWLISTSKIP          (1<<3)
#define SUPPRESSICON            (1<<4)
#define NOICON_TITLE            (1<<5)
#define Lenience                (1<<6)
#define StickyIcon              (1<<7)
#define CirculateSkipIcon       (1<<8)
#define CirculateSkip           (1<<9)
#define ClickToFocus            (1<<10)
#define SloppyFocus             (1<<11)
#define SHOW_ON_MAP    (1<<12) /* switch to desk when it gets mapped? */
#define ALL_COMMON_FLAGS (STARTICONIC|ONTOP|STICKY|WINDOWLISTSKIP| \
			  SUPPRESSICON|NOICON_TITLE|Lenience|StickyIcon| \
			  CirculateSkipIcon|CirculateSkip|ClickToFocus| \
			  SloppyFocus|SHOW_ON_MAP)

#define BORDER         (1<<13) /* Is this decorated with border*/
#define TITLE          (1<<14) /* Is this decorated with title */
#define MAPPED         (1<<15) /* is it mapped? */
#define ICONIFIED      (1<<16) /* is it an icon now? */
#define TRANSIENT      (1<<17) /* is it a transient window? */
#define RAISED         (1<<18) /* if its a sticky window, needs raising? */
#define VISIBLE        (1<<19) /* is the window fully visible */
#define ICON_OURS      (1<<20) /* is the icon window supplied by the app? */
#define PIXMAP_OURS    (1<<21)/* is the icon pixmap ours to free? */
#define SHAPED_ICON    (1<<22)/* is the icon shaped? */
#define MAXIMIZED      (1<<23)/* is the window maximized? */
#define DoesWmTakeFocus		(1<<24)
#define DoesWmDeleteWindow	(1<<25)
/* has the icon been moved by the user? */
#define ICON_MOVED              (1<<26)
/* was the icon unmapped, even though the window is still iconified
 * (Transients) */
#define ICON_UNMAPPED           (1<<27)
/* Sent an XMapWindow, but didn't receive a MapNotify yet.*/
#define MAP_PENDING             (1<<28)
#define HintOverride            (1<<29)
#define MWMButtons              (1<<30)
#define MWMBorders              (1<<31)


/* flags to suppress/enable title bar buttons */
#define BUTTON1     1
#define BUTTON2     2
#define BUTTON3     4
#define BUTTON4     8
#define BUTTON5    16
#define BUTTON6    32
#define BUTTON7    64
#define BUTTON8   128
#define BUTTON9   256
#define BUTTON10  512

#ifdef WINDOWSHADE
/* we're sticking this at the end of the buttons window member
   since we don't want to use up any more flag bits */
#define WSHADE	(1<<31)
#endif

#include <stdlib.h>
extern void Reborder(void);
extern void SigDone(int);
extern void Restart(int nonsense);
extern void Done(int, char *);
extern void BlackoutScreen(void);
extern void UnBlackoutScreen(void);

extern int master_pid;

extern Display *dpy;

extern XContext FvwmContext;

extern Window BlackoutWin;

Bool fFvwmInStartup;

extern Boolean ShapesSupported;

extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

extern Atom _XA_MIT_PRIORITY_COLORS;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_SAVE_YOURSELF;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_DESKTOP;
extern Atom _XA_FVWM_STICKS_TO_GLASS;
extern Atom _XA_FVWM_CLIENT;
extern Atom _XA_OL_WIN_ATTR;
extern Atom _XA_OL_WT_BASE;
extern Atom _XA_OL_WT_CMD;
extern Atom _XA_OL_WT_HELP;
extern Atom _XA_OL_WT_NOTICE;
extern Atom _XA_OL_WT_OTHER;
extern Atom _XA_OL_DECOR_ADD;
extern Atom _XA_OL_DECOR_DEL;
extern Atom _XA_OL_DECOR_CLOSE;
extern Atom _XA_OL_DECOR_RESIZE;
extern Atom _XA_OL_DECOR_HEADER;
extern Atom _XA_OL_DECOR_ICON_NAME;

/* include this down here because FvwmWindows must be defined when including
 * this header file. */
#include "fvwmdebug.h"

#endif /* _FVWM_ */
