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

/* File: defaults.h
 *
 * Description:
 *      #defines for default values shall go into this file as well as tunable
 *      parameters.
 *
 * Created:
 *       23 Dec 1998 - Dominik Vogt <dominik_vogt@hp.com>
 */

#ifndef _DEFAULTS_
#define _DEFAULTS_

/* event handling */
#define CLOCK_SKEW_MS                  30000

/* bindings and mouse buttons */
/* Fvwm needs at least 3 buttons. X currently supports up to 5 buttons, so you
 * can use 3, 4 or 5 here. */
#define NUMBER_OF_MOUSE_BUTTONS            5
#define DEFAULT_BUTTONS_TO_GRAB            ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1)
#define DEFAULT_ALL_BUTTONS_MASK           \
((Button1Mask * (NUMBER_OF_MOUSE_BUTTONS >= 1)) | \
 (Button2Mask * (NUMBER_OF_MOUSE_BUTTONS >= 2)) | \
 (Button3Mask * (NUMBER_OF_MOUSE_BUTTONS >= 3)) | \
 (Button4Mask * (NUMBER_OF_MOUSE_BUTTONS >= 4)) | \
 (Button5Mask * (NUMBER_OF_MOUSE_BUTTONS >= 5)))
#define DEFAULT_ALL_BUTTONS_MOTION_MASK    \
((Button1MotionMask * (NUMBER_OF_MOUSE_BUTTONS >= 1)) | \
 (Button2MotionMask * (NUMBER_OF_MOUSE_BUTTONS >= 2)) | \
 (Button3MotionMask * (NUMBER_OF_MOUSE_BUTTONS >= 3)) | \
 (Button4MotionMask * (NUMBER_OF_MOUSE_BUTTONS >= 4)) | \
 (Button5MotionMask * (NUMBER_OF_MOUSE_BUTTONS >= 5)))

/* menus */
#define DEFAULT_CLICKTIME                150 /* ms */
#define DEFAULT_POPUP_DELAY               15 /* ms*10 */
#define DEFAULT_MENU_CLICKTIME             (3*DEFAULT_CLICKTIME)
#define DEFAULT_MOVE_THRESHOLD             3 /* pixels */

/* pager */
#define DEFAULT_MOVE_THRESHOLD             3 /* pixels */
#define DEFAULT_PAGER_WINDOW_BORDER_WIDTH  1 /* pixels */

/* fonts */
#define EXTRA_TITLE_FONT_HEIGHT            3
#define MIN_FONT_HEIGHT                    (EXTRA_TITLE_FONT_HEIGHT + 2)
#define MAX_FONT_HEIGHT                  256

/* window geometry */
#define DEFAULT_MIN_MAX_WINDOW_WIDTH     100
#define DEFAULT_MIN_MAX_WINDOW_HEIGHT    100
#define DEFAULT_MAX_MAX_WINDOW_WIDTH   32767
#define DEFAULT_MAX_MAX_WINDOW_HEIGHT  32767
/* this value is used in a bugfix */
#define WINDOW_FREAKED_OUT_HEIGHT      65500

/* window placement (cleverplacement) */
#define PLACEMENT_AVOID_STICKY 1
#define PLACEMENT_AVOID_ONTOP 5
#define PLACEMENT_AVOID_ICON 10   /*  Try hard no to place windows over icons */
/*#define PLACEMENT_AVOID_ICON 0*//*  Ignore Icons.  Place windows over them  */

/* movement */
#define DEFAULT_OPAQUE_MOVE_SIZE           5
#define DEFAULT_SNAP_ATTRACTION           -1
#define DEFAULT_SNAP_ATTRACTION_MODE       0
#define DEFAULT_SNAP_GRID_X                1
#define DEFAULT_SNAP_GRID_Y                1

/* paging */
#define DEFAULT_EDGE_SCROLL              100
/* Don't page if the pointer has moved for more than this many pixels between
 * two samples */
#define MAX_PAGING_MOVE_DISTANCE           5
#define DEFAULT_MOVE_RESISTANCE            0
#define DEFAULT_SCROLL_RESISTANCE          0

/* layers */
#define DEFAULT_BOTTOM_LAYER               2
#define DEFAULT_DEFAULT_LAYER              4
#define DEFAULT_TOP_LAYER                  6
#define DEFAULT_ROOT_WINDOW_LAYER         -1

/* decorations */
#define NR_LEFT_BUTTONS                    5
#define NR_RIGHT_BUTTONS                   5
#define NUMBER_OF_BUTTONS                  (NR_LEFT_BUTTONS + NR_RIGHT_BUTTONS)

/* window borders */
#define DEFAULT_BORDER_WIDTH               1
#define DEFAULT_HANDLE_WIDTH               7
#define MAX_BORDER_WIDTH                 500
#define MAX_HANDLE_WIDTH                   (MAX_BORDER_WIDTH)

/* module configuration */
#define DEFAULT_MODULE_TIMEOUT            30 /* seconds */

/* misc */
#define DEFAULT_EMULATE_MWM            False
#define DEFAULT_EMULATE_WIN            False

#define DEFAULT_USE_ACTIVE_DOWN_BUTTONS True
#define DEFAULT_USE_INACTIVE_BUTTONS    True

/* Gradients */
#define MAX_GRADIENT_SEGMENTS           1000
#define MAX_GRADIENT_COLORS            10000

/* Very long window names (600000 characters or more) seem to hang the X
 * server. */
#define MAX_WINDOW_NAME_LEN              200
#define MAX_ICON_NAME_LEN                200

#endif /* _DEFAULTS_ */
