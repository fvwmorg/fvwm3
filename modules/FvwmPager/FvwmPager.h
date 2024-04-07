/* -*-c-*- */

#ifndef FVWMPAGER_H
#define FVWMPAGER_H

#include "libs/Picture.h"
#include "libs/vpacket.h"
#include "libs/Flocale.h"
#include "libs/Module.h"
#include "libs/FScreen.h"

#define DEFAULT_PAGER_WINDOW_BORDER_WIDTH 1
#define DEFAULT_PAGER_WINDOW_MIN_SIZE 3
#define DEFAULT_PAGER_MOVE_THRESHOLD 3
#define DEFAULT_BALLOON_BORDER_WIDTH 1
#define DEFAULT_BALLOON_Y_OFFSET 3

struct fpmonitor {
	struct monitor *m;

	Window *CPagerWin;

        struct {
                int VxMax;
                int VyMax;
                int Vx;
                int Vy;
		int VxPages;           /* desktop size */
		int VyPages;
		int VWidth;            /* Size of virtual desktop */
		int VHeight;
        } virtual_scr;

	int scr_width; /* Size of DisplayWidth() */
	int scr_height; /* Size of DisplayHeight() */
	bool disabled;
	TAILQ_ENTRY(fpmonitor) entry;
};
TAILQ_HEAD(fpmonitors, fpmonitor);

extern struct fpmonitors	 fp_monitor_q;
struct fpmonitor		*fpmonitor_by_name(const char *);
struct fpmonitor		*fpmonitor_this(struct monitor *);

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
  struct monitor *m;
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
  Window icon_w;
  Window icon_pixmap_w;
  char *icon_name;
  char *window_name;
  char *res_name;
  char *res_class;
  char *window_label; /* This is displayed inside the mini window */
  FvwmPicture mini_icon;
  rectangle pager_view;
  rectangle icon_view;

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
  FvwmPicture *bgPixmap;                /* Pixmap used as background. */
  BalloonWindow balloon;
  int colorset;
  int highcolorset;
  int ballooncolorset;
  char *Dcolor;
  char *label;
  GC NormalGC;
  GC DashedGC;                  /* used for the pages boundary lines */
  GC HiliteGC;                  /* used for hilighting the active desk */
  GC rvGC;                      /* used for drawing hilighted desk title */
  unsigned long fp_mask;        /* used for the fpmonitor window */
  XSetWindowAttributes fp_attr; /* used for the fpmonitor window */
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
 * Shared variables.
 *
 */
/* Colors, Pixmaps, Fonts, etc. */
extern char		*HilightC;
extern char		*PagerFore;
extern char		*PagerBack;
extern char		*smallFont;
extern char		*ImagePath;
extern char		*WindowBack;
extern char		*WindowFore;
extern char		*font_string;
extern char		*BalloonFore;
extern char		*BalloonBack;
extern char		*BalloonFont;
extern char		*WindowHiFore;
extern char		*WindowHiBack;
extern char		*WindowLabelFormat;
extern char		*BalloonTypeString;
extern char		*BalloonBorderColor;
extern char		*BalloonFormatString;
extern Pixel		hi_pix;
extern Pixel		back_pix;
extern Pixel		fore_pix;
extern Pixel		focus_pix;
extern Pixel		win_back_pix;
extern Pixel		win_fore_pix;
extern Pixel		focus_fore_pix;
extern Pixel		win_hi_back_pix;
extern Pixel		win_hi_fore_pix;
extern Pixmap		default_pixmap;
extern FlocaleFont	*Ffont;
extern FlocaleFont	*FwindowFont;
extern FvwmPicture	*PixmapBack;
extern FvwmPicture	*HilightPixmap;
extern FlocaleWinString	*FwinString;

/* Sizes / Dimensions */
extern int		Rows;
extern int		desk1;
extern int		desk2;
extern int		desk_i;
extern int		ndesks;
extern int		Columns;
extern int		MoveThreshold;
extern int		BalloonBorderWidth;
extern int		BalloonYOffset;
extern unsigned int	WindowBorderWidth;
extern unsigned int	MinSize;
extern rectangle	pwindow;
extern rectangle	icon;

/* Settings */
extern int	windowcolorset;
extern int	activecolorset;
extern bool	xneg;
extern bool	yneg;
extern bool	icon_xneg;
extern bool	icon_yneg;
extern bool	MiniIcons;
extern bool	Swallowed;
extern bool	usposition;
extern bool	UseSkipList;
extern bool	StartIconic;
extern bool	LabelsBelow;
extern bool	ShapeLabels;
extern bool	win_pix_set;
extern bool	is_transient;
extern bool	HilightDesks;
extern bool	ShowBalloons;
extern bool	error_occured;
extern bool	FocusAfterMove;
extern bool	use_desk_label;
extern bool	win_hi_pix_set;
extern bool	WindowBorders3d;
extern bool	HideSmallWindows;
extern bool	ShowIconBalloons;
extern bool	use_no_separators;
extern bool	use_monitor_label;
extern bool	ShowPagerBalloons;
extern bool	do_focus_on_enter;
extern bool	fAlwaysCurrentDesk;
extern bool	CurrentDeskPerMonitor;
extern bool	use_dashed_separators;
extern bool	do_ignore_next_button_release;

/* Screen / Windows */
extern int		fd[2];
extern char		*MyName;
extern Window		icon_win;
extern Window		BalloonView;
extern Display		*dpy;
extern DeskInfo		*Desks;
extern ScreenInfo	Scr;
extern PagerWindow	*Start;
extern PagerWindow	*FocusWin;

/* Monitors */
extern struct fpmonitor		*current_monitor;
extern struct fpmonitor		*monitor_to_track;
extern char			*preferred_monitor;
extern struct fpmonitors	fp_monitor_q;

/*
 *
 * Subroutine Prototypes
 *
 */
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
void ExitPager(void);

/* Stuff in x_pager.c */
void change_colorset(int colorset);
void initialise_common_pager_fragments(void);
void initialize_pager(void);
void initialize_fpmonitor_windows(struct fpmonitor *);
void initialize_viz_pager(void);
Pixel GetColor(char *name);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void update_pr_transparent_windows(void);
void MovePage(bool is_new_desk);
void DrawGrid(int desk,Window ew,XRectangle *r);
void DrawIconGrid(int erase);
void SwitchToDesk(int Desk, struct fpmonitor *m);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t, bool do_force_redraw);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindow(bool is_new_page, bool is_new_desk);
void Hilight(PagerWindow *, int);
void Scroll(int x, int y, int Desk, bool do_scroll_icon);
void MoveWindow(XEvent *Event);
void BorderWindow(PagerWindow *t);
void BorderIconWindow(PagerWindow *t);
void LabelWindow(PagerWindow *t);
void LabelIconWindow(PagerWindow *t);
void PictureWindow(PagerWindow *t);
void PictureIconWindow(PagerWindow *t);
void ReConfigureIcons(bool do_reconfigure_desk_only);
void IconSwitchPage(XEvent *Event);
void IconMoveWindow(XEvent *Event,PagerWindow *t);
void HandleEnterNotify(XEvent *Event);
void HandleExpose(XEvent *Event);
void MapBalloonWindow(PagerWindow *t, bool is_icon_view);
void UnmapBalloonWindow(void);
void DrawInBalloonWindow(int i);
void HandleScrollDone(void);
int fpmonitor_get_all_widths(void);
int fpmonitor_get_all_heights(void);

#endif /* FVWMPAGER_H */
