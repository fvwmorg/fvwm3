#ifndef LIB_TARGET_H
#define LIB_TARGET_H

void fvwmlib_keyboard_shortcuts(
	Display *dpy, int screen, XEvent *Event, int x_move_size,
	int y_move_size, int *x_defect, int *y_defect, int ReturnEvent);

void fvwmlib_get_target_window(
	Display *dpy, int screen, char *MyName, Window *app_win,
	Bool return_subwindow);

Window fvwmlib_client_window(Display *dpy, Window input);

#endif /* LIB_TARGET_H */
