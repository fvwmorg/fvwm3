/* -*-c-*- */

#ifndef EVENTMASK_H
#define EVENTMASK_H

/* ---------------------------- global definitions ------------------------- */

#define XEVMASK_FRAMEW \
	(SubstructureRedirectMask | VisibilityChangeMask | \
	EnterWindowMask | LeaveWindowMask)
#define XEVMASK_FRAMEW_CAPTURE (XEVMASK_FRAMEW | StructureNotifyMask)
#define XEVMASK_TITLEW \
	(ButtonPressMask | ButtonReleaseMask | \
	OwnerGrabButtonMask | /*ButtonMotionMask | PointerMotionMask | */\
	EnterWindowMask | LeaveWindowMask)
#define XEVMASK_BUTTONW \
	XEVMASK_TITLEW
#define XEVMASK_PARENTW \
	(SubstructureRedirectMask)
#define XEVMASK_BORDERW \
	(ButtonPressMask | ButtonReleaseMask | \
	EnterWindowMask | LeaveWindowMask)
#define XEVMASK_CLIENTW \
	(StructureNotifyMask | PropertyChangeMask | \
	EnterWindowMask | LeaveWindowMask | \
	ColormapChangeMask | FocusChangeMask)
#define XEVMASK_ICONW \
	(ButtonPressMask | ButtonReleaseMask | \
	VisibilityChangeMask | ExposureMask | KeyPressMask | KeyReleaseMask | \
	EnterWindowMask | LeaveWindowMask | FocusChangeMask)
#define XEVMASK_ICONPW \
	XEVMASK_ICONW
#define XEVMASK_MENU \
	(ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyReleaseMask | \
	KeyPressMask | VisibilityChangeMask | ButtonMotionMask | \
	PointerMotionMask)
#define XEVMASK_TEAR_OFF_MENU \
	(XEVMASK_MENU | LeaveWindowMask | EnterWindowMask)
#define XEVMASK_MENUW \
	(ExposureMask | KeyPressMask | KeyReleaseMask)
#define XEVMASK_TEAR_OFF_MENUW \
	(XEVMASK_MENUW | EnterWindowMask | LeaveWindowMask | \
	StructureNotifyMask)
#define XEVMASK_TEAR_OFF_SUBMENUW \
	(XEVMASK_MENUW | LeaveWindowMask)
#define XEVMASK_PANFW \
	(ButtonPressMask | ButtonReleaseMask | KeyReleaseMask | KeyPressMask | \
	EnterWindowMask | LeaveWindowMask | VisibilityChangeMask)
#define XEVMASK_NOFOCUSW \
	(KeyPressMask | KeyReleaseMask | FocusChangeMask)
#define XEVMASK_MENUNFW \
	(KeyPressMask | KeyReleaseMask | FocusChangeMask)
#define XEVMASK_ORW \
	(FocusChangeMask)
#define XEVMASK_ROOTW \
	(LeaveWindowMask| EnterWindowMask | \
	PropertyChangeMask | SubstructureRedirectMask | KeyPressMask | \
	KeyReleaseMask | \
	SubstructureNotifyMask | ColormapChangeMask | \
	ButtonPressMask | ButtonReleaseMask)
#define XEVMASK_RESIZE \
	(ButtonPressMask | ButtonReleaseMask | KeyPressMask | \
	PointerMotionMask | ButtonMotionMask | ExposureMask)
#define XEVMASK_RESIZE_OPAQUE \
	(XEVMASK_RESIZE | PropertyChangeMask)

#endif /* EVENTMASK_H */
