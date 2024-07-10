/* -*-c-*- */
#ifndef FVWMLIB_FSCRREN_H
#define FVWMLIB_FSCRREN_H

#include "config.h"
#include "fvwm_x11.h"
#include "fvwmrect.h"

#include "queue.h"
#include "tree.h"

#include <X11/extensions/Xrandr.h>
#include <stdbool.h>

typedef struct
{
	XEvent *mouse_ev;
	const char *name;
	position xypos;
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
	rectangle ewmh_working_area;
	rectangle ewmh_dyn_working_area;
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
extern bool is_tracking_shared;

struct screen_info {
	const char		*name;
	int			 x, y, w, h;
	RROutput		 rr_output;

	TAILQ_ENTRY(screen_info) entry;
};
TAILQ_HEAD(screen_infos, screen_info);

extern struct screen_infos	 screen_info_q, screen_info_q_temp;

struct screen_info	*screen_info_new(void);
struct screen_info	*screen_info_by_name(const char *);

#define MONITOR_NEW 0x1
#define MONITOR_DISABLED 0x2
#define MONITOR_ENABLED 0x4
#define MONITOR_PRIMARY 0x8
#define MONITOR_CHANGED 0x100
#define MONITOR_FOUND 0x200

#define MONITOR_OUTSIDE_EDGE 0
#define MONITOR_INSIDE_EDGE 1

struct monitor {
	struct screen_info	*si;
	int			 flags;
	int			 emit;
	int			 number;
	int			 dx, dy;
	bool			 is_prev, was_primary;

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
		bool top;
		bool bottom;
		bool left;
		bool right;
	} edge;

        struct {
                int VxMax;
                int VyMax;
                int Vx;
                int Vy;

		int edge_thickness;
		int last_edge_thickness;

                int EdgeScrollX;
                int EdgeScrollY;

                int CurrentDesk;
                int prev_page_x;
                int prev_page_y;
                int prev_desk;
                int prev_desk_and_page_desk;
                int prev_desk_and_page_page_x;
                int prev_desk_and_page_page_y;

		bool is_swapping;
        } virtual_scr;

	PanFrame PanFrameTop;
	PanFrame PanFrameLeft;
	PanFrame PanFrameRight;
	PanFrame PanFrameBottom;
	bool pan_frames_mapped;

	RB_ENTRY(monitor) entry;
	TAILQ_ENTRY(monitor) oentry;
};
RB_HEAD(monitors, monitor);
TAILQ_HEAD(monitorsold, monitor);

extern struct monitors  monitors;
extern struct monitors	monitor_q;
extern struct monitorsold  monitorsold_q;
int			monitor_compare(struct monitor *, struct monitor *);
RB_PROTOTYPE(monitors, monitor, entry, monitor_compare);

int		 changed_monitor_count(void);
struct monitor	*monitor_resolve_name(const char *);
struct monitor	*monitor_by_xy(int, int);
struct monitor  *monitor_by_output(int);
struct monitor	*monitor_by_number(int);
struct monitor  *monitor_by_primary(void);
struct monitor  *monitor_by_last_primary(void);
struct monitor  *monitor_get_current(void);
struct monitor  *monitor_get_prev(void);
struct monitor  *monitor_get_global(void);
void		 monitor_init_contents(struct monitor *);
void		 monitor_dump_state(struct monitor *);
void		 monitor_output_change(Display *, XRRScreenChangeNotifyEvent *);
int		 monitor_get_all_widths(void);
int		 monitor_get_all_heights(void);
void		 monitor_assign_virtual(struct monitor *);
void		 monitor_refresh_module(Display *);
void		 checkPanFrames(struct monitor *);

#define FSCREEN_MANGLE_USPOS_HINTS_MAGIC ((short)-32109)

extern int randr_event;
extern const char *prev_focused_monitor;

/* Control */
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
