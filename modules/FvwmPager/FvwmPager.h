/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include <libs/Picture.h>
#include <libs/vpacket.h>
#include <libs/Flocale.h>

typedef struct ScreenInfo
{
  unsigned long screen;
  int MyDisplayWidth;	/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;	/* my copy of DisplayHeight(dpy, screen) */

  char *FvwmRoot;	/* the head of the fvwm window list */

  Window Root;
  Window Pager_w;

  Font PagerFont;	 /* font struct for window labels in pager (optional)*/

  GC NormalGC;		 /* used for window names and setting backgrounds */
  GC MiniIconGC;	 /* used for clipping mini-icons */
  GC whGC, wsGC, ahGC, asGC; /* used for 3d shadows on mini-windows */

  char	*Hilite;	 /* the fvwm window that is highlighted
			  * except for networking delays, this is the
			  * window which REALLY has the focus */
  unsigned VScale;	 /* Panner scale factor */
  int VxMax;		 /* Max location for top left of virt desk*/
  int VyMax;
  int VxPages;		 /* desktop size */
  int VyPages;
  int VWidth;		 /* Size of virtual desktop */
  int VHeight;
  int Vx;		 /* Current loc for top left of virt desk */
  int Vy;
  int CurrentDesk;
  Pixmap sticky_gray_pixmap;
  Pixmap light_gray_pixmap;
  Pixmap gray_pixmap;
  Pixel black;
} ScreenInfo;

typedef struct pager_window
{
  char *t;
  Window w;
  Window frame;
  int x;
  int y;
  int width;
  int height;
  int desk;
  int frame_x;
  int frame_y;
  int frame_width;
  int frame_height;
  int title_height;
  int border_width;
  int icon_x;
  int icon_y;
  int icon_width;
  int icon_height;
  Pixel text;
  Pixel back;
  window_flags flags;
  struct
  {
    unsigned is_mapped : 1;
  } myflags;
  Window icon_w;
  Window icon_pixmap_w;
  char *icon_name;
  char *window_name;
  char *res_name;
  char *res_class;
  char *window_label; /* This is displayed inside the mini window */
  FvwmPicture mini_icon;
  int pager_view_x;
  int pager_view_y;
  int pager_view_width;
  int pager_view_height;
  int icon_view_width;
  int icon_view_height;

  Window PagerView;
  Window IconView;

  struct pager_window *next;
} PagerWindow;


typedef struct balloon_window
{
  Window w;		 /* ID of balloon window */
  PagerWindow *pw;	 /* pager window it's associated with */
  FlocaleFont *Ffont;
  char *label;		 /* the label displayed inside the balloon */
  int height;		 /* height of balloon window based on font */
  int border;		 /* border width */
  int yoffset;		 /* pixels above (<0) or below (>0) pager win */
} BalloonWindow;


typedef struct desk_info
{
  Window w;
  Window title_w;
  Window CPagerWin;
  BalloonWindow balloon;
  FvwmPicture *bgPixmap;		/* Pixmap used as background. */
  int colorset;
  int highcolorset;
  int ballooncolorset;
  char *Dcolor;
  char *label;
  GC NormalGC;			/* used for desk labels */
  GC DashedGC;			/* used the the pages boundary lines */
  GC HiliteGC;			/* used for hilighting the active desk */
  GC rvGC;			/* used for drawing hilighted desk title */
  GC BalloonGC;
} DeskInfo;

typedef struct pager_string_list
{
  struct pager_string_list *next;
  int desk;
  int colorset;
  int highcolorset;
  int ballooncolorset;
  char *Dcolor;
  char *label;
  FvwmPicture *bgPixmap;		/* Pixmap used as background. */
} PagerStringList;

/*************************************************************************
 *
 * Subroutine Prototypes
 *
 *************************************************************************/
char *GetNextToken(char *indata,char **token);
void Loop(int *fd);
void DeadPipe(int nonsense) __attribute__((noreturn));
void process_message(FvwmPacket*);
void ParseOptions(void);

void list_add(unsigned long *body);
void list_configure(unsigned long *body);
void list_config_info(unsigned long *body);
void list_destroy(unsigned long *body);
void list_focus(unsigned long *body);
void list_toggle(unsigned long *body);
void list_new_page(unsigned long *body);
void list_new_desk(unsigned long *body);
void list_raise(unsigned long *body);
void list_lower(unsigned long *body);
void list_unknown(unsigned long *body);
void list_iconify(unsigned long *body);
void list_deiconify(unsigned long *body, unsigned long length);
void list_window_name(unsigned long *body,unsigned long type);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_mini_icon(unsigned long *body);
void list_restack(unsigned long *body, unsigned long length);
void list_property_change(unsigned long *body);
void list_end(void);
int My_XNextEvent(Display *dpy, XEvent *event);

/* Stuff in x_pager.c */
void change_colorset(int colorset);
void initialize_pager(void);
void initialize_viz_pager(void);
Pixel GetColor(char *name);
void nocolor(char *a, char *b);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void MovePage(Bool is_new_desk);
void DrawGrid(int desk,int erase);
void DrawIconGrid(int erase);
void SwitchToDesk(int Desk);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t, Bool do_force_redraw);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindow(void);
void Hilight(PagerWindow *, int);
void Scroll(int window_w, int window_h, int x, int y, int Desk,
	    Bool do_scroll_icon);
void MoveWindow(XEvent *Event);
void BorderWindow(PagerWindow *t);
void BorderIconWindow(PagerWindow *t);
void LabelWindow(PagerWindow *t);
void LabelIconWindow(PagerWindow *t);
void PictureWindow(PagerWindow *t);
void PictureIconWindow(PagerWindow *t);
void ReConfigureIcons(Bool do_reconfigure_desk_only);
void IconSwitchPage(XEvent *Event);
void IconMoveWindow(XEvent *Event,PagerWindow *t);
void HandleEnterNotify(XEvent *Event);
void HandleExpose(XEvent *Event);
void MoveStickyWindows(void);
void MapBalloonWindow(PagerWindow *t, Bool is_icon_view);
void UnmapBalloonWindow();
void DrawInBalloonWindow(int i);
