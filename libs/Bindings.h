/* -*-c-*- */

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
#define BIND_IS_STROKE_BINDING(t) ((t) == BIND_STROKE)

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	/* press = even number, release = press + 1 */
	BIND_BUTTONPRESS = 0,
	BIND_BUTTONRELEASE = 1,
	BIND_KEYPRESS = 2,
	BIND_PKEYPRESS = 4,
	BIND_STROKE = 6
} binding_t;

typedef struct Binding
{
	binding_t_t type;       /* Is it a mouse, key, or stroke binding */
	STROKE_CODE(void *Stroke_Seq;) /* stroke sequence */
		int Button_Key;         /* Mouse Button number or Keycode */
	char *key_name;         /* In case of keycode, give the key_name too */
	int Context;            /* Fvwm context, ie titlebar, frame, etc */
	int Modifier;           /* Modifiers for keyboard state */
	void *Action;           /* What to do? */
	void *Action2;          /* This one can be used too */
	struct Binding *NextBinding;
} Binding;

/* ---------------------------- interface functions ------------------------ */

void CollectBindingList(
	Display *dpy, Binding **pblist_src, Binding **pblist_dest,
	binding_t type, STROKE_ARG(void *stroke) int button, KeySym keysym,
	int modifiers, int contexts);
int AddBinding(
	Display *dpy, Binding **pblist, binding_t type,
	STROKE_ARG(void *stroke) int button, KeySym keysym, char *key_name,
	int modifiers, int contexts, void *action, void *action2);
void FreeBindingStruct(Binding *b);
void FreeBindingList(Binding *b);
void RemoveBinding(Binding **pblist, Binding *b, Binding *prev);
Bool RemoveMatchingBinding(
	Display *dpy, Binding **pblist, binding_t type,
	STROKE_ARG(char *stroke) int button, KeySym keysym, int modifiers,
	int contexts);
void *CheckBinding(
	Binding *blist, STROKE_ARG(char *stroke) int button_keycode,
	unsigned int modifier, unsigned int dead_modifiers, int Context,
	binding_t type);
void *CheckTwoBindings(
	Bool *ret_is_second_binding, Binding *blist, STROKE_ARG(char *stroke)
	int button_keycode, unsigned int modifier,unsigned int dead_modifiers,
	int Context, binding_t type, int Context2, binding_t type2);
Bool MatchBindingExactly(
	Binding *b, STROKE_ARG(void *stroke) int button, KeyCode keycode,
	unsigned int modifier, int Context, binding_t type);
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

#endif /* FVWMLIB_BINDINGS_H_H */
