/* -*-c-*- */

#ifndef FOCUS_POLICY_H
#define FOCUS_POLICY_H

/* Note: focus_policy.[ch] is meant to manage structures of type focus_policy_t
 * only.  No code dealing with *any* external data types belongs in here!  Put
 * it in focus.[ch] instead. */

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

#define FPOL_ANY_MODIFIER 0xff

/* ---------------------------- access macros ------------------------------ */

#define FP_DO_RAISE_FOCUSED_CLICK(fp) \
	((fp).do_raise_focused_click)
#define FP_DO_RAISE_UNFOCUSED_CLICK(fp) \
	((fp).do_raise_unfocused_click)
#define FP_DO_FOCUS_CLICK(fp) \
	((fp).do_focus_click)
#define FP_DO_RAISE_FOCUSED_CLIENT_CLICK(fp) \
	((fp).do_raise_focused_click.client)
#define FPS_RAISE_FOCUSED_CLIENT_CLICK(fp,x) \
	((fp).do_raise_focused_click.client = !!(x))
#define FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(fp) \
	((fp).do_raise_unfocused_click.client)
#define FPS_RAISE_UNFOCUSED_CLIENT_CLICK(fp,x) \
	((fp).do_raise_unfocused_click.client = !!(x))
#define FP_DO_RAISE_FOCUSED_DECOR_CLICK(fp) \
	((fp).do_raise_focused_click.decor)
#define FPS_RAISE_FOCUSED_DECOR_CLICK(fp,x) \
	((fp).do_raise_focused_click.decor = !!(x))
#define FP_DO_RAISE_UNFOCUSED_DECOR_CLICK(fp) \
	((fp).do_raise_unfocused_click.decor)
#define FPS_RAISE_UNFOCUSED_DECOR_CLICK(fp,x) \
	((fp).do_raise_unfocused_click.decor = !!(x))
#define FP_DO_RAISE_FOCUSED_ICON_CLICK(fp) \
	((fp).do_raise_focused_click.icon)
#define FPS_RAISE_FOCUSED_ICON_CLICK(fp,x) \
	((fp).do_raise_focused_click.icon = !!(x))
#define FP_DO_RAISE_UNFOCUSED_ICON_CLICK(fp) \
	((fp).do_raise_unfocused_click.icon)
#define FPS_RAISE_UNFOCUSED_ICON_CLICK(fp,x) \
	((fp).do_raise_unfocused_click.icon = !!(x))
#define FP_DO_FOCUS_ENTER(fp) \
	((fp).do_focus_enter)
#define FPS_FOCUS_ENTER(fp,x) \
	((fp).do_focus_enter = !!(x))
#define FP_DO_UNFOCUS_LEAVE(fp) \
	((fp).do_unfocus_leave)
#define FPS_UNFOCUS_LEAVE(fp,x) \
	((fp).do_unfocus_leave = !!(x))
#define FP_DO_FOCUS_CLICK_CLIENT(fp) \
	((fp).do_focus_click.client)
#define FPS_FOCUS_CLICK_CLIENT(fp,x) \
	((fp).do_focus_click.client = !!(x))
#define FP_DO_FOCUS_CLICK_DECOR(fp) \
	((fp).do_focus_click.decor)
#define FPS_FOCUS_CLICK_DECOR(fp,x) \
	((fp).do_focus_click.decor = !!(x))
#define FP_DO_FOCUS_CLICK_ICON(fp) \
	((fp).do_focus_click.icon)
#define FPS_FOCUS_CLICK_ICON(fp,x) \
	((fp).do_focus_click.icon = !!(x))
#define FP_DO_FOCUS_BY_PROGRAM(fp) \
	((fp).do_focus_by_program)
#define FPS_FOCUS_BY_PROGRAM(fp,x) \
	((fp).do_focus_by_program = !!(x))
#define FP_DO_FOCUS_BY_FUNCTION(fp) \
	((fp).do_focus_by_function)
#define FPS_FOCUS_BY_FUNCTION(fp,x) \
	((fp).do_focus_by_function = !!(x))
#define FP_DO_WARP_POINTER_ON_FOCUS_FUNC(fp) \
	((fp).do_warp_pointer_on_focus_func)
#define FPS_WARP_POINTER_ON_FOCUS_FUNC(fp,x) \
	((fp).do_warp_pointer_on_focus_func = !!(x))
#define FP_IS_LENIENT(fp) \
	((fp).is_lenient)
#define FPS_LENIENT(fp,x) \
	((fp).is_lenient = !!(x))
#define FP_USE_MOUSE_BUTTONS(fp) \
	((fp).use_mouse_buttons)
#define FPS_MOUSE_BUTTONS(fp,x) \
	((fp).use_mouse_buttons = (x))
#define FP_USE_MODIFIERS(fp) \
	((fp).use_modifiers)
#define FPS_MODIFIERS(fp,x) \
	((fp).use_modifiers = (x))
#define FP_DO_PASS_FOCUS_CLICK(fp) \
	((fp).do_pass_focus_click)
#define FPS_PASS_FOCUS_CLICK(fp,x) \
	((fp).do_pass_focus_click = !!(x))
#define FP_DO_PASS_RAISE_CLICK(fp) \
	((fp).do_pass_raise_click)
#define FPS_PASS_RAISE_CLICK(fp,x) \
	((fp).do_pass_raise_click = !!(x))
#define FP_DO_IGNORE_FOCUS_CLICK_MOTION(fp) \
	((fp).do_ignore_focus_click_motion)
#define FPS_IGNORE_FOCUS_CLICK_MOTION(fp,x) \
	((fp).do_ignore_focus_click_motion = !!(x))
#define FP_DO_IGNORE_RAISE_CLICK_MOTION(fp) \
	((fp).do_ignore_raise_click_motion)
#define FPS_IGNORE_RAISE_CLICK_MOTION(fp,x) \
	((fp).do_ignore_raise_click_motion = !!(x))
#define FP_DO_ALLOW_FUNC_FOCUS_CLICK(fp) \
	((fp).do_allow_func_focus_click)
#define FPS_ALLOW_FUNC_FOCUS_CLICK(fp,x) \
	((fp).do_allow_func_focus_click = !!(x))
#define FP_DO_ALLOW_FUNC_RAISE_CLICK(fp) \
	((fp).do_allow_func_raise_click)
#define FPS_ALLOW_FUNC_RAISE_CLICK(fp,x) \
	((fp).do_allow_func_raise_click = !!(x))
#define FP_DO_GRAB_FOCUS(fp) \
	((fp).do_open_grabs_focus)
#define FPS_GRAB_FOCUS(fp,x) \
	((fp).do_open_grabs_focus = !!(x))
#define FP_DO_GRAB_FOCUS_TRANSIENT(fp) \
	((fp).do_open_grabs_focus_transient)
#define FPS_GRAB_FOCUS_TRANSIENT(fp,x) \
	((fp).do_open_grabs_focus_transient = !!(x))
#define FP_DO_OVERRIDE_GRAB_FOCUS(fp) \
	((fp).do_override_grab_focus)
#define FPS_OVERRIDE_GRAB_FOCUS(fp,x) \
	((fp).do_override_grab_focus = !!(x))
#define FP_DO_RELEASE_FOCUS(fp) \
	((fp).do_close_releases_focus)
#define FPS_RELEASE_FOCUS(fp,x) \
	((fp).do_close_releases_focus = !!(x))
#define FP_DO_RELEASE_FOCUS_TRANSIENT(fp) \
	((fp).do_close_releases_focus_transient)
#define FPS_RELEASE_FOCUS_TRANSIENT(fp,x) \
	((fp).do_close_releases_focus_transient = !!(x))
#define FP_DO_OVERRIDE_RELEASE_FOCUS(fp) \
	((fp).do_override_release_focus)
#define FPS_OVERRIDE_RELEASE_FOCUS(fp,x) \
	((fp).do_override_release_focus = !!(x))
#define FP_DO_SORT_WINDOWLIST_BY(fp) \
	((fp).do_sort_windowlist_by)
#define FPS_SORT_WINDOWLIST_BY(fp,x) \
	((fp).do_sort_windowlist_by = !!(x))

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	FPOL_SORT_WL_BY_FOCUS = 0,
	FPOL_SORT_WL_BY_OPEN = 1
} fpol_sort_windowlist_t;

typedef enum
{
	FOCUS_SET_BY_CLICK_CLIENT,
	FOCUS_SET_BY_CLICK_DECOR,
	FOCUS_SET_BY_CLICK_ICON,
	FOCUS_SET_BY_ENTER,
	FOCUS_SET_BY_PROGRAM,
	FOCUS_SET_BY_FUNCTION,
	/* used internally to restore the focus after certain actions */
	FOCUS_SET_FORCE
} fpol_set_focus_by_t;

typedef struct
{
	unsigned client : 1;
	unsigned decor : 1;
	unsigned icon : 1;
} fpol_context_t;

typedef struct
{
	/* raising the window */
	fpol_context_t do_raise_focused_click;
	fpol_context_t do_raise_unfocused_click;
	/* focus transition */
	fpol_context_t do_focus_click;
	unsigned do_focus_enter : 1;
	unsigned do_unfocus_leave : 1;
	unsigned do_focus_by_program : 1;
	unsigned do_focus_by_function : 1;
	unsigned do_warp_pointer_on_focus_func : 1;
	/* application focus model */
	unsigned is_lenient : 1;
	/* click configuration */
	unsigned use_mouse_buttons : NUMBER_OF_EXTENDED_MOUSE_BUTTONS;
	unsigned use_modifiers : 8;
	/* recycling of focus and raise clicks */
	unsigned do_pass_focus_click : 1;
	unsigned do_pass_raise_click : 1;
	unsigned do_ignore_focus_click_motion : 1;
	unsigned do_ignore_raise_click_motion : 1;
	unsigned do_allow_func_focus_click : 1;
	unsigned do_allow_func_raise_click : 1;
	/* keeping track of the focus */
	unsigned do_open_grabs_focus : 1;
	unsigned do_open_grabs_focus_transient : 1;
	unsigned do_override_grab_focus : 1;
	unsigned do_close_releases_focus : 1;
	unsigned do_close_releases_focus_transient : 1;
	unsigned do_override_release_focus : 1;
	unsigned do_sort_windowlist_by : 1;
} focus_policy_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void fpol_init_default_fp(
	focus_policy_t *fp);
int fpol_query_allow_set_focus(
	focus_policy_t *fpol, fpol_set_focus_by_t set_by_mode);
int fpol_query_allow_user_focus(
	focus_policy_t *fpol);
int fpol_is_policy_changed(
	focus_policy_t *fpol);

#endif /* FOCUS_POLICY_H */
