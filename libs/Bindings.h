/* -*-c-*- */
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

#ifndef FVWMLIB_BINDINGS_H_H
#define FVWMLIB_BINDINGS_H_H

/* ---------------------------- global definitions -------------------------- */

#define binding_t_t    unsigned char

/* contexts for button presses */
#define C_NO_CONTEXT        0x00
#define C_WINDOW            0x01
#define C_TITLE             0x02
#define C_ICON              0x04
#define C_ROOT              0x08
/*                          0x10*/
/*                          0x20*/
#define C_L1                0x40
#define C_R1                0x80
#define C_L2               0x100
#define C_R2               0x200
#define C_L3               0x400
#define C_R3               0x800
#define C_L4              0x1000
#define C_R4              0x2000
#define C_L5              0x4000
#define C_R5              0x8000
#define C_UNMANAGED      0x10000
#define C_EWMH_DESKTOP   0x20000
#define C_F_TOPLEFT     0x100000
#define C_F_TOPRIGHT    0x200000
#define C_F_BOTTOMLEFT  0x400000
#define C_F_BOTTOMRIGHT 0x800000
#define C_SB_LEFT      0x1000000
#define C_SB_RIGHT     0x2000000
#define C_SB_TOP       0x4000000
#define C_SB_BOTTOM    0x8000000
#define C_FRAME       (C_F_TOPLEFT|C_F_TOPRIGHT|C_F_BOTTOMLEFT|C_F_BOTTOMRIGHT)
#define C_SIDEBAR     (C_SB_LEFT|C_SB_RIGHT|C_SB_TOP|C_SB_BOTTOM)
#define C_RALL        (C_R1|C_R2|C_R3|C_R4|C_R5)
#define C_LALL        (C_L1|C_L2|C_L3|C_L4|C_L5)
#define C_DECOR       (C_LALL|C_RALL|C_TITLE|C_FRAME|C_SIDEBAR)
#define C_ALL         (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|\
		       C_LALL|C_RALL|C_EWMH_DESKTOP)

#define ALL_MODIFIERS (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|\
		       Mod3Mask|Mod4Mask|Mod5Mask)

/* ---------------------------- global macros ------------------------------- */

#define BIND_IS_KEY_PRESS(t) ((t) == BIND_KEYPRESS || (t) == BIND_PKEYPRESS)
#define BIND_IS_KEY_RELEASE(t) \
	((t) == BIND_KEYRELEASE || (t) == BIND_PKEYRELEASE)
#define BIND_IS_KEY_BINDING(t) \
	((t) == BIND_KEYPRESS || (t) == BIND_PKEYPRESS || \
	 (t) == BIND_KEYRELEASE || (t) == BIND_PKEYRELEASE)
#define BIND_IS_PKEY_BINDING(t) \
	((t) == BIND_PKEYPRESS || (t) == BIND_PKEYRELEASE)
#define BIND_IS_MOUSE_BINDING(t) \
	((t) == BIND_BUTTONPRESS || (t) == BIND_BUTTONRELEASE)
#define BIND_IS_STROKE_BINDING(t) ((t) == BIND_STROKE)

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	/* press = even number, release = press + 1 */
	BIND_BUTTONPRESS = 0,
	BIND_BUTTONRELEASE = 1,
	BIND_KEYPRESS = 2,
	BIND_KEYRELEASE = 3,
	BIND_PKEYPRESS = 4,
	BIND_PKEYRELEASE = 5,
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

/* ---------------------------- interface functions ------------------------- */

Bool ParseContext(char *in_context, int *out_context_mask);
Bool ParseModifiers(char *in_modifiers, int *out_modifier_mask);
void CollectBindingList(
	Display *dpy, Binding **pblist_src, Binding **pblist_dest,
	binding_t type, STROKE_ARG(void *stroke) int button, KeySym keysym,
	int modifiers, int contexts);
int AddBinding(
	Display *dpy, Binding **pblist, binding_t type, STROKE_ARG(void *stroke)
	int button, KeySym keysym, char *key_name, int modifiers, int contexts,
	void *action, void *action2);
void FreeBindingStruct(Binding *b);
void FreeBindingList(Binding *b);
void RemoveBinding(Binding **pblist, Binding *b, Binding *prev);
Bool RemoveMatchingBinding(
	Display *dpy, Binding **pblist, binding_t type, STROKE_ARG(char *stroke)
	int button, KeySym keysym, int modifiers, int contexts);
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
