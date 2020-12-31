/* -*-c-*- */
#ifndef FVWMLIB_FSCRREN_H
#define FVWMLIB_FSCRREN_H

#include "config.h"
#include "fvwm_x11.h"
#include "fvwmrect.h"

typedef struct
{
	XEvent *mouse_ev;
	const char *name;
	struct
	{
		int x;
		int y;
	} xypos;
} fscreen_scr_arg;

typedef enum
{
	FSCREEN_GLOBAL  = -1,
	FSCREEN_CURRENT = -2,
	FSCREEN_PRIMARY = -3,
	FSCREEN_XYPOS   = -4,
	FSCREEN_BY_NAME = -5
} fscreen_scr_t;

typedef struct DesktopsInfo
{
	int desk;
	char *name;

	struct
	{
		int x;
		int y;
		int width;
		int height;
	} ewmh_working_area;
	struct
	{
		int x;
		int y;
		int width;
		int height;
	} ewmh_dyn_working_area;

	struct DesktopsInfo *next;
} DesktopsInfo;

typedef struct
{
	Window win;
	int isMapped;
	/* command which is executed when the pan frame is entered */
	char *command;
	/* command which is executed when the pan frame is left*/
	char *command_leave;
} PanFrame;

enum monitor_tracking
{
	MONITOR_TRACKING_G = 1,
	MONITOR_TRACKING_M,
};

extern enum monitor_tracking monitor_mode;

struct screen_info {
	const char		*name;
	int			 x, y, w, h;
	RROutput		 rr_output;

	TAILQ_ENTRY(screen_info) entry;
};
TAILQ_HEAD(screen_infos, screen_info);

extern struct screen_infos	 screen_info_q;

struct screen_info	*screen_info_new(void);
struct screen_info	*screen_info_by_name(const char *);

#define MONITOR_NEW 0x1
#define MONITOR_DISABLED 0x2
#define MONITOR_ENABLED 0x4
#define MONITOR_PRIMARY 0x8
#define MONITOR_CHANGED 0x10
#define MONITOR_ALL (MONITOR_DISABLED|MONITOR_ENABLED|MONITOR_CHANGED)

struct monitor {
	struct screen_info	*si;
	int			 flags;
	int			 emit;
	int			 dx, dy;

	/* info for some desktops; the first entries should be generic info
         * correct for any desktop not in the list
         */
	DesktopsInfo    *Desktops;

        /* Information about EWMH. */
        struct {
                unsigned NumberOfDesktops;
                unsigned MaxDesktops;
                unsigned CurrentNumberOfDesktops;
                Bool NeedsToCheckDesk;

                struct {
                        int left;
                        int right;
                        int top;
                        int bottom;
                } BaseStrut;

        } ewmhc;

        struct {
                int VxMax;
                int VyMax;
                int Vx;
                int Vy;

                int EdgeScrollX;
                int EdgeScrollY;

                int CurrentDesk;
                int prev_page_x;
                int prev_page_y;
                int prev_desk;
                int prev_desk_and_page_desk;
                int prev_desk_and_page_page_x;
                int prev_desk_and_page_page_y;
		int MyDisplayWidth;
		int MyDisplayHeight;
        } virtual_scr;

	PanFrame PanFrameTop;
	PanFrame PanFrameLeft;
	PanFrame PanFrameRight;
	PanFrame PanFrameBottom;

	TAILQ_ENTRY(monitor) entry;
};
TAILQ_HEAD(monitors, monitor);

extern struct monitors		monitor_q;

struct monitor	*monitor_resolve_name(const char *);
struct monitor	*monitor_by_xy(int, int);
struct monitor  *monitor_by_output(int);
struct monitor  *monitor_by_primary(void);
struct monitor  *monitor_get_current(void);
void		 monitor_init_contents(struct monitor *);
void		 monitor_dump_state(struct monitor *);
void		 monitor_output_change(Display *, XRRScreenChangeNotifyEvent *);
int		 monitor_get_all_widths(void);
int		 monitor_get_all_heights(void);
void		 monitor_add_new(void);
void		 monitor_assign_virtual(struct monitor *);

#define FSCREEN_MANGLE_USPOS_HINTS_MAGIC ((short)-32109)

extern int randr_event;
extern const char *prev_focused_monitor;

/* Control */
Bool FScreenIsEnabled(void);
void FScreenInit(Display *dpy);
void FScreenSelect(Display *dpy);
void FScreenSetPrimaryScreen(int scr);

/* Screen info */
Bool FScreenGetScrRect(fscreen_scr_arg *, fscreen_scr_t,
	int *, int *, int *, int *);
Bool FScreenGetScrId(fscreen_scr_arg *arg);
void FScreenTranslateCoordinates(
	fscreen_scr_arg *arg_src, fscreen_scr_t screen_src,
	fscreen_scr_arg *arg_dest, fscreen_scr_t screen_dest,
	int *x, int *y);
void FScreenGetResistanceRect(
	int wx, int wy, unsigned int ww, unsigned int wh, int *x0, int *y0,
	int *x1, int *y1);
Bool FScreenIsRectangleOnScreen(fscreen_scr_arg *, fscreen_scr_t,rectangle *);
const char	*FScreenOfPointerXY(int, int);
int		 monitor_get_count(void);
struct monitor	*FindScreenOfXY(int, int);

/* Clipping/positioning */
int FScreenClipToScreen(fscreen_scr_arg *, fscreen_scr_t,
	int *x, int *y, int w, int h);
void FScreenCenterOnScreen(fscreen_scr_arg *, fscreen_scr_t,
	int *x, int *y, int w, int h);

/* Geometry management */
int FScreenParseGeometryWithScreen(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	char **screen_return);
int FScreenParseGeometry(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return);
int  FScreenGetGeometry(
	char *parsestring, int *x_return, int *y_return,
	int *width_return, int *height_return, XSizeHints *hints, int flags);
void FScreenMangleScreenIntoUSPosHints(fscreen_scr_t screen, XSizeHints *hints);
int FScreenFetchMangledScreenFromUSPosHints(XSizeHints *hints);

#endif /* FVWMLIB_FSCRREN_H */
