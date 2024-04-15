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
  GC balloon_gc;

  int balloon_desk;
  char *balloon_label;   /* the label displayed inside the balloon */

  unsigned VScale;       /* Panner scale factor */
  Pixmap sticky_gray_pixmap;
  Pixmap light_gray_pixmap;
  Pixmap gray_pixmap;
  Pixel focus_win_fg; /* The fvwm focus pixel. */
  Pixel focus_win_bg;

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

typedef struct desk_style
{
	int desk;
	char *label;

	bool use_label_pixmap;		/* Stretch pixmap over label? */
	int cs;
	int hi_cs;
	int win_cs;
	int focus_cs;
	int balloon_cs;
	Pixel fg;			/* Store colors as pixels. */
	Pixel bg;
	Pixel hi_fg;
	Pixel hi_bg;
	Pixel win_fg;
	Pixel win_bg;
	Pixel focus_fg;
	Pixel focus_bg;
	FvwmPicture *bgPixmap;		/* Pixmap used as background. */
	FvwmPicture *hiPixmap;		/* Hilighted background pixmap. */
	GC label_gc;			/* Label GC. */
	GC dashed_gc;			/* Page boundary lines. */
	GC hi_bg_gc;			/* Hilighting monitor locations. */
	GC hi_fg_gc;			/* Hilighting desk labels. */
	GC win_hi_gc;			/* GCs for 3D borders. */
	GC win_sh_gc;
	GC focus_hi_gc;
	GC focus_sh_gc;

	TAILQ_ENTRY(desk_style) entry;
} DeskStyle;
TAILQ_HEAD(desk_styles, desk_style);
extern struct desk_styles	desk_style_q;

typedef struct desk_info
{
  Window w;
  Window title_w;
  BalloonWindow balloon;
  DeskStyle *style;
  struct fpmonitor *fp;         /* most recent monitor viewing desk. */
} DeskInfo;

/*
 *
 * Shared variables.
 *
 */
/* Colors, Pixmaps, Fonts, etc. */
extern char		*smallFont;
extern char		*ImagePath;
extern char		*font_string;
extern char		*BalloonFore;
extern char		*BalloonBack;
extern char		*BalloonFont;
extern char		*WindowLabelFormat;
extern char		*BalloonTypeString;
extern char		*BalloonBorderColor;
extern char		*BalloonFormatString;
extern Pixel		focus_win_fg;
extern Pixel		focus_win_bg;
extern Pixmap		default_pixmap;
extern FlocaleFont	*Ffont;
extern FlocaleFont	*FwindowFont;
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
extern bool	xneg;
extern bool	yneg;
extern bool	IsShared;
extern bool	icon_xneg;
extern bool	icon_yneg;
extern bool	MiniIcons;
extern bool	Swallowed;
extern bool	usposition;
extern bool	UseSkipList;
extern bool	StartIconic;
extern bool	LabelsBelow;
extern bool	ShapeLabels;
extern bool	is_transient;
extern bool	HilightDesks;
extern bool	ShowBalloons;
extern bool	HilightLabels;
extern bool	error_occured;
extern bool	FocusAfterMove;
extern bool	use_desk_label;
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
DeskStyle *FindDeskStyle(int desk);
void ExitPager(void);
void initialize_colorsets();

/* Stuff in x_pager.c */
void change_colorset(int colorset);
void initialise_common_pager_fragments(void);
void initialize_pager(void);
void initialize_fpmonitor_windows(struct fpmonitor *);
void initialize_viz_pager(void);
Pixel GetSimpleColor(char *name);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void update_pr_transparent_windows(void);
void MovePage();
void draw_desk_grid(int desk);
void draw_icon_grid(int erase);
void SwitchToDesk(int Desk, struct fpmonitor *m);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t, bool do_force_redraw);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindows(bool is_new_page, bool is_new_desk);
void Scroll(int x, int y, int Desk, bool do_scroll_icon);
void MoveWindow(XEvent *Event);
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
struct fpmonitor *fpmonitor_from_desk(int desk);
void initialize_desk_style_gcs(DeskStyle *style);
void update_desk_style_gcs(DeskStyle *style);
void update_desk_background(int desk);
void update_monitor_locations(int desk);
void update_monitor_backgrounds(int desk);
void update_window_background(PagerWindow *t);
void update_window_decor(PagerWindow *t);
void update_pager_window_decor(PagerWindow *t);
void update_icon_window_decor(PagerWindow *t);

#endif /* FVWMPAGER_H */
