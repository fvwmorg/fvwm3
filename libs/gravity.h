/* -*-c-*- */

#ifndef GRAVITY_H
#define GRAVITY_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	DIR_NONE = -1,
	DIR_N = 0,
	DIR_E = 1,
	DIR_S = 2,
	DIR_W = 3,
	DIR_MAJOR_MASK = 3,
	DIR_NE = 4,
	DIR_SE = 5,
	DIR_SW = 6,
	DIR_NW = 7,
	DIR_MINOR_MASK = 7,
	DIR_MASK = 7,
	DIR_C = 8,
	DIR_ALL_MASK = 8
} direction_t;

typedef enum
{
	MULTI_DIR_NONE = 0,
	MULTI_DIR_FIRST = (1 << DIR_N),
	MULTI_DIR_N =     (1 << DIR_N),
	MULTI_DIR_E =     (1 << DIR_E),
	MULTI_DIR_S =     (1 << DIR_S),
	MULTI_DIR_W =     (1 << DIR_W),
	MULTI_DIR_NE =    (1 << DIR_NE),
	MULTI_DIR_SE =    (1 << DIR_SE),
	MULTI_DIR_SW =    (1 << DIR_SW),
	MULTI_DIR_NW =    (1 << DIR_NW),
	MULTI_DIR_ALL =  ((1 << (DIR_MASK + 1)) - 1),
	MULTI_DIR_C =     (1 << DIR_C),
	MULTI_DIR_LAST =  (1 << DIR_ALL_MASK)
} multi_direction_t;

typedef enum
{
	ROTATION_0    = 0,
	ROTATION_90   = 1,
	ROTATION_180  = 2,
	ROTATION_270  = 3,
	ROTATION_MASK = 3
} rotation_t;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void gravity_get_offsets(int grav, int *xp,int *yp);
void gravity_move(int gravity, rectangle *rect, int xdiff, int ydiff);
void gravity_resize(int gravity, rectangle *rect, int wdiff, int hdiff);
void gravity_move_resize_parent_child(
	int child_gravity, rectangle *parent_diff_r, rectangle *child_r);
direction_t gravity_grav_to_dir(
	int grav);
int gravity_dir_to_grav(
	direction_t dir);
int gravity_combine_xy_grav(
	int grav_x, int grav_y);
void gravity_split_xy_grav(
	int *ret_grav_x, int *ret_grav_y, int in_grav);
int gravity_combine_xy_dir(
	int dir_x, int dir_y);
void gravity_split_xy_dir(
	int *ret_dir_x, int *ret_dir_y, int in_dir);
int gravity_override_dir(
	int dir_orig, int dir_mod);
int gravity_dir_to_sign_one_axis(
	direction_t dir);
direction_t gravity_parse_dir_argument(
	char *action, char **ret_action, direction_t default_ret);
char *gravity_dir_to_string(direction_t dir, char *default_str);
multi_direction_t gravity_parse_multi_dir_argument(
	char *action, char **ret_action);
void gravity_get_next_multi_dir(int dir_set, multi_direction_t *dir);
direction_t gravity_multi_dir_to_dir(multi_direction_t mdir);
void gravity_rotate_xy(rotation_t rot, int x, int y, int *ret_x, int *ret_y);
rotation_t gravity_add_rotations(rotation_t rot1, rotation_t rot2);

#endif /* GRAVITY_H */
