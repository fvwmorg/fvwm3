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

#ifndef EVENTMASK_H
#define EVENTMASK_H

/* ---------------------------- global definitions -------------------------- */

#define XEVMASK_FRAMEW \
	(SubstructureRedirectMask | VisibilityChangeMask | \
	EnterWindowMask | LeaveWindowMask)
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
	VisibilityChangeMask | ExposureMask | KeyPressMask | \
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
	(KeyPressMask|FocusChangeMask)
#define XEVMASK_MENUNFW \
	(KeyPressMask|KeyReleaseMask|FocusChangeMask)
#define XEVMASK_ORW \
	(FocusChangeMask)
#define XEVMASK_ROOTW \
	(LeaveWindowMask| EnterWindowMask | \
	PropertyChangeMask | SubstructureRedirectMask | KeyPressMask | \
	SubstructureNotifyMask | ColormapChangeMask | \
	STROKE_CODE(ButtonMotionMask | DEFAULT_ALL_BUTTONS_MOTION_MASK |) \
	ButtonPressMask | ButtonReleaseMask)
#define XEVMASK_RESIZE \
	(ButtonPressMask | ButtonReleaseMask | KeyPressMask | \
	PointerMotionMask | ButtonMotionMask | ExposureMask)
#define XEVMASK_RESIZE_OPAQUE \
	(XEVMASK_RESIZE | PropertyChangeMask)

#endif /* EVENTMASK_H */
