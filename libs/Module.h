#ifndef LIBS_MODULE_H
#define LIBS_MODULE_H
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

/*
** Module.c: code for modules to communicate with fvwm
*/

/**
 * Module packet header.
 **/

typedef struct {
    /* always holds START_FLAG value */
    unsigned long start_pattern;
    /* one of the M_xxx values, below */
    unsigned long type;
    /* number of unsigned longs entire packet, *including* header */
    unsigned long size;
    /* last time stamp received from the X server, in milliseconds */
    unsigned long timestamp;
} fvwm_packet_t;


/* Value of start_pattern */
#define START_FLAG 0xffffffff

/* Possible values of type */
#define M_NEW_PAGE               (1)
#define M_NEW_DESK               (1<<1)
#define M_OLD_ADD_WINDOW         (1<<2)
#define M_RAISE_WINDOW           (1<<3)
#define M_LOWER_WINDOW           (1<<4)
#define M_OLD_CONFIGURE_WINDOW   (1<<5)
#define M_FOCUS_CHANGE           (1<<6)
#define M_DESTROY_WINDOW         (1<<7)
#define M_ICONIFY                (1<<8)
#define M_DEICONIFY              (1<<9)
#define M_WINDOW_NAME            (1<<10)
#define M_ICON_NAME              (1<<11)
#define M_RES_CLASS              (1<<12)
#define M_RES_NAME               (1<<13)
#define M_END_WINDOWLIST         (1<<14)
#define M_ICON_LOCATION          (1<<15)
#define M_MAP                    (1<<16)

/* It turns out this is defined by <sys/stream.h> on Solaris 2.6.
   I suspect that simply redefining this will lead to trouble;
   at some point, these should probably be renamed (FVWM_MSG_ERROR?). */
#ifdef M_ERROR
#  undef M_ERROR
#endif
#define M_ERROR              (1<<17)

#define M_CONFIG_INFO        (1<<18)
#define M_END_CONFIG_INFO    (1<<19)
#define M_ICON_FILE          (1<<20)
#define M_DEFAULTICON        (1<<21)
#define M_STRING             (1<<22)
#define M_MINI_ICON          (1<<23)
#define M_WINDOWSHADE        (1<<24)
#define M_DEWINDOWSHADE      (1<<25)
#define M_LOCKONSEND         (1<<26)
#define M_SENDCONFIG         (1<<27)
#define M_RESTACK            (1<<28)
#define M_ADD_WINDOW         (1<<29)
#define M_CONFIGURE_WINDOW   (1<<30)
#define MAX_MESSAGES         31

/*  RBW - 04/16/1999 - GSFR changes.  */
#define HEADER_SIZE         4
/* #define MAX_BODY_SIZE      (24) */
#define MAX_BODY_SIZE      (256 - HEADER_SIZE)
#define MAX_PACKET_SIZE    (HEADER_SIZE+MAX_BODY_SIZE)

#define MAX_NEW_BODY_SIZE      (MAX_BODY_SIZE * sizeof(unsigned long))
#define MAX_NEW_PACKET_SIZE    ((HEADER_SIZE * sizeof(unsigned long)) + MAX_NEW_BODY_SIZE)




/************************************************************************
 *
 * Reads a single packet of info from fvwm.
 * unsigned long header[HEADER_SIZE];
 * unsigned long *body;
 * int fd[2];
 *
 * ReadFvwmPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *   body is a malloc'ed space which needs to be freed
 *
 **************************************************************************/
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body);


/*
** Called if the pipe is no longer open  
*/
extern void DeadPipe(int nonsense);  

/************************************************************************
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 ***********************************************************************/
void SendText(int *fd,char *message,unsigned long window);

/** Compatibility **/
#define SendInfo SendText


/***************************************************************************
 *
 * Sets the which-message-types-do-I-want mask for modules
 *
 **************************************************************************/
void SetMessageMask(int *fd, unsigned long mask);


/**
 * Gets a module configuration line from fvwm. Returns NULL if there are
 * no more lines to be had. "line" is a pointer to a char *.
 **/
void GetConfigLine(int *fd, char **line);


#endif
