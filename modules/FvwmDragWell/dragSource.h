/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef _DRAG_SOURCE_H
#define _DRAG_SOURCE_H

#include <X11/Xlib.h>
#include <X11/X.h>
#include "cursorStuff.h"
#include "commonStuff.h"




typedef struct DragSource_Struct {
  FILE *mfsErr;
  Display *display;
  Window rootWindow;	    /*root window of screen*/
  Window dropTargWin;	    /*the drop target window*/
  Window dragSrcWin;	    /*the drag source window*/
  int version;
  XdndAtoms *atomSel;	    /*Atoms used for xdnd*/
  float dragHalo; /*# of pixels the mouse has to move before a drag starts*/
  XdndCursor *cursors;
  unsigned int state;	    /*holds state of the drag*/
  unsigned short cachexRoot, cacheyRoot;
  unsigned short rx,ry,w,h; /*position of window in root*/
  Atom dropTargProperty;    /*holds the drop target property*/
} DragSource;

#define XDND_MIN(a,b) ((a<b) ? a :b)

/* These are the defines for the <state> bit of DragSource.  When saving state,
   the usual choice is between using many shorts and bit twiddling.  Using
   shorts is faster than bit twiddling, but uses more space.  Since I can
   envision many widgets each being DNDable(icons on desktop), we'll go with
   twiddling.  The format for the defines is groups of three, where the first
   define is the bit position, and the second and third define what a 0 or 1
   in that position means.
*/

#define XDND_IN_AWAREWIN 0x0UL /*pointer in win. that is XdndAware*/
#define XDND_NOTAWARE 0x0UL
#define XDND_AWARE 0x1UL

#define XDND_WAIT_4_STATUS 0x1UL  /*sent position, waiting for status event*/
#define XDND_NOTWAITING 0x0UL
#define XDND_WAITING 0x1UL

#define XDND_POS_CACHE 0x2UL /*cacheing the position*/
#define XDND_NOT_CACHED 0x0UL
#define XDND_CACHED 0x1UL

#define XDND_SEND_POS 0x3UL  /*use a rectangle to determine when to send */
#define XDND_RECTSEND 0x0UL  /*motion events to the drop target*/
#define XDND_ALWAYS_SEND 0x1UL

#define XDND_STATUS_RECVD_CNT 0x4UL  /*keeps track of the number of status*/
#define XDND_GOT_ZERO 0X0UL	     /*messages received*/
#define XDND_GOTONE_OR_MORE 0x1UL

#define XDND_DROPPABLE 0x5UL /*can we drop over the current widget?*/
#define XDND_CANT_DROP 0X0UL
#define XDND_CAN_DROP 0x1UL

#define XDND_BUTRELEASE_CACHE 0x6UL /*cache a button release event*/
#define XDND_BUT_NOTCACHED 0x0UL
#define XDND_BUTCACHED 0x1UL

#endif

