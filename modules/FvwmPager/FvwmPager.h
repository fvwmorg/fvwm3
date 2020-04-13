/* -*-c-*- */
#include "libs/Picture.h"
#include "libs/vpacket.h"
#include "libs/Flocale.h"

struct fpmonitor {
	char		*name;
	int		 is_primary, output;
	int 		 number;
	int		 win_count;
	int		 wants_refresh;
	int		 is_disabled;
	int		 is_current;

        struct {
                int VxMax;
                int VyMax;
                int Vx;
                int Vy;
		int VxPages;           /* desktop size */
		int VyPages;
		int VWidth;            /* Size of virtual desktop */
		int VHeight;

                int CurrentDesk;
		int MyDisplayWidth;
		int MyDisplayHeight;
        } virtual_scr;

	TAILQ_ENTRY(fpmonitor) entry;
};
TAILQ_HEAD(fpmonitors, fpmonitor);

struct fpmonitors		 fp_monitor_q;
struct fpmonitor		*fpmonitor_by_name(const char *);
struct fpmonitor		*fpmonitor_by_output(int);
struct fpmonitor		*fpmonitor_get_current(void);
struct fpmonitor 		*fpmonitor_this(void);

typedef struct ScreenInfo
{
  unsigned long screen;

  char *FvwmRoot;       /* the head of the fvwm window list */

  Window Root;
  Window Pager_w;
  Window label_w;
  Window balloon_w;

  Font PagerFont;        /* font struct for window labels in pager (optional)*/

  GC NormalGC;           /* used for window names and setting backgrounds */
  GC MiniIconGC;         /* used for clipping mini-icons */
  GC whGC, wsGC, ahGC, asGC; /* used for 3d shadows on mini-windows */
  GC label_gc;
  GC balloon_gc;

  int balloon_desk;
  char *balloon_label;   /* the label displayed inside the balloon */
  char  *Hilite;         /* the fvwm window that is highlighted
			  * except for networking delays, this is the
			  * window which REALLY has the focus */
  unsigned VScale;       /* Panner scale factor */
  //int CurrentDesk;
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
  struct fpmonitor *m;
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
  action_flags allowed_actions;
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
  int icon_view_x;
  int icon_view_y;
  int icon_view_width;
  int icon_view_height;

  Window PagerView;
  Window IconView;

  struct pager_window *next;
} PagerWindow;

typedef struct balloon_window
{
  FlocaleFont *Ffont;
  int height;            /* height of balloon window based on font */
  int desk;
} BalloonWindow;

typedef struct desk_info
{
  Window w;
  Window title_w;
  Window CPagerWin;
  FvwmPicture *bgPixmap;                /* Pixmap used as background. */
  BalloonWindow balloon;
  int colorset;
  int highcolorset;
  int ballooncolorset;
  char *Dcolor;
  char *label;
  GC NormalGC;
  GC DashedGC;                  /* used the the pages boundary lines */
  GC HiliteGC;                  /* used for hilighting the active desk */
  GC rvGC;                      /* used for drawing hilighted desk title */
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
  FvwmPicture *bgPixmap;                /* Pixmap used as background. */
} PagerStringList;

/*
 *
 * Subroutine Prototypes
 *
 */
char *GetNextToken(char *indata,char **token);
void Loop(int *fd);
RETSIGTYPE DeadPipe(int nonsense);
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
void list_reply(unsigned long *body);
int My_XNextEvent(Display *dpy, XEvent *event);

/* Stuff in x_pager.c */
void change_colorset(int colorset);
void initialize_pager(void);
void initialize_viz_pager(void);
Pixel GetColor(char *name);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void update_pr_transparent_windows(void);
void MovePage(Bool is_new_desk);
void DrawGrid(int desk,int erase,Window ew,XRectangle *r);
void DrawIconGrid(int erase);
void SwitchToDesk(int Desk);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t, Bool do_force_redraw);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindow(Bool is_new_page, Bool is_new_desk);
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
void MapBalloonWindow(PagerWindow *t, Bool is_icon_view);
void UnmapBalloonWindow(void);
void DrawInBalloonWindow(int i);
void HandleScrollDone(void);
