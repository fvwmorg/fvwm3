/* -*-c-*- */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#define CS_ROUND_UP          0x01
#define CS_UPDATE_MAX_DEFECT 0x02

void gravity_get_naked_geometry(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void gravity_add_decoration(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void get_relative_geometry(rectangle *rel_g, rectangle *abs_g);
void get_absolute_geometry(rectangle *abs_g, rectangle *rel_g);
void gravity_translate_to_northwest_geometry(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void gravity_translate_to_northwest_geometry_no_bw(
	int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void get_title_geometry(
	FvwmWindow *fw, rectangle *ret_g);
int get_title_gravity(
	FvwmWindow *fw);
void get_title_gravity_factors(
	FvwmWindow *fw, int *ret_fx, int *ret_fy);
Bool get_title_button_geometry(
	FvwmWindow *fw, rectangle *ret_g, int context);
void get_title_font_size_and_offset(
	FvwmWindow *fw, direction_t title_dir,
	Bool is_left_title_rotated_cw, Bool is_right_title_rotated_cw,
	Bool is_top_title_rotated, Bool is_bottom_title_rotated,
	int *size, int *offset);
void get_icon_corner(
	FvwmWindow *fw, rectangle *ret_g);
void get_shaded_geometry(
	FvwmWindow *fw, rectangle *small_g, rectangle *big_g);
void get_shaded_geometry_with_dir(
	FvwmWindow *fw, rectangle *small_g, rectangle *big_g,
	direction_t shade_dir);
void get_unshaded_geometry(
	FvwmWindow *fw, rectangle *ret_g);
void get_shaded_client_window_pos(
	FvwmWindow *fw, rectangle *ret_g);
void get_client_geometry(
	FvwmWindow *fw, rectangle *ret_g);
void get_window_borders(
	const FvwmWindow *fw, size_borders *borders);
void get_window_borders_no_title(
	const FvwmWindow *fw, size_borders *borders);
void set_window_border_size(
	FvwmWindow *fw, short used_width);
Bool is_window_border_minimal(
	FvwmWindow *fw);
void update_relative_geometry(FvwmWindow *fw);
void update_absolute_geometry(FvwmWindow *fw);
void maximize_adjust_offset(FvwmWindow *fw);
void constrain_size(
	FvwmWindow *fw, const XEvent *e, unsigned int *widthp,
	unsigned int *heightp, int xmotion, int ymotion, int flags);
void gravity_constrain_size(
	int gravity, FvwmWindow *t, rectangle *rect, int flags);
Bool get_visible_window_or_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g);
Bool get_visible_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g);
void get_icon_geometry(
	FvwmWindow *fw, rectangle *ret_g);
Bool get_visible_icon_title_geometry(
	FvwmWindow *fw, rectangle *ret_g);
Bool get_icon_title_geometry(
	FvwmWindow *fw, rectangle *ret_g);
Bool get_visible_icon_picture_geometry(
	FvwmWindow *fw, rectangle *ret_g);
Bool get_icon_picture_geometry(
	FvwmWindow *fw, rectangle *ret_g);
void broadcast_icon_geometry(FvwmWindow *fw, Bool do_force);
void move_icon_to_position(FvwmWindow *fw);
void modify_icon_position(FvwmWindow *fw, int dx, int dy);
void set_icon_position(FvwmWindow *fw, int x, int y);
void set_icon_picture_size(FvwmWindow *fw, int w, int h);
void resize_icon_title_height(FvwmWindow *fw, int dh);
void get_page_offset_rectangle(
	int *ret_page_x, int *ret_page_y, rectangle *r);
void get_page_offset(
	int *ret_page_x, int *ret_page_y, FvwmWindow *fw);
void get_page_offset_check_visible(
	int *ret_page_x, int *ret_page_y, FvwmWindow *fw);

#endif
