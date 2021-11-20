/* -*-c-*- */

#ifndef FVWMLIB_VPACKET_H
#define FVWMLIB_VPACKET_H
#include "fvwm_x11.h"
#include "fvwm/fvwm.h"

/*
  All new-style module packets (i.e., those that are not simply arrays
  of longs, as used by the older modules) should have a structure
  definition in this file.
*/

/*
  The M_CONFIGURE_WINDOW packet.
  This is the same packet as the M_ADD_WINDOW packet, the
  only difference being the type.
*/
typedef struct ConfigWinPacket
{
	/*** Alignment notes ***/
	/*** Note that this packet format will break on future 128 bit
	 *** platforms. ***/
	/*** Put long, Window, and pointers  here ***/
	unsigned long  w;      /* Window */
	unsigned long  frame; /* Window */
	unsigned long *fvwmwin;
	signed long    frame_x;
	signed long    frame_y;
	unsigned long  frame_width;
	unsigned long  frame_height;
	unsigned long  desk;
	unsigned long  monitor_id;
	unsigned long  monitor_name;
	unsigned long  layer;
	unsigned long  hints_base_width;
	unsigned long  hints_base_height;
	unsigned long  hints_width_inc;
	unsigned long  hints_height_inc;
	unsigned long  orig_hints_width_inc;
	unsigned long  orig_hints_height_inc;
	unsigned long  hints_min_width;
	unsigned long  hints_min_height;
	unsigned long  hints_max_width;
	unsigned long  hints_max_height;
	unsigned long  icon_w;        /* Window */
	unsigned long  icon_pixmap_w; /* Window */
	unsigned long  hints_win_gravity;
	unsigned long  TextPixel;
	unsigned long  BackPixel;
	unsigned long  ewmh_hint_layer;
	unsigned long  ewmh_hint_desktop;
	unsigned long  ewmh_window_type;

	/*** Put int here, fill with dummies to a multiple of 2 ***/

	/*** Put short here, fill with dummies to a multiple of 4 ***/
	unsigned short title_height;
	unsigned short border_width;
	unsigned short short_dummy_3;
	unsigned short short_dummy_4;

	/*** Put structures here ***/
	window_flags   flags;
	action_flags   allowed_actions;

} ConfigWinPacket;

typedef struct MiniIconPacket
{
	Window         w;
	Window         frame;
	FvwmWindow    *fvwmwin;
	unsigned long  width;
	unsigned long  height;
	unsigned long  depth;
	Pixmap         picture;
	Pixmap         mask;
	Pixmap         alpha;
	char           name[1];
} MiniIconPacket;

#endif /* FVWMLIB_VPACKET_H */
