#ifndef IN_XMANAGER_H
#define IN_XMANAGER_H

#define FONT_STRING "8x13"
#define DEFAULT_BUTTON_WIDTH 200
#define DEFAULT_BUTTON_HEIGHT 17
#define DEFAULT_NUM_COLS  1
#define DEFAULT_NUM_ROWS 0

extern void draw_managers (void);
extern void draw_manager (WinManager *man);

extern int which_box (WinManager *man, int x, int y);
extern Button *xy_to_button (WinManager *man, int x, int y);

extern void delete_windows_button (WinData *win);
extern void resort_windows_button (WinData *win);

extern void size_manager (WinManager *man);
extern void init_button_array (ButtonArray *array);
extern void init_boxes (void);
extern void set_shape (WinManager *man);
extern void draw_added_icon (WinManager *man);
extern void draw_deleted_icon (WinManager *man);
extern void move_highlight (WinManager *man, Button *button);
#ifdef MINI_ICONS
extern void set_win_picture (WinData *win, Pixmap picture, Pixmap mask,
			     unsigned int depth, unsigned int width,
			     unsigned int height);
#endif
extern void set_win_iconified (WinData *win, int iconified);
extern void set_win_state (WinData *win, int state);
extern void add_win_state (WinData *win, int flag);
extern void del_win_state (WinData *win, int flag);
extern void set_win_displaystring (WinData *win);
extern void set_manager_width (WinManager *man, int width);
extern int change_windows_manager (WinData *win);
extern void check_in_window (WinData *win);
extern void set_manager_window_mapping (WinManager *man, int flag);
extern void man_exposed (WinManager *man, XEvent *theEvent);
extern void force_manager_redraw (WinManager *man);

extern Button *button_above (WinManager *man, Button *b);
extern Button *button_below (WinManager *man, Button *b);
extern Button *button_right (WinManager *man, Button *b);
extern Button *button_left (WinManager *man, Button *b);
extern Button *button_next (WinManager *man, Button *b);
extern Button *button_prev (WinManager *man, Button *b);

extern void check_managers_consistency (void);

#endif
