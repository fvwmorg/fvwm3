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
 *
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _MENUS_
#define _MENUS_

/* Function types used for formatting menus */

#define FUNC_NO_WINDOW False
#define FUNC_NEEDS_WINDOW True

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

#include "../libs/fvwmlib.h"

typedef enum {
    /* menu types */
    SimpleMenu = 0,
#ifdef GRADIENT_BUTTONS
    HGradMenu,
    VGradMenu,
    DGradMenu,
    BGradMenu,
#endif
#ifdef PIXMAP_BUTTONS
    PixmapMenu,
    TiledPixmapMenu,
#endif
    SolidMenu
    /* max button is 8 (0x8) */
} MenuFaceType;

typedef struct MenuFeel {
    struct
    {
      unsigned char Animated : 1;
      unsigned char PopupImmediately : 1;
      unsigned char TitleWarp : 1;
    } f;
    int PopupOffsetPercent;
    int PopupOffsetAdd;
} MenuFeel;


typedef struct MenuFace {
    union {
#ifdef PIXMAP_BUTTONS
        Picture *p;
#endif
        Pixel back;
#ifdef GRADIENT_BUTTONS
        struct {
            int npixels;
            Pixel *pixels;
        } grad;
#endif
    } u;
    MenuFaceType type;
} MenuFace;

typedef struct MenuLook {
    MenuFace face;
    struct
    {
      unsigned char Hilight : 1;
      unsigned char hasActiveFore : 1;
      unsigned char hasActiveBack : 1;
      unsigned char hasStippleFore : 1;
      unsigned char LongSeparators : 1;
      unsigned char TriangleRelief : 1;
      unsigned char hasSideColor : 1;
    } f;
    char ReliefThickness;
    char TitleUnderlines;
    Picture *sidePic;
    Pixel sideColor;
    GC MenuGC;
    GC MenuActiveGC;
    GC MenuActiveBackGC;
    GC MenuStippleGC;
    GC MenuReliefGC;
    GC MenuShadowGC;
    ColorPair MenuColors;
    ColorPair MenuActiveColors;
    ColorPair MenuStippleColors;
    ColorPair MenuRelief;
    MyFont *pStdFont;
    int EntryHeight;              /* menu entry height */
} MenuLook;

typedef struct MenuStyle {
    char *name;
    struct MenuStyle *next;
    MenuLook look;
    MenuFeel feel;
} MenuStyle;

struct MenuRoot; /* forward declaration */

typedef struct MenuItem
{
    struct MenuRoot *mr;        /* the menu this item is in */
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */
    char *item;			/* the character string displayed on left*/
    char *item2;	        /* the character string displayed on right*/
    Picture *picture;           /* Pixmap to show  above label*/
    Picture *lpicture;          /* Pixmap to show to left of label */
    char *action;		/* action to be performed */
    short item_num;		/* item number of this menu */
    short x;			/* x coordinate for text (item) */
    short x2;			/* x coordinate for text (item2) */
    short xp;                   /* x coordinate for picture */
    short y_offset;		/* y coordinate for item */
    short y_height;		/* y height for item */
    short func_type;		/* type of built in function */
    Bool func_needs_window;
    short state;		/* video state, 0 = normal, 1 = reversed */
    short strlen;		/* strlen(item) */
    short strlen2;		/* strlen(item2) */
    short hotkey;		/* Hot key offset (pete@tecc.co.uk).
				   0 - No hot key
				   +ve - offset to hot key char in item
				   -ve - offset to hot key char in item2
				   (offsets have 1 added, so +1 or -1
				   refer to the *first* character)
				   */
    char chHotkey;
    short fIsSeparator;          /* 1 if this is a separator or a title */
} MenuItem;

typedef struct MenuRoot
{
    MenuItem *first;	/* first item in menu */
    MenuItem *last;	/* last item in menu */
    MenuItem *selected;	/* the selected item in menu */
#ifdef GRADIENT_BUTTONS
    struct
    {
      Pixmap stored;
      int width;
      int height;
      int y;
    } stored_item;
#endif

    struct MenuRoot *next;	/* next in list of root menus */
    struct MenuRoot *continuation; /* continuation of this menu
				    * (too tall for screen */
    /* can get the menu that this popped up through selected->mr when
       selected IS_POPUP_MENU_ITEM(selected) */
    struct MenuRoot *mrDynamicPrev; /* the menu that popped this up, if any */

    char *name;			/* name of root */
    Window w;			/* the window of the menu */
    short height;		/* height of the menu */
    short width;		/* width of the menu for 1st col */
    short width2;		/* width of the menu for 2nd col */
    short width0;               /* width of the menu-left-picture col */
    short items;		/* number of items in the menu */
    Bool backgroundset;         /* is win background set for this menu ?? */
    Bool in_use;
    int func;
    Picture *sidePic;
    Pixel sideColor;
    Bool colorize;
    short xoffset;
    MenuStyle *ms;        /* Menu Face    */
    union                 /* internal flags, deleted when menu pops down! */
    {
      /* need to change that type if we have more than 8 flags.
       * more that a word will entail some changes in the code! */
      unsigned char allflags;
      struct
      {
	unsigned painted : 1;
	unsigned is_left : 1;   /* menu direction relative to parent menu */
	unsigned is_right : 1;
	unsigned is_up : 1;
	unsigned is_down : 1;
      } f;
    } flags;
    int xanimation;      /* x distance window was moved by animation     */
} MenuRoot;
/* don't forget to initialise new members in NewMenuRoot()! */

typedef struct MenuGlobals {
    MenuRoot *all;
    struct MenuStyle *DefaultStyle;
    struct MenuStyle *LastStyle;
    int PopupDelay10ms;
    int DoubleClickTime;
} MenuGlobals;

typedef struct Binding
{
  char IsMouse;           /* Is it a mouse or key binding 1= mouse; */
  int Button_Key;         /* Mouse Button number of Keycode */
  char *key_name;         /* In case of keycode, give the key_name too */
  int Context;            /* Contex is Fvwm context, ie titlebar, frame, etc */
  int Modifier;           /* Modifiers for keyboard state */
  char *Action;           /* What to do? */
  struct Binding *NextBinding;
} Binding;

typedef struct
{
  int x;                  /* suggested x position */
  int y;                  /* suggested y position */
  float x_factor;         /* to take menu width into account (0, -1 or -0.5) */
  float y_factor;         /* same with height */
  Bool fRelative;         /* FALSE if referring to absolute screen position */
} MenuPosHints;

typedef struct
{
  MenuPosHints pos_hints;
  union
  {
    /* need to change that type if we have more than 8 flags.
     * more that a word will entail some changes in the code! */
    unsigned char allflags;
    struct
    {
      unsigned no_warp : 1;
      unsigned warp_title : 1;
      unsigned fixed : 1;
      unsigned select_in_place : 1;
      unsigned select_warp : 1;
      unsigned has_poshints : 1;
    } f;
  } flags;
} MenuOptions;

extern MenuPosHints lastMenuPosHints;
extern Bool fLastMenuPosHintsValid;


/* Return values for UpdateMenu, do_menu, menuShortcuts */
/* Just uses enum-s for their constant value, replaced a bunch of #define-s
 * before */
/* This is a lame hack, in that "_BUTTON" is added to mean a button-release
   caused the return-- the macros below help deal with the ugliness */
typedef enum {
    MENU_ERROR = -1,
    MENU_NOP = 0,
    MENU_DONE = 1,
    MENU_DONE_BUTTON = 2,  /* must be MENU_DONE + 1 */
    MENU_ABORTED = 3,
    MENU_ABORTED_BUTTON = 4, /* must be MENU_ABORTED + 1 */
    MENU_SUBMENU_DONE,
    MENU_DOUBLE_CLICKED,
    MENU_POPUP,
    MENU_POPDOWN,
    MENU_SELECTED,
    MENU_NEWITEM
} MenuStatus;

#define IS_MENU_RETURN(x) ((x)>=MENU_DONE && (x)<=MENU_ABORTED_BUTTON)
#define IS_MENU_BUTTON(x) ((x)==MENU_DONE_BUTTON || (x)==MENU_ABORTED_BUTTON)
#define MENU_ADD_BUTTON(x) ((x)==MENU_DONE || (x)==MENU_ABORTED?(x)+1:(x))
#define MENU_ADD_BUTTON_IF(y,x) (y?MENU_ADD_BUTTON((x)):(x))

/* Types of events for the FUNCTION builtin */
#define MOTION 'm'
#define IMMEDIATE 'i'
#define CLICK 'c'
#define DOUBLE_CLICK 'd'
#define ONE_AND_A_HALF_CLICKS 'o'

MenuRoot *FollowMenuContinuations(MenuRoot *mr,MenuRoot **pmrPrior);
void AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
			  Bool fWarpPointerToo, int cusDelay,
			  float *ppctMovement );

#endif /* _MENUS_ */

