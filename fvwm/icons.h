/* -*-c-*- */

#ifndef FVWM_ICONS_H
#define FVWM_ICONS_H

#include "fvwm.h"

#define ICON_HEIGHT(t) \
	((t)->icon_font->height + 2 * abs((t)->icon_title_relief))

int  get_visible_icon_window_count(FvwmWindow *fw);
void clear_icon(FvwmWindow *fw);
void setup_icon_title_size(FvwmWindow *fw);
void GetIconPicture(FvwmWindow *fw, Bool no_icon_window);
void AutoPlaceIcon(FvwmWindow *t, initial_window_options_t *win_opts,
    Bool do_move_immediately);
void ChangeIconPixmap(FvwmWindow *fw);
void RedoIconName(FvwmWindow *fw);
void DrawIconWindow(FvwmWindow *fw, Bool draw_title, Bool draw_pixmap,
    Bool focus_change, Bool reset_bg, XEvent *pev);
void CreateIconWindow(FvwmWindow *fw, int def_x, int def_y);
void Iconify(FvwmWindow *fw, initial_window_options_t *win_opts);
void DeIconify(FvwmWindow *);
void SetMapStateProp(const FvwmWindow *, int);

#endif /* FVWM_ICONS_H */
