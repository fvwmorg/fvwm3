/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ICONS_
#define _ICONS_

#ifdef NO_ICONS
#define ICON_HEIGHT(t) 1
#else
#define ICON_HEIGHT(t) ((t)->icon_font->height + 6)
#endif

int get_visible_icon_window_count(FvwmWindow *fw);
void clear_icon(FvwmWindow *fw);
void GetIconPicture(FvwmWindow *fw, Bool no_icon_window);
void AutoPlaceIcon(
	FvwmWindow *t, initial_window_options_type *win_opts,
	Bool do_move_immediately);
void ChangeIconPixmap(FvwmWindow *fw);
void RedoIconName(FvwmWindow *);
void DrawIconWindow(FvwmWindow *fw, XEvent *pev);
void SetIconPixmapSize(
	Pixmap *icon, unsigned int width, unsigned int height,
	unsigned int depth, unsigned int newWidth, unsigned int newHeight,
	unsigned int freeOldPixmap);
void CreateIconWindow(FvwmWindow *fw, int def_x, int def_y);
void Iconify(FvwmWindow *fw, initial_window_options_type *win_opts);
void DeIconify(FvwmWindow *);
void SetMapStateProp(const FvwmWindow *, int);


#endif /* _ICONS_ */
