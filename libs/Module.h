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

#include <X11/X.h>


/**
 * FVWM sends packets of this type to modules.
 **/

typedef struct {
    unsigned long start_pattern;       /* always holds START_FLAG value */
    unsigned long type;                /* one of the M_xxx values, below */
    unsigned long size;                /* number of unsigned longs in
					  entire packet, *including* header */
    unsigned long timestamp;           /* last time stamp received from the
					  X server, in milliseconds */
    unsigned long body[1];             /* variable size -- use
					  FvwmPacketBodySize to get size */
} FvwmPacket;


/** All size values in units of "unsigned long" **/
#define FvwmPacketHeaderSize        4
#define FvwmPacketBodySize(p)       ((p).size - FvwmPacketHeaderSize)
#define FvwmPacketMaxSize           256
#define FvwmPacketBodyMaxSize       (FvwmPacketMaxSize - FvwmPacketHeaderSize)

/** There seems to be some movement afoot to measure packet sizes in bytes.
    See fvwm/module_interface.c **/
#define FvwmPacketHeaderSize_byte  (FvwmPacketHeaderSize * sizeof(unsigned long))
#define FvwmPacketBodySize_byte(p) (FvwmPacketBodySize(p) * sizeof(unsigned long))
#define FvwmPacketMaxSize_byte     (FvwmPacketMaxSize * sizeof(unsigned long))
#define FvwmPacketBodyMaxSize_byte (FvwmPacketBodyMaxSize * sizeof(unsigned long))


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
/* The next one is not used, so we have a place for a new type (it was
 * M_LOCKONSEND which is no more needed). olicha Nov 13, 1999 */
#define M_NOTUSED            (1<<26)
#define M_SENDCONFIG         (1<<27)
#define M_RESTACK            (1<<28)
#define M_ADD_WINDOW         (1<<29)
#define M_CONFIGURE_WINDOW   (1<<30)
#define MAX_MESSAGES         31


/**
 * Reads a single packet of info from FVWM.
 * The packet is stored into static memory that is reused during
 * the next call to ReadFvwmPacket.  Callers, therefore, must copy
 * needed data before the next call to ReadFvwmPacket.
 **/
FvwmPacket* ReadFvwmPacket( int fd );


/************************************************************************
 *
 * SendFinishedStartupNotification - informs fvwm that the module has
 * finished its startup procedures and is fully operational now.
 *
 ***********************************************************************/
void SendFinishedStartupNotification(int *fd);

/************************************************************************
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 ***********************************************************************/
void SendText(int *fd, const char *message, unsigned long window);


/** Compatibility **/
#define SendInfo SendText

/************************************************************************
 *
 * SendFvwmPipe - Sends message to fvwm:  The message is a comma-delimited
 * string separated into its component sections and sent one by one to fvwm.
 * It is discouraged to use this function with a "synchronous" module.
 * (Form FvwmIconMan)
 *
 ***********************************************************************/
void SendFvwmPipe(int *fd, const char *message, unsigned long window);

/***************************************************************************
 *
 * Sets the which-message-types-do-I-want mask for modules
 *
 **************************************************************************/
void SetMessageMask(int *fd, unsigned long mask);

/***************************************************************************
 *
 * Sets the which-message-types-do-I-want to be lock on send for modules
 *
 **************************************************************************/
void SetSyncMask(int *fd, unsigned long mask);

/***************************************************************************
 *
 * Sets the which-message-types-I-do-not-want while the server is grabbed
 * and module transmission is locked at the same time.
 *
 **************************************************************************/
void SetNoGrabMask(int *fd, unsigned long mask);

/*
 * Used to ask for subset of module configuration lines.
 * Allows modules to get configuration lines more than once.
 */
void InitGetConfigLine(int *fd, char *match);


/**
 * Gets a module configuration line from fvwm. Returns NULL if there are
 * no more lines to be had. "line" is a pointer to a char *.
 **/
void GetConfigLine(int *fd, char **line);


/**
 * Parse the command line arguments given to the module by FVWM.
 * Input is the argc & argv from main(), and a flag to indicate
 * if we accept a module alias as argument #6.
 *
 * Returns a pointer to a ModuleArgs structure, or NULL if something
 * is not kosher.  The returned memory is a static buffer.
 **/

typedef struct {
    char* name;                /* module name */
    int to_fvwm;               /* file descriptor to send info back to FVWM */
    int from_fvwm;             /* file descriptor to read packets from FVWM */
    Window window;             /* window context of module */
    unsigned long decoration;  /* decoration context of module */
    int user_argc;             /* number of user-specified arguments */
    char** user_argv;          /* vector of user-specified arguments */
} ModuleArgs;


ModuleArgs* ParseModuleArgs( int argc, char* argv[], int use_arg6_as_alias );


#endif
