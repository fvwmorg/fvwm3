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

#define DEFAULT_CLICKTIME               150           /* ms */
#define DEFAULT_POPUP_DELAY              15           /* ms*10 */
#define DEFAULT_MENU_CLICKTIME           (3*DEFAULT_CLICKTIME)
#define DEFAULT_MOVE_THRESHOLD            3

#define DEFAULT_MIN_MAX_WINDOW_WIDTH    100
#define DEFAULT_MIN_MAX_WINDOW_HEIGHT   100
#define DEFAULT_MAX_MAX_WINDOW_WIDTH  32767
#define DEFAULT_MAX_MAX_WINDOW_HEIGHT 32767
#define WINDOW_FREAKED_OUT_HEIGHT     65500

#define DEFAULT_OPAQUE_MOVE_SIZE          5

#define DEFAULT_BOTTOM_LAYER              2
#define DEFAULT_DEFAULT_LAYER             4
#define DEFAULT_TOP_LAYER                 6
#define DEFAULT_ROOT_WINDOW_LAYER        -1

#define DEFAULT_EMULATE_MWM               False
#define DEFAULT_EMULATE_WIN               False

#define DEFAULT_USE_ACTIVE_DOWN_BUTTONS   True
#define DEFAULT_USE_INACTIVE_BUTTONS      True

#define DEFAULT_SNAP_ATTRACTION           -1
#define DEFAULT_SNAP_ATTRACTION_MODE      0
#define DEFAULT_SNAP_GRID_X               1
#define DEFAULT_SNAP_GRID_Y               1

#define DEFAULT_MOVE_SMOOTHNESS           30

/* Tunable parameters. */
#define MAX_GRADIENT_SEGMENTS          1000
#define MAX_GRADIENT_COLORS            10000
/* Don't page if the pointer has moved for more than this many pixels between
 * two samples */
#define MAX_PAGING_MOVE_DISTANCE          5

#endif /* _DEFAULTS_ */
