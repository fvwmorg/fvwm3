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

#ifndef _VPACKET_
#define _VPACKET_
#include <fvwm/gsfr.h>

/*
    =====================================================================
    All new-style module packets (i.e., those that are not simply arrays
    of long ints, as used by the older modules) should have a structure
    definition in this file.
    =====================================================================
*/




/*  ============================================================
    The M_CONFIGURE_WINDOW packet.
    This is the same packet as the M_ADD_WINDOW packet, the
    only difference being the type.
    ============================================================  */
/*  RBW- typedef struct config_win_packet  */
typedef struct ConfigWinPacket
{
Window             w;
Window             frame;
FvwmWindow         *fvwmwin;
unsigned long int  frame_x;
unsigned long int  frame_y;
unsigned long int  frame_width;
unsigned long int  frame_height;
unsigned long int  desk;
/* 
  Temp word for alignment - old flags used to be here. 
  - remove before next release.
*/
unsigned long int  dummy;

unsigned long int  title_height;
unsigned long int  border_width;
unsigned long int  hints_base_width;
unsigned long int  hints_base_height;
unsigned long int  hints_width_inc;
unsigned long int  hints_height_inc;
unsigned long int  hints_min_width;
unsigned long int  hints_min_height;
unsigned long int  hints_max_width;
unsigned long int  hints_max_height;
Window             icon_w;
Window             icon_pixmap_w;
unsigned long int  hints_win_gravity;
unsigned long int  TextPixel;
unsigned long int  BackPixel;

/*  Everything below this is post-GSFR  */
window_flags       flags;

} ConfigWinPacket;

#endif /* _VPACKET_ */
