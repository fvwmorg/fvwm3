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

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

#include <libs/fvwmlib.h>
#include "fvwm.h"



/*************************
 * MENU STYLE STRUCTURES *
 *************************/


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
      unsigned is_animated : 1;
      unsigned do_popup_immediately : 1;
      unsigned do_title_warp : 1;
      unsigned do_popup_as_root_menu : 1;
    } flags;
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
      unsigned do_hilight : 1;
      unsigned has_active_fore : 1;
      unsigned has_active_back : 1;
      unsigned has_stipple_fore : 1;
      unsigned has_long_separators : 1;
      unsigned has_triangle_relief : 1;
      unsigned has_side_color : 1;
    } flags;
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



/************************
 * MENU ITEM STRUCTURES *
 ************************/


struct MenuRoot; /* forward declaration */

/* IMPORTANT NOTE: Don't put members into this struct that can change while the
 * menu is visible! This will wreak havoc on recursive menus when they finally
 * get implemented. */
typedef struct MenuItem
{
    struct MenuRoot *mr;        /* the menu this item is in */
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */

    char *item;			/* the character string displayed on left*/
    char *item2;	        /* the character string displayed on right*/
    short strlen;		/* strlen(item) */
    short strlen2;		/* strlen(item2) */

    Picture *picture;           /* Pixmap to show  above label*/
    Picture *lpicture;          /* Pixmap to show to left of label */

    short x;			/* x coordinate for text (item) */
    short x2;			/* x coordinate for text (item2) */
    short xp;                   /* x coordinate for picture */
    short y_offset;		/* y coordinate for item */
    short y_height;		/* y height for item */

    char *action;		/* action to be performed */
    short func_type;		/* type of built in function */
    short hotkey;		/* Hot key offset (pete@tecc.co.uk).
				   0 - No hot key
				   +ve - offset to hot key char in item
				   -ve - offset to hot key char in item2
				   (offsets have 1 added, so +1 or -1
				   refer to the *first* character)
				   */
    char chHotkey;
    struct
    {
      unsigned is_separator;
      unsigned is_title;
      unsigned is_popup;
      unsigned is_menu;
    } flags;
} MenuItem;



/************************
 * MENU ROOT STRUCTURES *
 ************************/


typedef struct MenuRootCommon
{
} MenuRootCommon;

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
    short width0;               /* width of the menu-left-picture col */
    short width;		/* width of the menu for 1st col */
    short width2;		/* width of the menu for 2nd col */
    short width3;               /* width of the submenu triangle col */
    short xoffset;              /* the distance between the left border and the
				 * beginning of the menu items */
    short items;		/* number of items in the menu */
    Picture *sidePic;
    Pixel sideColor;
    /* Menu Face    */
    MenuStyle *ms;
    /* temporary flags, deleted when menu pops down! */
    struct
    {
      unsigned is_background_set : 1; /* is win background set? */
      unsigned is_in_use : 1;
      unsigned is_painted : 1;
      unsigned is_left : 1;   /* menu direction relative to parent menu */
      unsigned is_right : 1;
      unsigned is_up : 1;
      unsigned is_down : 1;
    } tflags;
    /* permanent flags */
    struct
    {
      unsigned has_side_color : 1;
    } flags;
    struct
    {
      char *popup_action;
      char *popdown_action;
    } dynamic;
    int xanimation;      /* x distance window was moved by animation     */
} MenuRoot;
/* don't forget to initialise new members in NewMenuRoot()! */



/***********************************
 * MENU OPTIONS AND POSITION HINTS *
 ***********************************/


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
  struct
  {
    unsigned no_warp : 1;
    unsigned warp_title : 1;
    unsigned fixed : 1;
    unsigned select_in_place : 1;
    unsigned select_warp : 1;
    unsigned has_poshints : 1;
  } flags;
} MenuOptions;

extern MenuPosHints lastMenuPosHints;
extern Bool fLastMenuPosHintsValid;



/****************************
 * MISCELLANEOUS MENU STUFF *
 ****************************/

typedef struct
{
  MenuRoot *menu;
  MenuRoot *menu_prior;
  FvwmWindow **pTmp_win;
  FvwmWindow *button_window;
  int *pcontext;
  XEvent *eventp;
  char **ret_paction;
  MenuOptions *pops;
  int cmenuDeep;
  struct
  {
    unsigned is_menu_from_frame_or_window_or_titlebar : 1;
    unsigned is_sticky : 1;
  } flags;
} MenuParameters;


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


typedef struct MenuInfo
{
  MenuRoot *all;
  struct MenuStyle *DefaultStyle;
  struct MenuStyle *LastStyle;
  int PopupDelay10ms;
  int DoubleClickTime;
  struct
  {
    unsigned use_animated_menus : 1;
  } flags;
} MenuInfo;

extern MenuInfo Menus;

#define IS_MENU_RETURN(x) ((x)>=MENU_DONE && (x)<=MENU_ABORTED_BUTTON)
#define IS_MENU_BUTTON(x) ((x)==MENU_DONE_BUTTON || (x)==MENU_ABORTED_BUTTON)
#define MENU_ADD_BUTTON(x) ((x)==MENU_DONE || (x)==MENU_ABORTED?(x)+1:(x))
#define MENU_ADD_BUTTON_IF(y,x) (y?MENU_ADD_BUTTON((x)):(x))



/**********************
 * EXPORTED FUNCTIONS *
 **********************/


MenuRoot *FollowMenuContinuations(MenuRoot *mr,MenuRoot **pmrPrior);
void AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
			  Bool fWarpPointerToo, int cusDelay,
			  float *ppctMovement );
MenuRoot *NewMenuRoot(char *name);
void AddToMenu(MenuRoot *, char *, char *, Bool, Bool);
void MakeMenu(MenuRoot *);
Bool PopUpMenu(MenuRoot *, int, int);
MenuStatus do_menu(MenuParameters *pmp);
MenuRoot *FindPopup(char *popup_name);
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuItem *mi, MenuOptions *pops);
void DestroyMenu(MenuRoot *mr, Bool recreate);
void MakeMenus(void);
void add_item_to_menu(F_CMD_ARGS);
void add_another_menu_item(char *action);
void destroy_menu(F_CMD_ARGS);
void ChangeMenuStyle(F_CMD_ARGS);
void DestroyMenuStyle(F_CMD_ARGS);
void SetMenuStyle(F_CMD_ARGS);
void UpdateAllMenuStyles(void);
void SetMenuCursor(Cursor cursor);


#endif /* _MENUS_ */

