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

#ifndef FVWM_H
#define FVWM_H

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

#define F_CMD_ARGS XEvent *eventp, Window w, FvwmWindow *tmp_win,\
unsigned long context,char *action, int *Module

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <libs/Picture.h>
#include "window_flags.h"

/* Allow GCC extensions to work, if you have GCC */

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(x)
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

/* the maximum number of mouse buttons fvwm knows about */
/* don't think that upping this to 5 will make everything
 * hunky-dory with 5 button mouses */
#define MAX_BUTTONS 3

#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
#endif

#define BOUNDARY_WIDTH 7    	/* border width */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define NULLSTR ((char *) NULL)

typedef struct
{
  int x;
  int y;
  int width;
  int height;
} rectangle;

/*
  For 1 style statement, there can be any number of IconBoxes.
  The name list points at the first one in the chain.
 */
typedef struct icon_boxes_struct
{
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

typedef struct MyFont
{
  XFontStruct *font;		/* font structure */
#ifdef I18N_MB
  XFontSet fontset;		/* font set */
#endif
  int height;			/* height of the font */
  int y;			/* Y coordinate to draw characters */
} MyFont;

typedef struct ColorPair
{
  Pixel fore;
  Pixel back;
} ColorPair;


#ifdef USEDECOR
struct FvwmDecor;		/* definition in screen.h */
#endif

typedef struct
{
  /* common flags (former flags in bits 0-12) */
  unsigned is_sticky : 1;
  /* static flags that do not change dynamically after the window has been
   * created */
  struct
  {
    unsigned do_circulate_skip : 1;
    unsigned do_circulate_skip_icon : 1;
    unsigned do_flip_transient : 1;
    unsigned do_grab_focus_when_created : 1;
    unsigned do_grab_focus_when_transient_created : 1;
    unsigned do_ignore_restack : 1;
    unsigned do_lower_transient : 1;
    unsigned do_not_show_on_map : 1;
    unsigned do_raise_transient : 1;
    unsigned do_resize_opaque : 1;
    unsigned do_stack_transient_parent : 1;
    unsigned do_start_iconic : 1;
    unsigned do_window_list_skip : 1;
#define FOCUS_MOUSE   0x0
#define FOCUS_CLICK   0x1
#define FOCUS_SLOPPY  0x2
#define FOCUS_NEVER   0x3
#define FOCUS_MASK    0x3
    unsigned focus_mode : 2;
    unsigned has_depressable_border : 1;
    unsigned has_mwm_border : 1;
    unsigned has_mwm_buttons : 1;
    unsigned has_mwm_override : 1;
    unsigned has_no_icon_title : 1;
    unsigned has_override_size : 1;
    unsigned is_fixed : 1;
    unsigned is_icon_sticky : 1;
    unsigned is_icon_suppressed : 1;
    unsigned is_lenient : 1;
  } s;
} common_flags_type;

typedef struct
{
  common_flags_type common;
  unsigned does_wm_delete_window : 1;
  unsigned does_wm_take_focus : 1;
  unsigned do_reuse_destroyed : 1;   /* Reuse this struct, don't free it,
				      * when destroying/recapturing window. */
  unsigned has_border : 1; /* Is this decorated with border*/
  unsigned has_title : 1; /* Is this decorated with title */
  unsigned is_fully_visible : 1; /* is the window fully visible */
  unsigned is_iconified : 1; /* is it an icon now? */
  unsigned is_iconified_by_parent : 1; /* To prevent iconified transients in a
					* parent icon from counting for Next */
  unsigned is_icon_entered : 1; /* is the pointer over the icon? */
  unsigned is_icon_ours : 1; /* is the icon window supplied by the app? */
  unsigned is_icon_shaped : 1; /* is the icon shaped? */
  unsigned is_icon_moved : 1; /* has the icon been moved by the user? */
  unsigned is_icon_unmapped : 1; /* was the icon unmapped, even though the
				  * window is still iconified (Transients) */
  unsigned is_mapped : 1; /* is it mapped? */
  unsigned is_map_pending : 1; /* Sent an XMapWindow, but didn't receive a
				* MapNotify yet.*/
  unsigned is_maximized : 1; /* is the window maximized? */
  unsigned is_name_changed : 1; /* Set if the client changes its WM_NAME.
				 * The source of twm contains an explanation
				 * why we need this information. */
  unsigned is_partially_visible : 1; /* is the window partially visible */
  unsigned is_pixmap_ours : 1; /* is the icon pixmap ours to free? */
  unsigned is_transient : 1; /* is it a transient window? */
  unsigned is_window_drawn_once : 1;
  unsigned is_deiconify_pending : 1; /* Sent an XUnmapWindow for deiconifying,
				      * but didn't receive a UnmapNotify yet.*/
  unsigned is_viewport_moved : 1; /* To prevent double move in MoveViewport.*/
  unsigned is_window_being_moved_opaque : 1;
  unsigned is_window_shaded : 1;
} window_flags;

/* only style.c and add_window.c are allowed to access this struct!! */
typedef struct
{
  common_flags_type common;
  unsigned do_decorate_transient : 1;
  unsigned do_place_random : 1;
  unsigned do_place_smart : 1;
  unsigned do_start_lowered : 1;
  unsigned has_border_width : 1;
  unsigned has_color_back : 1;
  unsigned has_color_fore : 1;
  unsigned has_decor : 1;
  unsigned has_handle_width : 1;
  unsigned has_icon : 1;
  unsigned has_icon_boxes : 1;
  unsigned has_max_window_size : 1;
#ifdef MINI_ICONS
  unsigned has_mini_icon : 1;
#endif
  unsigned has_mwm_decor : 1;
  unsigned has_mwm_functions : 1;
  unsigned has_no_border : 1;
  unsigned has_no_title : 1;
  unsigned has_ol_decor : 1;
  unsigned is_button_disabled : 10;
  unsigned use_layer : 1;
  unsigned use_no_pposition : 1;
  unsigned use_start_on_desk : 1;
  unsigned use_start_on_page_for_transient : 1;
  unsigned active_placement_honors_starts_on_page : 1;
  unsigned capture_honors_starts_on_page : 1;
  unsigned recapture_honors_starts_on_page : 1;
  unsigned icon_override : 2;
#define NO_ACTIVE_ICON_OVERRIDE 0
#define ICON_OVERRIDE           1
#define NO_ICON_OVERRIDE        2
#define ICON_OVERRIDE_MASK      3
} style_flags;

/* only style.c and add_window.c are allowed to access this struct!! */
typedef struct window_style
{
  struct window_style *next;
  char *name;		  	   /* the name of the window */
  char *icon_name; /* value */               /* icon name */
#ifdef MINI_ICONS
  char *mini_icon_name; /* mini_value */               /* mini icon name */
#endif
#ifdef USEDECOR
  char *decor_name; /* Decor */
#endif
  char *fore_color_name; /* ForeColor */
  char *back_color_name; /* BackColor */
  int border_width;
  int layer;
  int handle_width; /* resize_width */
  int start_desk; /* Desk */
  int start_page_x; /* PageX */
  int start_page_y; /* PageY */
  int max_window_width;
  int max_window_height;
  icon_boxes *IconBoxes;                /* pointer to iconbox(s) */
  style_flags flags;
  style_flags flag_mask;
  style_flags change_mask;
  unsigned has_style_changed : 1;
} window_style;

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

    rectangle frame_g;

    int boundary_width;
    int corner_width;

    rectangle title_g;

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
#ifdef I18N_MB
    char **name_list;		/* window name list */
    char **icon_name_list;	/* icon name list */
#endif
    XWindowAttributes attr;	/* the child window attributes */
    XSizeHints hints;		/* normal hints */
    XWMHints *wmhints;		/* WM hints */
    XClassHint class;
    int Desk;                   /* Tells which desktop this window is on */
    int FocusDesk;		/* Where (if at all) was it focussed */
    int DeIconifyDesk;          /* Desk to deiconify to, for StubbornIcons */
    Window transientfor;

    window_flags flags;

#ifdef MINI_ICONS
    char *mini_pixmap_file;
    Picture *mini_icon;
#endif
    char *icon_bitmap_file;

    rectangle orig_g;

    int maximized_ht;           /* maximized window height */

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

    int default_layer;
    int layer;

    int max_window_width;
    int max_window_height;

} FvwmWindow;

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask {
  struct
  {
    unsigned needs_current_desk : 1;
    unsigned needs_current_page : 1;
    unsigned needs_name : 1;
    unsigned needs_not_name : 1;
    unsigned use_circulate_hit : 1;
    unsigned use_circulate_hit_icon : 1;
    unsigned use_circulate_skip : 1;
    unsigned use_circulate_skip_icon : 1;
  } my_flags;
  window_flags flags;
  window_flags flag_mask;
  char *name;
  int layer;
} WindowConditionMask;

#if 0
/***************************************************************************
 * window flags definitions
 ***************************************************************************/
/* The first 13 items are mapped directly from the style structure's
 * flag value, so they MUST correspond to the first 13 entries in misc.h */
#define STARTICONIC             (1<<0)
#define GRABFOCUS               (1<<1) /* Grab focus when mapped? */
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
#define ALL_COMMON_FLAGS (STARTICONIC|STICKY|WINDOWLISTSKIP| \
			  SUPPRESSICON|NOICON_TITLE|Lenience|StickyIcon| \
			  CirculateSkipIcon|CirculateSkip|ClickToFocus| \
			  SloppyFocus|SHOW_ON_MAP|GRABFOCUS)

#define BORDER         (1<<13) /* Is this decorated with border*/
#define TITLE          (1<<14) /* Is this decorated with title */
#define MAPPED         (1<<15) /* is it mapped? */
#define ICONIFIED      (1<<16) /* is it an icon now? */
#define TRANSIENT      (1<<17) /* is it a transient window? */
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
#endif


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


extern void Reborder(void);
RETSIGTYPE SigDone(int);
RETSIGTYPE Restart(int nonsense);
extern void Done(int, char *) __attribute__((__noreturn__));
extern void setInitFunctionName(int n, const char *name);
extern const char *getInitFunctionName(int n);

extern void CaptureOneWindow(FvwmWindow *fw, Window window);
extern void CaptureAllWindows(void);

extern int master_pid;

extern Display *dpy;

extern XContext FvwmContext;

extern Bool fFvwmInStartup;
extern Bool DoingCommandLine;

extern Boolean ShapesSupported;

extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;
extern char *user_home_dir;

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

extern Atom _XA_WM_WINDOW_ROLE;
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_SM_CLIENT_ID;

extern Atom _XA_WIN_SX;
extern Atom _XA_MANAGER;
extern Atom _XA_ATOM_PAIR;
extern Atom _XA_WM_COLORMAP_NOTIFY;

/* include this down here because FvwmWindows must be defined when including
 * this header file. */
#include "fvwmdebug.h"

#endif /* _FVWM_ */
