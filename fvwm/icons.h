/* -*-c-*- */

#ifndef _ICONS_
#define _ICONS_

#ifdef NO_ICONS
#define ICON_HEIGHT(t) 1
#else
#define ICON_HEIGHT(t) \
	((t)->icon_font->height + 2*abs((t)->icon_title_relief))
#endif

int get_visible_icon_window_count(FvwmWindow *fw);
void clear_icon(FvwmWindow *fw);
void setup_icon_title_size(FvwmWindow *fw);
void GetIconPicture(FvwmWindow *fw, Bool no_icon_window);
void AutoPlaceIcon(
	FvwmWindow *t, initial_window_options_t *win_opts,
	Bool do_move_immediately);
void ChangeIconPixmap(FvwmWindow *fw);
void RedoIconName(FvwmWindow *fw);
void DrawIconWindow(
	FvwmWindow *fw, Bool draw_title, Bool draw_pixmap, Bool focus_change,
	Bool reset_bg, XEvent *pev);
void CreateIconWindow(FvwmWindow *fw, int def_x, int def_y);
void Iconify(FvwmWindow *fw, initial_window_options_t *win_opts);
void DeIconify(FvwmWindow *);
void SetMapStateProp(const FvwmWindow *, int);


#endif /* _ICONS_ */
