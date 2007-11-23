/* -*-c-*- */

#ifndef MENU_BINDINGS_H
#define MENU_BINDINGS_H

/* ---------------------------- included header files ---------------------- */

/* Do not #include any files - the file including this file has to take care of
 * it. */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- forward declarations ----------------------- */

struct MenuRoot;
struct MenuParameters;
struct MenuReturn;
struct MenuItem;

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	unsigned int keystate;
	unsigned int keycode;
	Time timestamp;
} double_keypress;

typedef enum
{
	SA_NONE = 0,
	SA_ENTER,
	SA_LEAVE,
	SA_MOVE_ITEMS,
	SA_FIRST,
	SA_LAST,
	SA_CONTINUE,
	SA_WARPBACK,
	SA_SELECT,
	SA_TEAROFF,
	SA_ABORT,
	SA_SCROLL
} menu_shortcut_action;

/* ---------------------------- exported variables (globals) --------------- */

/* Do not use global variable.  Full stop. */

/* ---------------------------- interface functions ------------------------ */

/* Before this function is called, all menu bindings created through
 * menu_binding() are permanent i.e. they can not be deleted (although
 * overridden).  After calling it, new bindings are stored in the regular list
 * and can be deleted by the user as usual.
 *
 * To be called by SetRCDefaults *only*. */
void menu_bindings_startup_complete(void);

/* Parse a menu binding and store it.
 *
 * To be called from bindings.c *only*. */
int menu_binding(
	Display *dpy, binding_t type, int button, KeySym keysym,
	int context, int modifier, char *action, char *menu_style);

/* Checks if the given mouse or keyboard event in the given context
 * corresponds to a menu binding.  If so, the binding is returned.  Otherwise
 * NULL is returned.
 *
 * To be called from menus.c *only*.
 */
Binding *menu_binding_is_mouse(XEvent* event, int context);
Binding *menu_binding_is_key(XEvent* event, int context);

/* Menu keyboard processing
 *
 * Function called instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 * TKP - uses XLookupString so that keypad numbers work with windowlist
 */
void menu_shortcuts(
	struct MenuRoot *mr, struct MenuParameters *pmp,
	struct MenuReturn *pmret, XEvent *event, struct MenuItem **pmi_current,
	double_keypress *pdkp, int *ret_menu_x, int *ret_menu_y);

#endif /* MENU_BINDINGS_H */
