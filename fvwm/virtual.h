/* -*-c-*- */

#ifndef _VIRTUAL_
#define _VIRTUAL_

int HandlePaging(
	XEvent *pev, int HorWarpSize, int VertWarpSize, int *xl, int *yt,
	int *delta_x, int *delta_y, Bool Grab, Bool fLoop,
	Bool do_continue_previous);
void checkPanFrames(void);
void raisePanFrames(void);
void initPanFrames(void);
Bool is_pan_frame(Window w);
void MoveViewport(int newx, int newy,Bool);
void goto_desk(int desk);
void do_move_window_to_desk(FvwmWindow *fw, int desk);
Bool get_page_arguments(char *action, int *page_x, int *page_y);
char *GetDesktopName(int desk);

#endif /* _VIRTUAL_ */
