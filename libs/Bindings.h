/* -*-c-*- */

#include <X11/Xutil.h>	/* XClassHint */

#ifndef FVWMLIB_BINDINGS_H_H
#define FVWMLIB_BINDINGS_H_H

/* ---------------------------- global definitions ------------------------- */

#define binding_t_t    unsigned char

/* ---------------------------- global macros ------------------------------ */

#define BIND_IS_KEY_PRESS(t) ((t) == BIND_KEYPRESS || (t) == BIND_PKEYPRESS)
#define BIND_IS_KEY_BINDING(t) ((t) == BIND_KEYPRESS || (t) == BIND_PKEYPRESS)
#define BIND_IS_PKEY_BINDING(t) ((t) == BIND_PKEYPRESS)
#define BIND_IS_MOUSE_BINDING(t) \
	((t) == BIND_BUTTONPRESS || (t) == BIND_BUTTONRELEASE)

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	/* press = even number, release = press + 1 */
	BIND_BUTTONPRESS = 0,
	BIND_BUTTONRELEASE = 1,
	BIND_KEYPRESS = 2,
	BIND_PKEYPRESS = 4,
} binding_t;

typedef struct Binding
{
	binding_t_t type;       /* Is it a mouse or key binding */
	int Button_Key;         /* Mouse Button number or Keycode */
	char *key_name;         /* In case of keycode, give the key_name too */
	int Context;            /* Fvwm context, ie titlebar, frame, etc */
	int Modifier;           /* Modifiers for keyboard state */
	void *Action;           /* What to do? */
	void *Action2;          /* This one can be used too */
	char *windowName;	/* Name of window (regex pattern) this binding
				   applies to. NULL means all windows. */
	struct Binding *NextBinding;
} Binding;

/* ---------------------------- interface functions ------------------------ */

void CollectBindingList(
	Display *dpy, Binding **pblist_src, Binding **pblist_dest,
	Bool *ret_are_similar_bindings_left, binding_t type,
	int button, KeySym keysym,
	int modifiers, int contexts, char *windowName);
int AddBinding(
	Display *dpy, Binding **pblist, binding_t type,
	int button, KeySym keysym, char *key_name,
	int modifiers, int contexts, void *action, void *action2,
	char *windowName);
void FreeBindingStruct(Binding *b);
void FreeBindingList(Binding *b);
void RemoveBinding(Binding **pblist, Binding *b, Binding *prev);
Bool RemoveMatchingBinding(
	Display *dpy, Binding **pblist, binding_t type,
	int button, KeySym keysym, int modifiers,
	int contexts);
void *CheckBinding(
	Binding *blist, int button_keycode,
	unsigned int modifier, unsigned int dead_modifiers, int Context,
	binding_t type, const XClassHint *win_class, const char *win_name);
void *CheckTwoBindings(
	Bool *ret_is_second_binding, Binding *blist,
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type, const XClassHint *win_class,
	const char *win_name, int Context2, binding_t type2,
	const XClassHint *win_class2, const char *win_name2);
void GrabWindowKey(
	Display *dpy, Window w, Binding *binding, unsigned int contexts,
	unsigned int dead_modifiers, Bool fGrab);
void GrabAllWindowKeys(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Bool fGrab);
void GrabWindowButton(
	Display *dpy, Window w, Binding *binding, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab);
void GrabAllWindowButtons(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab);
void GrabAllWindowKeysAndButtons(
	Display *dpy, Window w, Binding *blist, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab);
void GrabWindowKeyOrButton(
	Display *dpy, Window w, Binding *binding, unsigned int contexts,
	unsigned int dead_modifiers, Cursor cursor, Bool fGrab);
KeySym FvwmStringToKeysym(Display *dpy, char *key);
Bool bindingAppliesToWindow(Binding *binding, const XClassHint *win_class,
	const char *win_name);
Bool is_pass_through_action(const char *action);


#endif /* FVWMLIB_BINDINGS_H_H */
