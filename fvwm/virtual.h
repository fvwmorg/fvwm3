/* -*-c-*- */

#ifndef FVWM_VIRTUAL_H
#define FVWM_VIRTUAL_H

#include <stdbool.h>
#include "fvwm.h"

void calculate_page_sizes(struct monitor *, int, int);
int HandlePaging(
	XEvent *pev, position warp_size, position *p, position *delta,
	Bool Grab, Bool fLoop,	Bool do_continue_previous, int delay);
void raisePanFrames(void);
void initPanFrames(struct monitor *);
Bool is_pan_frame(Window w);
void MoveViewport(struct monitor *, int newx, int newy,Bool);
void goto_desk(int desk, struct monitor *);
void do_move_window_to_desk(FvwmWindow *fw, int desk);
Bool get_page_arguments(FvwmWindow *, char *action, int *page_x, int *page_y,
    struct monitor **);
char *GetDesktopName(struct monitor *, int desk);

struct desktop_cmd {
	int				 desk;
	char				*name;

	TAILQ_ENTRY(desktop_cmd)	 entry;
};
TAILQ_HEAD(desktop_cmds, desktop_cmd);
extern struct desktop_cmds	 desktop_cmd_q;

struct desk_fvwmwin {
	FvwmWindow			*fw;
	TAILQ_ENTRY(desk_fvwmwin)	 entry;
};
TAILQ_HEAD(desk_fvwmwins, desk_fvwmwin);

struct desktop_fw {
	int			 desk;
	struct desk_fvwmwins	 desk_fvwmwin_q;

	TAILQ_ENTRY(desktop_fw)	  entry;
};
TAILQ_HEAD(desktop_fws, desktop_fw);
extern struct desktop_fws	 desktop_fvwm_q;

bool desk_get_fw_urgent(struct monitor *, int);
int desk_get_fw_count(struct monitor *, int);
void desk_add_fw(FvwmWindow *);
void desk_del_fw(FvwmWindow *);

void apply_desktops_monitor(struct monitor *);

#endif /* FVWM_VIRTUAL_H */
