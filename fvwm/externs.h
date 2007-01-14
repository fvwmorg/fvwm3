/* -*-c-*- */
/*
 *       Copyright 1988 by Evans & Sutherland Computer Corporation,
 *                          Salt Lake City, Utah
 *  Portions Copyright 1989 by the Massachusetts Institute of Technology
 *                        Cambridge, Massachusetts
 *
 *                           All Rights Reserved
 *
 *    Permission to use, copy, modify, and distribute this software and
 *    its documentation  for  any  purpose  and  without  fee is hereby
 *    granted, provided that the above copyright notice appear  in  all
 *    copies and that both  that  copyright  notice  and  this  permis-
 *    sion  notice appear in supporting  documentation,  and  that  the
 *    names of Evans & Sutherland and M.I.T. not be used in advertising
 *    in publicity pertaining to distribution of the  software  without
 *    specific, written prior permission.
 *
 *    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 *    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-
 *    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR
 *    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-
 *    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA
 *    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE
 *    OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef EXTERNS_H
#define EXTERNS_H

void Done(int, char *) __attribute__((__noreturn__));
void setInitFunctionName(int n, const char *name);
const char *getInitFunctionName(int n);

extern char NoName[];
extern char NoClass[];
extern char NoResource[];
extern XGCValues Globalgcv;
extern unsigned long Globalgcm;
extern int master_pid;
extern Display *dpy;
extern int x_fd;
extern XContext FvwmContext;
extern Bool fFvwmInStartup;
extern Bool DoingCommandLine;
extern Bool debugging;
extern Bool debugging_stack_ring;
extern int GrabPointerState;
extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern int JunkWidth, JunkHeight, JunkBW, JunkDepth;
extern unsigned int JunkMask;
extern char *fvwm_userdir;
extern char *display_name;
extern Atom _XA_MIT_PRIORITY_COLORS;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_SAVE_YOURSELF;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_DESKTOP;
extern Atom _XA_OL_WIN_ATTR;
extern Atom _XA_OL_WT_BASE;
extern Atom _XA_OL_WT_CMD;
extern Atom _XA_OL_WT_HELP;
extern Atom _XA_OL_WT_NOTICE;
extern Atom _XA_OL_WT_OTHER;
extern Atom _XA_OL_DECOR_ADD;
extern Atom _XA_OL_DECOR_DEL;
extern Atom _XA_OL_DECOR_CLOSE;
extern Atom _XA_OL_DECOR_RESIZE;
extern Atom _XA_OL_DECOR_HEADER;
extern Atom _XA_OL_DECOR_ICON_NAME;
extern Atom _XA_WM_WINDOW_ROLE;
extern Atom _XA_WINDOW_ROLE;
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_SM_CLIENT_ID;
extern Atom _XA_WIN_SX;
extern Atom _XA_MANAGER;
extern Atom _XA_ATOM_PAIR;
extern Atom _XA_WM_COLORMAP_NOTIFY;
extern Atom _XA_XROOTPMAP_ID;
extern Atom _XA_XSETROOT_ID;
#endif /* _EXTERNS_ */
