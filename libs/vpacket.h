/* -*-c-*- */

#ifndef _VPACKET_
#define _VPACKET_
#include <fvwm/window_flags.h>

/*
  All new-style module packets (i.e., those that are not simply arrays
  of long ints, as used by the older modules) should have a structure
  definition in this file.
*/

/*
  The M_CONFIGURE_WINDOW packet.
  This is the same packet as the M_ADD_WINDOW packet, the
  only difference being the type.
*/
/*  RBW- typedef struct config_win_packet  */
typedef struct ConfigWinPacket
{
	/*** Alignment notes ***/
	/*** Note that this packet format will break on future 128 bit
	 *** platforms. ***/
	/*** Put long int, Window, and pointers  here ***/
	unsigned long int         w;      /* Window */
	unsigned long int         frame; /* Window */
	unsigned long int         *fvwmwin;
	signed long int    frame_x;
	signed long int    frame_y;
	unsigned long int  frame_width;
	unsigned long int  frame_height;
	unsigned long int  desk;
	/*
	  Temp word for alignment - old flags used to be here.
	  - remove before next release.
	  RBW - 05/01/2000 - layer has usurped this slot.
	  unsigned long int  dummy;
	*/
	unsigned long int  layer;

	unsigned long int  hints_base_width;
	unsigned long int  hints_base_height;
	unsigned long int  hints_width_inc;
	unsigned long int  hints_height_inc;
	unsigned long int  hints_min_width;
	unsigned long int  hints_min_height;
	unsigned long int  hints_max_width;
	unsigned long int  hints_max_height;
	unsigned long int  icon_w;        /* Window */
	unsigned long int  icon_pixmap_w; /* Window */
	unsigned long int  hints_win_gravity;
	unsigned long int  TextPixel;
	unsigned long int  BackPixel;

	/*  Everything below this is post-GSFR  */
	unsigned long int  ewmh_hint_layer;
	unsigned long int  ewmh_hint_desktop;
	unsigned long int  ewmh_window_type;

	/*** Put int here, fill with dummies to a multiple of 2 ***/

	/*** Put short int here, fill with dummies to a multiple of 4 ***/
	unsigned short int title_height;
	unsigned short int border_width;
	unsigned short int short_dummy_3;
	unsigned short int short_dummy_4;

	/*** Put structures here ***/
	window_flags       flags;
	action_flags       allowed_actions;

} ConfigWinPacket;

typedef struct MiniIconPacket
{
	Window             w;
	Window             frame;
	FvwmWindow         *fvwmwin;
	unsigned long int  width;
	unsigned long int  height;
	unsigned long int  depth;
	Pixmap             picture;
	Pixmap             mask;
	Pixmap             alpha;
	char               name[1];
} MiniIconPacket;

#endif /* _VPACKET_ */
