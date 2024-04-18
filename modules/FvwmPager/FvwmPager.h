/* -*-c-*- */

#ifndef FVWMPAGER_H
#define FVWMPAGER_H

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

typedef struct ScreenInfo
{
  unsigned long screen;

  Window Root;
  Window pager_w;
  Window icon_w;
  Window label_w;
  Window balloon_w;

  Font PagerFont;        /* font struct for window labels in pager (optional)*/

  GC NormalGC;           /* used for window names and setting backgrounds */
  GC MiniIconGC;         /* used for clipping mini-icons */

  unsigned VScale;       /* Panner scale factor */
  Pixmap sticky_gray_pixmap;
  Pixmap light_gray_pixmap;
  Pixmap gray_pixmap;
  Pixel focus_win_fg; /* The fvwm focus pixel. */
  Pixel focus_win_bg;

} ScreenInfo;

/* Store the balloon settings and current state. */
struct balloon_data
{
	/* Current state and toggles. */
	int cs;
	bool is_mapped;
	bool is_icon;
	bool show;

	/* Configuration */
	bool show_in_pager;
	bool show_in_icon;
	int border_width;
	int y_offset;
	char *label_format;

	/* Resources */
	char *label;
	GC gc;
	FlocaleFont *Ffont;
};

typedef struct pager_window
{
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
	Pixel balloon_fg;
	Pixel balloon_bg;
	Pixel balloon_border;
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
  DeskStyle *style;
  struct fpmonitor *fp;         /* most recent monitor viewing desk. */
} DeskInfo;

/*
 * Shared variables.
 */
/* Colors, Pixmaps, Fonts, etc. */
extern char		*smallFont;
extern char		*ImagePath;
extern char		*font_string;
extern char		*BalloonFont;
extern char		*WindowLabelFormat;
extern Pixel		focus_win_fg;
extern Pixel		focus_win_bg;
extern Pixmap		default_pixmap;
extern FlocaleFont	*Ffont;
extern FlocaleFont	*FwindowFont;
extern FlocaleWinString	*FwinString;
extern Atom		wm_del_win;

/* Sizes / Dimensions */
extern int		Rows;
extern int		desk1;
extern int		desk2;
extern int		desk_i;
extern int		ndesks;
extern int		desk_w;
extern int		desk_h;
extern int		label_h;
extern int		Columns;
extern int		MoveThreshold;
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
extern bool	HilightLabels;
extern bool	error_occured;
extern bool	FocusAfterMove;
extern bool	use_desk_label;
extern bool	WindowBorders3d;
extern bool	HideSmallWindows;
extern bool	use_no_separators;
extern bool	use_monitor_label;
extern bool	do_focus_on_enter;
extern bool	fAlwaysCurrentDesk;
extern bool	CurrentDeskPerMonitor;
extern bool	use_dashed_separators;
extern bool	do_ignore_next_button_release;

/* Screen / Windows */
extern int			fd[2];
extern char			*MyName;
extern Display			*dpy;
extern DeskInfo			*Desks;
extern ScreenInfo		Scr;
extern PagerWindow		*Start;
extern PagerWindow		*FocusWin;
extern struct balloon_data	Balloon;

/* Monitors */
extern struct fpmonitor		*current_monitor;
extern struct fpmonitor		*monitor_to_track;
extern char			*preferred_monitor;
extern struct fpmonitors	fp_monitor_q;

/*
 * Shared methods prototypes
 */
/* FvwmPager.c methods */
DeskStyle *FindDeskStyle(int desk);
void ExitPager(void);

/* x_pager.c methods */
void HandleScrollDone(void);
rectangle set_vp_size_and_loc(struct fpmonitor *, bool is_icon);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void MovePage();
void draw_desk_grid(int desk);
void AddNewWindow(PagerWindow *prev);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveResizePagerView(PagerWindow *t, bool do_force_redraw);
void MoveStickyWindows(bool is_new_page, bool is_new_desk);
void MapBalloonWindow(PagerWindow *t, bool is_icon_view);
char *get_label(const PagerWindow *pw, const char *fmt);
void UnmapBalloonWindow(void);

/* x_update.c methods */
void update_monitor_locations(int desk);
void update_monitor_backgrounds(int desk);
void update_desk_background(int desk);
void update_window_background(PagerWindow *t);
void update_pr_transparent_windows(void);
void update_desk_style_gcs(DeskStyle *style);
void update_pager_window_decor(PagerWindow *t);
void update_icon_window_decor(PagerWindow *t);
void update_window_decor(PagerWindow *t);

/* fpmonitor.c methods */
struct fpmonitor	*fpmonitor_new(struct monitor *);
struct fpmonitor	*fpmonitor_this(struct monitor *);
struct fpmonitor	*fpmonitor_by_name(const char *);
struct fpmonitor	*fpmonitor_from_desk(int desk);
struct fpmonitor	*fpmonitor_from_xy(int x, int y);
struct fpmonitor	*fpmonitor_from_n(int n);
void			fpmonitor_disable(struct fpmonitor *);
int			fpmonitor_get_all_widths(void);
int			fpmonitor_get_all_heights(void);
int			fpmonitor_count(void);

/* messages.c methods */
void process_message(FvwmPacket *packet);
void set_desk_label(int desk, const char *label);
void set_desk_size(bool update_label);
void parse_monitor_line(char *tline);
void parse_desktop_size_line(char *tline);
void parse_desktop_configuration_line(char *tline);
void update_monitor_to_track(struct fpmonitor **fp_track,
			     char *name, bool update_prefered);

/* init_pager.c methods */
void init_fvwm_pager(void);
void initialize_colorset(DeskStyle *style);
void initialize_fpmonitor_windows(struct fpmonitor *fp);
void initialize_desk_style_gcs(DeskStyle *style);

#endif /* FVWMPAGER_H */
