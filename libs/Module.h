/* -*-c-*- */
#ifndef LIBS_MODULE_H
#define LIBS_MODULE_H

/*
** Module.c: code for modules to communicate with fvwm
*/

#include <X11/X.h>
#include "libs/fvwmlib.h"

/**
 * fvwm sends packets of this type to modules.
 **/

typedef struct
{
	/* always holds START_FLAG value */
	unsigned long start_pattern;
	/* one of the M_xxx values, below */
	unsigned long type;
	/* number of unsigned longs in entire packet, *including* header */
	unsigned long size;
	/* last time stamp received from the X server, in milliseconds */
	unsigned long timestamp;
	/* variable size -- use FvwmPacketBodySize to get size */
	unsigned long body[1];
} FvwmPacket;

typedef struct
{
	Window w;
	Window frame;
	void *fvwmwin;
} FvwmWinPacketBodyHeader;

/*
 * If you modify constants here, please regenerate Constants.pm in perllib.
 */


/** All size values in units of "unsigned long" **/
#define FvwmPacketHeaderSize        4
#define FvwmPacketBodySize(p)       ((p).size - FvwmPacketHeaderSize)
#define FvwmPacketMaxSize           256
#define FvwmPacketBodyMaxSize       (FvwmPacketMaxSize - FvwmPacketHeaderSize)

/** There seems to be some movement afoot to measure packet sizes in bytes.
    See fvwm/module_interface.c **/
#define FvwmPacketHeaderSize_byte  \
	(FvwmPacketHeaderSize * sizeof(unsigned long))
#define FvwmPacketBodySize_byte(p) \
	(FvwmPacketBodySize(p) * sizeof(unsigned long))
#define FvwmPacketMaxSize_byte \
	(FvwmPacketMaxSize * sizeof(unsigned long))
#define FvwmPacketBodyMaxSize_byte \
	(FvwmPacketBodyMaxSize * sizeof(unsigned long))


/* Value of start_pattern */
#define START_FLAG 0xffffffff

#define ModuleFinishedStartupResponse "NOP FINISHED STARTUP"
#define ModuleUnlockResponse          "NOP UNLOCK"

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
#define M_VISIBLE_NAME       (1<<26)
#define M_SENDCONFIG         (1<<27)
#define M_RESTACK            (1<<28)
#define M_ADD_WINDOW         (1<<29)
#define M_CONFIGURE_WINDOW   (1<<30)
#define M_EXTENDED_MSG       (1<<31)
#define MAX_MESSAGES         31
#define MAX_MSG_MASK         0x7fffffff

/* to get more than the old maximum of 32 messages, the 32nd bit is reserved to
 * mark another 31 messages that have this bit and another one set.
 * When handling received messages, the message type can be compared to the
 * MX_... macro.  When using one of the calls that accepts a message mask, a
 * separate call has to be made that ors the MX_... macros.  The normal
 * M_... and MX_... macros must *never* be or'ed in one of these operations'
 */
#define MX_VISIBLE_ICON_NAME      ((1<<0) | M_EXTENDED_MSG)
#define MX_ENTER_WINDOW           ((1<<1) | M_EXTENDED_MSG)
#define MX_LEAVE_WINDOW           ((1<<2) | M_EXTENDED_MSG)
#define MX_PROPERTY_CHANGE        ((1<<3) | M_EXTENDED_MSG)
#define MX_REPLY		  ((1<<4) | M_EXTENDED_MSG)
#define MAX_EXTENDED_MESSAGES     5
#define DEFAULT_XMSG_MASK         0x00000000
#define MAX_XMSG_MASK             0x0000001f

#define MAX_TOTAL_MESSAGES   (MAX_MESSAGES + MAX_EXTENDED_MESSAGES)

/* for MX_PROPERTY_CHANGE */
#define MX_PROPERTY_CHANGE_NONE        0
#define MX_PROPERTY_CHANGE_BACKGROUND  1
#define MX_PROPERTY_CHANGE_SWALLOW     2

/**
 * Reads a single packet of info from fvwm.
 * The packet is stored into static memory that is reused during
 * the next call to ReadFvwmPacket.  Callers, therefore, must copy
 * needed data before the next call to ReadFvwmPacket.
 **/
FvwmPacket* ReadFvwmPacket( int fd );


/*
 *
 * SendFinishedStartupNotification - informs fvwm that the module has
 * finished its startup procedures and is fully operational now.
 *
 */
void SendFinishedStartupNotification(int *fd);

/*
 *
 * SendText - Sends arbitrary text/command back to fvwm
 *
 */
void SendText(int *fd, const char *message, unsigned long window);


/** Compatibility **/
#define SendInfo SendText

/*
 *
 * SendUnlockNotification - informs fvwm that the module has
 * finished it's procedures and fvwm may proceed.
 *
 */
void SendUnlockNotification(int *fd);

/*
 *
 * SendQuitNotification - informs fvwm that the module has
 * finished and may be killed.
 *
 */
void SendQuitNotification(int *fd);

/*
 *
 * SendFvwmPipe - Sends message to fvwm:  The message is a comma-delimited
 * string separated into its component sections and sent one by one to fvwm.
 * It is discouraged to use this function with a "synchronous" module.
 * (Form FvwmIconMan)
 *
 */
void SendFvwmPipe(int *fd, const char *message, unsigned long window);

/*
 *
 * Sets the which-message-types-do-I-want mask for modules
 *
 */
void SetMessageMask(int *fd, unsigned long mask);

/*
 *
 * Sets the which-message-types-do-I-want to be lock on send for modules
 *
 */
void SetSyncMask(int *fd, unsigned long mask);

/*
 *
 * Sets the which-message-types-I-do-not-want while the server is grabbed
 * and module transmission is locked at the same time.
 *
 */
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

/* expands certain variables in a command to be sent by a module */
char *module_expand_action(
	Display *dpy, int screen , char *in_action, rectangle *r,
	char *forecolor, char *backcolor);

/**
 * Parse the command line arguments given to the module by fvwm.
 * Input is the argc & argv from main(), and a flag to indicate
 * if we accept a module alias as argument #6.
 *
 * Returns a pointer to a ModuleArgs structure, or NULL if something
 * is not kosher.  The returned memory is a static buffer.
 **/

typedef struct
{
	/* module name */
	char* name;
	/* length of the module name */
	int namelen;
	/* file descriptor to send info back to fvwm */
	int to_fvwm;
	/* file descriptor to read packets from fvwm */
	int from_fvwm;
	/* window context of module */
	Window window;
	/* decoration context of module */
	unsigned long decoration;
	/* number of user-specified arguments */
	int user_argc;
	/* vector of user-specified arguments */
	char** user_argv;
} ModuleArgs;

ModuleArgs* ParseModuleArgs( int argc, char* argv[], int use_arg6_as_alias );

#endif
